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

linked_l *prof_time (linked_l *node)
{
    uint32_t  frames     = 0;
    uint32_t  cycles     = 0;

    if (node            != NULL)
    {
        frames           = IPORTGET(TIM_FrameCounter) - node -> FRAMES;
        node  -> FRAMES  = frames;
    ////////////if (node -> time == 0.0)
    ////////////{
        if (frames      == 0) // time duration falls within the same frame
        {
            cycles       = IPORTGET(TIM_CycleCounter) - node -> CYCLES;
        }
        else
        {
            cycles       = frames * V32_CYCLES_PER_FRAME; // convert frames to cycles
            cycles       = cycles - node  -> CYCLES;       // remove non-inclusive cycles
        }
        node  -> time    = (float) cycles / (V32_FRAMES_PER_SECOND * V32_CYCLES_PER_FRAME);
        node  -> CYCLES  = cycles - (frames * V32_CYCLES_PER_FRAME);
////////////    }
    }

    return (node);
}
