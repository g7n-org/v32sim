////////////////////////////////////////////////////////////////////////////////////////
//
// gpu.c: software model of the Vircon32 GPU
//
// Keeps a 640x360 RGB framebuffer and carries out the GPU_Command port commands
// (clear screen, draw region with optional zoom and/or rotation). The geometry,
// the sampling corrections, the pixel-capacity accounting and the three blending
// modes follow DesktopEmulator (ConsoleLogic/V32GPU.cpp, Emulator/VideoOutput.cpp).
// Sampling is nearest-neighbour, as the emulator configures OpenGL.
//
// gpu_screenshot() writes the framebuffer as an RGB PNG using stored (uncompressed)
// deflate blocks, so no PNG/zlib/GD library is needed.
//
////////////////////////////////////////////////////////////////////////////////////////
#include "defines.h"
#include "globals.h"
#include "ioports.h"
#include "vtex.h"
#include "biosfont.h"
#include <math.h>
#include <time.h>

#define  GPU_SCREEN_W            640
#define  GPU_SCREEN_H            360
#define  GPU_SCREEN_PIXELS       (GPU_SCREEN_W * GPU_SCREEN_H)
#define  GPU_TEXTURE_SIZE        1024.0f
#define  GPU_CAPACITY            (9 * GPU_SCREEN_PIXELS)
#define  GPU_SCALING_PENALTY     0.15f
#define  GPU_ROTATION_PENALTY    0.25f
#define  GPU_CLEAR_PENALTY       (-0.50f)

static uint8_t  framebuffer[GPU_SCREEN_H][GPU_SCREEN_W][3];

