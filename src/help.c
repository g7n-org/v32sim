#include "defines.h"

void  usage (int8_t *program)
{
    fprintf (stdout, "Usage: %s [OPTION]... [CARTFILE.v32]\n", program);
    fprintf (stdout, "Debugger/Simulator for Vircon32 Fantasy Console (version %s)\n\n", VERSION);
    fprintf (stdout, "Mandatory arguments to long options are mandatory ");
    fprintf (stdout, "for short options too.\n\n");
    fprintf (stdout, " -B, --biosfile=FILE       load this BIOS V32 file as BIOS\n");
    fprintf (stdout, " -b, --break=OFFSET|LABEL  set breakpoint at OFFSET/LABEL\n");
    fprintf (stdout, " -C, --command-file=FILE   load this file with sim commands\n");
    fprintf (stdout, " -c, --colors              enable colorized output\n");
    fprintf (stdout, " -d, --deref-addr          output address of dereference\n");
    fprintf (stdout, " -D, --debug               enable simulator debug output (development)\n");
    fprintf (stdout, " -e, --errorcheck          enable runtime error checking\n");
    fprintf (stdout, "     --bios-asm-debug=FILE load BIOS asm labels from FILE\n");
    fprintf (stdout, "     --bios-c-debug=FILE   load BIOS C labels from FILE\n");
    fprintf (stdout, "     --cart-asm-debug=FILE load CART asm labels from FILE\n");
    fprintf (stdout, "     --cart-c-debug=FILE   load CART C labels from FILE\n");
    fprintf (stdout, "     --debug-dir=DIR       also look for debug files in DIR (first)\n");
    fprintf (stdout, "     --source              show the source line for each stop (asm; C with -L C)\n");
    fprintf (stdout, " -E, --entry-point=OFFSET  set simulator entry point\n");
    fprintf (stdout, " -L, --langmode=MODE       label/source mode: C or lua (experimental)\n");
    fprintf (stdout, " -M, --memcfile=FILE       load this file as a MEMCARD\n");
    fprintf (stdout, " -n, --no-debug            do not process any debug files\n");
    fprintf (stdout, " -p, --profile             enable instruction profiling\n");
    fprintf (stdout, " -r, --run                 do not enable single-step mode\n");
    fprintf (stdout, " -S, --bios-start          break at BIOS code start\n");
    fprintf (stdout, " -w, --watch-for=OPCODE    run until OPCODE is encountered\n");
    fprintf (stdout, " -v, --verbose             enable more verbose output\n");
    fprintf (stdout, " -V, --version             display version information and exit\n");
    fprintf (stdout, "     --v32kbd[=PORT]       emulate a v32kbd keyboard on gamepad PORT (default 0)\n");
    fprintf (stdout, "     --kbd-input=SOURCE    host key source: stdin, fd:N, FIFO/pipe/file (implies --v32kbd)\n");
    fprintf (stdout, "     --fast                do not throttle to real time (frames run as fast as possible)\n");
    fprintf (stdout, "     --frame-us=N          wall-clock microseconds per frame (default 16666; 0 = --fast)\n");
    fprintf (stdout, "     --max-frames=N        stop after N frames\n");
    fprintf (stdout, "     --fixed-clock[=Y,D,S] TIM_CurrentDate/Time start at year Y, day-of-year D, S seconds\n");
    fprintf (stdout, "                           (default 2026,1,0) instead of the host clock (reproducible runs)\n");
    fprintf (stdout, "     --seed=N              seed the RNG_CurrentValue generator with N at startup\n");
    fprintf (stdout, "     --tty                 full-screen interactive mode: the text console on this terminal, keys\n");
    fprintf (stdout, "                           to a v32kbd (port 0 unless --v32kbd=N); Ctrl-] = debugger\n");
    fprintf (stdout, "     --console-color=MODE  text console colors: none, 16, 256, true, auto (--tty: auto)\n");
    fprintf (stdout, "     --cursor-color=COLOR  recolor the cursor block (BIOS character 20) on the text console\n");
    fprintf (stdout, "     --console             print the BIOS-font text screen when the run ends\n");
    fprintf (stdout, "     --dump=N[@ADDR]       (or ADDR:N) after the run print N RAM words (signed, one per line) from ADDR\n");
    fprintf (stdout, "                           (hex, or a label; NAME also tries global_NAME), default RAM start\n");
    fprintf (stdout, "     --profile-out=FILE    when the run ends write how many times each instruction address was\n");
    fprintf (stdout, "                           executed (needs no prompt and no -p); debug labels add a third column\n");
    fprintf (stdout, "     --profile-format=FMT  text (default) or bin (V32PROF1 header, then LE32 address,count pairs)\n");
    fprintf (stdout, "     --profile-top=N       print the N hottest addresses to stderr when the run ends\n");
    fprintf (stdout, "     --profile-by=KIND     address (default) or function: with function, --profile-top and the text\n");
    fprintf (stdout, "                           --profile-out add up the addresses of each function (the closest label)\n");
    fprintf (stdout, "     --fixed-cycles        reads of TIM_CycleCounter return 0 (programs seeding from it repeat)\n");
    fprintf (stdout, "     --symbol-file=FILE    also read data symbols from FILE (v32cc's NAME.asm.sym format); the ones\n");
    fprintf (stdout, "                           next to the V32 file are read automatically\n");
    fprintf (stdout, " -q, --batch               for scripts: commands from stdin or -C without prompt or echo; no banner,\n");
    fprintf (stdout, "                           instruction line, display list or register summary; stdout holds only what\n");
    fprintf (stdout, "                           print, console, ... print; acknowledgements and stop reasons go to stderr;\n");
    fprintf (stdout, "                           a failed expect or timed-out until ends the run with status 1\n");
    fprintf (stdout, " -x, --exit-status         exit status tells how the program ended: 0 HLT, 2 frame limit,\n");
    fprintf (stdout, "                           3 system error (stops instead of entering the BIOS handler)\n");
    fprintf (stdout, "                           (always: 1 a check failed, 4 --batch could not parse a command, N 'quit N')\n");
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
            fprintf (stdout, "  asm [@ADDR|-x] INSTR    assemble an instruction (patch memory or run once)\n");
            fprintf (stdout, "  break [0xMEM|LABEL]     set breakpoint (or list them)\n");
            fprintf (stdout, "  unbreak #               remove breakpoint #\n");
            fprintf (stdout, "  continue                resume execution\n");
            fprintf (stdout, "  run N                   resume for N frames (also: wait N)\n");
            fprintf (stdout, "  until \"TEXT\" [timeout N]  resume until the text console shows TEXT\n");
            fprintf (stdout, "  expect ...              check a value or the text console (exit status 1 if it fails)\n");
            fprintf (stdout, "  step                    step to next instruction\n");
            fprintf (stdout, "  next                    next (skip subroutines)\n");
            fprintf (stdout, "  ignore                  ignore this instruction\n");
            fprintf (stdout, "  backtrace               display list of calls\n");
            fprintf (stdout, "  print[/F] XYZ           one-time display of XYZ (register, address, label, C global)\n");
            fprintf (stdout, "  x[/F] XYZ [COUNT]       display COUNT words from XYZ\n");
            fprintf (stdout, "  display[/F] XYZ [LABEL] add displaylist item\n");
            fprintf (stdout, "  undisplay #             remove displaylist item #\n");
            fprintf (stdout, "  label [0xMEM LABEL]     list or add label list item\n");
            fprintf (stdout, "  unlabel #               remove label #\n");
            fprintf (stdout, "  watch LOC OP VAL [LBL]  set a watchpoint (register, IOPort, memory)\n");
            fprintf (stdout, "  watchlist               list watchpoints\n");
            fprintf (stdout, "  unwatch #|LABEL         remove a watchpoint\n");
            fprintf (stdout, "  set [NAME = VALUE]      set system feature, register, memory, IOPort\n");
            fprintf (stdout, "  replace IP:X IR:Y IV:Z  patch instruction memory\n");
            fprintf (stdout, "  screenshot [FILE]       write the GPU screen to a PNG file\n");
            fprintf (stdout, "  save [FILE]             write the machine state to a file\n");
            fprintf (stdout, "  restore FILE            restore a saved machine state\n");
            fprintf (stdout, "  gamepad [#] [COMMAND]   view/simulate gamepad state\n");
            fprintf (stdout, "  console [FILE]          show BIOS-font text drawn on the screen, as text (or write it to FILE)\n");
            fprintf (stdout, "  kbd [SUBCOMMAND]        v32kbd keyboard: type text, press keys, host input\n");
            fprintf (stdout, "  mailbox [CHANNEL]       v32dev mailbox state of the memory card\n");
            fprintf (stdout, "  load cart|memc:file     load component into memory\n");
            fprintf (stdout, "  unload bios|cart|memc   unload component from memory\n");
            fprintf (stdout, "  inventory               system resource inventory\n");
            fprintf (stdout, "  profile                 show profiling report\n");
            fprintf (stdout, "  version                 show the simulator version\n");
            fprintf (stdout, "  help [COMMAND] / ?      display this help\n");
            fprintf (stdout, "  quit [N]                exit the simulator (with exit status N)\n\n");
            fprintf (stdout, "Commands may be abbreviated (s = step, n = next, c = continue, p = print,\n");
            fprintf (stdout, "se = set, pro = profile, in = inventory, lo = load, ...). Blank lines repeat\n");
            fprintf (stdout, "the previous step/next/continue. Lines starting with # are comments.\n");
            break;

        case INPUT_GAMEPAD:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    gamepad\n");
            fprintf (stdout, "    gamepad# COMMAND\n");
            fprintf (stdout, "    gamepad COMMAND\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    Display current values, or manipulate the gamepads\n");
            fprintf (stdout, "    (0-3), with the following commands:\n\n");
            fprintf (stdout, "    select      - switch to the indicated gamepad (needs a number, 0-3)\n");
            fprintf (stdout, "    connect     - connect the selected gamepad (active)\n");
            fprintf (stdout, "    disconnect  - disconnect the selected gamepad (inactive)\n");
            fprintf (stdout, "    left        - toggle left d-pad button (pressed/not pressed)\n");
            fprintf (stdout, "    right       - toggle right d-pad button (pressed/not pressed)\n");
            fprintf (stdout, "    up          - toggle up d-pad button (pressed/not pressed)\n");
            fprintf (stdout, "    down        - toggle down d-pad button (pressed/not pressed)\n");
            fprintf (stdout, "    start       - toggle start button (pressed/not pressed)\n");
            fprintf (stdout, "    A|B|X|Y|L|R - toggle indicated button (pressed/not pressed)\n\n");
            fprintf (stdout, "NOTES:\n");
            fprintf (stdout, "    With a gamepad number, that gamepad is selected first; a number\n");
            fprintf (stdout, "    alone selects the gamepad and shows its state. Commands may be\n");
            fprintf (stdout, "    abbreviated (le, ri, u, d, s, di, c, se).\n");
            fprintf (stdout, "    When pressed, the button's value will be a positive number\n");
            fprintf (stdout, "    of frames it has been pressed. When not pressed, a negative\n");
            fprintf (stdout, "    number of frames it has not been pressed\n\n");
            break;

        case INPUT_KBD:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    kbd                      show the keyboard state\n");
            fprintf (stdout, "    kbd attach [PORT]        emulate a v32kbd on gamepad PORT (default 0)\n");
            fprintf (stdout, "    kbd detach\n");
            fprintf (stdout, "    kbd type TEXT            type text (C escapes: \\n \\r \\t \\b \\e \\xHH \\uHHHH)\n");
            fprintf (stdout, "    kbd key KEY [MODE]       KEY: a, f1, enter, esc, up, ctrl+c, shift+tab, 0x41 ...\n");
            fprintf (stdout, "                             MODE: tap (default), down, up\n");
            fprintf (stdout, "    kbd input SOURCE|off     host input: stdin, fd:N, a FIFO/pipe/file path\n");
            fprintf (stdout, "    kbd clear                forget queued keys and text\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    The v32io keyboard looks like a gamepad to the program: Left/Right is a\n");
            fprintf (stdout, "    strobe, Up/Down says press/release and Start A B X Y L R carry a 7-bit key\n");
            fprintf (stdout, "    code. One key event is delivered per frame. Characters outside a US layout\n");
            fprintf (stdout, "    are sent as BIOS-font typed characters. With a terminal as host input the\n");
            fprintf (stdout, "    terminal is raw while the program runs; Ctrl-] returns to the prompt.\n");
            break;

        case INPUT_CONSOLE:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    console                      show the text screen\n");
            fprintf (stdout, "    console FILE                 write the text screen to FILE\n");
            fprintf (stdout, "    console --attrs[=fg|bg|all] [FILE]\n");
            fprintf (stdout, "                                 the text screen followed by grids of its colors\n");
            fprintf (stdout, "    console color MODE           none, 16, 256, true or auto\n");
            fprintf (stdout, "    console cursor COLOR [CODE]  recolor the cursor block (character 20 and any CODEs);\n");
            fprintf (stdout, "                                 COLOR: a name, #RRGGBB, or off\n");
            fprintf (stdout, "    console status\n\nDESCRIPTION:\n");
            fprintf (stdout, "    Show the characters drawn with the BIOS texture (region = character code):\n");
            fprintf (stdout, "    the BIOS font on its 64x18 grid, or v32c++'s 80x24 terminal, as UTF-8 text.\n");
            fprintf (stdout, "    The GPU multiply color of each draw becomes the cell's foreground (the paper\n");
            fprintf (stdout, "    draw of the 80x24 terminal its background), shown with ANSI colors reduced to\n");
            fprintf (stdout, "    the level asked: none (default for this command), 16, 256 or true (24 bit);\n");
            fprintf (stdout, "    --tty uses the level the terminal advertises unless one is set. ClearScreen\n");
            fprintf (stdout, "    empties the screen. With --console the text is also printed when the run ends.\n\n");
            fprintf (stdout, "    --attrs adds, after the text, a grid per attribute: \"-- fg --\" (the text color)\n");
            fprintf (stdout, "    and with =bg or =all \"-- bg --\" (the background). A grid has the same rows and\n");
            fprintf (stdout, "    columns as the text; every cell is the hex digit of its color among the 16 ANSI\n");
            fprintf (stdout, "    colors (0 black, 1 red, 2 green, 3 yellow, 4 blue, 5 magenta, 6 cyan, 7 light gray,\n");
            fprintf (stdout, "    8-F the bright versions: 9 bright red, F white, the default), or '.' for a blank,\n");
            fprintf (stdout, "    so a test can check that a word is red. A name that is not one of the words above is\n");
            fprintf (stdout, "    a file to write (the screenshot command writes the pixels to a PNG).\n");
            break;

        case INPUT_MAILBOX:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    mailbox [CHANNEL]\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    Show the v32dev mailbox in the last 16384 words of the memory card:\n");
            fprintf (stdout, "    liveness, and for each channel (0-3) the host and cart parts, the ring\n");
            fprintf (stdout, "    indices, descriptors and pending data.\n");
            break;

        case INPUT_REPLACE:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    replace IP:0xADDR IR:0xINSTRUCT IV:0xIMMEDIATE\n");
            fprintf (stdout, "    replace IR:0xINSTRUCT\n");
            fprintf (stdout, "    replace IR:0xINSTRUCT IV:0xIMMEDIATE\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    A \"Game-Genie\"-style patch of instruction memory. IP:0xADDR\n");
            fprintf (stdout, "    picks the address to patch (default: the current IP), IR:\n");
            fprintf (stdout, "    overwrites the instruction word there, and IV: overwrites\n");
            fprintf (stdout, "    the immediate word that follows it (if the new instruction\n");
            fprintf (stdout, "    takes one). Items are applied left to right.\n\n");
            fprintf (stdout, "NOTE:\n");
            fprintf (stdout, "    The patch is written to memory and stays there. It does not\n");
            fprintf (stdout, "    alter the instruction already fetched for this step, so it\n");
            fprintf (stdout, "    takes effect the next time execution reaches that address.\n\n");
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

        case INPUT_BTRACE:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    backtrace\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
			fprintf (stdout, "    Display a most recent to least recent list of subroutine calls\n\n");
            break;

        case INPUT_LABEL:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    label\n");
            fprintf (stdout, "    label 0xMEM_ADDR LABEL\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    List or set a label for the indicated memory offset. An entry is\n");
            fprintf (stdout, "    made in the label list, allowing for label<->offset lookups to be\n");
            fprintf (stdout, "    performed, utilized by a number of simulator features.\n\n");
            break;

        case INPUT_CONTINUE:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    continue\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    Resume execution until next trigger is encountered.\n\n");
            break;

        case INPUT_SET:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    set setting=true|false\n");
            fprintf (stdout, "    set R#|SP|BP|CR|SR|DR|IP|IR|IV=0xTHEVALUE\n");
            fprintf (stdout, "    set\n");
            fprintf (stdout, "    set 0xMEM_ADDR=0xTHEVALUE\n");
            fprintf (stdout, "    set 0xIOP=0xTHEVALUE\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    Set some system setting or resource to a specified value.\n");
            fprintf (stdout, "    This facility offers a way, during runtime, to alter the\n");
            fprintf (stdout, "    state of registers, memory, and IOPorts for use in testing\n");
            fprintf (stdout, "    (setting IP is a jump: execution continues from the new IP).\n");
            fprintf (stdout, "    With no arguments, the current settings are listed. The IOPort\n");
            fprintf (stdout, "    may also be given by its symbolic name (GPU_ClearColor=...).\n");
            fprintf (stdout, "    or enabling desired conditions. It is also used for setting\n");
            fprintf (stdout, "    simulator configurations during runtime.\n\n");
            fprintf (stdout, "SETTINGS:\n");
            fprintf (stdout, "    color    true/false    set color output\n");
            fprintf (stdout, "    deref    true/false    set deref addr\n");
            fprintf (stdout, "    profile  true/false    set instruction profiling\n");
            fprintf (stdout, "    debug    true/false    set simulator debug mode\n");
            fprintf (stdout, "    errorchk true/false    set simulator error checking\n");
            fprintf (stdout, "    verbose  true/false    set simulator verbosity\n");
            fprintf (stdout, "    source   true/false    show source lines (assembly or C) at the prompt\n");
            fprintf (stdout, "    mode     asm|c|lua     which source the overlay shows\n");
            fprintf (stdout, "    (values: true/false, on/off, yes/no, 1/0)\n\n");
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

        case INPUT_ASM:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    asm INSTRUCTION\n");
            fprintf (stdout, "    asm @0xADDRESS INSTRUCTION\n");
            fprintf (stdout, "    asm -x INSTRUCTION\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    Assemble one Vircon32 instruction. Without options the machine\n");
            fprintf (stdout, "    code replaces the instruction at the current IP (or at ADDRESS);\n");
            fprintf (stdout, "    with -x it is executed once and memory is left alone.\n\n");
            fprintf (stdout, "    Operands follow the Vircon32 assembler: registers (R0-R15, CR, SR,\n");
            fprintf (stdout, "    DR, BP, SP), numbers (decimal, 0xHEX, 0bBIN, 0OCT, floats), labels\n");
            fprintf (stdout, "    from loaded debug files, IOPort names, and the GPU/SPU constants.\n\n");
            fprintf (stdout, "EXAMPLES:\n");
            fprintf (stdout, "    asm mov R1, 0x10\n");
            fprintf (stdout, "    asm @0x20000100 out GPU_Command, GPUCommand_ClearScreen\n");
            fprintf (stdout, "    asm -x iadd R0, -1\n\n");
            fprintf (stdout, "NOTE:\n");
            fprintf (stdout, "    A replacement may not be longer than the instruction it replaces;\n");
            fprintf (stdout, "    a shorter one is padded with MOV R0, R0 (a no-op).\n\n");
            break;

        case INPUT_SAVE:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    save [FILENAME]\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    Write the whole machine state (registers, RAM, IOPorts, GPU, SPU,\n");
            fprintf (stdout, "    timers) to a file; default 'v32sim.DATESTAMP.sav'.\n\n");
            break;

        case INPUT_RESTORE:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    restore FILENAME\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    Restore a state written by 'save'. The BIOS, cartridge and memory\n");
            fprintf (stdout, "    card loaded now must be the ones in use when it was saved.\n\n");
            break;

        case INPUT_SCREENSHOT:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    screenshot [FILENAME]\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    Output the current state of the Vircon32 screen to a PNG file.\n");
            fprintf (stdout, "    The default filename will be 'v32sim.DATESTAMP.png', unless an\n");
            fprintf (stdout, "    alternate FILENAME is specified.\n\n");
            fprintf (stdout, "NOTE:\n");
            fprintf (stdout, "    Various simulator settings can enhance the appearance of this\n");
            fprintf (stdout, "    screenshot, such as 'screengrid', 'boundbox', and 'noextra'\n\n");
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

        case INPUT_PROFILE:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    profile\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    Display current profiling report for instructions and\n");
            fprintf (stdout, "    subroutines called. Only works if profiling has  been\n");
            fprintf (stdout, "    enabled.\n\n");
            break;

        case INPUT_PRINT:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    print/fmt REGISTER\n");
            fprintf (stdout, "    print/fmt [REGISTER]\n");
            fprintf (stdout, "    print/fmt 0xMEM_ADDR\n");
            fprintf (stdout, "    print/fmt [0xMEM_ADDR]\n");
            fprintf (stdout, "    print/fmt 0xMEMADDR1-0xMEMADDR2\n");
            fprintf (stdout, "    print/fmt [0xMEMADDR1-0xMEMADDR2]\n");
            fprintf (stdout, "    print/fmt 0xIOP\n");
            fprintf (stdout, "    print/fmt IOPORT_SYMBOL\n");
            fprintf (stdout, "    print/fmt reg\n");
            fprintf (stdout, "    print/fmt SYMBOL[+N]       a label or C global: the word it names\n");
            fprintf (stdout, "    print/fmt [SYMBOL[+N]]     the same\n");
            fprintf (stdout, "    print/fmt [[SYMBOL]]       the word that the variable SYMBOL points to\n");
            fprintf (stdout, "    print/fmt &SYMBOL[+N]      the address of the symbol\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    A hexadecimal number of up to 3 digits is an IOPort, longer ones a\n");
            fprintf (stdout, "    memory address. REGISTER is R0-R15, BP, SP, CR, SR, DR, IP, IR, IV\n");
            fprintf (stdout, "    ('reg' shows R0-R15). A one-time output of the indicated value. Typically used when one\n");
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
            fprintf (stdout, "    print   [0x003FFFFE]  dereference address, show contents\n");
            fprintf (stdout, "    print/d counter       the C global 'counter' as a signed number\n");
            fprintf (stdout, "    print   buf+2         the third word of the array 'buf'\n");
            fprintf (stdout, "    print   &f            the address of the function f\n\n");
            fprintf (stdout, "SYMBOLS:\n");
            fprintf (stdout, "    A symbol is a label of the debug files (functions, assembly labels) or a global\n");
            fprintf (stdout, "    variable of a C program: v32cc -g writes NAME.asm.sym, which is read automatically\n");
            fprintf (stdout, "    when it is next to the V32 file (or --symbol-file=FILE; without it, the\n");
            fprintf (stdout, "    '%%define global_NAME' lines of NAME.asm are used). NAME is also tried as global_NAME,\n");
            fprintf (stdout, "    and a name that matches exactly wins over one that differs in case. 'label 0xADDR NAME'\n");
            fprintf (stdout, "    defines a symbol by hand. A register name or an IOPort symbol cannot be used as a symbol\n");
            fprintf (stdout, "    name. '+N' / '-N' (decimal or 0x) is a number of words. The same forms are accepted by\n");
            fprintf (stdout, "    display, x, watch, expect and --dump.\n\n");
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
            fprintf (stdout, "    /s    display as string\n");
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

        case INPUT_QUIT:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    quit [STATUS]\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    Leave the simulator (end of input, ie Ctrl-D, does the same). STATUS is the\n");
            fprintf (stdout, "    exit status of the process (0-255); without it the status says how the run\n");
            fprintf (stdout, "    went: 1 if an expect failed or an until timed out, 4 if --batch could not\n");
            fprintf (stdout, "    parse a command, otherwise as described by -x.\n\n");
            break;

        case INPUT_VERSION:
            fprintf (stdout, "SYNOPSIS:\n");
            fprintf (stdout, "    version\n\n");
            fprintf (stdout, "DESCRIPTION:\n");
            fprintf (stdout, "    Display the simulator version (also: v32sim --version).\n\n");
            break;

		case INPUT_WATCH:
			fprintf (stdout, "SYNOPSIS:\n");
			fprintf (stdout, "    watch[/d] LOCATION OP VALUE [LABEL]\n\n");
			fprintf (stdout, "DESCRIPTION:\n");
			fprintf (stdout, "    Set a watchpoint. Execution breaks when the comparison BECOMES\n");
			fprintf (stdout, "    true (it is edge-triggered: a condition that stays true does not\n");
			fprintf (stdout, "    break again until it has been false in between).\n\n");
			fprintf (stdout, "    LOCATION: a register (R0-R15, BP, SP, CR, SR, DR, IP, IR, IV),\n");
			fprintf (stdout, "              an IOPort (symbol, or 0x with up to 3 digits), or a\n");
			fprintf (stdout, "              memory word (0x with more digits), or a symbol: counter or\n");
			fprintf (stdout, "              [counter] (also [buf+2]), see 'help print'\n");
			fprintf (stdout, "    OP:       =, ==, !=, <, >, <=, >=\n");
			fprintf (stdout, "    VALUE:    a number, a register, an IOPort symbol, or [0xMEM],\n");
			fprintf (stdout, "              [IOPORT], [REG], [SYMBOL] to compare with another location;\n");
			fprintf (stdout, "              a bare symbol or &SYMBOL is the symbol's address\n");
			fprintf (stdout, "    LABEL:    optional name for the watchpoint\n\n");
			fprintf (stdout, "EXAMPLES:\n");
			fprintf (stdout, "    watch SP <= 0x20000000 stack_underflow\n");
			fprintf (stdout, "    watch 0x20001000 != 0\n");
			fprintf (stdout, "    watch TIM_FrameCounter = 60\n");
			fprintf (stdout, "    watch R1 > [0x20000010]\n");
			fprintf (stdout, "    watch [counter] == 6 w         watch the C global 'counter'\n");
			fprintf (stdout, "    watch/d [neg] < 0              signed comparison\n\n");
			fprintf (stdout, "NOTES:\n");
			fprintf (stdout, "    With no arguments, watch lists the watchpoints. Comparisons are\n");
			fprintf (stdout, "    unsigned 32-bit, or signed with /d (watch/d [counter] < 0).\n\n");
			break;

		case INPUT_WATCHLIST:
			fprintf (stdout, "SYNOPSIS:\n");
			fprintf (stdout, "    watchlist\n\n");
			fprintf (stdout, "DESCRIPTION:\n");
			fprintf (stdout, "    Display all active watchpoints with their current status.\n\n");
			break;

		case INPUT_UNWATCH:
			fprintf (stdout, "SYNOPSIS:\n");
			fprintf (stdout, "    unwatch INDEX\n");
			fprintf (stdout, "    unwatch LABEL\n\n");
			fprintf (stdout, "DESCRIPTION:\n");
			fprintf (stdout, "    Remove a watchpoint by its index (from watchlist) or label.\n\n");
			fprintf (stdout, "EXAMPLES:\n");
			fprintf (stdout, "    unwatch 0\n");
			fprintf (stdout, "    unwatch stack_underflow\n\n");
			break;

		case INPUT_RUN:
			fprintf (stdout, "SYNOPSIS:\n");
			fprintf (stdout, "    run N\n");
			fprintf (stdout, "    wait N\n\n");
			fprintf (stdout, "DESCRIPTION:\n");
			fprintf (stdout, "    Continue for N frames, then stop at the prompt (a breakpoint or watchpoint that\n");
			fprintf (stdout, "    triggers first stops it earlier). Frames are counted from now, so no frame number\n");
			fprintf (stdout, "    has to be worked out: 'run 60' is a second of the console.\n\n");
			break;

		case INPUT_UNTIL:
			fprintf (stdout, "SYNOPSIS:\n");
			fprintf (stdout, "    until \"TEXT\" [timeout FRAMES] [rows N]\n\n");
			fprintf (stdout, "DESCRIPTION:\n");
			fprintf (stdout, "    Continue until the text console (see 'help console') shows TEXT, then stop at the\n");
			fprintf (stdout, "    prompt. TEXT is looked for inside every row; a TEXT starting with ^ must start the\n");
			fprintf (stdout, "    row, one ending with $ must end it. 'rows N' looks at the last N rows only (the rows\n");
			fprintf (stdout, "    'console' prints). The cursor block is read as a blank.\n\n");
			fprintf (stdout, "    The screen is looked at at the end of each frame, and only once it has CHANGED since\n");
			fprintf (stdout, "    the command was given: what is on the screen already does not end the wait (use\n");
			fprintf (stdout, "    'expect console' for the screen as it is). After 'timeout' frames (default 3600) the\n");
			fprintf (stdout, "    wait is given up with a message; with --batch that ends the run with exit status 1.\n\n");
			fprintf (stdout, "EXAMPLES:\n");
			fprintf (stdout, "    kbd type \"dir\\n\"\n");
			fprintf (stdout, "    until \"A:\\>$\" rows 1 timeout 600       the next DOS prompt, on the last row\n\n");
			break;

		case INPUT_EXPECT:
			fprintf (stdout, "SYNOPSIS:\n");
			fprintf (stdout, "    expect[/d] LOCATION OP VALUE\n");
			fprintf (stdout, "    expect print[/d] LOCATION OP VALUE\n");
			fprintf (stdout, "    expect console \"TEXT\" [rows N]\n\n");
			fprintf (stdout, "DESCRIPTION:\n");
			fprintf (stdout, "    Check a condition now. LOCATION, OP and VALUE are those of 'watch'; TEXT is looked\n");
			fprintf (stdout, "    for as 'until' does (^ and $ anchor it to a row). When the check fails a message\n");
			fprintf (stdout, "    goes to stderr (for the console, with the screen), the exit status of the simulator\n");
			fprintf (stdout, "    becomes 1, and with --batch the run ends at once. A passing check is silent.\n\n");
			fprintf (stdout, "EXAMPLES:\n");
			fprintf (stdout, "    expect R0 == 5\n");
			fprintf (stdout, "    expect/d [neg] < 0\n");
			fprintf (stdout, "    expect console \"Volume in drive\"\n\n");
			break;

		case INPUT_EXAMINE:
			fprintf (stdout, "SYNOPSIS:\n");
			fprintf (stdout, "    x[/fmt] SYMBOL|0xADDR [COUNT]\n\n");
			fprintf (stdout, "DESCRIPTION:\n");
			fprintf (stdout, "    Show COUNT words of memory from the symbol or address, one per line, as 'print'\n");
			fprintf (stdout, "    does (/D lists instructions). COUNT defaults to the size of a C variable (the whole\n");
			fprintf (stdout, "    array), otherwise 1. See 'help print' for symbols.\n\n");
			fprintf (stdout, "EXAMPLES:\n");
			fprintf (stdout, "    x/d buf               the array buf\n");
			fprintf (stdout, "    x/D work 10           the first ten instructions of work\n\n");
			break;
    }
}
