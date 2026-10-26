#include "defines.h"
#include <math.h>

////////////////////////////////////////////////////////////////////////////////////////
//
// RNG_CurrentValue, as the console implements it (V32RNG.cpp): a linear congruential
// generator with the constants of C++11's minstd_rand. The port holds the value the next
// read will return; a read returns it and moves on to
//
//     next = (current * 48271) % 0x7FFFFFFF
//
// The generator starts at 1 (and again at every reset). A write of 0 is ignored, any
// other value is stored with its sign bit cleared. Note that 0x80000000 therefore stores
// 0, which the console does not guard against: the generator then returns 0 forever.
// That is reproduced on purpose (v32sim matches the DesktopEmulator, quirks included).
//
////////////////////////////////////////////////////////////////////////////////////////
#define  GPU_POINT_MAX_X         1640        // screen width 640 + 1000
#define  GPU_POINT_MAX_Y         1360        // screen height 360 + 1000
#define  RNG_MULTIPLIER          48271
#define  RNG_MODULUS             0x7FFFFFFF
#define  PORT_MASK               0x7FF       // the control bus decodes 11 bits (G13)

static int32_t  rng_next  (int32_t  current)
{
    return ((int32_t) (((int64_t) current * RNG_MULTIPLIER) % RNG_MODULUS));
}

static int32_t  rng_stored  (int32_t  written,  int32_t  current)
{
    if (written == 0)
    {
        return (current);                      // ignored
    }
    return ((int32_t) ((uint32_t) written & 0x7FFFFFFF));
}

////////////////////////////////////////////////////////////////////////////////////////
//
// clamping as V32GPUWriters.cpp does it: out-of-range values are accepted, but corrected
//
static int32_t  clamp_i32  (int32_t  value,  int32_t  low,  int32_t  high)
{
    return ((value < low) ? low : ((value > high) ? high : value));
}

