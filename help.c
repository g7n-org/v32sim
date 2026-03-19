#include "defines.h"

void  usage (int8_t *program)
{
    fprintf (stdout, "Usage: %s [OPTION]... [CARTFILE.v32]\n", program);
    fprintf (stdout, "Debugger/Simulator for Vircon32 Fantasy Console\n\n");
    fprintf (stdout, "Mandatory arguments to long options are mandatory ");
    fprintf (stdout, "for short options too.\n\n");
    fprintf (stdout, " -B, --biosfile=FILE       load this BIOS V32 file as BIOS\n");
    fprintf (stdout, " -b, --break=OFFSET|LABEL  set breakpoint at OFFSET/LABEL\n");
    fprintf (stdout, " -C, --command-file=FILE   load this file with sim commands\n");
    fprintf (stdout, " -c, --colors          enable colorized output\n");
    fprintf (stdout, " -d, --deref-addr      output address of dereference\n");
    fprintf (stdout, " -E, --entry-point=OFFSET  set simulator entry point\n");
    fprintf (stdout, " -n, --no-debug        do not process any debug files\n");
    fprintf (stdout, " -r, --run         do not enable single-step mode\n");
    fprintf (stdout, " -w, --watch-for=OPCODE    run until OPCODE is encountered\n");
    fprintf (stdout, " -v, --verbose         enable more verbose output\n");
    fprintf (stdout, " -h, --help        display this information\n\n");
    fprintf (stdout, "FILE   is any path plus the filename desired\n");
    fprintf (stdout, "OFFSET is the full 32-bit/4-byte memory addres (hex)\n");
    fprintf (stdout, "OPCODE is the full 32-bit/4-byte instruction hex\n\n");

    exit (0);
}

