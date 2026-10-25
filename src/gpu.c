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
// The text console. Draws with the BIOS texture (id -1) are also kept as characters, so
// that a text interface can be read (and shown on a terminal) without looking at pixels.
// Two layouts are recognized, both with region = character code:
//
//   BIOS      the BIOS font as print_at() lays it out: regions 14-255, 10x20 pixel
//             cells, 64x18 characters
//   v32term   the 80x24 terminal of v32c++'s libc: 8x15 pixel cells, glyphs drawn
//             slightly scaled from regions the library defines on the BIOS texture
//             (body 256+C, shade blocks 769-776); the cell's background is a "paper"
//             draw (region 768) that comes first, so it blanks the cell. Tail
//             regions (512+C, 1024+C) are the descenders of the same character
//
// The layout that was drawn to last is the one shown. ClearScreen empties both.
//
#define  CON_A_COLS  64
#define  CON_A_ROWS  18
#define  CON_B_COLS  80
#define  CON_B_ROWS  24

//
// A cell: the character and the colors it was drawn with. Colors are the GPU's
// multiply color (0xAABBGGRR); the foreground is the glyph draw's, the background the
// "paper" draw's (v32term). Alpha is ignored.
//
typedef struct
{
    uint8_t   ch;
    uint32_t  fg;
    uint32_t  bg;
} ccell_t;

#define  CON_WHITE  0xFFFFFFFFu

static ccell_t   console_a[CON_A_ROWS][CON_A_COLS];
static ccell_t   console_b[CON_B_ROWS][CON_B_COLS];
static uint8_t   console_use_b  = FALSE;
uint32_t         console_gen    = 0;           // changes whenever the text or its colors do

int8_t           console_color   = -1;         // -1 unset (tty: auto, dump: none), 0 none, 1 16, 2 256, 3 true
uint32_t         console_cursor  = 0;          // 0xAABBGGRR, 0 = leave the guest's color
uint8_t          console_cursor_codes[8] = { 20 };
int              console_ncursor = 1;

static void  console_cell (ccell_t *c, uint8_t ch, uint32_t fg, int setbg, uint32_t bg)
{
    fg  = fg & 0x00FFFFFF;
    bg  = bg & 0x00FFFFFF;
    if ((c -> ch != ch) || (c -> fg != fg) || ((setbg != 0) && (c -> bg != bg)))
    {
        c -> ch  = ch;
        c -> fg  = fg;
        if (setbg != 0)  c -> bg  = bg;
        console_gen  = console_gen + 1;
    }
}

static void  console_put (int32_t region, int32_t x, int32_t y, uint32_t mult)
{
    if ((x < 0) || (y < 0))
    {
        return;
    }
    if ((region >= 14) && (region <= 255))
    {
        if (((x % 10) == 0) && ((y % 20) == 0) && ((x / 10) < CON_A_COLS) && ((y / 20) < CON_A_ROWS))
        {
            console_use_b  = FALSE;
            console_cell (&console_a[y / 20][x / 10], (uint8_t) region, mult, 0, 0);
        }
        return;
    }
    if (((x % 8) != 0) || ((y % 15) != 0) || ((x / 8) >= CON_B_COLS) || ((y / 15) >= CON_B_ROWS))
    {
        return;
    }
    if (region == 768)                                   // paper: the cell starts blank
    {
        console_use_b  = TRUE;
        console_cell (&console_b[y / 15][x / 8], 0, CON_WHITE, 1, mult);
    }
    else if ((region >= 256 + 14) && (region <= 256 + 255))
    {
        console_use_b  = TRUE;
        console_cell (&console_b[y / 15][x / 8], (uint8_t) (region - 256), mult, 0, 0);
    }
    else if ((region >= 769) && (region <= 776))        // shade blocks 17-20
    {
        console_use_b  = TRUE;
        console_cell (&console_b[y / 15][x / 8], (uint8_t) (17 + (region - 769) / 2), mult, 0, 0);
    }
}

static void  console_clear (void)
{
    memset (console_a, 0, sizeof (console_a));
    memset (console_b, 0, sizeof (console_b));
    console_gen  = console_gen + 1;
}

