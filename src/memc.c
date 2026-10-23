#include "defines.h"
#include "memc.h"
#include <fcntl.h>
#include <errno.h>

////////////////////////////////////////////////////////////////////////////////////////
//
// memc.c: memory card persistence and the v32dev mailbox transport
//
// This follows the DesktopEmulator (V32Console::RunNextFrame) and the v32dev emulator
// patch of the v32io project:
//
//   frame boundary =   [end of frame N]   flush the dirty pages of the card to its file
//                      [start of N+1]     take the host-owned area of the mailbox from
//                                         the file (if the mailbox is live)
//
// A flush is done in two phases, so that a process that reads the file never sees an
// index that announces data which is not there yet:
//
//   phase 1   every dirty page, except the header and host-owned area of a live
//             mailbox (those belong to the host) and the 16 control words of each
//             cart-owned part
//   phase 2   the control words ("commit words": ring indices, flags) of every
//             cart-owned part that had a dirty page
//
// The file is accessed with pread()/pwrite() on one descriptor, so there is no
// stdio buffering between the simulator and another process: each write() is visible
// at once, in the order it was issued.
//
////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    int32_t  first;
    int32_t  words;
    int      phase;
} seg_t;

static struct
{
    int        fd;                           // the card file, -1 if none
    int        writable;                     // opened read-write
    char       path[1024];
    uint8_t    attached;
    uint8_t    tracking;                     // writes mark pages dirty
    uint8_t    pending;                      // some page is dirty
    uint8_t    dirty[MEMC_NUM_PAGES];
    uint8_t    mailbox;                      // header valid and not disabled
    uint8_t    disabled;                     // V32DEV_DISABLE
    uint32_t   beat;                         // emulator heartbeat
    uint32_t   host_beat[DEV_CHANNELS];      // last host heartbeat seen
    uint32_t   host_still[DEV_CHANNELS];     // syncs in a row without a change
    uint32_t   syncs;                        // host area reads
    uint32_t   changes;                      // ...of which the host had changed it
    uint32_t   flushes;
    uint64_t   words_written;
} mc = { .fd = -1 };

static seg_t     segs[2 * MEMC_NUM_PAGES + 16];
static uint8_t   iobuf[MEMC_WORDS * 4];

////////////////////////////////////////////////////////////////////////////////////////
//
// direct access to card words (the page is allocated while a card is attached)
//
static data_t  *card (void)
{
    return ((memory + V32_PAGE_MEMC) -> data);
}

static uint32_t  cw (uint32_t word)
{
    data_t  *c  = card ();

    return ((c == NULL) ? 0 : (c + word) -> value.raw);
}

static void  cw_set (uint32_t word, uint32_t value)
{
    data_t  *c  = card ();

    if (c != NULL)
    {
        (c + word) -> value.raw  = value;
    }
}

static uint32_t  le32 (const uint8_t *b)
{
    return ((uint32_t) b[0] | ((uint32_t) b[1] << 8) | ((uint32_t) b[2] << 16) | ((uint32_t) b[3] << 24));
}

////////////////////////////////////////////////////////////////////////////////////////
//
// header_valid(): does the card hold a v32dev header that matches this layout?
//
static int  header_valid (void)
{
    return ((cw (DEV_REGION_START + DEV_H_MAGIC)         == DEV_MAGIC)         &&
            (cw (DEV_REGION_START + DEV_H_VERSION)       == DEV_VERSION)       &&
            (cw (DEV_REGION_START + DEV_H_REGION_WORDS)  == DEV_REGION_WORDS)  &&
            (cw (DEV_REGION_START + DEV_H_CHANNELS)      == DEV_CHANNELS)      &&
            (cw (DEV_REGION_START + DEV_H_PART_WORDS)    == DEV_PART_WORDS)    &&
            (cw (DEV_REGION_START + DEV_H_CONTROL_WORDS) == DEV_CONTROL_WORDS));
}

