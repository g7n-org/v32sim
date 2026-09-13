#ifndef _GAMEPAD_H
#define _GAMEPAD_H

struct gamepad_data
{
    uint8_t  connected;
    int32_t  button[11];
};
typedef struct gamepad_data  gamepad_t;

#endif
