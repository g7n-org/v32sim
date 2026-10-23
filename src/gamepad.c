#include "defines.h"

////////////////////////////////////////////////////////////////////////////////////////
//
// gamepad.c: the gamepad backing store
//
// The console keeps, for each of the 4 gamepad ports, the state of its 11 controls as
// a signed frame count: the number of frames it has been pressed (positive) or not
// pressed (negative), clamped to +/-3600. The INP ports show the selected gamepad's
// copy. These routines change a control the way V32GamepadController does:
//
//   * a control that is already in the requested state is left alone (otherwise its
//     count would restart)
//   * pressing a direction releases the opposite one: the console never shows Left and
//     Right, or Up and Down, pressed together. v32io devices are designed around this
//   * nothing happens for a gamepad that is not connected
//
// Control numbers are the INP port number minus 0x402: Left, Right, Up, Down, Start,
// A, B, X, Y, L, R.
//
////////////////////////////////////////////////////////////////////////////////////////

#define  GP_FIRST_PORT   INP_GamepadLeft
#define  GP_CONTROLS     11
#define  GP_UNPRESSED    (-3600)

////////////////////////////////////////////////////////////////////////////////////////
//
// gamepad_control(): press or release one control in the backing store. The INP ports
// are not touched: they are brought up to date by gamepad_publish() (at the frame
// boundary, or at once for the debugger)
//
void  gamepad_control (uint8_t  pad, uint8_t  control, uint8_t  pressed)
{
    gamepad_t  *g     = NULL;
    int32_t    *b     = NULL;
    uint8_t     was   = FALSE;

    if ((pad >= V32_NUM_GAMEPADS) || (control >= GP_CONTROLS))
    {
        return;
    }
    g  = gamepad + pad;
    if (g -> connected != TRUE)
    {
        return;
    }
    b    = g -> button;
    was  = (b[control] > 0) ? TRUE : FALSE;
    if (was == ((pressed != FALSE) ? TRUE : FALSE))
    {
        return;
    }
    b[control]  = (pressed != FALSE) ? 1 : -1;

    if (pressed != FALSE)
    {
        switch (control)
        {
            case 0:  if (b[1] > 0)  b[1]  = -1;  break;   // Left pressed: release Right
            case 1:  if (b[0] > 0)  b[0]  = -1;  break;
            case 2:  if (b[3] > 0)  b[3]  = -1;  break;   // Up pressed: release Down
            case 3:  if (b[2] > 0)  b[2]  = -1;  break;
        }
    }
}

uint8_t  gamepad_pressed (uint8_t  pad, uint8_t  control)
{
    if ((pad >= V32_NUM_GAMEPADS) || (control >= GP_CONTROLS))
    {
        return (FALSE);
    }
    return (((gamepad + pad) -> button[control] > 0) ? TRUE : FALSE);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// gamepad_publish(): copy a gamepad's backing store to the INP ports, if it is the
// selected gamepad (the ports show the selected gamepad only)
//
void  gamepad_publish (uint8_t  pad)
{
    uint8_t    control  = 0;
    data_t    *port     = NULL;

    if ((pad >= V32_NUM_GAMEPADS) || ((uint32_t) IPORTGET(INP_SelectedGamepad) != pad))
    {
        return;
    }
    for (control = 0; control < GP_CONTROLS; control = control + 1)
    {
        port  = ioports_ptr ((uint16_t) (GP_FIRST_PORT + control));
        if (port != NULL)
        {
            port -> value.i32  = (gamepad + pad) -> button[control];
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// gamepad_reset(): all controls one minute unpressed (what a disconnect does)
//
void  gamepad_reset (uint8_t  pad)
{
    uint8_t  control  = 0;

    if (pad >= V32_NUM_GAMEPADS)
    {
        return;
    }
    for (control = 0; control < GP_CONTROLS; control = control + 1)
    {
        (gamepad + pad) -> button[control]  = GP_UNPRESSED;
    }
    gamepad_publish (pad);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// gamepad_connect(): connect or disconnect a gamepad (the debugger's `gamepad connect`
// and the v32io devices). It goes through the INP port so that the selected gamepad's
// ports follow; a disconnect also releases every control
//
void  gamepad_connect (uint8_t  pad, uint8_t  connected)
{
    int32_t  selected  = IPORTGET(INP_SelectedGamepad);

    if (pad >= V32_NUM_GAMEPADS)
    {
        return;
    }
    SYSPORTSET(INP_SelectedGamepad, pad);
    SYSPORTSET(INP_GamepadConnected, (connected != FALSE) ? 1 : 0);
    SYSPORTSET(INP_SelectedGamepad, selected);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// gamepad_io(): the debugger's toggle of one control of the selected gamepad. The
// port number is that of the control (INP_GamepadLeft ... INP_GamepadButtonR)
//
void  gamepad_io (uint16_t  gamepadport)
{
    uint8_t  pad      = (uint8_t) IPORTGET(INP_SelectedGamepad);
    uint8_t  control  = (uint8_t) (gamepadport - GP_FIRST_PORT);
    int32_t  value    = IPORTGET(gamepadport);

    gamepad_control (pad, control, (value > 0) ? FALSE : TRUE);   // pressed -> release
    gamepad_publish (pad);
}
