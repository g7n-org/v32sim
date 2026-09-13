#include "defines.h"

////////////////////////////////////////////////////////////////////////////////////////
//
// sigint(): process SIGINT signal
//
void sigint (int32_t  signum)
{
    runflag        = FALSE;
    signal (SIGINT, sigint);
    if (colorflag == TRUE)
    {
        fprintf (stdout, "\e[1;31m");
    }
    fprintf (stdout, "[signal] SIGINT encountered!\n");

    if (colorflag == TRUE)
    {
        fprintf (stdout, "\e[1;32m");
    }

    fprintf (stdout, "v32sim> ");

    if (colorflag == TRUE)
    {
        fprintf (stdout, "\e[m");
    }
}
