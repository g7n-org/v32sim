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
    fprintf (stdout, " -c, --colors              enable colorized output\n");
    fprintf (stdout, " -d, --deref-addr          output address of dereference\n");
    fprintf (stdout, " -e, --errorcheck          enable runtime error checking\n");
    fprintf (stdout, " -E, --entry-point=OFFSET  set simulator entry point\n");
    fprintf (stdout, " -M, --memcfile=FILE       load this file as a MEMCARD\n");
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

void  help  (uint8_t  item)
{
    switch (item)
    {
        case INPUT_HELP:
            fprintf (stdout, "v32sim commands           description\n");
            fprintf (stdout, "========================= ============================\n");
            fprintf (stdout, "  break 0xMEM|LABEL       set breakpoint\n");
            fprintf (stdout, "  continue                resume execution\n");
            fprintf (stdout, "  print XYZ               one-time display of XYZ\n");
            fprintf (stdout, "  display XYZ LABEL       add displaylist item:\n");
            fprintf (stdout, "  label 0xMEM LABEL       add label list item\n");
            fprintf (stdout, "  load memory:file        load component into memory\n");
            fprintf (stdout, "  unload memory           unload component from memory\n");
            fprintf (stdout, "  next                    next (skip subroutines)\n");
            fprintf (stdout, "  step                    step to next instruction\n");
            fprintf (stdout, "  inventory               system resource inventory\n");
            fprintf (stdout, "  ignore                  ignore this instruction\n");
            fprintf (stdout, "  replace X Y Z           replace:\n");
            fprintf (stdout, "  set NAME = VALUE        set system feature\n");
            fprintf (stdout, "  unbreak #/undisplay #   remove item from list\n");
            fprintf (stdout, "  unlabel #               remove label #\n");
            fprintf (stdout, "  help/?                  display this help\n");
            fprintf (stdout, "  quit                    exit the simulator\n");
            break;

        case INPUT_REPLACE:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    replace IP:0xADDR IR:0xINSTRUCT IV:0xIMMEDIATE\n");
            fprintf (stdout, "    replace IR:0xINSTRUCT\n");
            fprintf (stdout, "    replace IR:0xINSTRUCT IV:0xIMMEDIATE\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    Somewhat of a one-shot \"Game-Genie\"-style command,\n");
            fprintf (stdout, "    allowing for the replacement of the indicated system\n");
            fprintf (stdout, "    register(s) with the specified value(s). This will\n");
            fprintf (stdout, "    not replace the values in memory (like a set would),\n");
            fprintf (stdout, "    making it more for a temporary change.\n\n");
            break;

        case INPUT_BREAK:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    break\n");
            fprintf (stdout, "    break 0xMEM_ADDR\n");
            fprintf (stdout, "    break LABEL\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    List or set a breakpoint for the indicated value. A breakpoint\n");
            fprintf (stdout, "    is a trigger to stop execution and present the prompt for input.\n");
            fprintf (stdout, "    This is typically some BIOS or CART offset that would align with\n");
            fprintf (stdout, "    an instruction to process. It can also be used to break when a\n");
            fprintf (stdout, "    subroutine is called, based on its label, which exists at an offset\n\n");
            break;

        case INPUT_LABEL:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    label\n");
            fprintf (stdout, "    label 0xMEM_ADDR LABEL\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    List or set a label for the indicated memory offset. An entry is\n");
            fprintf (stdout, "    made in the label list, allowing for label<->offset lookups to be\n");
            fprintf (stdout, "    performed, utilized by a number of simulator features.\n\n");

        case INPUT_CONTINUE:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    continue\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    Resume execution until next trigger is encountered.\n\n");
            break;

        case INPUT_SET:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    set setting=true|false\n");
            fprintf (stdout, "    set R#|IP|IR|IV=0xTHEVALUE\n");
            fprintf (stdout, "    set 0xMEM_ADDR=0xTHEVALUE\n");
            fprintf (stdout, "    set 0xIOP=0xTHEVALUE\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    Set some system setting or resource to a specified value.\n");
            fprintf (stdout, "    This facility offers a way, during runtime, to alter the\n");
            fprintf (stdout, "    state of registers, memory, and IOPorts for use in testing\n");
            fprintf (stdout, "    or enabling desired conditions. It is also used for setting\n");
            fprintf (stdout, "    simulator configurations during runtime.\n\n");
            fprintf (stdout, "SETTINGS:\n");
            fprintf (stdout, "    color    true/false    set color output\n");
            fprintf (stdout, "    deref    true/false    set deref addr\n");
            fprintf (stdout, "    debug    true/false    set simulator debug mode\n");
            fprintf (stdout, "    errorchk true/false    set simulator error checking\n");
            fprintf (stdout, "    verbose  true/false    set simulator verbosity\n\n");
            fprintf (stdout, "NOTE:\n");
            fprintf (stdout, "    Setting system resources is done outside any access-control\n");
            fprintf (stdout, "    bounds: those resources that are read-only will be impair\n");
            fprintf (stdout, "    the use of set.\n\n");
            fprintf (stdout, "    Assigned values can be any of 0bBINARY, 0OCTAL, +/-SIGNED,\n");
            fprintf (stdout, "    UNSIGNED, 0xHEXADCML, or F.LOAT\n\n");
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
            fprintf (stdout, "    Skip execution of current instruction; cycle and frame counters\n");
            fprintf (stdout, "    are not adjusted, although IP/IR/IV proceed to next instruction\n");
            fprintf (stdout, "    in sequence. Used to avoid an error-causing instruction but not\n");
            fprintf (stdout, "    stop execution.\n\n");
            fprintf (stdout, "NOTE:\n");
            fprintf (stdout, "    Could cause runtime problems depending on what is ignored.\n\n");
            break;

        case INPUT_INVENTORY:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    inventory\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    Display a system resources overview, meant to reveal the current\n");
            fprintf (stdout, "    operational state of the system (CARTs/MEMCARDs loaded, space in\n");
            fprintf (stdout, "    use, etc.)\n\n");
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

        case INPUT_PRINT:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    print/fmt REGISTER\n");
            fprintf (stdout, "    print/fmt [REGISTER]\n");
            fprintf (stdout, "    print/fmt 0xMEM_ADDR\n");
            fprintf (stdout, "    print/fmt [0xMEM_ADDR]\n");
            fprintf (stdout, "    print/fmt 0xMEMADDR1-0xMEMADDR2\n");
            fprintf (stdout, "    print/fmt [0xMEMADDR1-0xMEMADDR2]\n");
            fprintf (stdout, "    print/fmt 0xIOP LABEL\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    A one-time output of the indicated value. Typically used when one\n");
            fprintf (stdout, "    does not want to establish a displaylist item. Functionality is\n");
            fprintf (stdout, "    pretty much identical to that of the display command.\n\n");
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
            fprintf (stdout, "EXAMPLES:\n");
            fprintf (stdout, "    print/o R4            print R4's contents as octal\n");
            fprintf (stdout, "    print   0x500         print CAR_Connected IOPort value\n");
            fprintf (stdout, "    print   [0x003FFFFE]  dereference address, show contents\n\n");
            break;

        case INPUT_DISPLAY:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    display/fmt REGISTER LABEL\n");
            fprintf (stdout, "    display/fmt [REGISTER] LABEL\n");
            fprintf (stdout, "    display/fmt 0xMEM_ADDR LABEL\n");
            fprintf (stdout, "    display/fmt [0xMEM_ADDR] LABEL\n");
            fprintf (stdout, "    display/fmt 0xMEMADDR1-0xMEMADDR2\n");
            fprintf (stdout, "    display/fmt [0xMEMADDR1-0xMEMADDR2]\n");
            fprintf (stdout, "    display/fmt 0xIOP LABEL\n\n");
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
            fprintf (stdout, "    instead be displayed. System registers (IP, IR, IV) are also valid.\n\n");
            fprintf (stdout, "EXAMPLES:\n");
            fprintf (stdout, "    display/o R4            display R4's contents as octal\n");
            fprintf (stdout, "    display   0x500         display CAR_Connected IOPort value\n");
            fprintf (stdout, "    display   [0x003FFFFE]  dereference address, show contents\n\n");
            break;

        case INPUT_UNDO:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    unbreak #\n");
            fprintf (stdout, "    undisplay #\n");
            fprintf (stdout, "    unlabel #\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    Remove indicated list item (by index number). Remaining list items\n");
            fprintf (stdout, "    will be re-indexed\n\n");
            break;
    }
}
