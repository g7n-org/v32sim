////////////////////////////////////////////////////////////////////////////////////////
//
// spu.c: model of the Vircon32 sound processing unit (without audio output)
//
// Keeps the sound table (BIOS sound + up to 1024 cartridge sounds), the 16 channels,
// and implements the SPU port semantics of DesktopEmulator (ConsoleLogic/V32SPU.cpp,
// V32SPUWriters.cpp): selection pointers, clamping, read-only ports, play/pause/stop
// commands, and per-frame playback (position advance, looping, stop at end of sound).
// No samples are mixed or played; the point is that programs observe the same port
// behaviour they would on the console (channel state, position, and so on).
//
////////////////////////////////////////////////////////////////////////////////////////
#include "defines.h"
#include "globals.h"
#include "ioports.h"
#include <math.h>

#define  SPU_MAX_SOUNDS          1024
#define  SPU_CHANNELS            16
#define  SPU_SAMPLES_PER_FRAME   (44100 / 60)

typedef struct
{
    uint32_t  length;                // samples
    uint32_t *samples;               // each word: left (low 16) | right (high 16)
    int32_t   loop;                  // PlayWithLoop
    int32_t   loopstart;
    int32_t   loopend;
} sound_t;

typedef struct
{
    int32_t   state;                 // SPUChannelState_*
    int32_t   sound;                 // assigned sound id (-1 = BIOS)
    float     volume;
    float     speed;
    int32_t   loop;
    double    position;
} channel_t;

static sound_t    sounds[SPU_MAX_SOUNDS];
static sound_t    biossound;
static channel_t  channels[SPU_CHANNELS];
static uint32_t   loaded_sounds   = 0;
static int32_t    selsound        = -1;
static int32_t    selchannel      = 0;

#define  SPUP(port)   ((*(ioports+SPU_PORT)) + ((port) & 0xFF))

