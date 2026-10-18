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

////////////////////////////////////////////////////////////////////////////////////////
//
// sigcrash(): SIGSEGV/SIGBUS/SIGFPE/SIGABRT handler. A crash inside the simulator is a
// simulator bug (or a corrupt cart pushing it somewhere it should not go); print the
// simulated machine state so the report is useful, flush any memory card changes, and
// die with the default action (core dump / signal exit status).
//
// Only async-signal-tolerant calls are really safe here, but the process is already
// doomed, so stdio is used for readability.
//
static uint32_t  rr (uint8_t id)
{
    return ((reg != NULL) ? (reg+id) -> value.raw : 0);
}

void sigcrash (int32_t  signum)
{
    static volatile sig_atomic_t  inside  = 0;
    const char  *name  = "signal";

    if (inside != 0)
    {
        signal (signum, SIG_DFL);
        raise  (signum);
        return;
    }
    inside  = 1;

    switch (signum)
    {
        case SIGSEGV:  name  = "SIGSEGV";  break;
        case SIGBUS:   name  = "SIGBUS";   break;
        case SIGFPE:   name  = "SIGFPE";   break;
        case SIGABRT:  name  = "SIGABRT";  break;
    }

    fflush  (stdout);
    fprintf (stderr, "\n[%s] v32sim %s crashed\n", name, VERSION);
    fprintf (stderr, "------------------------------------\n");
    fprintf (stderr, "IP: 0x%.8X  (rom_offset 0x%.8X)\n", rr (IP), rom_offset);
    fprintf (stderr, "IR: 0x%.8X\n", rr (IR));
    fprintf (stderr, "IV: 0x%.8X\n", rr (IV));
    fprintf (stderr, "SP: 0x%.8X  BP: 0x%.8X\n", rr (SP), rr (BP));
    fprintf (stderr, "R0-R3: 0x%.8X 0x%.8X 0x%.8X 0x%.8X\n",
                     rr (R0), rr (R1), rr (R2), rr (R3));
    fprintf (stderr, "sys_error: %d  runflag: %u  action: %u\n", (int) sys_error, runflag, (unsigned) action);
    fprintf (stderr, "------------------------------------\n");
    fprintf (stderr, "Please report this, with the command line and cart, to the v32sim maintainer.\n");

    memc_save ();

    signal (signum, SIG_DFL);
    raise  (signum);
}
