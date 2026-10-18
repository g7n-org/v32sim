////////////////////////////////////////////////////////////////////////////////////////
//
// savestate.c: save and restore the simulated machine
//
// A state file holds the registers and program counter, all IOPort values, RAM and the
// memory card (zero runs compressed), the GPU region tables and framebuffer, the SPU
// channel/sound settings, and the gamepad state. It also records a checksum of the BIOS
// and cartridge loaded when it was written: a state is only restored into a machine
// that has the same BIOS and cartridge.
//
// Not part of a state: breakpoints, watchpoints, the display list, labels, the
// backtrace and profiling statistics (those belong to the debugging session, not the
// machine).
//
// All words are written little endian.
//
////////////////////////////////////////////////////////////////////////////////////////
#include "defines.h"
#include "globals.h"
#include "ioports.h"
#include "vtex.h"

#define  SAVE_MAGIC              "V32SIMSV"
#define  SAVE_VERSION            1
#define  RAM_WORDS               (4 * 1024 * 1024)

static const uint32_t  port_qty[NUM_PORT_CATEGORIES] =
{
    NUM_TIM_PORTS, NUM_RNG_PORTS, NUM_GPU_PORTS, NUM_SPU_PORTS,
    NUM_INP_PORTS, NUM_CAR_PORTS, NUM_MEM_PORTS
};

////////////////////////////////////////////////////////////////////////////////////////
//
// word I/O helpers (shared with gpu.c and spu.c through globals.h)
//
int  state_wr32 (FILE *f, uint32_t v)
{
    uint8_t  b[4] = { (uint8_t) v, (uint8_t) (v >> 8), (uint8_t) (v >> 16), (uint8_t) (v >> 24) };

    return ((fwrite (b, 1, 4, f) == 4) ? 0 : -1);
}

int  state_rd32 (FILE *f, uint32_t *v)
{
    uint8_t  b[4];

    if (fread (b, 1, 4, f) != 4)
    {
        return (-1);
    }
    *v  = (uint32_t) b[0] | ((uint32_t) b[1] << 8) | ((uint32_t) b[2] << 16) | ((uint32_t) b[3] << 24);
    return (0);
}

#define  W(v)   do { if (state_wr32 (f, (uint32_t) (v)) != 0) return (-1); } while (0)
#define  R(v)   do { if (state_rd32 (f, &(v)) != 0) return (-1); } while (0)

////////////////////////////////////////////////////////////////////////////////////////
//
// memory ranges: (zero run length, literal count, literals) groups until 'words' are done
//
static int  write_range (FILE *f, uint32_t first, uint32_t words)
{
    uint32_t  i      = 0;
    uint32_t  zeros  = 0;
    uint32_t  lits   = 0;
    uint32_t  j      = 0;
    word_t   *w      = NULL;

    while (i < words)
    {
        zeros  = 0;
        while ((i + zeros < words) &&
               ((w = memory_get (first + i + zeros, TRUE)) != NULL) && (w -> raw == 0))
        {
            zeros  = zeros + 1;
        }
        lits  = 0;
        while ((i + zeros + lits < words) &&
               ((w = memory_get (first + i + zeros + lits, TRUE)) != NULL) && (w -> raw != 0))
        {
            lits  = lits + 1;
        }
        W (zeros);
        W (lits);
        for (j = 0; j < lits; j = j + 1)
        {
            w  = memory_get (first + i + zeros + j, TRUE);
            W (w -> raw);
        }
        if ((zeros == 0) && (lits == 0))
        {
            return (-1);   // unreadable word: cannot happen for a loaded page
        }
        i  = i + zeros + lits;
    }
    return (0);
}

