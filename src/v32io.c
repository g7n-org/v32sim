#include "defines.h"
#include "biosfont.h"
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

////////////////////////////////////////////////////////////////////////////////////////
//
// v32io.c: the v32kbd keyboard on a gamepad port (see v32io.h)
//
// Text -> events pipeline:
//
//   host bytes (kbd type, --kbd-input)          txt[]   pending text, UTF-8 with
//        |                                              terminal escape sequences
//        v   drain(): one token at a time, only while the event queue has room
//   key events  (code, press/release)           q[]     at most 256 events
//        |
//        v   deliver(): one event per frame, in update_ioports()
//   gamepad controls                            the INP ports of the gamepad
//
// Because the text waits in txt[] until the queue has room, nothing is lost when a lot
// of text arrives at once (a paste, a script): it is typed at the protocol's pace of
// one event per frame, as fast as the simulator runs frames.
//
////////////////////////////////////////////////////////////////////////////////////////

#define  TXT_MAX     (1024 * 1024)
#define  READ_MAX    4096
#define  KEYOP       0xFF                 // in the text stream: KEYOP, 1=press|0=release, code
#define  DRAIN_ROOM  16                   // room an input token may need in the queue

typedef struct
{
    uint8_t  code;
    uint8_t  pressed;
    uint8_t  cont;                        // payload of the codepage prefix before it
} kev_t;

static struct
{
    int             port;                 // gamepad port of the device, -1 = none
    kev_t           q[V32KBD_MAX_EVENTS];
    int             qhead;
    int             qcount;
    uint8_t         shift;                // modifiers held by the text injector
    uint8_t         ctrl;
    uint8_t         alt;

    uint8_t        *txt;                  // pending text
    size_t          txtlen;
    size_t          txtpos;
    int             txtfinal;             // the end of txt is the end of its last token

    int             fd;                   // host input source, -1 = none
    int             keepfd;               // do not close it (stdin)
    int             istty;
    int             eofignore;            // FIFO or terminal: no data is not the end
    int             raw;                  // terminal is in raw mode
    struct termios  saved;
    char            spec[256];

    uint32_t        delivered;            // events delivered to the program
    uint32_t        dropped;              // events lost because the queue was full
    uint32_t        unmapped;             // characters the BIOS font does not have
    uint32_t        chars;                // characters typed
} kbd = { .port = -1, .fd = -1 };

////////////////////////////////////////////////////////////////////////////////////////
//
// the event queue
//
static int  q_room (void)
{
    return (V32KBD_MAX_EVENTS - kbd.qcount);
}

static void  q_pop (void)
{
    kbd.qhead   = (kbd.qhead + 1) % V32KBD_MAX_EVENTS;
    kbd.qcount  = kbd.qcount - 1;
}

//
// q_add(): add events that must stay together (a codepage prefix and its payload).
// When the program is not reading events the oldest are discarded, but never half of a
// character: if the front is the payload of a prefix that was already reported, there is
// no room and the new events are lost (the policy of the emulator)
//
static void  q_add (const kev_t *ev, int n)
{
    int  i  = 0;

    while ((kbd.qcount + n > V32KBD_MAX_EVENTS) && (kbd.qcount > 0))
    {
        if (kbd.q[kbd.qhead].cont != 0)
        {
            kbd.dropped  = kbd.dropped + (uint32_t) n;
            return;
        }
        q_pop ();
        kbd.dropped  = kbd.dropped + 1;
        if ((kbd.qcount > 0) && (kbd.q[kbd.qhead].cont != 0))
        {
            q_pop ();
            kbd.dropped  = kbd.dropped + 1;
        }
    }
    for (i = 0; i < n; i = i + 1)
    {
        kbd.q[(kbd.qhead + kbd.qcount) % V32KBD_MAX_EVENTS]  = ev[i];
        kbd.qcount                                           = kbd.qcount + 1;
    }
}

static void  key_event (int code, int pressed)
{
    kev_t  ev;

    ev.code     = (uint8_t) (code & 127);
    ev.pressed  = (pressed != 0) ? 1 : 0;
    ev.cont     = 0;
    q_add (&ev, 1);
}

