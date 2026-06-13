#include "defines.h"

////////////////////////////////////////////////////////////////////////////////////////
//
// timediff_ns(): calculate timespec difference in nanoseconds
//
slli  timediff_ns (TimeSpec *start, TimeSpec *end)
{
    slli  result  = 0;

    result        = (slli) (end -> tv_sec - start -> tv_sec);
    result        = result * 1000000000LL;
    result        = result + (slli) (end -> tv_nsec - start -> tv_nsec);

    return (result);
}

void prof_time (linked_l *node)
{
    uint32_t  frames  = IPORTGET(TIM_FrameCounter) - node -> FRAMES;
    uint32_t  cycles  = IPORTGET(TIM_CycleCounter) - node -> CYCLES;
    uint32_t  tally   = 0;
}