////////////////////////////////////////////////////////////////////////////////////////
//
// blend(): combine one source pixel (color 0-255 floats, alpha 0-1) into the
// framebuffer using the active blending mode
//
static void  blend (int x, int y, float r, float g, float b, float a, int32_t mode)
{
    float     src[3]  = { r, g, b };
    uint8_t  *dst     = framebuffer[y][x];
    int       i       = 0;
    float     v       = 0.0f;

    for (i = 0; i < 3; i = i + 1)
    {
        switch (mode)
        {
            case GPUBlendingMode_Add:
                v = dst[i] + src[i] * a;
                break;
            case GPUBlendingMode_Subtract:
                v = dst[i] - src[i] * a;
                break;
            default: // alpha
                v = src[i] * a + dst[i] * (1.0f - a);
                break;
        }
        if (v < 0.0f)   v = 0.0f;
        if (v > 255.0f) v = 255.0f;
        dst[i] = (uint8_t) (v + 0.5f);
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// gpu_clear(): fill the framebuffer with the clear color, blended like any draw
//
static void  gpu_clear (void)
{
    uint32_t  c     = (uint32_t) IPORTGET(GPU_ClearColor);
    int32_t   mode  = IPORTGET(GPU_ActiveBlending);
    int       x     = 0;
    int       y     = 0;

    for (y = 0; y < GPU_SCREEN_H; y = y + 1)
    {
        for (x = 0; x < GPU_SCREEN_W; x = x + 1)
        {
            blend (x, y, (float) (c & 0xFF), (float) ((c >> 8) & 0xFF),
                         (float) ((c >> 16) & 0xFF), (float) (c >> 24) / 255.0f, mode);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// gpu_reset(): power-on state of the framebuffer (black)
//
void  gpu_reset (void)
{
    memset (framebuffer, 0, sizeof (framebuffer));
}

////////////////////////////////////////////////////////////////////////////////////////
//
// gpu_new_frame(): restore the per-frame drawing capacity
//
void  gpu_new_frame (void)
{
    SYSPORTSET(GPU_RemainingPixels, GPU_CAPACITY);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// gpu_texel(): fetch pixel (x, y) of the selected texture as 0xAABBGGRR, or 0 if
// outside the texture
//
static uint32_t  gpu_texel (vtex_t *tex, int x, int y)
{
    word_t  *w  = NULL;

    if ((x < 0) || (y < 0) || (x >= tex -> wide) || (y >= tex -> high))
    {
        return (0);
    }
    w  = memory_get (tex -> offset + (uint32_t) y * tex -> wide + (uint32_t) x, TRUE);
    return ((w == NULL) ? 0 : w -> raw);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// The text console: draws with the BIOS texture (id -1) put the character whose code is
// the region number at a cell of the BIOS font grid (10x20 pixels, 64x18 cells, as
// print_at() lays text out). The grid is kept as text so that a text interface can be
// read without looking at pixels; ClearScreen empties it.
//
#define  CON_COLS    64
#define  CON_ROWS    18
#define  CON_CELL_W  10
#define  CON_CELL_H  20

static uint8_t  console_grid[CON_ROWS][CON_COLS];

static void  console_put (int32_t region, int32_t x, int32_t y)
{
    if ((region < 14) || (region > 255) || (x < 0) || (y < 0))
    {
        return;
    }
    if (((x % CON_CELL_W) != 0) || ((y % CON_CELL_H) != 0))
    {
        return;     // not on the character grid: some other BIOS graphic
    }
    if (((x / CON_CELL_W) < CON_COLS) && ((y / CON_CELL_H) < CON_ROWS))
    {
        console_grid[y / CON_CELL_H][x / CON_CELL_W]  = (uint8_t) region;
    }
}

//
// gpu_console_show(): the grid as UTF-8 text, trailing blanks and blank lines at the
// end removed. Returns the number of lines shown
//
int  gpu_console_show (FILE *out)
{
    int  row = 0, col = 0, last = -1, end = 0, k = 0, n = 0;
    uint16_t  u = 0;

    for (row = 0; row < CON_ROWS; row = row + 1)
    {
        for (col = 0; col < CON_COLS; col = col + 1)
        {
            if ((console_grid[row][col] != 0) && (console_grid[row][col] != ' '))  last = row;
        }
    }
    for (row = 0; row <= last; row = row + 1)
    {
        end = -1;
        for (col = 0; col < CON_COLS; col = col + 1)
        {
            if ((console_grid[row][col] != 0) && (console_grid[row][col] != ' '))  end = col;
        }
        for (col = 0; col <= end; col = col + 1)
        {
            uint8_t  c = console_grid[row][col];

            u = (c == 0) ? ' ' : (c >= 32 && c < 127) ? c : 0;
            if ((u == 0) && (c != 0))
            {
                for (k = 0; k < (int) BIOS_CHARS; k = k + 1)
                {
                    if (bios_chars[k].code == c)  { u = bios_chars[k].unicode;  break; }
                }
                if (u == 0)  u = '?';
            }
            if (u < 0x80)        fputc (u, out);
            else if (u < 0x800)  { fputc (0xC0 | (u >> 6), out);  fputc (0x80 | (u & 0x3F), out); }
            else                 { fputc (0xE0 | (u >> 12), out);  fputc (0x80 | ((u >> 6) & 0x3F), out);  fputc (0x80 | (u & 0x3F), out); }
        }
        fputc ('\n', out);
        n = n + 1;
    }
    return (n);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// gpu_draw(): DrawRegion with optional scaling and rotation
//
static void  gpu_draw (uint8_t scaling, uint8_t rotation)
{
    int32_t   remaining  = IPORTGET(GPU_RemainingPixels);
    int32_t   seltex     = IPORTGET(GPU_SelectedTexture);
    int32_t   mode       = IPORTGET(GPU_ActiveBlending);
    uint32_t  mult       = (uint32_t) IPORTGET(GPU_MultiplyColor);
    int32_t   minX       = IPORTGET(GPU_RegionMinX);
    int32_t   minY       = IPORTGET(GPU_RegionMinY);
    int32_t   maxX       = IPORTGET(GPU_RegionMaxX);
    int32_t   maxY       = IPORTGET(GPU_RegionMaxY);
    int32_t   hotX       = IPORTGET(GPU_RegionHotspotX);
    int32_t   hotY       = IPORTGET(GPU_RegionHotspotY);
    float     px         = (float) IPORTGET(GPU_DrawingPointX);
    float     py         = (float) IPORTGET(GPU_DrawingPointY);
    float     sx         = ioports_get (GPU_DrawingScaleX, TRUE).f32;
    float     sy         = ioports_get (GPU_DrawingScaleY, TRUE).f32;
    float     angle      = ioports_get (GPU_DrawingAngle,  TRUE).f32;
    vtex_t   *tex        = NULL;
    int32_t   rw         = abs (maxX - minX) + 1;
    int32_t   rh         = abs (maxY - minY) + 1;
    float     renderw    = (float) rw;
    float     renderh    = (float) rh;
    int32_t   needed     = 0;
    float     cost       = 1.0f;
    float     tminx      = minX + 0.5f;
    float     tmaxx      = maxX + 0.5f;
    float     tminy      = minY + 0.5f;
    float     tmaxy      = maxY + 0.5f;
    float     vx[4];
    float     vy[4];
    float     acos_      = 0.0f;
    float     asin_      = 0.0f;
    int       i          = 0;

    if (remaining < 0)
    {
        return; // already out of capacity this frame
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // pixel capacity accounting
    //
    if (scaling)
    {
        renderw  = renderw * fabsf (sx);
        renderh  = renderh * fabsf (sy);
    }
    {
        int32_t  ew  = ((int) renderw < GPU_SCREEN_W) ? (int) renderw : GPU_SCREEN_W;
        int32_t  eh  = ((int) renderh < GPU_SCREEN_H) ? (int) renderh : GPU_SCREEN_H;

        if (scaling)  cost = cost + GPU_SCALING_PENALTY;
        if (rotation) cost = cost + GPU_ROTATION_PENALTY;
        needed = (int32_t) (cost * ew * eh);
    }
    remaining = remaining - needed;
    if (remaining < 0)
    {
        SYSPORTSET(GPU_RemainingPixels, -1);
        return;
    }
    SYSPORTSET(GPU_RemainingPixels, remaining);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // locate the selected texture
    //
    if (seltex == -1)
    {
        tex = bios_vtex;
    }
    else if ((seltex >= 0) && (cart_vtex != NULL) && ((uint32_t) seltex < cart_vtex -> qty))
    {
        tex = cart_vtex + seltex;
    }
    if (seltex == -1)
    {
        console_put (IPORTGET(GPU_SelectedRegion), (int32_t) px - hotX + minX, (int32_t) py - hotY + minY);
    }
    if ((tex == NULL) || (tex -> size == 0))
    {
        return;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // texture coordinates, with the emulator's sub-pixel correction for zooming
    //
    if (scaling)
    {
        if (fabsf (sx) > 1.0f)
        {
            float  corr  = 0.5f - 1.0f / (2.0f * fabsf (sx));

            if (tminx < tmaxx) { tminx -= corr; tmaxx += corr; }
            else               { tmaxx -= corr; tminx += corr; }
        }
        if (fabsf (sy) > 1.0f)
        {
            float  corr  = 0.5f - 1.0f / (2.0f * fabsf (sy));

            if (tminy < tmaxy) { tminy -= corr; tmaxy += corr; }
            else               { tmaxy -= corr; tminy += corr; }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // quad geometry: 0=(min,min) 1=(max,min) 2=(min,max) 3=(max,max), relative to
    // the hotspot, then scaled, rotated and translated
    //
    {
        int  rminx  = minX - hotX;
        int  rminy  = minY - hotY;
        int  rmaxx  = rminx + rw;
        int  rmaxy  = rminy + rh;

        vx[0] = (float) rminx;  vy[0] = (float) rminy;
        vx[1] = (float) rmaxx;  vy[1] = (float) rminy;
        vx[2] = (float) rminx;  vy[2] = (float) rmaxy;
        vx[3] = (float) rmaxx;  vy[3] = (float) rmaxy;
    }
    if (rotation)
    {
        acos_  = cosf (angle);
        asin_  = sinf (angle);
    }
    for (i = 0; i < 4; i = i + 1)
    {
        if (scaling)
        {
            vx[i] = vx[i] * sx;
            vy[i] = vy[i] * sy;
        }
        if (rotation)
        {
            float  cx  = vx[i];
            float  cy  = vy[i];

            vx[i] = cx * acos_ - cy * asin_;
            vy[i] = cx * asin_ + cy * acos_;
        }
        vx[i] = vx[i] + px;
        vy[i] = vy[i] + py;
        if (scaling)
        {
            if (sx < 0.0f) vx[i] = vx[i] + 1.0f;
            if (sy < 0.0f) vy[i] = vy[i] + 1.0f;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // rasterize the parallelogram: for every pixel centre inside the bounding box,
    // solve P = V0 + s (V1-V0) + t (V2-V0); inside when 0 <= s,t < 1
    //
    {
        float  e1x  = vx[1] - vx[0];
        float  e1y  = vy[1] - vy[0];
        float  e2x  = vx[2] - vx[0];
        float  e2y  = vy[2] - vy[0];
        float  det  = e1x * e2y - e1y * e2x;
        float  lox  = vx[0];
        float  hix  = vx[0];
        float  loy  = vy[0];
        float  hiy  = vy[0];
        int    x0   = 0;
        int    x1   = 0;
        int    y0   = 0;
        int    y1   = 0;
        int    x    = 0;
        int    y    = 0;
        float  mr   = (float) (mult & 0xFF);
        float  mg   = (float) ((mult >> 8) & 0xFF);
        float  mb   = (float) ((mult >> 16) & 0xFF);
        float  ma   = (float) (mult >> 24);

        if (fabsf (det) < 1e-9f)
        {
            return; // degenerate (zero scale)
        }
        for (i = 1; i < 4; i = i + 1)
        {
            if (vx[i] < lox) lox = vx[i];
            if (vx[i] > hix) hix = vx[i];
            if (vy[i] < loy) loy = vy[i];
            if (vy[i] > hiy) hiy = vy[i];
        }
        x0 = (int) floorf (lox);  if (x0 < 0) x0 = 0;
        y0 = (int) floorf (loy);  if (y0 < 0) y0 = 0;
        x1 = (int) ceilf  (hix);  if (x1 > GPU_SCREEN_W) x1 = GPU_SCREEN_W;
        y1 = (int) ceilf  (hiy);  if (y1 > GPU_SCREEN_H) y1 = GPU_SCREEN_H;

        for (y = y0; y < y1; y = y + 1)
        {
            for (x = x0; x < x1; x = x + 1)
            {
                float     dx  = ((float) x + 0.5f) - vx[0];
                float     dy  = ((float) y + 0.5f) - vy[0];
                float     s   = (dx * e2y - dy * e2x) / det;
                float     t   = (e1x * dy - e1y * dx) / det;
                float     u   = 0.0f;
                float     v   = 0.0f;
                uint32_t  c   = 0;
                float     a   = 0.0f;

                if ((s < 0.0f) || (s >= 1.0f) || (t < 0.0f) || (t >= 1.0f))
                {
                    continue;
                }
                u  = tminx + s * (tmaxx - tminx);
                v  = tminy + t * (tmaxy - tminy);
                c  = gpu_texel (tex, (int) floorf (u), (int) floorf (v));
                a  = ((float) (c >> 24) / 255.0f) * (ma / 255.0f);
                blend (x, y, (float) (c & 0xFF)         * mr / 255.0f,
                             (float) ((c >> 8) & 0xFF)  * mg / 255.0f,
                             (float) ((c >> 16) & 0xFF) * mb / 255.0f,
                       a, mode);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// gpu_command(): execute a value written to GPU_Command (unknown commands are
// ignored, as on the hardware)
//
void  gpu_command (int32_t command)
{
    int32_t  remaining  = 0;
    int32_t  needed     = 0;

    switch (command)
    {
        case GPUCommand_ClearScreen:
            remaining  = IPORTGET(GPU_RemainingPixels);
            if (remaining < 0)
            {
                break;
            }
            needed     = (int32_t) ((1.0f + GPU_CLEAR_PENALTY) * GPU_SCREEN_PIXELS);
            remaining  = remaining - needed;
            if (remaining < 0)
            {
                SYSPORTSET(GPU_RemainingPixels, -1);
                break;
            }
            SYSPORTSET(GPU_RemainingPixels, remaining);
            gpu_clear ();
            memset (console_grid, 0, sizeof (console_grid));
            break;

        case GPUCommand_DrawRegion:            gpu_draw (FALSE, FALSE); break;
        case GPUCommand_DrawRegionZoomed:      gpu_draw (TRUE,  FALSE); break;
        case GPUCommand_DrawRegionRotated:     gpu_draw (FALSE, TRUE);  break;
        case GPUCommand_DrawRegionRotozoomed:  gpu_draw (TRUE,  TRUE);  break;
        default: break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// save states: the framebuffer, stored as one word (0x00BBGGRR) per pixel
//
int  gpu_save (FILE *f)
{
    int  x  = 0;
    int  y  = 0;

    for (y = 0; y < GPU_SCREEN_H; y = y + 1)
    {
        for (x = 0; x < GPU_SCREEN_W; x = x + 1)
        {
            uint8_t  *p  = framebuffer[y][x];

            if (state_wr32 (f, (uint32_t) p[0] | ((uint32_t) p[1] << 8) | ((uint32_t) p[2] << 16)) != 0)
            {
                return (-1);
            }
        }
    }
    return (0);
}

int  gpu_restore (FILE *f)
{
    int       x  = 0;
    int       y  = 0;
    uint32_t  v  = 0;

    for (y = 0; y < GPU_SCREEN_H; y = y + 1)
    {
        for (x = 0; x < GPU_SCREEN_W; x = x + 1)
        {
            if (state_rd32 (f, &v) != 0)
            {
                return (-1);
            }
            framebuffer[y][x][0]  = (uint8_t) v;
            framebuffer[y][x][1]  = (uint8_t) (v >> 8);
            framebuffer[y][x][2]  = (uint8_t) (v >> 16);
        }
    }
    return (0);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// PNG output
//
static uint32_t  crc_table[256];
static uint8_t   crc_ready  = FALSE;

static uint32_t  crc_update (uint32_t crc, const uint8_t *buf, size_t len)
{
    size_t  i  = 0;

    if (crc_ready == FALSE)
    {
        uint32_t  n  = 0;
        uint32_t  k  = 0;
        uint32_t  c  = 0;

        for (n = 0; n < 256; n = n + 1)
        {
            c  = n;
            for (k = 0; k < 8; k = k + 1)
            {
                c  = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            crc_table[n]  = c;
        }
        crc_ready  = TRUE;
    }
    for (i = 0; i < len; i = i + 1)
    {
        crc  = crc_table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
    }
    return (crc);
}

static void  put32 (uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t) (v >> 24);
    p[1] = (uint8_t) (v >> 16);
    p[2] = (uint8_t) (v >> 8);
    p[3] = (uint8_t) v;
}

static int  png_chunk (FILE *f, const char *type, const uint8_t *data, uint32_t len)
{
    uint8_t   hdr[8];
    uint8_t   end[4];
    uint32_t  crc  = 0xFFFFFFFFu;

    put32 (hdr, len);
    memcpy (hdr + 4, type, 4);
    crc  = crc_update (crc, (const uint8_t *) type, 4);
    crc  = crc_update (crc, data, len);
    put32 (end, crc ^ 0xFFFFFFFFu);
    if ((fwrite (hdr, 1, 8, f) != 8) ||
        ((len > 0) && (fwrite (data, 1, len, f) != len)) ||
        (fwrite (end, 1, 4, f) != 4))
    {
        return (-1);
    }
    return (0);
}

uint8_t  gpu_screenshot (const char *filename)
{
    FILE     *f         = fopen (filename, "wb");
    uint8_t   sig[8]    = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
    uint8_t   ihdr[13];
    size_t    rawlen    = (size_t) GPU_SCREEN_H * (1 + GPU_SCREEN_W * 3);
    size_t    zlen      = 2 + rawlen + 5 * (rawlen / 65535 + 1) + 4;
    uint8_t  *raw       = NULL;
    uint8_t  *z         = NULL;
    size_t    zpos      = 0;
    size_t    pos       = 0;
    uint32_t  a         = 1;
    uint32_t  b         = 0;
    size_t    i         = 0;
    int       y         = 0;
    uint8_t   ok        = FALSE;

    if (f == NULL)
    {
        return (FALSE);
    }

    raw  = (uint8_t *) malloc (rawlen);
    z    = (uint8_t *) malloc (zlen);
    if ((raw != NULL) && (z != NULL))
    {
        for (y = 0; y < GPU_SCREEN_H; y = y + 1)
        {
            raw[pos]  = 0; // filter: none
            pos       = pos + 1;
            memcpy (raw + pos, framebuffer[y], GPU_SCREEN_W * 3);
            pos       = pos + GPU_SCREEN_W * 3;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // zlib stream made of stored deflate blocks
        //
        z[zpos++]  = 0x78;
        z[zpos++]  = 0x01;
        for (pos = 0; pos < rawlen; )
        {
            size_t  n    = rawlen - pos;
            uint8_t fin  = 0;

            if (n > 65535) n = 65535;
            fin          = (pos + n >= rawlen) ? 1 : 0;
            z[zpos++]    = fin;
            z[zpos++]    = (uint8_t) (n & 0xFF);
            z[zpos++]    = (uint8_t) (n >> 8);
            z[zpos++]    = (uint8_t) (~n & 0xFF);
            z[zpos++]    = (uint8_t) ((~n >> 8) & 0xFF);
            memcpy (z + zpos, raw + pos, n);
            zpos         = zpos + n;
            pos          = pos + n;
        }
        for (i = 0; i < rawlen; i = i + 1)
        {
            a  = (a + raw[i]) % 65521;
            b  = (b + a) % 65521;
        }
        put32 (z + zpos, (b << 16) | a);
        zpos  = zpos + 4;

        put32 (ihdr,     GPU_SCREEN_W);
        put32 (ihdr + 4, GPU_SCREEN_H);
        ihdr[8]   = 8;  // bit depth
        ihdr[9]   = 2;  // truecolor RGB
        ihdr[10]  = 0;
        ihdr[11]  = 0;
        ihdr[12]  = 0;

        ok  = ((fwrite (sig, 1, 8, f) == 8) &&
               (png_chunk (f, "IHDR", ihdr, 13) == 0) &&
               (png_chunk (f, "IDAT", z, (uint32_t) zpos) == 0) &&
               (png_chunk (f, "IEND", NULL, 0) == 0)) ? TRUE : FALSE;
    }
    free (raw);
    free (z);
    if (fclose (f) != 0)
    {
        ok  = FALSE;
    }
    return (ok);
}
