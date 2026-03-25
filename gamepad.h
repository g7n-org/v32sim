#ifndef _GAMEPAD_H
#define _GAMEPAD_H

struct gamepad_data
{
    uint8_t  connected;
    int32_t  left;
    int32_t  right;
    int32_t  up;
    int32_t  down;
    int32_t  start;
    int32_t  A;
    int32_t  B;
    int32_t  X;
    int32_t  Y;
    int32_t  L;
    int32_t  R;
};
typedef struct gamepad_data  gamepad_t;

#endif