static const ccell_t *console_cells (int *rows, int *cols)
{
    if (console_use_b == TRUE)
    {
        *rows  = CON_B_ROWS;
        *cols  = CON_B_COLS;
        return (&console_b[0][0]);
    }
    *rows  = CON_A_ROWS;
    *cols  = CON_A_COLS;
    return (&console_a[0][0]);
}

//
// gpu_console_utf8(): the text of a BIOS font code, as UTF-8; returns the byte count
//
static int  console_utf8 (uint8_t c, char *out)
{
    uint16_t  u  = (c == 0) ? ' ' : (c >= 32 && c < 127) ? c : 0;
    int       k  = 0;

    if ((u == 0) && (c != 0))
    {
        for (k = 0; k < (int) BIOS_CHARS; k = k + 1)
        {
            if (bios_chars[k].code == c)  { u = bios_chars[k].unicode;  break; }
        }
        if (u == 0)  u = '?';
    }
    if (u < 0x80)   { out[0] = (char) u;  return (1); }
    if (u < 0x800)  { out[0] = (char) (0xC0 | (u >> 6));  out[1] = (char) (0x80 | (u & 0x3F));  return (2); }
    out[0] = (char) (0xE0 | (u >> 12));  out[1] = (char) (0x80 | ((u >> 6) & 0x3F));  out[2] = (char) (0x80 | (u & 0x3F));
    return (3);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Colors on a terminal. No curses library is needed: all the terminal is asked for is
// SGR color sequences, and the three levels every modern terminal knows are produced
// directly (16 colors, the 256 color cube and grays, 24 bit). A GPU color is brought
// to the nearest color of the level by distance in RGB.
//
static const uint8_t  ansi16[16][3] =
{
    {   0,   0,   0 }, { 205,   0,   0 }, {   0, 205,   0 }, { 205, 205,   0 },
    {   0,   0, 238 }, { 205,   0, 205 }, {   0, 205, 205 }, { 229, 229, 229 },
    { 127, 127, 127 }, { 255,   0,   0 }, {   0, 255,   0 }, { 255, 255,   0 },
    {  92,  92, 255 }, { 255,   0, 255 }, {   0, 255, 255 }, { 255, 255, 255 }
};

static int  dist2 (int r, int g, int b, int pr, int pg, int pb)
{
    return ((r - pr) * (r - pr) + (g - pg) * (g - pg) + (b - pb) * (b - pb));
}

static int  nearest16 (uint32_t c)
{
    int  r = (int) (c & 0xFF), g = (int) ((c >> 8) & 0xFF), b = (int) ((c >> 16) & 0xFF);
    int  i = 0, best = 0, bd = 1 << 30, d = 0;

    for (i = 0; i < 16; i = i + 1)
    {
        d  = dist2 (r, g, b, ansi16[i][0], ansi16[i][1], ansi16[i][2]);
        if (d < bd)  { bd = d;  best = i; }
    }
    return (best);
}

static int  nearest256 (uint32_t c)
{
    static const int  lv[6]  = { 0, 95, 135, 175, 215, 255 };
    int  r = (int) (c & 0xFF), g = (int) ((c >> 8) & 0xFF), b = (int) ((c >> 16) & 0xFF);
    int  ri = 0, gi = 0, bi = 0, i = 0, gray = 0, gl = 0;
    int  dc = 0, dg = 0;

    for (i = 1; i < 6; i = i + 1)
    {
        if (abs (r - lv[i]) < abs (r - lv[ri]))  ri = i;
        if (abs (g - lv[i]) < abs (g - lv[gi]))  gi = i;
        if (abs (b - lv[i]) < abs (b - lv[bi]))  bi = i;
    }
    dc    = dist2 (r, g, b, lv[ri], lv[gi], lv[bi]);
    gray  = (r + g + b) / 3;
    gl    = (gray - 8 + 5) / 10;
    if (gl < 0)  gl = 0;
    if (gl > 23) gl = 23;
    dg    = dist2 (r, g, b, 8 + 10 * gl, 8 + 10 * gl, 8 + 10 * gl);
    return ((dg < dc) ? 232 + gl : 16 + 36 * ri + 6 * gi + bi);
}

//
// sgr(): the escape sequence that sets a foreground and background (0 = default)
//
static int  sgr (char *out, int mode, uint32_t fg, uint32_t bg)
{
    int  n = 0, f = 0, b = 0;

    n  = sprintf (out, "\033[0");
    if ((fg & 0xFFFFFF) != 0xFFFFFF)
    {
        if (mode == 3)       n += sprintf (out + n, ";38;2;%u;%u;%u", fg & 0xFF, (fg >> 8) & 0xFF, (fg >> 16) & 0xFF);
        else if (mode == 2)  n += sprintf (out + n, ";38;5;%d", nearest256 (fg));
        else                 { f = nearest16 (fg);  n += sprintf (out + n, ";%d", (f < 8) ? 30 + f : 90 + f - 8); }
    }
    if ((bg & 0xFFFFFF) != 0)
    {
        if (mode == 3)       n += sprintf (out + n, ";48;2;%u;%u;%u", bg & 0xFF, (bg >> 8) & 0xFF, (bg >> 16) & 0xFF);
        else if (mode == 2)  n += sprintf (out + n, ";48;5;%d", nearest256 (bg));
        else                 { b = nearest16 (bg);  n += sprintf (out + n, ";%d", (b < 8) ? 40 + b : 100 + b - 8); }
    }
    n += sprintf (out + n, "m");
    return (n);
}

//
// gpu_console_auto(): the color level the terminal advertises
//
int  gpu_console_auto (void)
{
    const char *ct = getenv ("COLORTERM");
    const char *tm = getenv ("TERM");

    if ((getenv ("NO_COLOR") != NULL) && (getenv ("NO_COLOR")[0] != '\0'))  return (0);
    if ((ct != NULL) && (strstr (ct, "truecolor") || strstr (ct, "24bit")))  return (3);
    if ((tm != NULL) && strstr (tm, "256color"))                              return (2);
    if ((tm != NULL) && (strcmp (tm, "dumb") != 0) && (tm[0] != '\0'))        return (1);
    return (0);
}

int  gpu_console_set_color (const char *name)
{
    if      (0 == strcmp (name, "none") || 0 == strcmp (name, "off") || 0 == strcmp (name, "0"))  console_color = 0;
    else if (0 == strcmp (name, "16"))                                                             console_color = 1;
    else if (0 == strcmp (name, "256"))                                                            console_color = 2;
    else if (0 == strcmp (name, "true") || 0 == strcmp (name, "24") || 0 == strcmp (name, "truecolor") || 0 == strcmp (name, "on"))  console_color = 3;
    else if (0 == strcmp (name, "auto"))                                                           console_color = (int8_t) gpu_console_auto ();
    else  return (-1);
    console_gen  = console_gen + 1;
    return (0);
}

//
// gpu_console_set_cursor(): the color the cursor is shown in (a name, #RRGGBB or
// 0xRRGGBB), or "off". The cursor is a drawn solid block, character 20 unless more
// codes are listed
//
int  gpu_console_set_cursor (const char *name)
{
    static const struct { const char *n; uint32_t rgb; } names[] =
    {
        { "black", 0x000000 }, { "red", 0xCD0000 }, { "green", 0x00CD00 }, { "yellow", 0xCDCD00 },
        { "blue", 0x5C5CFF }, { "magenta", 0xCD00CD }, { "cyan", 0x00CDCD }, { "white", 0xE5E5E5 },
        { "orange", 0xFF8000 }, { "gray", 0x7F7F7F }, { NULL, 0 }
    };
    unsigned long  v  = 0;
    char          *e  = NULL;
    int            i  = 0;

    if (0 == strcmp (name, "off"))  { console_cursor = 0;  console_gen = console_gen + 1;  return (0); }
    for (i = 0; names[i].n != NULL; i = i + 1)
    {
        if (0 == strcmp (name, names[i].n))  { v = names[i].rgb;  goto have; }
    }
    if (name[0] == '#')  name = name + 1;
    v  = strtoul (name, &e, 16);
    if ((e == name) || (*e != '\0') || (v > 0xFFFFFF))  return (-1);
have:
    console_cursor  = 0x01000000u | (((v >> 16) & 0xFF) | (v & 0xFF00) | ((v & 0xFF) << 16));   // to 0xAABBGGRR
    console_gen     = console_gen + 1;
    return (0);
}

void  gpu_console_set_cursor_codes (const uint8_t *codes, int n)
{
    int  i  = 0;

    console_ncursor  = 0;
    for (i = 0; (i < n) && (i < 8); i = i + 1)  console_cursor_codes[console_ncursor++]  = codes[i];
}

void  gpu_console_status (FILE *out)
{
    static const char *lv[]  = { "none", "16 colors", "256 colors", "24 bit" };

    fprintf (out, "console: %s layout, color %s%s, cursor %s",
                  (console_use_b == TRUE) ? "80x24" : "64x18",
                  (console_color < 0) ? "unset (tty: auto, dump: none)" : lv[(int) console_color],
                  "", (console_cursor == 0) ? "as drawn" : "recolored");
    if (console_cursor != 0)  fprintf (out, " (#%02X%02X%02X)", console_cursor & 0xFF, (console_cursor >> 8) & 0xFF, (console_cursor >> 16) & 0xFF);
    fprintf (out, "\n");
}

//
// gpu_console_render(): the screen as text for a terminal. tty = 1 emits every row
// (each cleared to its end, ready to be placed at the top of the screen); 0 trims
// blank rows at the end and blanks at the end of each row. Returns a malloc'd string
//
char *gpu_console_render (int tty, size_t *length)
{
    int            rows = 0, cols = 0, row = 0, col = 0, last = -1, end = 0, k = 0, i = 0;
    int            mode = (console_color < 0) ? ((tty != 0) ? gpu_console_auto () : 0) : console_color;
    const ccell_t *g    = console_cells (&rows, &cols);
    char          *out  = (char *) malloc ((size_t) (rows * cols * 40 + rows * 16 + 64));
    size_t         n    = 0;
    uint32_t       cf = CON_WHITE, cb = 0, fg = 0, bg = 0;
    char           u8[4];

    if (out == NULL)  return (NULL);
    for (row = 0; row < rows; row = row + 1)
    {
        for (col = 0; col < cols; col = col + 1)
        {
            const ccell_t *c = g + row * cols + col;
            if (((c -> ch != 0) && (c -> ch != ' ')) || (((c -> bg & 0xFFFFFF) != 0) && (mode > 0)))  last = row;
        }
    }
    for (row = 0; row < ((tty != 0) ? rows : last + 1); row = row + 1)
    {
        end = -1;
        for (col = 0; col < cols; col = col + 1)
        {
            const ccell_t *c = g + row * cols + col;
            if (((c -> ch != 0) && (c -> ch != ' ')) || (((c -> bg & 0xFFFFFF) != 0) && (mode > 0)))  end = col;
        }
        cf = CON_WHITE;  cb = 0;
        for (col = 0; col <= end; col = col + 1)
        {
            const ccell_t *c = g + row * cols + col;

            fg = c -> fg;  bg = c -> bg;
            if (console_cursor != 0)
            {
                for (i = 0; i < console_ncursor; i = i + 1)
                {
                    if (c -> ch == console_cursor_codes[i])  fg = console_cursor;
                }
            }
            if (mode > 0)
            {
                if (((fg & 0xFFFFFF) != (cf & 0xFFFFFF)) || ((bg & 0xFFFFFF) != (cb & 0xFFFFFF)))
                {
                    n   = n + (size_t) sgr (out + n, mode, fg, bg);
                    cf  = fg;  cb  = bg;
                }
            }
            k  = console_utf8 (c -> ch, u8);
            memcpy (out + n, u8, (size_t) k);  n = n + (size_t) k;
        }
        if ((mode > 0) && ((cf & 0xFFFFFF) != 0xFFFFFF || (cb & 0xFFFFFF) != 0))
        {
            memcpy (out + n, "\033[0m", 4);  n = n + 4;
        }
        if (tty != 0)  { memcpy (out + n, "\033[K", 3);  n = n + 3; }
        out[n++]  = '\n';
    }
    if (length != NULL)  *length = n;
    out[n]  = '\0';
    return (out);
}

//
// gpu_console_show(): the screen as text on a stream (blank rows at the end trimmed)
//
int  gpu_console_show (FILE *out)
{
    size_t  n    = 0;
    char   *text = gpu_console_render (0, &n);
    size_t  i    = 0;
    int     rows = 0;

    if (text == NULL)  return (0);
    fwrite (text, 1, n, out);
    for (i = 0; i < n; i = i + 1)  if (text[i] == '\n')  rows = rows + 1;
    free (text);
    return (rows);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Reading the text console from the command language (until, expect console).
//
// console_rows(): the rows `console` would print, as plain text: no colors, trailing
// blanks removed, and the solid block (character 20, the cursor) read as a blank so that
// a blinking cursor never changes what the screen says. The rows run to the last one
// with anything on it. Returns the number of rows; *text receives a malloc'd array of
// malloc'd strings (free with console_rows_free()).
//
static int  console_rows (char ***text)
{
    int            rows = 0, cols = 0, row = 0, col = 0, last = -1, end = 0, k = 0;
    const ccell_t *g    = console_cells (&rows, &cols);
    char         **out  = (char **) calloc ((size_t) rows + 1, sizeof (char *));
    char           u8[4];
    size_t         n    = 0;

    if (out == NULL)  { *text = NULL;  return (0); }
    for (row = 0; row < rows; row = row + 1)
    {
        out[row]  = (char *) malloc ((size_t) cols * 3 + 1);
        if (out[row] == NULL)  continue;
        end = -1;
        for (col = 0; col < cols; col = col + 1)
        {
            const ccell_t *c = g + row * cols + col;
            if ((c -> ch != 0) && (c -> ch != ' ') && (c -> ch != 20))  end = col;
        }
        n = 0;
        for (col = 0; col <= end; col = col + 1)
        {
            const ccell_t *c = g + row * cols + col;
            if (c -> ch == 20)  { out[row][n++] = ' ';  continue; }
            k  = console_utf8 (c -> ch, u8);
            memcpy (out[row] + n, u8, (size_t) k);  n = n + (size_t) k;
        }
        out[row][n]  = '\0';
        if (end >= 0)  last = row;
    }
    *text  = out;
    return (last + 1);
}

static void  console_rows_free (char **text, int allocated)
{
    int  row  = 0;

    if (text == NULL)  return;
    for (row = 0; row < allocated; row = row + 1)  free (text[row]);
    free (text);
}

static int  console_row_count (void)
{
    int  rows = 0, cols = 0;

    console_cells (&rows, &cols);
    return (rows);
}

//
// gpu_console_snapshot(): the screen as one plain string (rows joined by newlines), what
// `until` compares from frame to frame. malloc'd.
//
char *gpu_console_snapshot (void)
{
    char   **text  = NULL;
    int      used  = console_rows (&text);
    int      row   = 0;
    size_t   len   = 1;
    char    *out   = NULL;

    for (row = 0; row < used; row = row + 1)  len = len + strlen (text[row]) + 1;
    out  = (char *) calloc (len, 1);
    for (row = 0; (out != NULL) && (row < used); row = row + 1)
    {
        strcat (out, text[row]);
        strcat (out, "\n");
    }
    console_rows_free (text, console_row_count ());
    return (out);
}

//
// gpu_console_match(): is PATTERN on the screen? The pattern is looked for inside each of
// the last `lastrows` rows (0 = all of them) in turn: a pattern that starts with '^' must
// match from the start of a row, one that ends with '$' up to its end (so "A:\>$" is a
// row that ends with the DOS prompt and "^Ready" a row that starts with Ready).
//
int  gpu_console_match (const char *pattern, uint32_t lastrows)
{
    char     **text    = NULL;
    int        used    = console_rows (&text);
    int        first   = 0;
    int        row     = 0;
    size_t     plen    = strlen (pattern);
    size_t     rlen    = 0;
    int        head    = FALSE;
    int        tail    = FALSE;
    int        found   = FALSE;
    char       want[512];
    const char *hit    = NULL;

    if ((plen > 0) && (pattern[0] == '^'))                    { head = TRUE;  pattern = pattern + 1;  plen = plen - 1; }
    if ((plen > 0) && (pattern[plen - 1] == '$'))             { tail = TRUE;  plen = plen - 1; }
    snprintf (want, sizeof (want), "%.*s", (int) plen, pattern);
    plen  = strlen (want);

    if ((lastrows > 0) && ((int) lastrows < used))  first = used - (int) lastrows;
    for (row = first; (row < used) && (found == FALSE); row = row + 1)
    {
        rlen = strlen (text[row]);
        if (plen > rlen)  continue;
        if ((head == TRUE) && (tail == TRUE))      found = (rlen == plen) && (0 == strcmp (text[row], want));
        else if (head == TRUE)                     found = (0 == strncmp (text[row], want, plen));
        else if (tail == TRUE)                     found = (0 == strcmp (text[row] + (rlen - plen), want));
        else
        {
            hit    = (plen == 0) ? text[row] : strstr (text[row], want);
            found  = (hit != NULL);
        }
    }
    console_rows_free (text, console_row_count ());
    return (found);
}

//
// gpu_console_attrs(): the colors of the cells `console` prints, as grids of one character
// per cell laid out exactly like the text (so row N, column M of a grid describes row N,
// column M of the text): the 16-color index of the cell's color as a hex digit (0 black,
// 1 red, 2 green, 3 yellow, 4 blue, 5 magenta, 6 cyan, 7 light gray, 8-F the bright
// versions, F being the default white), or '.' for a blank. WHAT is 1 for the foreground,
// 2 for the background, 3 for both (each preceded by a line "-- fg --" / "-- bg --").
//
void  gpu_console_attrs (FILE *out, int what)
{
    int            rows = 0, cols = 0, row = 0, col = 0, last = -1, end = 0, pass = 0, i = 0;
    const ccell_t *g    = console_cells (&rows, &cols);
    uint32_t       color = 0;

    for (row = 0; row < rows; row = row + 1)
    {
        for (col = 0; col < cols; col = col + 1)
        {
            const ccell_t *c = g + row * cols + col;
            if ((c -> ch != 0) && (c -> ch != ' ') && (c -> ch != 20))  last = row;
        }
    }
    for (pass = 1; pass <= 2; pass = pass + 1)
    {
        if ((what & pass) == 0)  continue;
        fprintf (out, "-- %s --\n", (pass == 1) ? "fg" : "bg");
        for (row = 0; row <= last; row = row + 1)
        {
            end = -1;
            for (col = 0; col < cols; col = col + 1)
            {
                const ccell_t *c = g + row * cols + col;
                if ((c -> ch != 0) && (c -> ch != ' ') && (c -> ch != 20))  end = col;
            }
            for (col = 0; col <= end; col = col + 1)
            {
                const ccell_t *c = g + row * cols + col;
                color  = (pass == 1) ? c -> fg : c -> bg;
                if (pass == 1)
                {
                    for (i = 0; i < console_ncursor; i = i + 1)
                    {
                        if ((console_cursor != 0) && (c -> ch == console_cursor_codes[i]))  color = console_cursor;
                    }
                }
                if ((c -> ch == 0) || (c -> ch == ' ') || (c -> ch == 20))
                {
                    fputc ((pass == 2) ? (int) "0123456789ABCDEF"[nearest16 (color)] : '.', out);
                }
                else
                {
                    fputc ("0123456789ABCDEF"[nearest16 (color)], out);
                }
            }
            fputc ('\n', out);
        }
    }
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
        console_put (IPORTGET(GPU_SelectedRegion), (int32_t) px - hotX + minX, (int32_t) py - hotY + minY, mult);
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
            console_clear ();
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
