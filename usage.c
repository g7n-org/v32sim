#include "defines.h"

void  usage (int8_t *program)
{
    fprintf (stdout, "Usage: %s [OPTION]... CARTFILE.v32\n", program);
    fprintf (stdout, "Debugger/Simulator for Vircon32 Fantasy Console\n\n");
    fprintf (stdout, "Mandatory arguments to long options are mandatory ");
    fprintf (stdout, "for short options too.\n\n");
    fprintf (stdout, " -B, --biosfile=FILE       load this BIOS V32 file as BIOS\n");
    fprintf (stdout, " -b, --break=OFFSET|LABEL  set breakpoint at OFFSET/LABEL\n");
    fprintf (stdout, " -C, --command-file=FILE   load this file with sim commands\n");
    fprintf (stdout, " -c, --colors              enable colorized output\n");
    fprintf (stdout, " -d, --deref-addr          output address of dereference\n");
    fprintf (stdout, " -E, --entry-point=OFFSET  set simulator entry point\n");
    fprintf (stdout, " -n, --no-debug            do not process any debug files\n");
    fprintf (stdout, " -r, --run                 do not enable single-step mode\n");
    fprintf (stdout, " -w, --watch-for=OPCODE    run until OPCODE is encountered\n");
    fprintf (stdout, " -v, --verbose             enable more verbose output\n");
    fprintf (stdout, " -h, --help                display this information\n\n");
    fprintf (stdout, "FILE   is any path plus the filename desired\n");
    fprintf (stdout, "OFFSET is the full 32-bit/4-byte memory addres (hex)\n");
    fprintf (stdout, "OPCODE is the full 32-bit/4-byte instruction hex\n\n");

    exit (0);
}