static int  read_range (FILE *f, uint32_t first, uint32_t words)
{
    uint32_t  i      = 0;
    uint32_t  zeros  = 0;
    uint32_t  lits   = 0;
    uint32_t  j      = 0;
    uint32_t  v      = 0;

    while (i < words)
    {
        R (zeros);
        R (lits);
        if ((zeros > words - i) || (lits > words - i - zeros))
        {
            return (-1);
        }
        for (j = 0; j < zeros; j = j + 1)
        {
            SYSMEMSET (first + i + j, 0);
        }
        for (j = 0; j < lits; j = j + 1)
        {
            R (v);
            SYSMEMSET (first + i + zeros + j, v);
        }
        i  = i + zeros + lits;
    }
    return (0);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// GPU region tables
//
static int  write_regions (FILE *f, vtex_t *tex)
{
    uint32_t  i  = 0;

    W ((tex != NULL) && (tex -> region != NULL));
    if ((tex == NULL) || (tex -> region == NULL))
    {
        return (0);
    }
    for (i = 0; i < V32_REGIONS_PER_TEXTURE; i = i + 1)
    {
        region_t  *r  = tex -> region + i;

        W ((uint16_t) r -> minX | ((uint32_t) (uint16_t) r -> minY << 16));
        W ((uint16_t) r -> maxX | ((uint32_t) (uint16_t) r -> maxY << 16));
        W ((uint16_t) r -> hotX | ((uint32_t) (uint16_t) r -> hotY << 16));
    }
    return (0);
}

static int  read_regions (FILE *f, vtex_t *tex)
{
    uint32_t  present  = 0;
    uint32_t  a        = 0;
    uint32_t  b        = 0;
    uint32_t  c        = 0;
    uint32_t  i        = 0;

    R (present);
    if (present == 0)
    {
        return (0);
    }
    if ((tex == NULL) || (tex -> region == NULL))
    {
        return (-1);
    }
    for (i = 0; i < V32_REGIONS_PER_TEXTURE; i = i + 1)
    {
        region_t  *r  = tex -> region + i;

        R (a);  R (b);  R (c);
        r -> minX  = (int16_t) (a & 0xFFFF);  r -> minY  = (int16_t) (a >> 16);
        r -> maxX  = (int16_t) (b & 0xFFFF);  r -> maxY  = (int16_t) (b >> 16);
        r -> hotX  = (int16_t) (c & 0xFFFF);  r -> hotY  = (int16_t) (c >> 16);
    }
    return (0);
}

static uint32_t  cart_textures (void)
{
    return ((cart_vtex != NULL) ? cart_vtex -> qty : 0);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// state_save(): returns TRUE on success
//
uint8_t  state_save (const char *filename)
{
    FILE      *f    = fopen (filename, "wb");
    uint32_t   i    = 0;
    uint32_t   j    = 0;
    uint32_t   bsum = (memory + V32_PAGE_BIOS) -> data != NULL ? (memory + V32_PAGE_BIOS) -> checksum : 0;
    uint32_t   csum = (memory + V32_PAGE_CART) -> data != NULL ? (memory + V32_PAGE_CART) -> checksum : 0;

#define  WB(v)   do { if (state_wr32 (f, (uint32_t) (v)) != 0) goto fail; } while (0)

    if (f == NULL)
    {
        return (FALSE);
    }

    if (fwrite (SAVE_MAGIC, 1, 8, f) != 8)  goto fail;
    WB (SAVE_VERSION);
    WB (bsum);
    WB (csum);

    for (i = 0; i < NUM_REGISTERS; i = i + 1)
    {
        WB ((reg + i) -> value.raw);
    }
    WB (rom_offset);

    for (i = 0; i < NUM_PORT_CATEGORIES; i = i + 1)
    {
        for (j = 0; j < port_qty[i]; j = j + 1)
        {
            WB ((*(ioports + i) + j) -> value.raw);
        }
    }

    if (write_range (f, 0x00000000, RAM_WORDS) != 0)  goto fail;

    WB ((memory + V32_PAGE_MEMC) -> data != NULL);
    if ((memory + V32_PAGE_MEMC) -> data != NULL)
    {
        if (write_range (f, MEMC_FIRST_ADDR, MEMC_WORDS) != 0)  goto fail;
    }

    if (write_regions (f, bios_vtex) != 0)  goto fail;
    WB (cart_textures ());
    for (i = 0; i < cart_textures (); i = i + 1)
    {
        if (write_regions (f, cart_vtex + i) != 0)  goto fail;
    }

    for (i = 0; i < V32_NUM_GAMEPADS; i = i + 1)
    {
        WB ((gamepad + i) -> connected);
        for (j = 0; j < 11; j = j + 1)
        {
            WB ((gamepad + i) -> button[j]);
        }
    }

    if (gpu_save (f) != 0)  goto fail;
    if (spu_save (f) != 0)  goto fail;

    if (fclose (f) != 0)
    {
        remove (filename);
        return (FALSE);
    }
    return (TRUE);

fail:
    fclose (f);
    remove (filename);
    return (FALSE);
#undef WB
}

////////////////////////////////////////////////////////////////////////////////////////
//
// state_restore(): returns TRUE on success. The file is checked against the loaded BIOS
// and cartridge before anything is changed. A file that is damaged part-way through
// leaves the machine in a mixed state; the message says so.
//
uint8_t  state_restore (const char *filename)
{
    FILE      *f          = fopen (filename, "rb");
    char       magic[8];
    uint32_t   version    = 0;
    uint32_t   biossum    = 0;
    uint32_t   cartsum    = 0;
    uint32_t   v          = 0;
    uint32_t   i          = 0;
    uint32_t   j          = 0;
    uint32_t   ntex       = 0;
    uint32_t   saved[NUM_REGISTERS];
    uint32_t   saved_rom  = 0;
    uint8_t    ok         = FALSE;

    if (f == NULL)
    {
        fprintf (stderr, "[ERROR] restore: could not open '%s'\n", filename);
        return (FALSE);
    }

    if ((fread (magic, 1, 8, f) != 8) || (memcmp (magic, SAVE_MAGIC, 8) != 0))
    {
        fprintf (stderr, "[ERROR] restore: '%s' is not a v32sim state file\n", filename);
        fclose (f);
        return (FALSE);
    }
    if ((state_rd32 (f, &version) != 0) || (version != SAVE_VERSION))
    {
        fprintf (stderr, "[ERROR] restore: unsupported state file version\n");
        fclose (f);
        return (FALSE);
    }
    if ((state_rd32 (f, &biossum) != 0) || (state_rd32 (f, &cartsum) != 0))
    {
        fprintf (stderr, "[ERROR] restore: '%s' is truncated\n", filename);
        fclose (f);
        return (FALSE);
    }
    if ((biossum != ((memory + V32_PAGE_BIOS) -> data != NULL ? (memory + V32_PAGE_BIOS) -> checksum : 0)) ||
        (cartsum != ((memory + V32_PAGE_CART) -> data != NULL ? (memory + V32_PAGE_CART) -> checksum : 0)))
    {
        fprintf (stderr, "[ERROR] restore: '%s' was saved with a different BIOS or cartridge\n", filename);
        fclose (f);
        return (FALSE);
    }

    do
    {
        for (i = 0; i < NUM_REGISTERS; i = i + 1)
        {
            if (state_rd32 (f, &saved[i]) != 0)  goto truncated;
        }
        if (state_rd32 (f, &saved_rom) != 0)  goto truncated;

        // ioports: write the stored values directly (no side effects, no checks)
        for (i = 0; i < NUM_PORT_CATEGORIES; i = i + 1)
        {
            for (j = 0; j < port_qty[i]; j = j + 1)
            {
                if (state_rd32 (f, &v) != 0)  goto truncated;
                (*(ioports + i) + j) -> value.raw  = v;
            }
        }

        if (read_range (f, 0x00000000, RAM_WORDS) != 0)  goto truncated;

        if (state_rd32 (f, &v) != 0)  goto truncated;
        if (v != 0)
        {
            if ((memory + V32_PAGE_MEMC) -> data == NULL)
            {
                fprintf (stderr, "[ERROR] restore: the state has a memory card, none is loaded\n");
                goto damaged;
            }
            if (read_range (f, MEMC_FIRST_ADDR, MEMC_WORDS) != 0)  goto truncated;
        }

        if (read_regions (f, bios_vtex) != 0)  goto truncated;
        if (state_rd32 (f, &ntex) != 0)  goto truncated;
        if (ntex != cart_textures ())
        {
            fprintf (stderr, "[ERROR] restore: texture count mismatch\n");
            goto damaged;
        }
        for (i = 0; i < ntex; i = i + 1)
        {
            if (read_regions (f, cart_vtex + i) != 0)  goto truncated;
        }

        for (i = 0; i < V32_NUM_GAMEPADS; i = i + 1)
        {
            if (state_rd32 (f, &v) != 0)  goto truncated;
            (gamepad + i) -> connected  = (uint8_t) v;
            for (j = 0; j < 11; j = j + 1)
            {
                if (state_rd32 (f, &v) != 0)  goto truncated;
                (gamepad + i) -> button[j]  = (int32_t) v;
            }
        }

        if (gpu_restore (f) != 0)  goto truncated;
        if (spu_restore (f) != 0)  goto truncated;
    }
    while (0);

    // registers last (so the machine is only moved once everything has been read)
    for (i = 0; i < NUM_REGISTERS; i = i + 1)
    {
        (reg + i) -> value.raw  = saved[i];
    }
    rom_offset  = saved_rom;
    REG(IP)     = rom_offset;
    REG(IR)     = ISYSMEMGET (REG(IP));
    REG(IV)     = (0 < (REG(IR) & IMMVAL_MASK)) ? ISYSMEMGET (REG(IP) + 1) : 0;
    ok          = TRUE;
    goto done;

truncated:
    fprintf (stderr, "[ERROR] restore: '%s' is truncated or damaged\n", filename);
damaged:
    fprintf (stderr, "[WARNING] restore: the machine state may be partially restored\n");
done:
    fclose (f);
    return (ok);
}
