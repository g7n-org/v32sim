#include "defines.h"

void  usage (int8_t *program)
{
    fprintf (stdout, "Usage: %s [OPTION]... CARTFILE.v32\n", program);
    fprintf (stdout, "Debugger/Simulator for Vircon32 Fantasy Console\n\n");
    fprintf (stdout, "Mandatory arguments to long options are mandatory ");
    fprintf (stdout, "for short options too.\n\n");
    fprintf (stdout, " -B, --biosfile=FILE      load this BIOS V32 file as BIOS\n");
    fprintf (stdout, " -C, --command-file=FILE  load this BIOS V32 file as BIOS\n");
//    fprintf (stdout, " -c, --colors           enable colorized output\n");
    fprintf (stdout, " -i, --index-math         output index math after decode\n");
    fprintf (stdout, " -r, --run                do not enable single-step mode\n");
    fprintf (stdout, " -s, --seek-to=OFFSET     run until OFFSET is encountered\n");
    fprintf (stdout, " -v, --verbose            enable more verbose output\n");
    fprintf (stdout, " -h, --help               display this information\n\n");
}