////////////////////////////////////////////////////////////////////////////////////////
//
// memc_attach(): the card has been loaded into memory from this file. Open the file
// for the rest of the session, and decide whether the mailbox is live
//
void  memc_attach (const char *path)
{
    const char *env  = getenv ("V32DEV_DISABLE");

    memc_detach ();
    snprintf (mc.path, sizeof (mc.path), "%s", path);
    memset (mc.dirty, 0, sizeof (mc.dirty));
    mc.pending   = FALSE;
    mc.beat      = 0;
    mc.syncs     = 0;
    mc.changes   = 0;
    mc.flushes   = 0;
    mc.words_written = 0;
    memset (mc.host_beat,  0, sizeof (mc.host_beat));
    memset (mc.host_still, 0, sizeof (mc.host_still));

    mc.fd        = open (mc.path, O_RDWR);
    mc.writable  = (mc.fd >= 0);
    if (mc.fd < 0)
    {
        mc.fd    = open (mc.path, O_RDONLY);
        if (mc.fd >= 0)
        {
            fprintf (stderr, "[WARNING:MEMC] '%s' is read-only: changes will not be saved\n", mc.path);
        }
        else
        {
            fprintf (stderr, "[WARNING:MEMC] could not reopen '%s': changes will not be saved\n", mc.path);
        }
    }

    mc.attached  = TRUE;
    mc.tracking  = TRUE;
    mc.disabled  = ((env != NULL) && (0 != strcmp (env, "0")));
    mc.mailbox   = FALSE;

    if ((mc.disabled == FALSE) && (mc.fd >= 0) && (header_valid () != 0))
    {
        mc.mailbox  = TRUE;
        cw_set (DEV_REGION_START + DEV_H_EMU_FLAGS, 1);
        cw_set (DEV_REGION_START + DEV_H_EMU_BEAT,  0);
        fprintf (verbose, "[MEMC] v32dev mailbox active\n");
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// memc_detach(): write out what is pending and let go of the file
//
void  memc_detach (void)
{
    if (mc.attached == TRUE)
    {
        memc_flush ();
    }
    if (mc.fd >= 0)
    {
        close (mc.fd);
    }
    mc.fd        = -1;
    mc.writable  = FALSE;
    mc.attached  = FALSE;
    mc.tracking  = FALSE;
    mc.mailbox   = FALSE;
    mc.pending   = FALSE;
    memset (mc.dirty, 0, sizeof (mc.dirty));
}

uint8_t  memc_attached (void)
{
    return (mc.attached);
}

uint8_t  memc_mailbox (void)
{
    return (mc.mailbox);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// memc_touch(): a write to card word N (called from memory_set())
//
void  memc_touch (uint32_t word)
{
    if ((mc.tracking == TRUE) && (word < MEMC_WORDS))
    {
        mc.dirty[word >> MEMC_PAGE_SHIFT]  = 1;
        mc.pending                         = TRUE;
    }
}

void  memc_touch_all (void)
{
    if (mc.tracking == TRUE)
    {
        memset (mc.dirty, 1, sizeof (mc.dirty));
        mc.pending  = TRUE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// plan_save(): which card words must be written for the dirty pages. Fills segs[] and
// returns how many segments there are
//
static int  plan_save (void)
{
    int32_t   ex_first[1 + DEV_CHANNELS];     // words that phase 1 leaves out
    int32_t   ex_end[1 + DEV_CHANNELS];
    int       nex     = 0;
    int       nseg    = 0;
    int       page    = 0;
    int       first   = 0;
    int       chan    = 0;
    int       k       = 0;
    int32_t   cursor  = 0;
    int32_t   run0    = 0;
    int32_t   run1    = 0;
    int32_t   p0      = 0;
    int32_t   p1      = 0;
    int       touched = 0;

    if (mc.mailbox == TRUE)
    {
        // the header and the host-owned area are contiguous, and not ours to write
        ex_first[nex]  = DEV_REGION_START;
        ex_end[nex]    = DEV_CART_START;
        nex            = nex + 1;
        for (chan = 0; chan < DEV_CHANNELS; chan = chan + 1)
        {
            ex_first[nex]  = DEV_CART_START + chan * DEV_PART_WORDS;
            ex_end[nex]    = ex_first[nex] + DEV_CONTROL_WORDS;
            nex            = nex + 1;
        }
    }

    // phase 1: runs of dirty pages, minus the excluded words
    page = 0;
    while (page < MEMC_NUM_PAGES)
    {
        if (mc.dirty[page] == 0)
        {
            page  = page + 1;
            continue;
        }
        first = page;
        while ((page < MEMC_NUM_PAGES) && (mc.dirty[page] != 0))
        {
            page  = page + 1;
        }
        run0    = first * MEMC_PAGE_WORDS;
        run1    = page  * MEMC_PAGE_WORDS;
        cursor  = run0;
        for (k = 0; k < nex; k = k + 1)
        {
            if (ex_end[k] <= cursor)
            {
                continue;
            }
            if (ex_first[k] >= run1)
            {
                break;
            }
            if (ex_first[k] > cursor)
            {
                segs[nseg].first  = cursor;
                segs[nseg].words  = ex_first[k] - cursor;
                segs[nseg].phase  = 1;
                nseg              = nseg + 1;
            }
            if (ex_end[k] > cursor)
            {
                cursor  = ex_end[k];
            }
        }
        if (cursor < run1)
        {
            segs[nseg].first  = cursor;
            segs[nseg].words  = run1 - cursor;
            segs[nseg].phase  = 1;
            nseg              = nseg + 1;
        }
    }

    // phase 2: the commit words of every cart-owned part that was touched
    if (mc.mailbox == TRUE)
    {
        for (chan = 0; chan < DEV_CHANNELS; chan = chan + 1)
        {
            p0       = DEV_CART_START + chan * DEV_PART_WORDS;
            p1       = p0 + DEV_PART_WORDS;
            touched  = 0;
            for (page = (p0 >> MEMC_PAGE_SHIFT); page <= ((p1 - 1) >> MEMC_PAGE_SHIFT); page = page + 1)
            {
                if (mc.dirty[page] != 0)
                {
                    touched  = 1;
                }
            }
            if (touched != 0)
            {
                segs[nseg].first  = p0;
                segs[nseg].words  = DEV_CONTROL_WORDS;
                segs[nseg].phase  = 2;
                nseg              = nseg + 1;
            }
        }
    }

    return (nseg);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// put_seg(): write card words [first, first+words) to the file, little endian, at the
// file offset 8 + 4*first (after the signature)
//
static int  put_seg (const seg_t *s)
{
    int32_t  i      = 0;
    size_t   total  = (size_t) s -> words * 4;
    size_t   done   = 0;
    ssize_t  n      = 0;
    uint32_t w      = 0;

    for (i = 0; i < s -> words; i = i + 1)
    {
        w                = cw ((uint32_t) (s -> first + i));
        iobuf[4*i + 0]   = (uint8_t) w;
        iobuf[4*i + 1]   = (uint8_t) (w >> 8);
        iobuf[4*i + 2]   = (uint8_t) (w >> 16);
        iobuf[4*i + 3]   = (uint8_t) (w >> 24);
    }
    while (done < total)
    {
        n  = pwrite (mc.fd, iobuf + done, total - done, (off_t) (2 * wordsize) + (off_t) s -> first * 4 + (off_t) done);
        if (n <= 0)
        {
            if ((n < 0) && (errno == EINTR))
            {
                continue;
            }
            return (-1);
        }
        done  = done + (size_t) n;
    }
    mc.words_written  = mc.words_written + (uint64_t) s -> words;
    return (0);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// memc_flush(): write the dirty pages to the file, data before commit words
//
void  memc_flush (void)
{
    int  nseg   = 0;
    int  phase  = 0;
    int  k      = 0;
    int  bad    = 0;

    if ((mc.attached == FALSE) || (mc.pending == FALSE) || (card () == NULL))
    {
        return;
    }
    if ((mc.fd < 0) || (mc.writable == FALSE))
    {
        memset (mc.dirty, 0, sizeof (mc.dirty));
        mc.pending  = FALSE;
        return;
    }

    nseg = plan_save ();
    for (phase = 1; phase <= 2; phase = phase + 1)
    {
        for (k = 0; k < nseg; k = k + 1)
        {
            if ((segs[k].phase == phase) && (put_seg (segs + k) != 0))
            {
                bad  = 1;
            }
        }
    }
    if (bad != 0)
    {
        fprintf (stderr, "[ERROR:MEMC] error writing '%s'\n", mc.path);
    }
    else
    {
        fprintf (verbose, "[MEMC] saved '%s' (%d segments)\n", mc.path, nseg);
    }

    mc.flushes  = mc.flushes + 1;
    memset (mc.dirty, 0, sizeof (mc.dirty));
    mc.pending  = FALSE;
}

//
// memc_save(): flush at exit, unload, or crash
//
void  memc_save (void)
{
    memc_flush ();
}

////////////////////////////////////////////////////////////////////////////////////////
//
// sync_host(): start of a frame with a live mailbox. Bump the heartbeat that tells
// programs the mailbox is kept live (it lives in card memory only), and take the whole
// host-owned area from the file. A short or failed read is retried next frame. These
// words are not program writes: they are not marked dirty and never saved back
//
static void  sync_host (void)
{
    data_t   *c      = card ();
    size_t    bytes  = (size_t) DEV_HOST_WORDS * 4;
    ssize_t   n      = 0;
    int32_t   i      = 0;
    int       chan   = 0;
    uint32_t  w      = 0;
    uint32_t  beat   = 0;
    int       diff   = 0;

    if (c == NULL)
    {
        return;
    }
    mc.beat  = mc.beat + 1;
    cw_set (DEV_REGION_START + DEV_H_EMU_FLAGS, 1);
    cw_set (DEV_REGION_START + DEV_H_EMU_BEAT,  mc.beat);

    n  = pread (mc.fd, iobuf, bytes, (off_t) (2 * wordsize) + (off_t) DEV_HOST_START * 4);
    if (n != (ssize_t) bytes)
    {
        return;
    }
    for (i = 0; i < DEV_HOST_WORDS; i = i + 1)
    {
        w  = le32 (iobuf + 4*i);
        if ((c + DEV_HOST_START + i) -> value.raw != w)
        {
            (c + DEV_HOST_START + i) -> value.raw  = w;
            diff                                   = 1;
        }
    }
    mc.syncs    = mc.syncs + 1;
    mc.changes  = mc.changes + (uint32_t) diff;

    for (chan = 0; chan < DEV_CHANNELS; chan = chan + 1)
    {
        beat  = cw (DEV_HOST_START + chan * DEV_PART_WORDS + DEV_HP_BEAT);
        if (beat != mc.host_beat[chan])
        {
            mc.host_beat[chan]   = beat;
            mc.host_still[chan]  = 0;
        }
        else
        {
            mc.host_still[chan]  = mc.host_still[chan] + 1;
        }
    }
}

void  memc_power_on (void)
{
    if ((mc.attached == TRUE) && (mc.mailbox == TRUE))
    {
        sync_host ();
    }
}

void  memc_frame (void)
{
    if (mc.attached == FALSE)
    {
        return;
    }
    memc_flush ();
    if (mc.mailbox == TRUE)
    {
        sync_host ();
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// the mailbox region as one block (savestates keep the live one: see savestate.c)
//
void  memc_region_get (uint32_t *out)
{
    int32_t  i  = 0;

    for (i = 0; i < DEV_REGION_WORDS; i = i + 1)
    {
        out[i]  = cw (DEV_REGION_START + i);
    }
}

void  memc_region_put (const uint32_t *in)
{
    int32_t  i  = 0;

    for (i = 0; i < DEV_REGION_WORDS; i = i + 1)
    {
        cw_set (DEV_REGION_START + i, in[i]);
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// 'mailbox' command
//
static const char  *class_name (uint32_t cls)
{
    switch (cls)
    {
        case 0:   return ("none");
        case 1:   return ("block");
        case 2:   return ("char");
        case 3:   return ("pipe");
        case 4:   return ("socket");
        default:  return ((cls >= 128) ? "custom" : "?");
    }
}

static const char  *kind_name (uint32_t cls, uint32_t kind)
{
    if (kind == 0)  return ("any");
    switch (cls)
    {
        case 1:   return ((kind == 1) ? "storage" : "?");
        case 2:   return ((kind == 1) ? "serial"  : "?");
        case 3:   return ((kind == 1) ? "stream"  : "?");
        case 4:   return ((kind == 1) ? "tcp" : (kind == 2) ? "udp" : (kind == 3) ? "netplay" : "?");
        default:  return ("?");
    }
}

static void  status_text (uint32_t st, char *out, size_t len)
{
    snprintf (out, len, "%s%s%s%s", (st & 1) ? "READY " : "", (st & 2) ? "ERROR " : "",
                                     (st & 4) ? "HUP " : "", (st & 7) ? "" : "-");
}

static uint32_t  ring_count (uint32_t head, uint32_t tail)
{
    if ((head >= DEV_RING_WORDS) || (tail >= DEV_RING_WORDS))
    {
        return (0xFFFFFFFF);          // corrupt index
    }
    return ((head + DEV_RING_WORDS - tail) % DEV_RING_WORDS);
}

static void  show_ring (const char *name, uint32_t ring_base, uint32_t head, uint32_t tail, int chars)
{
    uint32_t  n   = ring_count (head, tail);
    uint32_t  i   = 0;
    uint32_t  w   = 0;
    uint32_t  max = 0;

    if ((n == 0) || (n == 0xFFFFFFFF))
    {
        return;
    }
    max = (n > 64) ? 64 : n;
    fprintf (stdout, "      %s pending:", name);
    for (i = 0; i < max; i = i + 1)
    {
        w  = cw (ring_base + ((tail + i) % DEV_RING_WORDS));
        fprintf (stdout, " %X", w);
    }
    if (max < n)
    {
        fprintf (stdout, " ... (%u more)", n - max);
    }
    fprintf (stdout, "\n");
    if (chars != 0)
    {
        fprintf (stdout, "      %s text   : \"", name);
        for (i = 0; i < max; i = i + 1)
        {
            w  = cw (ring_base + ((tail + i) % DEV_RING_WORDS)) & 0xFF;
            if (w == '\n')                   fprintf (stdout, "\\n");
            else if (w == '\r')              fprintf (stdout, "\\r");
            else if ((w >= 32) && (w < 127)) fprintf (stdout, "%c", (int) w);
            else                             fprintf (stdout, "\\x%02X", w);
        }
        fprintf (stdout, "\"\n");
    }
}

void  memc_show (int channel)
{
    int       chan    = 0;
    uint32_t  hp      = 0;
    uint32_t  cp      = 0;
    uint32_t  type    = 0;
    uint32_t  cls     = 0;
    uint32_t  kind    = 0;
    uint32_t  hflags  = 0;
    uint32_t  h2c_h   = 0;
    uint32_t  h2c_t   = 0;
    uint32_t  c2h_h   = 0;
    uint32_t  c2h_t   = 0;
    uint32_t  n1      = 0;
    uint32_t  n2      = 0;
    int       k       = 0;
    int       alive   = 0;
    char      hst[32];
    char      cst[32];

    if (mc.attached == FALSE)
    {
        fprintf (stdout, "No memory card is loaded.\n");
        return;
    }

    fprintf (stdout, "Memory card '%s'\n", mc.path);
    fprintf (stdout, "  file    : %s\n", (mc.fd < 0) ? "not open (changes are not saved)" :
                                         (mc.writable == FALSE) ? "read-only (changes are not saved)" : "open, read-write");
    fprintf (stdout, "  saving  : %s, %u flushes, %llu words written\n",
                     (mc.pending == TRUE) ? "changes pending until the end of the frame" : "up to date",
                     mc.flushes, (unsigned long long) mc.words_written);
    if (mc.mailbox == FALSE)
    {
        fprintf (stdout, "  mailbox : %s\n", (mc.disabled == TRUE) ? "disabled (V32DEV_DISABLE)" :
                                             (header_valid () == 0) ? "none (no v32dev header on the card)" : "off");
        return;
    }
    fprintf (stdout, "  mailbox : LIVE  emulator heartbeat %u, host area read %u times, host changed it %u times\n",
                     mc.beat, mc.syncs, mc.changes);

    for (chan = 0; chan < DEV_CHANNELS; chan = chan + 1)
    {
        if ((channel >= 0) && (channel != chan))
        {
            continue;
        }
        hp      = DEV_HOST_START + chan * DEV_PART_WORDS;
        cp      = DEV_CART_START + chan * DEV_PART_WORDS;
        type    = cw (hp + DEV_HP_TYPE);
        cls     = (type >> 24) & 255;
        kind    = type & 0x00FFFFFF;
        hflags  = cw (hp + DEV_HP_FLAGS);
        h2c_h   = cw (hp + DEV_HP_H2C_HEAD);
        h2c_t   = cw (cp + DEV_CP_H2C_TAIL);
        c2h_h   = cw (cp + DEV_CP_C2H_HEAD);
        c2h_t   = cw (hp + DEV_HP_C2H_TAIL);
        alive   = (((hflags & 1) != 0) && (mc.host_still[chan] < DEV_HOST_STILL) && (type != 0));
        status_text (cw (hp + DEV_HP_STATUS), hst, sizeof (hst));
        status_text (cw (cp + DEV_CP_STATUS), cst, sizeof (cst));

        if ((type == 0) && (channel < 0) && ((cw (cp + DEV_CP_FLAGS) & 1) == 0))
        {
            fprintf (stdout, "  ch%d  unused\n", chan);
            continue;
        }
        fprintf (stdout, "  ch%d  %s:%s (type 0x%08X)  host %s (beat %u, quiet %u frames, session %u)\n",
                         chan, class_name (cls), kind_name (cls, kind), type,
                         (alive != 0) ? "ALIVE" : ((hflags & 1) ? "STALE" : "absent"),
                         cw (hp + DEV_HP_BEAT), mc.host_still[chan], cw (hp + DEV_HP_SESSION));
        fprintf (stdout, "      program: %s (session %u)   status host [%s] program [%s]\n",
                         (cw (cp + DEV_CP_FLAGS) & 1) ? "open" : "not open", cw (cp + DEV_CP_SESSION), hst, cst);
        n1  = ring_count (h2c_h, h2c_t);
        n2  = ring_count (c2h_h, c2h_t);
        fprintf (stdout, "      host->program  head %u tail %u: ", h2c_h, h2c_t);
        if (n1 == 0xFFFFFFFF)  fprintf (stdout, "CORRUPT INDEX\n");
        else                   fprintf (stdout, "%u words waiting\n", n1);
        fprintf (stdout, "      program->host  head %u tail %u: ", c2h_h, c2h_t);
        if (n2 == 0xFFFFFFFF)  fprintf (stdout, "CORRUPT INDEX\n");
        else                   fprintf (stdout, "%u words waiting\n", n2);
        fprintf (stdout, "      host descriptor:");
        for (k = 0; k < 8; k = k + 1)
        {
            fprintf (stdout, " %X", cw (hp + DEV_HP_DESC + k));
        }
        fprintf (stdout, "\n      program request:");
        for (k = 0; k < 8; k = k + 1)
        {
            fprintf (stdout, " %X", cw (cp + DEV_CP_DESC + k));
        }
        fprintf (stdout, "\n");
        if (channel >= 0)
        {
            show_ring ("host->program", hp + DEV_CONTROL_WORDS, h2c_h, h2c_t, (cls == 2));
            show_ring ("program->host", cp + DEV_CONTROL_WORDS, c2h_h, c2h_t, (cls == 2));
        }
    }
}