static sound_t *sound_ptr (int32_t id)
{
    return ((id < 0) ? &biossound : &sounds[id]);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// refresh the read-back ports from the pointed sound / channel
//
static void  refresh_sound (void)
{
    sound_t  *s  = sound_ptr (selsound);

    SPUP(SPU_SoundLength)      -> value.i32  = (int32_t) s -> length;
    SPUP(SPU_SoundPlayWithLoop)-> value.i32  = s -> loop;
    SPUP(SPU_SoundLoopStart)   -> value.i32  = s -> loopstart;
    SPUP(SPU_SoundLoopEnd)     -> value.i32  = s -> loopend;
}

static void  refresh_channel (void)
{
    channel_t  *c  = &channels[selchannel];

    SPUP(SPU_ChannelState)         -> value.i32  = c -> state;
    SPUP(SPU_ChannelAssignedSound) -> value.i32  = c -> sound;
    SPUP(SPU_ChannelVolume)        -> value.f32  = c -> volume;
    SPUP(SPU_ChannelSpeed)         -> value.f32  = c -> speed;
    SPUP(SPU_ChannelLoopEnabled)   -> value.i32  = c -> loop;
    SPUP(SPU_ChannelPosition)      -> value.i32  = (int32_t) c -> position;
}

static void  sound_defaults (sound_t *s)
{
    s -> loop       = 0;
    s -> loopstart  = 0;
    s -> loopend    = (int32_t) s -> length - 1;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// spu_reset(): power-on state
//
void  spu_reset (void)
{
    int  i  = 0;

    selsound    = -1;
    selchannel  = 0;
    for (i = 0; i < SPU_CHANNELS; i = i + 1)
    {
        channels[i].state     = SPUChannelState_Stopped;
        channels[i].sound     = -1;
        channels[i].volume    = 0.5f;
        channels[i].speed     = 1.0f;
        channels[i].loop      = 0;
        channels[i].position  = 0.0;
    }
    sound_defaults (&biossound);
    for (i = 0; i < SPU_MAX_SOUNDS; i = i + 1)
    {
        sound_defaults (&sounds[i]);
    }
    SPUP(SPU_GlobalVolume)  -> value.f32  = 1.0f;
    SPUP(SPU_SelectedSound) -> value.i32  = -1;
    SPUP(SPU_SelectedChannel) -> value.i32  = 0;
    refresh_sound ();
    refresh_channel ();
}

////////////////////////////////////////////////////////////////////////////////////////
//
// sound loading: id -1 is the BIOS sound, 0..n-1 cartridge sounds. 'samples' is
// owned by the SPU afterwards.
//
void  spu_sound_load (int32_t id, uint32_t length, uint32_t *samples)
{
    sound_t  *s  = NULL;

    if ((id < -1) || (id >= SPU_MAX_SOUNDS))
    {
        free (samples);
        return;
    }
    s  = sound_ptr (id);
    free (s -> samples);
    s -> samples  = samples;
    s -> length   = length;
    sound_defaults (s);
    refresh_sound ();
}

void  spu_cart_insert (uint32_t count)
{
    loaded_sounds  = (count > SPU_MAX_SOUNDS) ? SPU_MAX_SOUNDS : count;
}

void  spu_cart_remove (void)
{
    int  i  = 0;

    for (i = 0; i < SPU_MAX_SOUNDS; i = i + 1)
    {
        free (sounds[i].samples);
        sounds[i].samples  = NULL;
        sounds[i].length   = 0;
        sound_defaults (&sounds[i]);
    }
    for (i = 0; i < SPU_CHANNELS; i = i + 1)
    {
        if (channels[i].sound >= 0)
        {
            channels[i].sound     = -1;
            channels[i].state     = SPUChannelState_Stopped;
            channels[i].position  = 0.0;
        }
    }
    loaded_sounds  = 0;
    if (selsound >= 0)
    {
        selsound  = -1;
        SPUP(SPU_SelectedSound) -> value.i32  = -1;
    }
    refresh_sound ();
    refresh_channel ();
}

////////////////////////////////////////////////////////////////////////////////////////
//
// channel commands
//
static void  play_channel (channel_t *c)
{
    if ((c -> state == SPUChannelState_Stopped) ||
        (c -> state == SPUChannelState_Playing))
    {
        c -> loop      = sound_ptr (c -> sound) -> loop;
        c -> position  = 0.0;
    }
    c -> state  = SPUChannelState_Playing;
}

static void  stop_channel (channel_t *c)
{
    c -> state     = SPUChannelState_Stopped;
    c -> position  = 0.0;
}

static float  clampf (float v, float lo, float hi)
{
    return ((v < lo) ? lo : ((v > hi) ? hi : v));
}

static int32_t  clampi (int32_t v, int32_t lo, int32_t hi)
{
    return ((v < lo) ? lo : ((v > hi) ? hi : v));
}

////////////////////////////////////////////////////////////////////////////////////////
//
// spu_write(): a write to an SPU port has been accepted by ioports_set(); apply the
// SPU semantics (validation, clamping, side effects). Writes that the console would
// ignore leave the port value untouched.
//
void  spu_write (uint16_t port, int32_t i32, float f32)
{
    sound_t    *s  = sound_ptr (selsound);
    channel_t  *c  = &channels[selchannel];
    int         i  = 0;

    switch (port)
    {
        case SPU_Command:
            switch (i32)
            {
                case SPUCommand_PlaySelectedChannel:  play_channel (c);  break;
                case SPUCommand_PauseSelectedChannel: c -> state = SPUChannelState_Paused;  break;
                case SPUCommand_StopSelectedChannel:  stop_channel (c);  break;
                case SPUCommand_PauseAllChannels:
                    for (i = 0; i < SPU_CHANNELS; i = i + 1)
                        if (channels[i].state == SPUChannelState_Playing)
                            channels[i].state = SPUChannelState_Paused;
                    break;
                case SPUCommand_ResumeAllChannels:
                    for (i = 0; i < SPU_CHANNELS; i = i + 1)
                        if (channels[i].state == SPUChannelState_Paused)
                            play_channel (&channels[i]);
                    break;
                case SPUCommand_StopAllChannels:
                    for (i = 0; i < SPU_CHANNELS; i = i + 1)
                        if (channels[i].state != SPUChannelState_Stopped)
                            stop_channel (&channels[i]);
                    break;
                default: break;
            }
            refresh_channel ();
            break;

        case SPU_GlobalVolume:
            if (!isnan (f32) && !isinf (f32))
            {
                SPUP(port) -> value.f32  = clampf (f32, 0.0f, 2.0f);
            }
            break;

        case SPU_SelectedSound:
            if ((i32 >= -1) && (i32 < (int32_t) loaded_sounds))
            {
                selsound  = i32;
                SPUP(port) -> value.i32  = i32;
                refresh_sound ();
            }
            break;

        case SPU_SelectedChannel:
            if ((i32 >= 0) && (i32 < SPU_CHANNELS))
            {
                selchannel  = i32;
                SPUP(port) -> value.i32  = i32;
                refresh_channel ();
            }
            break;

        case SPU_SoundPlayWithLoop:
            s -> loop  = (i32 != 0) ? 1 : 0;
            refresh_sound ();
            break;

        case SPU_SoundLoopStart:
            i32  = clampi (i32, 0, (int32_t) s -> length - 1);
            s -> loopstart  = (i32 < s -> loopend) ? i32 : s -> loopend;
            refresh_sound ();
            break;

        case SPU_SoundLoopEnd:
            i32  = clampi (i32, 0, (int32_t) s -> length - 1);
            s -> loopend  = (i32 > s -> loopstart) ? i32 : s -> loopstart;
            refresh_sound ();
            break;

        case SPU_ChannelAssignedSound:
            if ((i32 >= -1) && (i32 < (int32_t) loaded_sounds) &&
                (c -> state == SPUChannelState_Stopped))
            {
                c -> sound  = i32;
                refresh_channel ();
            }
            break;

        case SPU_ChannelVolume:
            if (!isnan (f32) && !isinf (f32))
            {
                c -> volume  = clampf (f32, 0.0f, 8.0f);
                refresh_channel ();
            }
            break;

        case SPU_ChannelSpeed:
            if (!isnan (f32) && !isinf (f32))
            {
                c -> speed  = clampf (f32, 0.0f, 128.0f);
                refresh_channel ();
            }
            break;

        case SPU_ChannelLoopEnabled:
            c -> loop  = (i32 != 0) ? 1 : 0;
            refresh_channel ();
            break;

        case SPU_ChannelPosition:
            c -> position  = (double) clampi (i32, 0, (int32_t) sound_ptr (c -> sound) -> length - 1);
            refresh_channel ();
            break;

        default:   // SoundLength and ChannelState are read-only
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// spu_new_frame(): advance every playing channel by one frame of samples
//
void  spu_new_frame (void)
{
    int  i  = 0;
    int  n  = 0;

    for (i = 0; i < SPU_CHANNELS; i = i + 1)
    {
        channel_t  *c  = &channels[i];
        sound_t    *s  = NULL;

        if (c -> state != SPUChannelState_Playing)
        {
            continue;
        }
        s  = sound_ptr (c -> sound);
        for (n = 0; n < SPU_SAMPLES_PER_FRAME; n = n + 1)
        {
            double  prev  = c -> position;

            c -> position  = c -> position + c -> speed;
            if ((c -> loop) && (s -> loopend > s -> loopstart))
            {
                if ((prev <= s -> loopend) && (c -> position > s -> loopend))
                {
                    c -> position  = s -> loopstart +
                                     fmod (c -> position - s -> loopstart,
                                           (double) (s -> loopend - s -> loopstart));
                }
            }
            if (c -> position > (double) ((int32_t) s -> length - 1))
            {
                stop_channel (c);
                break;
            }
        }
    }
    refresh_channel ();
}

////////////////////////////////////////////////////////////////////////////////////////
//
// save states: selections, channels, and the loop settings of the loaded sounds
//
#define  SPW(v)   do { if (state_wr32 (f, (uint32_t) (v)) != 0) return (-1); } while (0)
#define  SPR(v)   do { if (state_rd32 (f, &(v)) != 0) return (-1); } while (0)

int  spu_save (FILE *f)
{
    int       i  = 0;
    uint64_t  p  = 0;

    SPW (loaded_sounds);
    SPW (selsound);
    SPW (selchannel);
    for (i = 0; i < SPU_CHANNELS; i = i + 1)
    {
        memcpy (&p, &channels[i].position, sizeof (p));
        SPW (channels[i].state);
        SPW (channels[i].sound);
        SPW (*(uint32_t *) &channels[i].volume);
        SPW (*(uint32_t *) &channels[i].speed);
        SPW (channels[i].loop);
        SPW ((uint32_t) p);
        SPW ((uint32_t) (p >> 32));
    }
    SPW (biossound.loop);  SPW (biossound.loopstart);  SPW (biossound.loopend);
    for (i = 0; i < (int) loaded_sounds; i = i + 1)
    {
        SPW (sounds[i].loop);  SPW (sounds[i].loopstart);  SPW (sounds[i].loopend);
    }
    return (0);
}

int  spu_restore (FILE *f)
{
    int       i   = 0;
    uint32_t  v   = 0;
    uint32_t  lo  = 0;
    uint32_t  hi  = 0;
    uint64_t  p   = 0;

    SPR (v);
    if (v != loaded_sounds)
    {
        return (-1);
    }
    SPR (v);  selsound    = (int32_t) v;
    SPR (v);  selchannel  = (int32_t) v;
    if ((selsound < -1) || (selsound >= (int32_t) loaded_sounds) ||
        (selchannel < 0) || (selchannel >= SPU_CHANNELS))
    {
        return (-1);
    }
    for (i = 0; i < SPU_CHANNELS; i = i + 1)
    {
        SPR (v);  channels[i].state   = (int32_t) v;
        SPR (v);  channels[i].sound   = (int32_t) v;
        SPR (v);  memcpy (&channels[i].volume, &v, sizeof (v));
        SPR (v);  memcpy (&channels[i].speed,  &v, sizeof (v));
        SPR (v);  channels[i].loop    = (int32_t) v;
        SPR (lo);
        SPR (hi);
        p  = ((uint64_t) hi << 32) | lo;
        memcpy (&channels[i].position, &p, sizeof (p));
        if ((channels[i].sound < -1) || (channels[i].sound >= (int32_t) loaded_sounds))
        {
            return (-1);
        }
    }
    SPR (v);  biossound.loop       = (int32_t) v;
    SPR (v);  biossound.loopstart  = (int32_t) v;
    SPR (v);  biossound.loopend    = (int32_t) v;
    for (i = 0; i < (int) loaded_sounds; i = i + 1)
    {
        SPR (v);  sounds[i].loop       = (int32_t) v;
        SPR (v);  sounds[i].loopstart  = (int32_t) v;
        SPR (v);  sounds[i].loopend    = (int32_t) v;
    }
    refresh_sound ();
    refresh_channel ();
    return (0);
}
