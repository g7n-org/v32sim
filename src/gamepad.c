#include "defines.h"

void  gamepad_io (uint16_t  gamepadport)
{
    int32_t  value  = 0;
    value           = IPORTGET(gamepadport); 
    if (value      >  0) // releasing
    {
        value       = -1;
    }
    else // pressing
    {
        value       = 1;
    }
    SYSPORTSET(gamepadport, value); 
}