void  init_ioports  (void)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    int32_t    count               = 0;
    int32_t    index               = 0;
    int32_t    value               = 0;
    int32_t   *iptr                = 0;
    int8_t    *nptr                = NULL;
    data_t    *pptr                = NULL;
    size_t     len                 = 0;
    size_t     size                = 0;
    struct tm *current_time_tm;
    time_t     current_time_raw;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // ioports is the top-level (double pointer) nexus, each category is a single-
    // pointer array hanging off of each element of ioports.
    //
    ioports                        = (data_t **) ralloc (sizeof (data_t *),
                                                         NUM_PORT_CATEGORIES,
                                                         FLAG_RETERR);
    if (ioports                   == NULL)
    {
        fprintf (stderr, "[error] failed to allocate memory for 'ioports'\n");
        exit (IOPORTS_ALLOC_FAIL);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // gamepad ports backing store: a 4-element array of gamepad_t
    //
    size                              = sizeof (gamepad_t);
    len                               = 4;
    gamepad                           = (gamepad_t *) ralloc (size, len, FLAG_ZERO);
    for (index                        = 0;
         index                       <  V32_NUM_GAMEPADS;
         index                        = index + 1)
    {
        (gamepad+index) -> connected  = FALSE;
        for (count                    = INP_GamepadLeft    - 0x402;
             count                   <= INP_GamepadButtonR - 0x402;
             count                    = count + 1)
        {
            iptr                      = (gamepad+index) -> button;
            *(iptr+count)             = -3600; // apparent default
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // TIM ports: allocate and initialize
    //
    len                                    = NUM_TIM_PORTS;
    size                                   = sizeof (data_t);
    *(ioports+TIM_PORT)                    = (data_t *) ralloc (size, len, FLAG_NONE);
    pptr                                   = *(ioports+TIM_PORT);
    pptr -> qty                            = NUM_TIM_PORTS;

    current_time_raw                       = time (NULL); // obtain current time (raw)
    current_time_tm                        = localtime (&current_time_raw);
    for (index                             = 0;
         index                            <  NUM_TIM_PORTS;
         index                             = index + 1)
    {
        (pptr+index) -> value.i32          = 0x00000000;
        (pptr+index) -> flag               = FLAG_READ;
        (pptr+index) -> fmt                = FORMAT_UNSIGNED;
        len                                = 32;
        size                               = sizeof (int8_t);
        (pptr+index) -> name               = (int8_t *) ralloc (size, len, FLAG_NONE);
        nptr                               = (pptr+index) -> name;

        switch (TIM_CurrentDate | index)
        {
            case TIM_CurrentDate:
                value                      = current_time_tm -> tm_year + 1900;
                value                      = value << 16;
                value                     |= current_time_tm -> tm_yday;      // days count from 0
                if (fixed_clock           == TRUE)
                {
                    value                  = (fixed_year << 16) | (fixed_day & 0xFFFF);
                }
                (pptr+index) -> value.i32  = value;
                (pptr+index) -> fmt        = FORMAT_DEFAULT;
                sprintf (nptr, "TIM_CurrentDate");
                break;

            case TIM_CurrentTime:
                value                      = current_time_tm -> tm_hour * 3600;
                value                     += current_time_tm -> tm_min  * 60;
                value                     += current_time_tm -> tm_sec;
                if (fixed_clock           == TRUE)
                {
                    value                  = fixed_secs;
                }
                (pptr+index) -> value.i32  = value;
                (pptr+index) -> fmt        = FORMAT_DEFAULT;
                sprintf (nptr, "TIM_CurrentTime");
                break;

            case TIM_FrameCounter:
                sprintf (nptr, "TIM_FrameCounter");
                break;

            case TIM_CycleCounter:
                sprintf (nptr, "TIM_CycleCounter");
                break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // RNG ports: allocate and initialize
    //
    len                                    = NUM_RNG_PORTS;
    size                                   = sizeof (data_t);
    *(ioports+RNG_PORT)                    = (data_t *) ralloc (size, len, FLAG_NONE);
    pptr                                   = *(ioports+RNG_PORT);
    pptr -> qty                            = NUM_RNG_PORTS;

    (pptr+0) -> value.i32                  = 1;       // the generator never holds 0
    if (rng_seed_set                      == TRUE)    // --seed=N is a write of N to the port
    {
        (pptr+0) -> value.i32              = rng_stored ((int32_t) rng_seed, 1);
    }
    (pptr+0) -> flag                       = FLAG_READ | FLAG_WRITE;
    (pptr+0) -> fmt                        = FORMAT_SIGNED;
    len                                    = 32;
    size                                   = sizeof (int8_t);
    (pptr+0) -> name                       = (int8_t *) ralloc (size, len, FLAG_NONE);
    nptr                                   = (pptr+0) -> name;
    sprintf (nptr, "RNG_CurrentValue");

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // GPU ports: allocate and initialize
    //
    len                                    = NUM_GPU_PORTS;
    size                                   = sizeof (data_t);
    *(ioports+GPU_PORT)                    = (data_t *) ralloc (size, len, FLAG_NONE);
    pptr                                   = *(ioports+GPU_PORT);
    pptr -> qty                            = NUM_GPU_PORTS;

    for (index                             = 0;
         index                            <  NUM_GPU_PORTS;
         index                             = index + 1)
    {
        (pptr+index) -> value.i32          = 0x00000000;
        (pptr+index) -> flag               = FLAG_READ | FLAG_WRITE;
        (pptr+index) -> fmt                = FORMAT_SIGNED;
        len                                = 32;
        size                               = sizeof (int8_t);
        (pptr+index) -> name               = (int8_t *) ralloc (size, len, FLAG_NONE);
        nptr                               = (pptr+index) -> name;

        switch (GPU_Command | index)
        {
            case GPU_Command:
                (pptr+index) -> flag       = FLAG_WRITE;
                (pptr+index) -> fmt        = FORMAT_HEX;
                sprintf (nptr, "GPU_Command");
                break;

            case GPU_RemainingPixels:
                (pptr+index) -> value.i32  = 2073600;
                (pptr+index) -> flag       = FLAG_READ;
                sprintf (nptr, "GPU_RemainingPixels");
                break;

            case GPU_ClearColor:
                (pptr+index) -> value.i32  = 0xFF000000; // opaque black (0xAABBGGRR)
                sprintf (nptr, "GPU_ClearColor");
                (pptr+index) -> fmt        = FORMAT_HEX;
                break;

            case GPU_MultiplyColor:
                (pptr+index) -> value.i32  = 0xFFFFFFFF;
                sprintf (nptr, "GPU_MultiplyColor");
                break;

            case GPU_ActiveBlending:
                (pptr+index) -> value.i32  = 0x20;
                sprintf (nptr, "GPU_ActiveBlending");
                break;

            case GPU_SelectedTexture:
                (pptr+index) -> value.i32  = -1;
                sprintf (nptr, "GPU_SelectedTexture");
                break;

            case GPU_SelectedRegion:
                sprintf (nptr, "GPU_SelectedRegion");
                break;

            case GPU_DrawingPointX:
                sprintf (nptr, "GPU_DrawingPointX");
                break;

            case GPU_DrawingPointY:
                sprintf (nptr, "GPU_DrawingPointY");
                break;

            case GPU_DrawingScaleX:
                (pptr+index) -> value.f32  = 1.0;
                (pptr+index) -> fmt        = FORMAT_FLOAT;
                sprintf (nptr, "GPU_DrawingScaleX");
                break;

            case GPU_DrawingScaleY:
                (pptr+index) -> value.f32  = 1.0;
                (pptr+index) -> fmt        = FORMAT_FLOAT;
                sprintf (nptr, "GPU_DrawingScaleY");
                break;

            case GPU_DrawingAngle:
                (pptr+index) -> value.f32  = 0.0;
                (pptr+index) -> fmt        = FORMAT_FLOAT;
                sprintf (nptr, "GPU_DrawingAngle");
                break;

            case GPU_RegionMinX:
                sprintf (nptr, "GPU_RegionMinX");
                break;

            case GPU_RegionMinY:
                sprintf (nptr, "GPU_RegionMinY");
                break;

            case GPU_RegionMaxX:
                sprintf (nptr, "GPU_RegionMaxX");
                break;

            case GPU_RegionMaxY:
                sprintf (nptr, "GPU_RegionMaxY");
                break;

            case GPU_RegionHotspotX:
                sprintf (nptr, "GPU_RegionHotspotX");
                break;

            case GPU_RegionHotspotY:
                sprintf (nptr, "GPU_RegionHotspotY");
                break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // SPU ports: allocate and initialize
    //
    len                                    = NUM_SPU_PORTS;
    size                                   = sizeof (data_t);
    *(ioports+SPU_PORT)                    = (data_t *) ralloc (size, len, FLAG_NONE);
    pptr                                   = *(ioports+SPU_PORT);
    pptr -> qty                            = NUM_SPU_PORTS;

    for (index                             = 0;
         index                            <  NUM_SPU_PORTS;
         index                             = index + 1)
    {
        (pptr+index) -> value.i32          = 0x00000000;
        (pptr+index) -> flag               = FLAG_READ | FLAG_WRITE;
        (pptr+index) -> fmt                = FORMAT_SIGNED;
        len                                = 32;
        size                               = sizeof (int8_t);
        (pptr+index) -> name               = (int8_t *) ralloc (size, len, FLAG_NONE);
        nptr                               = (pptr+index) -> name;

        switch (SPU_Command | index)
        {
            case SPU_Command:
                (pptr+index) -> flag       = FLAG_WRITE;
                (pptr+index) -> fmt        = FORMAT_HEX;
                sprintf (nptr, "SPU_Command");
                break;

            case SPU_GlobalVolume:
                (pptr+index) -> value.f32  = 1.0;
                sprintf (nptr, "SPU_GlobalVolume");
                break;

            case SPU_SelectedSound:
                (pptr+index) -> value.i32  = -1;
                sprintf (nptr, "SPU_SelectedSound");
                break;

            case SPU_SelectedChannel:
                sprintf (nptr, "SPU_SelectedChannel");
                break;

            case SPU_SoundLength:
                (pptr+index) -> flag       = FLAG_READ;
                sprintf (nptr, "SPU_SoundLength");
                break;

            case SPU_SoundPlayWithLoop:
                sprintf (nptr, "SPU_SoundPlayWithLoop");
                break;

            case SPU_SoundLoopStart:
                sprintf (nptr, "SPU_SoundLoopStart");
                break;

            case SPU_SoundLoopEnd:
                sprintf (nptr, "SPU_SoundLoopEnd");
                break;

            case SPU_ChannelState:
                (pptr+index) -> flag       = FLAG_READ;
                (pptr+index) -> value.i32  = 0x40;
                sprintf (nptr, "SPU_ChannelState");
                break;

            case SPU_ChannelAssignedSound:
                (pptr+index) -> value.i32  = -1;
                sprintf (nptr, "SPU_ChannelAssignedSound");
                break;

            case SPU_ChannelVolume:
                (pptr+index) -> value.f32  = 0.5;
                sprintf (nptr, "SPU_ChannelVolume");
                break;

            case SPU_ChannelSpeed:
                (pptr+index) -> value.f32  = 1.0;
                sprintf (nptr, "SPU_ChannelSpeed");
                break;

            case SPU_ChannelLoopEnabled:
                sprintf (nptr, "SPU_ChannelLoopEnabled");
                break;

            case SPU_ChannelPosition:
                (pptr+index) -> value.f32  = 0.0;
                sprintf (nptr, "SPU_ChannelPosition");
                break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // INP ports: allocate and initialize
    //
    len                                    = NUM_INP_PORTS;
    size                                   = sizeof (data_t);
    *(ioports+INP_PORT)                    = (data_t *) ralloc (size, len, FLAG_NONE);
    pptr                                   = *(ioports+INP_PORT);
    pptr -> qty                            = NUM_INP_PORTS;

    for (index                             = 0;
         index                            <  NUM_INP_PORTS;
         index                             = index + 1)
    {
        (pptr+index) -> value.i32          = -3600; // not pressed, negative value
        (pptr+index) -> flag               = FLAG_READ;
        (pptr+index) -> fmt                = FORMAT_SIGNED;
        len                                = 32;
        size                               = sizeof (int8_t);
        (pptr+index) -> name               = (int8_t *) ralloc (size, len, FLAG_NONE);
        nptr                               = (pptr+index) -> name;

        switch (INP_SelectedGamepad | index)
        {
            case INP_SelectedGamepad:
                (pptr+index) -> value.i32  = 0;
                (pptr+index) -> flag       = FLAG_READ | FLAG_WRITE;
                sprintf (nptr, "INP_SelectedGamepad");
                break;

            case INP_GamepadConnected:
                (pptr+index) -> value.i32  = 0;
                (pptr+index) -> fmt        = FORMAT_BOOLEAN;
                sprintf (nptr, "INP_GamepadConnected");
                break;

            case INP_GamepadLeft:
                sprintf (nptr, "INP_GamepadLeft");
                break;

            case INP_GamepadRight:
                sprintf (nptr, "INP_GamepadRight");
                break;

            case INP_GamepadUp:
                sprintf (nptr, "INP_GamepadUp");
                break;

            case INP_GamepadDown:
                sprintf (nptr, "INP_GamepadDown");
                break;

            case INP_GamepadButtonStart:
                sprintf (nptr, "INP_GamepadButtonStart");
                break;

            case INP_GamepadButtonA:
                sprintf (nptr, "INP_GamepadButtonA");
                break;

            case INP_GamepadButtonB:
                sprintf (nptr, "INP_GamepadButtonB");
                break;

            case INP_GamepadButtonX:
                sprintf (nptr, "INP_GamepadButtonX");
                break;

            case INP_GamepadButtonY:
                sprintf (nptr, "INP_GamepadButtonY");
                break;

            case INP_GamepadButtonL:
                sprintf (nptr, "INP_GamepadButtonL");
                break;

            case INP_GamepadButtonR:
                sprintf (nptr, "INP_GamepadButtonR");
                break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // CAR ports: allocate and initialize
    //
    len                                    = NUM_CAR_PORTS;
    size                                   = sizeof (data_t);
    *(ioports+CAR_PORT)                    = (data_t *) ralloc (size, len, FLAG_NONE);
    pptr                                   = *(ioports+CAR_PORT);
    pptr -> qty                            = NUM_CAR_PORTS;

    for (index                             = 0;
         index                            <  NUM_CAR_PORTS;
         index                             = index + 1)
    {
        (pptr+index) -> value.i32          = 0x00000000;
        (pptr+index) -> flag               = FLAG_READ;
        (pptr+index) -> fmt                = FORMAT_UNSIGNED;
        len                                = 32;
        size                               = sizeof (int8_t);
        (pptr+index) -> name               = (int8_t *) ralloc (size, len, FLAG_NONE);
        nptr                               = (pptr+index) -> name;

        switch (CAR_Connected | index)
        {
            case CAR_Connected:
                (pptr+index) -> fmt        = FORMAT_BOOLEAN;
                sprintf (nptr, "CAR_Connected");
                break;

            case CAR_ProgramROMSize:
                sprintf (nptr, "CAR_ProgramROMSize");
                break;

            case CAR_NumberOfTextures:
                (pptr+index) -> fmt        = FORMAT_SIGNED;
                sprintf (nptr, "CAR_NumberOfTextures");
                break;

            case CAR_NumberOfSounds:
                (pptr+index) -> fmt        = FORMAT_SIGNED;
                sprintf (nptr, "CAR_NumberOfSounds");
                break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // MEM ports: allocate and initialize
    //
    len                                    = NUM_MEM_PORTS;
    size                                   = sizeof (data_t);
    *(ioports+MEM_PORT)                    = (data_t *) ralloc (size, len, FLAG_NONE);
    pptr                                   = *(ioports+MEM_PORT);
    pptr -> qty                            = NUM_MEM_PORTS;
    pptr -> value.i32                      = 0x00000000;
    pptr -> flag                           = FLAG_READ;
    pptr -> fmt                            = FORMAT_BOOLEAN;
    len                                    = 32;
    size                                   = sizeof (int8_t);
    pptr -> name                           = (int8_t *) ralloc (size, len, FLAG_NONE);
    nptr                                   = pptr -> name;
    sprintf (nptr, "MEM_Connected");

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // the console's first frame starts with the timer's change-of-frame step, so a
    // program sees TIM_FrameCounter = 1 in its first frame
    //
    timer_change_frame ();
}

////////////////////////////////////////////////////////////////////////////////////////
//
// ioports_chk(): check that the address being transacted is valid
//
// returns a TRUE or FALSE
//
uint8_t  ioports_chk  (uint16_t  portaddr, uint8_t  mode, uint8_t  sys_force)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize local variables
    //
    uint16_t  type          = (portaddr & 0x0700) >> 8;  // port category
    uint16_t  attr          = (portaddr & 0x00FF);       // item within category
    uint8_t   flag          = FLAG_NONE;                 // short form access
    uint8_t   result        = TRUE;
    data_t   *pptr          = NULL;                      // pointer for sanity
    int8_t    error         = ERROR_NONE;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // ensure this function was called correctly (specify specific mode of
    // ioport transaction)
    //
    if ((mode              != FLAG_READ)  &&
        (mode              != FLAG_WRITE))
    {
        fprintf (stderr, "[ERROR] invalid IOPorts access mode\n");
        exit (IOPORTS_ACCESS_ERROR);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // based on the mode, set the likely error to occur (if an error occurs)
    //
    if (mode               == FLAG_READ)
    {
        error               = ERROR_PORT_READ;
    }
    else
    {
        error               = ERROR_PORT_WRITE;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // make sure the port being requested is a valid port within the category
    //
    if (type               <  7)
    {
        pptr                = *(ioports+type);
        if (attr           >= (pptr+0) -> qty)
        {
            fprintf (verbose, "[ERROR] invalid %.3s port 0x%.3hX\n",
                              (pptr+0) -> name, portaddr);
            sys_error       = error;
            result          = FALSE;
        }
    }
    else
    {
        fprintf (verbose, "[ERROR] invalid port 0x%.3hX\n", portaddr);
        sys_error           = error;
        result              = FALSE;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // if we get here: valid port within category, valid access method, now check
    // to make sure it is valid for the read/write transaction requested
    //
    if (result             != FALSE)
    {
        pptr                = *(ioports+type);
        flag                = (pptr+attr) -> flag;
        if ((flag & mode)  != mode)
        {
            if (sys_force  == FALSE)
            {
                if (mode   == FLAG_READ)
                {
                    fprintf (verbose, "[ERROR] port '%s' not accessible via READ!\n",
                                     (pptr+attr) -> name);
                }
                else
                {
                    fprintf (verbose, "[ERROR] port '%s' not accessible via WRITE!\n",
                                     (pptr+attr) -> name);
                }
                sys_error   = error;
                result      = FALSE;
            }
        }
    }

    return (result);
}

int16_t  ioports_num (uint8_t *symbol)
{
    data_t   *dtmp         = NULL;
    int32_t   count        = 0;
    int16_t   portnum      = -1;
    uint32_t  value        = 0;

    switch (symbol[0])
    {
        case 'T': // TIM_
        case 't': // TIM_
            value          = TIM_PORT;
            break;

        case 'R': // RNG_
        case 'r': // RNG_
            value          = RNG_PORT;
            break;

        case 'G': // GPU_
        case 'g': // GPU_
            value          = GPU_PORT;
            break;

        case 'S': // SPU_
        case 's': // SPU_
            value          = SPU_PORT;
            break;

        case 'I': // INP_
        case 'i': // INP_
            value          = INP_PORT;
            break;

        case 'C': // CAR_
        case 'c': // CAR_
            value          = CAR_PORT;
            break;

        case 'M': // MEM_
        case 'm': // MEM_
            value          = MEM_PORT;
            break;

        default:
            value          = -1;
            break;
    }

    if (value             != -1)
    {
        dtmp               = *(ioports+value);
        for (count         = 0;
             count        <  dtmp -> qty;
             count         = count + 1)
        {
            if (0         == (strcasecmp ((dtmp+count) -> name, symbol)))
            {
                portnum    = (value << 8) | count;
                break;
            }
        }
    }

    return (portnum);
}

data_t  *ioports_ptr  (uint16_t  portaddr)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize local variables
    //
    uint16_t  type          = (portaddr & 0x0700) >> 8;  // port category
    uint16_t  attr          = (portaddr & 0x00FF);       // item within category
    data_t   *result        = NULL;

    if (ioports_chk (portaddr, FLAG_READ, TRUE) == TRUE)
    {
        result              = (*(ioports+type)) + attr;
    }

    return (result);
}

word_t  ioports_get  (uint16_t  portaddr, uint8_t  sys_force)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize local variables
    //
    data_t   *pptr                        = NULL;   // pointer for sanity
    uint16_t  type                        = 0;      // port category
    uint16_t  attr                        = 0;      // item within category
    uint8_t   check                       = FALSE;  // valid port status
    word_t    result;

    portaddr                              = portaddr & PORT_MASK;
    result.raw                            = 0x00000000;

    check                                 = ioports_chk (portaddr, FLAG_READ, sys_force);
    if (check                            == TRUE)
    {
        type                              = (portaddr & 0x0700) >> 8;
        attr                              = (portaddr & 0x00FF);
        pptr                              = *(ioports+type);
        switch (portaddr)
        {
            ////////////////////////////////////////////////////////////////////////////
            //
            // all the ports that transact in floating point values
            //
            case GPU_DrawingScaleX:
            case GPU_DrawingScaleY:
            case GPU_DrawingAngle:
            case SPU_GlobalVolume:
            case SPU_ChannelVolume:
            case SPU_ChannelSpeed:
                result.f32                = (pptr+attr) -> value.f32;
                break;

            case RNG_CurrentValue: // return the current value, then step the generator
                result.i32                = (pptr+attr) -> value.i32;
                (pptr+attr) -> value.i32  = rng_next (result.i32);
                break;

            default:
                result.i32                = (pptr+attr) -> value.i32;
                break;
        }
    }

    return (result);
}

uint8_t  ioports_set (uint16_t  portaddr, int32_t  i32, float  f32, uint8_t  sys_force)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize local variables
    //
    uint32_t  value                    = 0;
    uint16_t  type                     = (portaddr & 0x0700) >> 8;  // port category
    uint16_t  attr                     = (portaddr & 0x00FF);
    int32_t   num                      = 0;
    int32_t   id                       = 0;
    uint8_t   check                    = FALSE;
    data_t   *pptr                     = *(ioports+type);           // port pointer
    region_t *rptr                     = NULL;                      // region pointer
    int32_t  *iptr                     = NULL;                      // integer ptr
    int32_t  *bptr                     = NULL;                      // button ptr (int)
    float    *fptr                     = NULL;                      // float pointer

    portaddr                           = portaddr & PORT_MASK;
    type                               = (portaddr & 0x0700) >> 8;
    attr                               = (portaddr & 0x00FF);
    pptr                               = *(ioports+type);
    check                              = ioports_chk (portaddr, FLAG_WRITE, sys_force);
    if (check                         == TRUE)
    {
        iptr                           = &((pptr+attr) -> value.i32);
        fptr                           = &((pptr+attr) -> value.f32);
        switch (portaddr)
        {
            case RNG_CurrentValue:
                *iptr                  = rng_stored (i32, *iptr);
                break;

            case GPU_Command:

                ////////////////////////////////////////////////////////////////////////
                //
                // Execute the GPU command
                //
                *iptr                  = i32;

                gpu_command (i32);
                break;

            case GPU_SelectedTexture:

                ////////////////////////////////////////////////////////////////////////
                //
                // bounds check 1: ensure the specified  texture id falls  within
                // the range indicated by the technical specifications  (possible
                // technical limits);  exceeding these  bounds will  result  in a
                // silent fail (transaction is ignored)
                //
                if ((i32              <  -1) ||
                    (i32              >= 256))
                {
                    fprintf (debug, "[ioports_set] invalid texture id %d\n", i32);
                    break;
                }

                ////////////////////////////////////////////////////////////////////////
                //
                // sanity check: if the texture specified is a valid id, but CART
                // either lacks textures or is not loaded, silently fail  (ignore
                // the transaction)
                //
                if ((i32              >= 0) &&
                    (cart_vtex        == NULL))
                {
                    fprintf (debug, "[ioports_set] no CART textures present (%d)\n", i32);
                    break;
                }

                ////////////////////////////////////////////////////////////////////////
                //
                // bounds check 2: ensure the (guaranteed now to be technically
                // valid in-range according to specifications) texture id falls
                // within the range of available textures. Deviations from this
                // range will result in a silent fail (transaction is ignored)
                //
                if ((cart_vtex        != NULL) &&
                    (i32              >= (cart_vtex)  -> qty))
                {
                    fprintf (debug, "[ioports_set] invalid CART texture id (%d)\n", i32);
                    break;
                }
                
                ////////////////////////////////////////////////////////////////////////
                //
                // Back up current selected region data in current texture
                //
                num                    = IPORTGET(GPU_SelectedTexture);
                id                     = IPORTGET(GPU_SelectedRegion);
                if (num               == -1)
                {
                    rptr               = bios_vtex -> region;
                }
                else if ((num         >= 0)   &&
                         (num         <  256) &&
                         (num         <= (cart_vtex) -> qty))
                {
                    rptr               = (cart_vtex+num) -> region;
                }

                if ((rptr             != NULL) &&
                    (id               >= 0)    &&
                    (id               <  4096))
                {
                    (rptr+id) -> minX  = IPORTGET(GPU_RegionMinX);
                    (rptr+id) -> minY  = IPORTGET(GPU_RegionMinY);
                    (rptr+id) -> maxX  = IPORTGET(GPU_RegionMaxX);
                    (rptr+id) -> maxY  = IPORTGET(GPU_RegionMaxY);
                    (rptr+id) -> hotX  = IPORTGET(GPU_RegionHotspotX);
                    (rptr+id) -> hotY  = IPORTGET(GPU_RegionHotspotY);
                }

                ////////////////////////////////////////////////////////////////////////
                //
                // Set the (texture) port
                //
                *iptr                  = i32;
                num                    = i32;

                ////////////////////////////////////////////////////////////////////////
                //
                // Restore region data based on selected texture
                //
                if (num               == -1)
                {
                    rptr               = bios_vtex       -> region;
                }
                else
                {
                    rptr               = (cart_vtex+num) -> region;
                }

                if (rptr              == NULL)
                {
                    fprintf (debug, "[ioports_set] region ptr is NULL\n");
                    break;
                }

                if ((id               >= 0) &&
                    (id               <  4096))
                {
                    PORTSET(GPU_RegionMinX,     (rptr+id) -> minX);
                    PORTSET(GPU_RegionMinY,     (rptr+id) -> minY);
                    PORTSET(GPU_RegionMaxX,     (rptr+id) -> maxX);
                    PORTSET(GPU_RegionMaxY,     (rptr+id) -> maxY);
                    PORTSET(GPU_RegionHotspotX, (rptr+id) -> hotX);
                    PORTSET(GPU_RegionHotspotY, (rptr+id) -> hotY);
                }
                break;

            case GPU_SelectedRegion:
                ////////////////////////////////////////////////////////////////////////
                //
                // region bounds check: ensure the region id falls with  the
                // allowed range; if not, silently fail (ignore transaction)
                //
                if ((i32              <  0) ||
                    (i32              >= 4096))
                {
                    fprintf (debug, "[ioports_set] invalid region id %d\n", i32);
                    break;
                }

                ////////////////////////////////////////////////////////////////////////
                //
                // ensure that the selected texture exists (especially if 
                // referencing a CART-based one)
                //
                num                    = IPORTGET(GPU_SelectedTexture);
                if ((num              <= -2)   ||
                    (num              >= 256))
                {
                    fprintf (debug, "[ioports_set] invalid texture id %d\n", num);
                    break;
                }

                ////////////////////////////////////////////////////////////////////////
                //
                // if a CART texture, make sure there are CART textures to access
                //
                if ((num              >= 0) &&
                    (cart_vtex        == NULL))
                {
                    fprintf (debug, "[ioports_set] no CART textures loaded\n");
                    break;
                }

                ////////////////////////////////////////////////////////////////////////
                //
                // if a CART texture, make sure the texture is valid based on CART
                //
                if ((num              >= 0)    &&
                    (cart_vtex        != NULL) &&
                    (num              >= cart_vtex -> qty))
                {
                    fprintf (debug, "[ioports_set] invalid texture id %d\n", num);
                    break;
                }

                ////////////////////////////////////////////////////////////////////////
                //
                // Back up current selected region data in current texture
                //
                id                     = IPORTGET(GPU_SelectedRegion);
                if (num               == -1)
                {
                    rptr               = bios_vtex       -> region;
                }
                else
                {
                    rptr               = (cart_vtex+num) -> region;
                }

                if (rptr              != NULL)
                {
                    (rptr+id) -> minX  = IPORTGET(GPU_RegionMinX);
                    (rptr+id) -> minY  = IPORTGET(GPU_RegionMinY);
                    (rptr+id) -> maxX  = IPORTGET(GPU_RegionMaxX);
                    (rptr+id) -> maxY  = IPORTGET(GPU_RegionMaxY);
                    (rptr+id) -> hotX  = IPORTGET(GPU_RegionHotspotX);
                    (rptr+id) -> hotY  = IPORTGET(GPU_RegionHotspotY);
                }

                ////////////////////////////////////////////////////////////////////////
                //
                // Set the (region) port
                //
                *iptr                  = i32;
                id                     = i32;

                ////////////////////////////////////////////////////////////////////////
                //
                // Restore region data based on selected region
                //
                if (num               == -1)
                {
                    rptr               = bios_vtex       -> region;
                }
                else
                {
                    rptr               = (cart_vtex+num) -> region;
                }

                if (rptr              != NULL)
                {
                    PORTSET(GPU_RegionMinX,     (rptr+id) -> minX);
                    PORTSET(GPU_RegionMinY,     (rptr+id) -> minY);
                    PORTSET(GPU_RegionMaxX,     (rptr+id) -> maxX);
                    PORTSET(GPU_RegionMaxY,     (rptr+id) -> maxY);
                    PORTSET(GPU_RegionHotspotX, (rptr+id) -> hotX);
                    PORTSET(GPU_RegionHotspotY, (rptr+id) -> hotY);
                }
                break;

            ////////////////////////////////////////////////////////////////////////////
            //
            // all the ports that transact in floating point values
            //
            case GPU_DrawingScaleX:
            case GPU_DrawingScaleY:
            case GPU_DrawingAngle:
                ////////////////////////////////////////////////////////////////////////
                //
                // NaN and infinity are ignored, anything else is clamped to +-1024
                //
                if ((isnan (f32) != 0) || (isinf (f32) != 0))
                {
                    break;
                }
                *fptr             = (f32 < -1024.0f) ? -1024.0f : ((f32 > 1024.0f) ? 1024.0f : f32);
                break;

            case GPU_ActiveBlending:
                ////////////////////////////////////////////////////////////////////////
                //
                // only the three blending modes (0x20 alpha, 0x21 add, 0x22 subtract)
                // are accepted; any other value is ignored, not stored
                //
                if ((i32 >= 0x20) && (i32 <= 0x22))
                {
                    *iptr         = i32;
                }
                break;

            case GPU_DrawingPointX:
                *iptr             = clamp_i32 (i32, -1000, GPU_POINT_MAX_X);
                break;

            case GPU_DrawingPointY:
                *iptr             = clamp_i32 (i32, -1000, GPU_POINT_MAX_Y);
                break;

            case GPU_RegionMinX:
            case GPU_RegionMinY:
            case GPU_RegionMaxX:
            case GPU_RegionMaxY:
                *iptr             = clamp_i32 (i32, 0, 1023);        // inside the texture
                break;

            case GPU_RegionHotspotX:
            case GPU_RegionHotspotY:
                *iptr             = clamp_i32 (i32, -1024, 2047);    // one texture either side
                break;

            ////////////////////////////////////////////////////////////////////////////
            //
            // SPU ports have behavior of their own (see spu.c)
            //
            case SPU_Command:
            case SPU_GlobalVolume:
            case SPU_SelectedSound:
            case SPU_SelectedChannel:
            case SPU_SoundLength:
            case SPU_SoundPlayWithLoop:
            case SPU_SoundLoopStart:
            case SPU_SoundLoopEnd:
            case SPU_ChannelState:
            case SPU_ChannelAssignedSound:
            case SPU_ChannelVolume:
            case SPU_ChannelSpeed:
            case SPU_ChannelLoopEnabled:
            case SPU_ChannelPosition:
                spu_write (portaddr, i32, f32);
                break;

            case INP_SelectedGamepad:
                if ((i32         <  0) ||
                    (i32         >  3))
                {
                    fprintf (debug, "[ioports_set] invalid gamepad id %d\n", i32);
                    break;
                }

                ////////////////////////////////////////////////////////////////////////
                //
                // obtain current selected Gamepad
                //
                id                = IPORTGET(INP_SelectedGamepad);

                ////////////////////////////////////////////////////////////////////////
                //
                // check that the current gamepad is connected
                //
                check             = IPORTGET(INP_GamepadConnected);

                ////////////////////////////////////////////////////////////////////////
                //
                // update the port to reflect the new selected Gamepad
                //
                *iptr             = i32;

                ////////////////////////////////////////////////////////////////////////
                //
                // restore the selected gamepad's connected state from backing store
                //
                SYSPORTSET(INP_GamepadConnected, (gamepad+i32) -> connected);

                ////////////////////////////////////////////////////////////////////////
                //
                // back up existing gamepad values to gamepad backing store; 
                // restore current gamepad values in SelectedGamepad from 
                // backing store
                //
                for (value             = INP_GamepadLeft;
                     value            <= INP_GamepadButtonR;
                     value             = value + 1)
                {
                    num                = value - 0x402;
                    bptr               = (gamepad+i32) -> button;
                    SYSPORTSET(value, *(bptr+num));
                }
                break;

            case INP_GamepadConnected:
                ////////////////////////////////////////////////////////////////////////
                //
                // obtain current selected Gamepad
                //
                id                     = IPORTGET(INP_SelectedGamepad);
                if ((id               <  0) ||
                    (id               >  3))
                {
                    fprintf (debug, "[ioports_set] invalid gamepad id %d\n", id);
                    break;
                }

                ////////////////////////////////////////////////////////////////////////
                //
                // update the port to reflect the current selected Gamepad state
                //
                *iptr                  = i32;

                ////////////////////////////////////////////////////////////////////////
                //
                // update the gamepad backing store; a gamepad that is disconnected
                // has all its controls released (a minute unpressed)
                //
                (gamepad+id) -> connected  = (i32 != 0) ? 1 : 0;
                if (i32                   == 0)
                {
                    gamepad_reset ((uint8_t) id);
                }
                break;

            case INP_GamepadLeft:
            case INP_GamepadRight:
            case INP_GamepadUp:
            case INP_GamepadDown:
            case INP_GamepadButtonStart:
            case INP_GamepadButtonA:
            case INP_GamepadButtonB:
            case INP_GamepadButtonX:
            case INP_GamepadButtonY:
            case INP_GamepadButtonL:
            case INP_GamepadButtonR:
                ////////////////////////////////////////////////////////////////////////
                //
                // obtain current selected Gamepad
                //
                id                     = IPORTGET(INP_SelectedGamepad);

                if ((id               <  0) ||
                    (id               >  3))
                {
                    fprintf (debug, "[ioports_set] invalid gamepad id %d\n", i32);
                    break;
                }

                ////////////////////////////////////////////////////////////////////////
                //
                // obtain current selected Gamepad
                //
                check                  = IPORTGET(INP_GamepadConnected);
                if (check             != TRUE)
                {
                    fprintf (debug, "[ioports_set] gamepad %d not connected\n", id);
                    break;
                }

                ////////////////////////////////////////////////////////////////////////
                //
                // update the port to reflect the current selected Gamepad state
                //
                *iptr                  = i32;

                ////////////////////////////////////////////////////////////////////////
                //
                // update the gamepad backing store
                //
                bptr                   = (gamepad+id) -> button;
                *(bptr+(attr-2))       = i32;
                break;

            default: // catch all- the standard transaction for external setting
                *iptr                  = i32;
                break;
        }
    }

    return (check);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// update_ioports(): maintenance function to be run after each frame has completed
//
void  update_ioports (void)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize local variables
    //
    int32_t   index                     = 0;
    int32_t   button                    = 0;     // specific port in the category
    int32_t  *iptr                      = NULL;
    uint8_t  *cptr                      = NULL;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // each frame the count of every control moves away from 0 by 1 (be it negative -
    // not pressed, or positive - pressed), within a +/-3600 range (a minute)
    //
    for (index                          = 0;
         index                         <  V32_NUM_GAMEPADS;
         index                          = index + 1)
    {
        cptr                            = &(gamepad+index) -> connected;
        if (*cptr                      == TRUE) // only update if connected
        {
            for (button                 = INP_GamepadLeft    - 0x402;
                 button                <= INP_GamepadButtonR - 0x402;
                 button                 = button + 1)
            {
                iptr                    = (gamepad+index) -> button;
                if (*(iptr+button)     <  0) // if button NOT pressed
                {
                    *(iptr+button)      = *(iptr+button) - 1;
                    if (*(iptr+button) <  -3600)
                    {
                        *(iptr+button)  = -3600;
                    }
                }
                else
                {
                    *(iptr+button)      = *(iptr+button) + 1;
                    if (*(iptr+button) >  3600)
                    {
                        *(iptr+button)  = 3600;
                    }
                }
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // v32io devices (a v32kbd reports one key event per frame) set their controls
    // now: a control they change is seen by the program with a count of 1 in this
    // frame, as when the DesktopEmulator applies host input before it runs a frame
    //
    v32io_frame ();

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // the ports show the selected gamepad
    //
    index                               = IPORTGET(INP_SelectedGamepad);
    if ((index                         >= 0) &&
        (index                         <  V32_NUM_GAMEPADS))
    {
        gamepad_publish ((uint8_t) index);
    }
}
