#include "defines.h"

void  usage (int8_t *program)
{
    fprintf (stdout, "Usage: %s [OPTION]... --cartfile cart.v32\n", program);
    fprintf (stdout, "Debugger/Simulator for Vircon32 Fantasy Console\n\n");
    fprintf (stdout, "Mandatory arguments to long options are mandatory ");
    fprintf (stdout, "for short options too.\n\n");
    fprintf (stdout, " -B, --biosfile=FILE    load this BIOS V32 as BIOS\n");
    fprintf (stdout, " -C, --cartfile=FILE    load this CART V32 as CART\n");
//    fprintf (stdout, " -c, --colors           enable colorized output\n");
//    fprintf (stdout, " -F, --fullstep         do not enable single-step\n");
//    fprintf (stdout, " -s, --stop-at=OFFSET   fullstep until OFFSET\n");
//    fprintf (stdout, " -v, --verbose          enable more verbose output\n");
    fprintf (stdout, " -h, --help             display this information\n\n");
}