static void  tap (int code)
{
    key_event (code, 1);
    key_event (code, 0);
}

//
// a typed character of the BIOS font: both events are key releases, so a program
// with a library from before the extension never sees a key press
//
static void  type_bios (int character)
{
    kev_t  ev[2];

    ev[0].code     = (uint8_t) ((character >= 128) ? V32KEY_PAGE_HIGH : V32KEY_PAGE_LOW);
    ev[0].pressed  = 0;
    ev[0].cont     = 0;
    ev[1].code     = (uint8_t) (character & 0x7F);
    ev[1].pressed  = 0;
    ev[1].cont     = 1;
    q_add (ev, 2);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// modifiers: shift, control and alt are held across consecutive characters that need
// them, and released when the next one does not (or when there is no more text)
//
static void  set_mods (int shift, int ctrl, int alt)
{
    shift  = (shift != 0);
    ctrl   = (ctrl  != 0);
    alt    = (alt   != 0);

    if ((kbd.shift != 0) && (shift == 0))  { key_event (V32KEY_LSHIFT, 0);  kbd.shift  = 0; }
    if ((kbd.ctrl  != 0) && (ctrl  == 0))  { key_event (V32KEY_LCTRL,  0);  kbd.ctrl   = 0; }
    if ((kbd.alt   != 0) && (alt   == 0))  { key_event (V32KEY_LALT,   0);  kbd.alt    = 0; }
    if ((kbd.shift == 0) && (shift != 0))  { key_event (V32KEY_LSHIFT, 1);  kbd.shift  = 1; }
    if ((kbd.ctrl  == 0) && (ctrl  != 0))  { key_event (V32KEY_LCTRL,  1);  kbd.ctrl   = 1; }
    if ((kbd.alt   == 0) && (alt   != 0))  { key_event (V32KEY_LALT,   1);  kbd.alt    = 1; }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// a US layout: the key (unshifted character) that types a printable ASCII character,
// and whether shift is needed. The shifted symbols are those of v32kbd_symbol() of the
// keyboard.h library
//
static void  us_layout (int c, int *code, int *shift)
{
    static const char  from[]  = "!@#$%^&*()_+{}|:\"<>?~";
    static const char  to[]    = "1234567890-=[]\\;',./`";
    const char        *p       = NULL;

    *shift  = 0;
    *code   = c;
    if ((c >= 'A') && (c <= 'Z'))
    {
        *code   = c + 32;
        *shift  = 1;
    }
    else if ((p = strchr (from, c)) != NULL)
    {
        *code   = to[p - from];
        *shift  = 1;
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// bios_code(): the BIOS font character that a Unicode character types, 0 if the font
// does not have it
//
static int  bios_code (uint32_t unicode)
{
    int  low   = 0;
    int  high  = (int) BIOS_CHARS - 1;
    int  mid   = 0;

    if ((unicode >= 32) && (unicode <= 126))
    {
        return ((int) unicode);
    }
    while (low <= high)
    {
        mid  = (low + high) / 2;
        if (bios_chars[mid].unicode == unicode)  return (bios_chars[mid].code);
        if (bios_chars[mid].unicode <  unicode)  low   = mid + 1;
        else                                     high  = mid - 1;
    }
    return (0);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// type_char(): one decoded character (a code point, or an ASCII control code), typed
// the way a keyboard types it
//
static void  type_char (uint32_t c, int alt)
{
    int  code   = 0;
    int  shift  = 0;

    kbd.chars  = kbd.chars + 1;
    if (c < 32 || c == 127)
    {
        switch (c)
        {
            case '\r':
            case '\n':  set_mods (0, 0, alt);  tap (V32KEY_ENTER);      break;
            case '\t':  set_mods (0, 0, alt);  tap (V32KEY_TAB);        break;
            case 8:
            case 127:   set_mods (0, 0, alt);  tap (V32KEY_BACKSPACE);  break;
            case 0:                                                      break;
            case 0x1C:  set_mods (0, 1, alt);  tap ('\\');              break;
            case 0x1E:
            case 0x1F:  kbd.unmapped  = kbd.unmapped + 1;               break;
            default:    set_mods (0, 1, alt);  tap ('a' + (int) c - 1); break;   // Ctrl-A ...
        }
    }
    else if (c < 127)
    {
        us_layout ((int) c, &code, &shift);
        set_mods (shift, 0, alt);
        tap (code);
    }
    else
    {
        code  = bios_code (c);
        if (code == 0)
        {
            kbd.unmapped  = kbd.unmapped + 1;
        }
        else
        {
            set_mods (0, 0, alt);
            type_bios (code);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// escape(): a terminal escape sequence at the start of p (p[0] is ESC). Returns the
// bytes used, 0 if the sequence may continue in bytes that have not arrived
//
static void  key_mods (int code, int mod)
{
    // xterm modifier parameter: 1 + (1 shift, 2 alt, 4 ctrl)
    mod = (mod > 1) ? mod - 1 : 0;
    set_mods (mod & 1, mod & 4, mod & 2);
    tap (code);
}

static int  escape (const uint8_t *p, int n, int final)
{
    int  i         = 2;
    int  param[4]  = { 0, 0, 0, 0 };
    int  np        = 0;
    int  have      = 0;
    int  code      = 0;
    int  mod       = 0;
    int  fc        = 0;

    if (n < 2)
    {
        if (final == 0)  return (0);
        set_mods (0, 0, 0);
        tap (V32KEY_ESCAPE);
        return (1);
    }

    if (p[1] == '[')                                     // CSI
    {
        while ((i < n) && (((p[i] >= '0') && (p[i] <= '9')) || (p[i] == ';')))
        {
            if (p[i] == ';')
            {
                if (np < 3)  np  = np + 1;
                have  = 0;
            }
            else
            {
                param[np]  = param[np] * 10 + (p[i] - '0');
                have       = 1;
            }
            i  = i + 1;
        }
        (void) have;
        if (i >= n)
        {
            return ((final == 0) ? 0 : n);
        }
        fc   = p[i];
        mod  = (np >= 1) ? param[1] : 0;
        code = 0;
        switch (fc)
        {
            case 'A':  code  = V32KEY_UP;     break;
            case 'B':  code  = V32KEY_DOWN;   break;
            case 'C':  code  = V32KEY_RIGHT;  break;
            case 'D':  code  = V32KEY_LEFT;   break;
            case 'Z':  code  = V32KEY_TAB;    mod  = 2;  break;           // back-tab
            case '~':
                switch (param[0])
                {
                    case 3:   code  = V32KEY_DELETE;                  break;
                    case 11:  case 12:  case 13:  case 14:  case 15:
                              code  = V32KEY_F1 + (param[0] - 11);    break;   // F1-F5
                    case 17:  case 18:  case 19:  case 20:  case 21:
                              code  = V32KEY_F1 + 5 + (param[0] - 17); break;  // F6-F10
                    case 23:  case 24:
                              code  = V32KEY_F1 + 10 + (param[0] - 23); break; // F11-F12
                }
                break;
        }
        if (code != 0)
        {
            key_mods (code, mod);
        }
        return (i + 1);
    }

    if (p[1] == 'O')                                     // SS3
    {
        if (n < 3)
        {
            return ((final == 0) ? 0 : n);
        }
        switch (p[2])
        {
            case 'A':  code  = V32KEY_UP;     break;
            case 'B':  code  = V32KEY_DOWN;   break;
            case 'C':  code  = V32KEY_RIGHT;  break;
            case 'D':  code  = V32KEY_LEFT;   break;
            case 'P':  case 'Q':  case 'R':  case 'S':
                       code  = V32KEY_F1 + (p[2] - 'P');  break;
        }
        if (code != 0)
        {
            key_mods (code, 0);
        }
        return (3);
    }

    if (p[1] == 0x1B)                                    // ESC ESC: an Escape key
    {
        set_mods (0, 0, 0);
        tap (V32KEY_ESCAPE);
        return (1);
    }

    type_char (p[1], 1);                                 // ESC x: Alt+x
    return (2);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// token(): decode and type one unit of input. Returns the bytes used, 0 if more bytes
// are needed to tell
//
static int  token (const uint8_t *p, int n, int final)
{
    int       len  = 0;
    int       i    = 0;
    uint32_t  cp   = 0;

    if (p[0] == KEYOP)                                   // 'kbd key': an explicit key event
    {
        if (n < 3)
        {
            return ((final == 0) ? 0 : n);
        }
        key_event (p[2], p[1]);
        return (3);
    }
    if (p[0] == 0x1B)
    {
        return (escape (p, n, final));
    }
    if (p[0] < 0x80)
    {
        type_char (p[0], 0);
        return (1);
    }

    len = ((p[0] & 0xE0) == 0xC0) ? 2 : ((p[0] & 0xF0) == 0xE0) ? 3 : ((p[0] & 0xF8) == 0xF0) ? 4 : 0;
    if (len == 0)
    {
        return (1);                                      // stray byte
    }
    if (n < len)
    {
        return ((final == 0) ? 0 : n);                   // cut off
    }
    cp  = p[0] & (0xFF >> (len + 1));
    for (i = 1; i < len; i = i + 1)
    {
        if ((p[i] & 0xC0) != 0x80)
        {
            return (i);                                  // not UTF-8: skip what we read
        }
        cp  = (cp << 6) | (p[i] & 0x3F);
    }
    type_char (cp, 0);
    return (len);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// drain(): type pending text while the queue has room for it
//
static void  drain (void)
{
    int  used  = 0;

    while ((kbd.txtpos < kbd.txtlen) && (q_room () >= DRAIN_ROOM))
    {
        used  = token (kbd.txt + kbd.txtpos, (int) (kbd.txtlen - kbd.txtpos), kbd.txtfinal);
        if (used <= 0)
        {
            break;
        }
        kbd.txtpos  = kbd.txtpos + (size_t) used;
    }
    if (kbd.txtpos >= kbd.txtlen)
    {
        kbd.txtpos  = 0;
        kbd.txtlen  = 0;
        if (((kbd.shift != 0) || (kbd.ctrl != 0) || (kbd.alt != 0)) && (q_room () >= 4))
        {
            set_mods (0, 0, 0);                          // nothing more to type
        }
    }
}

//
// v32io_kbd_text(): add host text (UTF-8, with terminal escape sequences) to what is
// to be typed. 'final' says the text ends with a complete token: a lone ESC is then the
// Escape key
//
void  v32io_kbd_text (const uint8_t *text, size_t len, int final)
{
    uint8_t  *grown  = NULL;

    if (len == 0)
    {
        return;
    }
    if (kbd.txtlen + len > TXT_MAX)
    {
        len  = TXT_MAX - kbd.txtlen;
    }
    grown  = (uint8_t *) realloc (kbd.txt, kbd.txtlen + len + 1);
    if (grown == NULL)
    {
        return;
    }
    kbd.txt  = grown;
    memcpy (kbd.txt + kbd.txtlen, text, len);
    kbd.txtlen    = kbd.txtlen + len;
    kbd.txtfinal  = final;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// the host terminal
//
static void  term_restore (void)
{
    if ((kbd.raw != 0) && (kbd.fd >= 0))
    {
        tcsetattr (kbd.fd, TCSANOW, &kbd.saved);
    }
    kbd.raw  = 0;
}

void  v32io_term_suspend (void)
{
    term_restore ();
}

void  v32io_term_resume (void)
{
    struct termios  t;

    if ((kbd.istty == 0) || (kbd.fd < 0) || (kbd.raw != 0))
    {
        return;
    }
    if (tcgetattr (kbd.fd, &kbd.saved) != 0)
    {
        return;
    }
    t            = kbd.saved;
    t.c_lflag   &= ~(tcflag_t) (ICANON | ECHO | ISIG | IEXTEN);
    t.c_iflag   &= ~(tcflag_t) (ICRNL | INLCR | IGNCR | IXON | BRKINT);
    t.c_cc[VMIN]   = 0;
    t.c_cc[VTIME]  = 0;
    if (tcsetattr (kbd.fd, TCSANOW, &t) == 0)
    {
        kbd.raw  = 1;
    }
}

void  v32io_shutdown (void)
{
    term_restore ();
}

////////////////////////////////////////////////////////////////////////////////////////
//
// the host input source
//
static void  source_close (void)
{
    term_restore ();
    if ((kbd.fd >= 0) && (kbd.keepfd == 0))
    {
        close (kbd.fd);
    }
    kbd.fd      = -1;
    kbd.keepfd  = 0;
    kbd.istty   = 0;
    kbd.spec[0] = '\0';
}

int  v32io_kbd_source (const char *spec)
{
    struct stat  st;
    int          fd  = -1;
    int          n   = 0;

    source_close ();
    if ((spec == NULL) || (0 == strcmp (spec, "off")) || (spec[0] == '\0'))
    {
        return (0);
    }

    if ((0 == strcmp (spec, "stdin")) || (0 == strcmp (spec, "-")))
    {
        fd           = STDIN_FILENO;
        kbd.keepfd   = 1;
    }
    else if (1 == sscanf (spec, "fd:%d%n", &fd, &n) && (spec[n] == '\0') && (fd >= 0))
    {
        kbd.keepfd   = 1;
    }
    else
    {
        fd  = open (spec, O_RDONLY | O_NONBLOCK);
        if (fd < 0)
        {
            fprintf (stderr, "[ERROR] kbd: cannot open '%s' as a host input\n", spec);
            return (-1);
        }
    }

    kbd.fd     = fd;
    kbd.istty  = isatty (fd);
    kbd.eofignore = 0;
    if ((fstat (fd, &st) == 0) && S_ISFIFO (st.st_mode) && (fd != STDIN_FILENO))
    {
        kbd.eofignore  = 1;                              // writers come and go
    }
    if (kbd.istty != 0)
    {
        kbd.eofignore  = 1;
    }
    fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) | O_NONBLOCK);
    snprintf (kbd.spec, sizeof (kbd.spec), "%s", spec);
    atexit (v32io_shutdown);
    if (runflag == TRUE)
    {
        v32io_term_resume ();
    }
    return (0);
}

//
// source_poll(): take what the host has typed. Only while the program runs: at the
// prompt (or when single-stepping) the input belongs to the debugger
//
static void  source_poll (void)
{
    uint8_t  buf[READ_MAX];
    ssize_t  n     = 0;
    ssize_t  i     = 0;
    ssize_t  keep  = 0;
    size_t   room  = 0;

    if ((kbd.fd < 0) || (runflag != TRUE))
    {
        return;
    }
    if (kbd.txtlen > (size_t) (4 * READ_MAX))
    {
        return;                                          // plenty waiting: backpressure
    }
    room  = READ_MAX;
    n     = read (kbd.fd, buf, room);
    if (n < 0)
    {
        if ((errno != EAGAIN) && (errno != EINTR) && (errno != EWOULDBLOCK))
        {
            source_close ();
        }
        return;
    }
    if (n == 0)
    {
        if (kbd.eofignore == 0)
        {
            fprintf (verbose, "[v32kbd] end of the host input\n");
            source_close ();
        }
        return;
    }

    // Ctrl-] from a terminal is not for the program: it asks for the debugger
    for (i = 0; i < n; i = i + 1)
    {
        if ((kbd.istty != 0) && (buf[i] == V32KBD_ATTENTION))
        {
            fprintf (stdout, "\n[v32kbd] attention (Ctrl-])\n");
            runflag  = FALSE;
        }
        else
        {
            buf[keep]  = buf[i];
            keep       = keep + 1;
        }
    }
    v32io_kbd_text (buf, (size_t) keep, (n < (ssize_t) room) ? 1 : 0);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// the device
//
int  v32io_kbd_attach (int port)
{
    if ((port < 0) || (port >= V32_NUM_GAMEPADS))
    {
        fprintf (stderr, "[ERROR] kbd: no gamepad %d (valid: 0-3)\n", port);
        return (-1);
    }
    if ((kbd.port >= 0) && (kbd.port != port))
    {
        v32io_kbd_detach ();
    }
    kbd.port  = port;
    gamepad_connect ((uint8_t) port, TRUE);
    v32io_kbd_clear ();
    return (0);
}

void  v32io_kbd_detach (void)
{
    if (kbd.port >= 0)
    {
        gamepad_connect ((uint8_t) kbd.port, FALSE);
    }
    kbd.port  = -1;
    v32io_kbd_clear ();
}

int  v32io_kbd_port (void)
{
    return (kbd.port);
}

void  v32io_kbd_clear (void)
{
    kbd.qhead   = 0;
    kbd.qcount  = 0;
    kbd.txtlen  = 0;
    kbd.txtpos  = 0;
    kbd.shift   = 0;
    kbd.ctrl    = 0;
    kbd.alt     = 0;
}

//
// deliver(): an event reaches the gamepad. The code goes on Start, A, B, X, Y, L, R;
// Up (pressed) or Down (released) says what happened; the strobe switches side. A
// control that is already in the state asked for is left alone, and pressing one
// direction releases its opposite, so each event leaves a consistent state
//
static void  deliver (const kev_t *ev)
{
    uint8_t  pad   = (uint8_t) kbd.port;
    int      bit   = 0;
    int      left  = 0;

    for (bit = 0; bit < 7; bit = bit + 1)
    {
        gamepad_control (pad, (uint8_t) (4 + bit), (uint8_t) ((ev -> code >> bit) & 1));
    }
    gamepad_control (pad, (ev -> pressed != 0) ? 2 : 3, TRUE);
    left  = gamepad_pressed (pad, 0);
    gamepad_control (pad, (left != 0) ? 1 : 0, TRUE);
}

void  v32io_frame (void)
{
    kev_t  ev;

    if (kbd.port < 0)
    {
        return;
    }
    if ((gamepad + kbd.port) -> connected != TRUE)
    {
        v32io_kbd_clear ();                              // someone unplugged it
        return;
    }
    source_poll ();
    drain ();
    if (kbd.qcount > 0)
    {
        ev  = kbd.q[kbd.qhead];
        q_pop ();
        deliver (&ev);
        kbd.delivered  = kbd.delivered + 1;
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// kbd type: text with C escapes (\n \r \t \b \e \\ \" \xHH \uHHHH), optionally in quotes
//
int  v32io_kbd_type (const char *arg)
{
    size_t    len  = strlen (arg);
    uint8_t  *out  = NULL;
    size_t    o    = 0;
    size_t    i    = 0;
    unsigned  v    = 0;
    int       k    = 0;
    int       d    = 0;

    if (kbd.port < 0)
    {
        fprintf (stderr, "[ERROR] kbd: no v32kbd is attached (use 'kbd attach N' or --v32kbd)\n");
        return (-1);
    }
    if ((len >= 2) && (arg[0] == '"') && (arg[len - 1] == '"'))
    {
        arg  = arg + 1;
        len  = len - 2;
    }
    out  = (uint8_t *) malloc (len * 4 + 1);
    if (out == NULL)
    {
        return (-1);
    }
    for (i = 0; i < len; i = i + 1)
    {
        if ((arg[i] != '\\') || (i + 1 >= len))
        {
            out[o++]  = (uint8_t) arg[i];
            continue;
        }
        i  = i + 1;
        switch (arg[i])
        {
            case 'n':  out[o++]  = '\n';  break;
            case 'r':  out[o++]  = '\r';  break;
            case 't':  out[o++]  = '\t';  break;
            case 'b':  out[o++]  = 8;     break;
            case 'e':  out[o++]  = 0x1B;  break;
            case '\\': out[o++]  = '\\';  break;
            case '"':  out[o++]  = '"';   break;
            case 'x':
            case 'u':
                v  = 0;
                for (k = 0, d = (arg[i] == 'x') ? 2 : 4; (k < d) && (i + 1 < len) && isxdigit ((unsigned char) arg[i + 1]); k = k + 1)
                {
                    i  = i + 1;
                    v  = v * 16 + (unsigned) ((arg[i] <= '9') ? arg[i] - '0' : (tolower ((unsigned char) arg[i]) - 'a' + 10));
                }
                if (arg[i - k] == 'x' && 0)  { }
                if (v < 0x80)           { out[o++]  = (uint8_t) v; }
                else if (v < 0x800)     { out[o++]  = (uint8_t) (0xC0 | (v >> 6));  out[o++]  = (uint8_t) (0x80 | (v & 0x3F)); }
                else                    { out[o++]  = (uint8_t) (0xE0 | (v >> 12)); out[o++]  = (uint8_t) (0x80 | ((v >> 6) & 0x3F));
                                          out[o++]  = (uint8_t) (0x80 | (v & 0x3F)); }
                break;
            default:   out[o++]  = '\\';  out[o++]  = (uint8_t) arg[i];  break;
        }
    }
    v32io_kbd_text (out, o, 1);
    free (out);
    return (0);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// kbd key: a key by name (or a character, or 0xNN), optionally with modifiers:
// "ctrl+alt+delete", "shift+tab", "a". Mode: tap (press and release), down, up
//
static const struct
{
    const char *name;
    int         code;
} keynames[] =
{
    { "up", V32KEY_UP }, { "down", V32KEY_DOWN }, { "left", V32KEY_LEFT }, { "right", V32KEY_RIGHT },
    { "capslock", V32KEY_CAPSLOCK }, { "lshift", V32KEY_LSHIFT }, { "rshift", V32KEY_RSHIFT },
    { "backspace", V32KEY_BACKSPACE }, { "tab", V32KEY_TAB }, { "lctrl", V32KEY_LCTRL },
    { "rctrl", V32KEY_RCTRL }, { "lalt", V32KEY_LALT }, { "enter", V32KEY_ENTER },
    { "return", V32KEY_ENTER }, { "ralt", V32KEY_RALT }, { "escape", V32KEY_ESCAPE },
    { "esc", V32KEY_ESCAPE }, { "lgui", V32KEY_LGUI }, { "rgui", V32KEY_RGUI },
    { "delete", V32KEY_DELETE }, { "del", V32KEY_DELETE }, { "space", ' ' },
    { "shift", V32KEY_LSHIFT }, { "ctrl", V32KEY_LCTRL }, { "alt", V32KEY_LALT },
    { "gui", V32KEY_LGUI }, { NULL, 0 }
};

static int  key_code (const char *name)
{
    int       i     = 0;
    char      lc[32];
    char     *end   = NULL;
    long      v     = 0;

    if (strlen (name) >= sizeof (lc))
    {
        return (0);
    }
    for (i = 0; name[i] != '\0'; i = i + 1)
    {
        lc[i]  = (char) tolower ((unsigned char) name[i]);
    }
    lc[i]  = '\0';

    for (i = 0; keynames[i].name != NULL; i = i + 1)
    {
        if (0 == strcmp (keynames[i].name, lc))  return (keynames[i].code);
    }
    if ((lc[0] == 'f') && (lc[1] != '\0'))
    {
        v  = strtol (lc + 1, &end, 10);
        if ((*end == '\0') && (v >= 1) && (v <= 12))  return (V32KEY_F1 + (int) v - 1);
    }
    if ((lc[0] == '0') && (lc[1] == 'x'))
    {
        v  = strtol (lc, &end, 16);
        if ((*end == '\0') && (v > 0) && (v < 128))  return ((int) v);
    }
    if (lc[1] == '\0')
    {
        if ((lc[0] > 32) && (lc[0] < 127) && (strchr ("!@#$%^&*()_+{}|:\"<>?~", lc[0]) == NULL))
        {
            return (lc[0]);
        }
    }
    return (0);
}

static void  op (int code, int pressed)
{
    uint8_t  b[3];

    b[0] = KEYOP;
    b[1] = (uint8_t) (pressed != 0);
    b[2] = (uint8_t) (code & 127);
    v32io_kbd_text (b, 3, 1);
}

int  v32io_kbd_key (const char *spec, const char *mode)
{
    char      work[128];
    char     *part[6];
    int       n      = 0;
    int       i      = 0;
    int       code   = 0;
    int       m[6];
    char     *p      = NULL;

    if (kbd.port < 0)
    {
        fprintf (stderr, "[ERROR] kbd: no v32kbd is attached (use 'kbd attach N' or --v32kbd)\n");
        return (-1);
    }
    snprintf (work, sizeof (work), "%s", spec);

    // split at '+', but a lone '+' is the key itself
    p  = work;
    while ((n < 6) && (*p != '\0'))
    {
        part[n++]  = p;
        while ((*p != '\0') && !((*p == '+') && (p != part[n - 1]) && (p[1] != '\0')))
        {
            p  = p + 1;
        }
        if (*p == '+')
        {
            *p  = '\0';
            p   = p + 1;
        }
    }
    if (n == 0)
    {
        fprintf (stderr, "[ERROR] kbd key: which key?\n");
        return (-1);
    }
    code  = key_code (part[n - 1]);
    if (code == 0)
    {
        fprintf (stderr, "[ERROR] kbd key: unknown key '%s' (try a letter, f1-f12, enter, esc, tab, backspace,\n"
                         "        delete, up, down, left, right, space, or 0xNN; use 'kbd type' for text)\n", part[n - 1]);
        return (-1);
    }
    for (i = 0; i < n - 1; i = i + 1)
    {
        m[i]  = key_code (part[i]);
        if ((m[i] != V32KEY_LSHIFT) && (m[i] != V32KEY_LCTRL) && (m[i] != V32KEY_LALT) && (m[i] != V32KEY_LGUI))
        {
            fprintf (stderr, "[ERROR] kbd key: '%s' is not a modifier (shift, ctrl, alt, gui)\n", part[i]);
            return (-1);
        }
    }

    // the events join the text still waiting, so that they stay in the order typed
    if ((mode != NULL) && (0 == strcasecmp (mode, "down")))
    {
        op (code, 1);
    }
    else if ((mode != NULL) && (0 == strcasecmp (mode, "up")))
    {
        op (code, 0);
    }
    else if ((mode == NULL) || (mode[0] == '\0') || (0 == strcasecmp (mode, "tap")))
    {
        for (i = 0; i < n - 1; i = i + 1)  op (m[i], 1);
        op (code, 1);
        op (code, 0);
        for (i = n - 2; i >= 0; i = i - 1) op (m[i], 0);
    }
    else
    {
        fprintf (stderr, "[ERROR] kbd key: '%s' is not tap, down or up\n", mode);
        return (-1);
    }
    return (0);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// kbd (status)
//
void  v32io_show (void)
{
    if (kbd.port < 0)
    {
        fprintf (stdout, "v32kbd: not attached\n");
        return;
    }
    fprintf (stdout, "v32kbd: on gamepad %d (%s)\n", kbd.port,
                     ((gamepad + kbd.port) -> connected == TRUE) ? "connected" : "disconnected");
    fprintf (stdout, "  events waiting: %d of %d   text waiting: %zu bytes   modifiers held: %s%s%s\n",
                     kbd.qcount, V32KBD_MAX_EVENTS, kbd.txtlen - kbd.txtpos,
                     kbd.shift ? "shift " : "", kbd.ctrl ? "ctrl " : "", (kbd.shift || kbd.ctrl || kbd.alt) ? (kbd.alt ? "alt" : "") : "none");
    fprintf (stdout, "  delivered: %u events   dropped: %u   characters typed: %u   not in the BIOS font: %u\n",
                     kbd.delivered, kbd.dropped, kbd.chars, kbd.unmapped);
    if (kbd.fd >= 0)
    {
        fprintf (stdout, "  host input: %s%s\n", kbd.spec, kbd.istty ? " (terminal; Ctrl-] returns to the debugger)" : "");
    }
    else
    {
        fprintf (stdout, "  host input: none\n");
    }
}
