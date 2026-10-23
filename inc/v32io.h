#ifndef  _V32IO_H
#define  _V32IO_H

////////////////////////////////////////////////////////////////////////////////////////
//
// v32io.h: emulated v32io peripherals
//
// v32io devices (the RP2350 adapter, or the v32io patch of the DesktopEmulator) look
// like an ordinary gamepad to the console and carry their data on its 11 controls. This
// module emulates the keyboard, v32kbd, on one of the 4 gamepad ports:
//
//   Left / Right   strobe: it switches side at every key event
//   Up / Down      the event is a key press (Up) or a key release (Down)
//   Start A B X Y L R   the 7 bit key code, least significant bit first
//
// A key event is delivered every frame, at most one per frame, exactly as the
// DesktopEmulator does. The events come from
//
//   * the debugger:  kbd type "text\n"   kbd key ctrl+c   kbd key f1 down
//   * a host input source (--kbd-input): a terminal (raw mode: arrow keys, Ctrl keys
//     and escape sequences become key events), a pipe, a FIFO or a file
//
// Text is turned into key events as a keyboard on a US layout would (shift is held
// across consecutive characters that need it), and characters that are not typed on a
// US layout (accents, box drawing) are sent as "typed characters": a codepage prefix
// event followed by a payload event, as the v32io protocol defines.
//
// See docs/v32io/PROTOCOLS.md of the v32io project for the protocol.
//
////////////////////////////////////////////////////////////////////////////////////////

// key codes of the keys that have no ASCII character of their own
#define  V32KEY_NONE             0
#define  V32KEY_UP               1
#define  V32KEY_DOWN             2
#define  V32KEY_LEFT             3
#define  V32KEY_RIGHT            4
#define  V32KEY_CAPSLOCK         5
#define  V32KEY_LSHIFT           6
#define  V32KEY_RSHIFT           7
#define  V32KEY_BACKSPACE        8
#define  V32KEY_TAB              9
#define  V32KEY_LCTRL            10
#define  V32KEY_RCTRL            11
#define  V32KEY_LALT             12
#define  V32KEY_ENTER            13
#define  V32KEY_F1               14
#define  V32KEY_F12              25
#define  V32KEY_RALT             26
#define  V32KEY_ESCAPE           27
#define  V32KEY_LGUI             28
#define  V32KEY_RGUI             29
#define  V32KEY_PAGE_HIGH        30       // codepage prefix: BIOS characters 128-255
#define  V32KEY_PAGE_LOW         31       // codepage prefix: BIOS characters 0-127
#define  V32KEY_DELETE           127

#define  V32KBD_MAX_EVENTS       256      // events waiting to be delivered
#define  V32KBD_ATTENTION        0x1D     // Ctrl-]: back to the debugger prompt

// v32io.c
int       v32io_kbd_attach    (int);                      // device on a gamepad port
void      v32io_kbd_detach    (void);
int       v32io_kbd_port      (void);                     // -1 = none
int       v32io_kbd_source    (const char *);             // host input; NULL/"off" = none
void      v32io_kbd_clear     (void);                     // forget queued events and text
int       v32io_kbd_type      (const char *);             // text with C escapes
int       v32io_kbd_key       (const char *, const char *); // "ctrl+c", "down|up|tap"
void      v32io_kbd_text      (const uint8_t *, size_t, int); // raw bytes (UTF-8, ESC seqs)
void      v32io_frame         (void);                     // once per frame, before it runs
void      v32io_term_resume   (void);                     // execution continues
void      v32io_term_suspend  (void);                     // the prompt takes the terminal
void      v32io_show          (void);                     // 'kbd' command
void      v32io_shutdown      (void);
void      v32io_tty           (int);                      // --tty full-screen mode
int       v32io_tty_on        (void);
void      v32io_tty_frame     (void);

#endif
