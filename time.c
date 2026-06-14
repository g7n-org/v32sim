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

void  prof_time (void)
{
    if (csub               != NULL)
    {
        csub -> CYCLES      = csub -> CYCLES + csub -> COUNT;
        csub -> FRAMES      = csub -> CYCLES / V32_CYCLES_PER_FRAME;
        csub -> time        = (float) csub -> CYCLES / (float) (V32_FRAMES_PER_SECOND * V32_CYCLES_PER_FRAME);
        csub -> CYCLES      = csub -> CYCLES % V32_CYCLES_PER_FRAME;
    }
}
