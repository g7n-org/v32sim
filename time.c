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