void  help  (uint8_t  item)
{
    switch (item)
    {
        case INPUT_HELP:
            fprintf (stdout, "  (b)reak 0xMEM|LABEL   - set breakpoint\n");
            fprintf (stdout, "  (c)ontinue        - resume execution\n");
            fprintf (stdout, "  (p)rint XYZ       - one-time display of XYZ\n");
            fprintf (stdout, "  (d)isplay XYZ LABEL   - add displaylist item:\n");
            fprintf (stdout, "    R#          -   general register\n");
            fprintf (stdout, "    [R#]        -   dereferenced register\n");
            fprintf (stdout, "    I(P|R|V)        -   system register\n");
            fprintf (stdout, "    0xMEM_ADDR      -   4-byte memory address\n");
            fprintf (stdout, "    [0xMEM_ADDR]    -   dereferenced address\n");
            fprintf (stdout, "    0xMEM-0xADDR    -   memory range\n");
            fprintf (stdout, "    [0xMEM-0xADDR]      -   deref memory range\n");
            fprintf (stdout, "    0xIOP           -   IOPort address\n");
            fprintf (stdout, "  (l)abel 0xMEM LABEL   - add label list item\n");
            fprintf (stdout, "  (n)ext        - next (skip subroutines)\n");
            fprintf (stdout, "  (s)tep        - step to next instruction\n");
            fprintf (stdout, "  (i)gnore          - ignore this instruction\n");
            fprintf (stdout, "  (r)eplace X Y Z       - replace:\n");
            fprintf (stdout, "    IP:0xMEM_ADDR       -   with this IP value\n");
            fprintf (stdout, "    IR:0xINSTRUCT       -   with this IR value\n");
            fprintf (stdout, "    IV:0xIMMEDIAT       -   with this IV value\n");
            fprintf (stdout, "  set NAME = VALUE      - set system feature\n");
            fprintf (stdout, "    color:  true/false  -   set color output\n");
            fprintf (stdout, "    deref:  true/false  -   set deref addr\n");
            fprintf (stdout, "    I[PRV]: 0x0ADDRESS  -   set system register\n");
            fprintf (stdout, "    R#:     0xTHEVALUE  -   set register to value\n");
            fprintf (stdout, "    0xMEM:  0xTHEVALUE  -   set memory to value\n");
            fprintf (stdout, "    0xIOP:  0xTHEVALUE  -   set ioport to value\n");
            fprintf (stdout, "  (u)nXYZ           - remove item from list\n");
            fprintf (stdout, "    break           -   remove breakpoint #\n");
            fprintf (stdout, "    display         -   remove displaypoint #\n");
            fprintf (stdout, "    label           -   remove label #\n");
            fprintf (stdout, "  (h)elp/(?)        - display this help\n");
            fprintf (stdout, "  (q)uit        - exit the simulator\n");
            break;

        case INPUT_BREAK:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    break\n");
            fprintf (stdout, "    break 0xMEM_ADDR\n");
            fprintf (stdout, "    break LABEL\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    List or set a breakpoint for the indicated value. A breakpoint\n");
            fprintf (stdout, "    is a trigger to stop execution and present the prompt for input.\n\n");
            break;

        case INPUT_CONTINUE:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    continue\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    Resume execution until next trigger is encountered.\n\n");
            break;

        case INPUT_STEP:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    step\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    Single-step execution of current instruction.\n\n");
            break;

        case INPUT_NEXT:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    next\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    Single-step execution, treating subroutines as one step.\n\n");
            break;

        case INPUT_IGNORE:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    ignore\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    Skip execution of current instruction\n\n");
            fprintf (stdout, "NOTE:\n");
            fprintf (stdout, "    Could cause runtime problems depending on what is ignored.\n\n");
            break;

        case INPUT_LOAD:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    load bios:path/to/biosfile.v32\n");
            fprintf (stdout, "    load cart:path/to/cartfile.v32\n");
            fprintf (stdout, "    load memc:path/to/memcfile.v32\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    Load indicated memory page with file contents\n\n");
            fprintf (stdout, "NOTE:\n");
            fprintf (stdout, "    Meant primarily as the means for runtime swapping out\n");
            fprintf (stdout, "    of MEMCARDs, but can also be used to load a CART or a\n");
            fprintf (stdout, "    BIOS once the simulator is running\n\n");
            break;

        case INPUT_UNLOAD:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    unload bios\n");
            fprintf (stdout, "    unload cart\n");
            fprintf (stdout, "    unload memc\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    Unload indicated memory page, deallocating contents\n\n");
            fprintf (stdout, "NOTE:\n");
            fprintf (stdout, "    Meant primarily as the means for runtime swapping out\n");
            fprintf (stdout, "    of MEMCARDs, but can also be used to unload a BIOS or\n");
            fprintf (stdout, "    a CART once the simulator is running\n\n");
            break;

        case INPUT_DISPLAY:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    display[/fmt] REGISTER [LABEL]\n");
            fprintf (stdout, "    display[/fmt] 0xMEM_ADDR [LABEL]\n");
            fprintf (stdout, "    display[/fmt] 0xIOP [LABEL]\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    Add a displaylist item (displayed at each showing of the prompt).\n");
            fprintf (stdout, "    An optional LABEL can be provided. If the item is enclosed within\n");
            fprintf (stdout, "    square brackets, it will be dereferenced, showing indirect data.\n\n");
            fprintf (stdout, "FORMATTING:\n");
            fprintf (stdout, "    Optional formatting can be applied (otherwise default formatting\n");
            fprintf (stdout, "    will be used, based on the type):\n\n");
            fprintf (stdout, "    /b    display in binary\n");
            fprintf (stdout, "    /B    display as a boolean (TRUE or FALSE)\n");
            fprintf (stdout, "    /d    display as signed decimal\n");
            fprintf (stdout, "    /D    decode value as instruction data\n");
            fprintf (stdout, "    /f    display as floating point decimal\n");
            fprintf (stdout, "    /o    display as octal\n");
            fprintf (stdout, "    /u    display as unsigned decimal\n");
            fprintf (stdout, "    /x    display as lowercase hexadecimal\n");
            fprintf (stdout, "    /X    display as uppercase hexadecimal (default)\n\n");
            fprintf (stdout, "NOTE:\n");
            fprintf (stdout, "    If no label is provided to an IOPort, its symbolic port name will\n");
            fprintf (stdout, "    instead be displayed\n\n");
            fprintf (stdout, "EXAMPLE:\n");
            fprintf (stdout, "    display/o R4            display R4's contents as octal\n");
            fprintf (stdout, "    display   [0x003FFFFE]  dereference address, show contents\n\n");
            break;
    }
}
