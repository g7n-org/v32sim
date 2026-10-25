#include "defines.h"

////////////////////////////////////////////////////////////////////////////////////////
//
// global variables - important file and memory/register resources
//
FILE     *display;
FILE     *devnull;
FILE     *debug;
FILE     *verbose;
int8_t   *biosfile;
int8_t   *cartfile;
int8_t   *memcfile;
int8_t   *commandfile;
int8_t   *biosasmdebug;
int8_t   *cartasmdebug;
int8_t   *bioscdebug;
uint8_t   sourceflag;          // show source line overlay
int8_t   *debugdir;          // extra place to look for debug files
int8_t   *cartcdebug;
data_t   *reg;
int8_t   *token_label;

////////////////////////////////////////////////////////////////////////////////////////
//
// Variables related to IOPorts and memory
//
data_t   **ioports;
vtex_t    *bios_vtex;  // for managing textures and regions within textures
vtex_t    *cart_vtex;  // for managing textures and regions within textures
gamepad_t *gamepad;
mem_t     *memory;
int8_t     sys_error;
uint8_t    sys_reg_show;

////////////////////////////////////////////////////////////////////////////////////////
//
// flags
//
uint8_t   action;
uint8_t   modeflag;
uint8_t   biosasmdebugflag;
uint8_t   bioscdebugflag;
uint8_t   cartasmdebugflag;
uint8_t   cartcdebugflag;
uint8_t   profileflag;
uint8_t   runflag;
uint8_t   colorflag;
uint8_t   branchflag;
uint8_t   ignoreflag;
uint8_t   retflag;
uint8_t   nextflag;
uint8_t   derefaddr;
uint8_t   errorcheck;
uint8_t   haltflag;
uint8_t   exitstatus;      // process exit status: 0 HLT, 2 frame limit, 3 system error (-x)
uint8_t   consoleflag;     // --console: print the text screen when the run ends
int8_t   *dumpspec;        // --dump=N[@ADDR|@LABEL]: print N RAM words after the run
uint8_t   exitflag;        // -x: the exit status reports how the program ended
int8_t   *symbolfile;      // --symbol-file=FILE: data symbols (v32cc NAME.asm.sym format)
int8_t   *kbdinput;        // --kbd-input: host source of v32kbd keys
int32_t   kbdport;         // --v32kbd: gamepad port of the keyboard, -1 = none
uint8_t   turboflag;
uint8_t   waitflag;
uint8_t   wordsize;
uint32_t  next_bt_lvl;
uint32_t  profcount;
uint32_t  profcountsub;
uint32_t  profcountopcode[64];
uint32_t  rom_offset;
uint32_t  seek_word;
uint32_t  watch_word;
linked_l *bpoint; // breakpoint list
linked_l *dpoint; // display list
linked_l *lpoint; // label list
linked_l *mpoint; // tracking allocated memory
linked_l *ppoint; // subroutine profiling list
linked_l *csub;   // current subroutine
linked_l *tpoint; // breakpoint waitlist, then backtrace list
linked_l *next_tpoint;  // position for next (when nexting over subroutine)
wpoint_t *wpoint; // watchpoint list

////////////////////////////////////////////////////////////////////////////////////////
//
// initialize our opcodes array with the available instructions
//
opcode_t  lookup[64]                =
{
    { "HLT"  }, { "WAIT"  }, { "JMP"   }, { "CALL" },
    { "RET"  }, { "JT"    }, { "JF"    }, { "IEQ"  },
    { "INE"  }, { "IGT"   }, { "IGE"   }, { "ILT"  },
    { "ILE"  }, { "FEQ"   }, { "FNE"   }, { "FGT"  },
    { "FGE"  }, { "FLT"   }, { "FLE"   }, { "MOV"  },
    { "LEA"  }, { "PUSH"  }, { "POP"   }, { "IN"   },
    { "OUT"  }, { "MOVS"  }, { "SETS"  }, { "CMPS" },
    { "CIF"  }, { "CFI"   }, { "CIB"   }, { "CFB"  },
    { "NOT"  }, { "AND"   }, { "OR"    }, { "XOR"  },
    { "BNOT" }, { "SHL"   }, { "IADD"  }, { "ISUB" },
    { "IMUL" }, { "IDIV"  }, { "IMOD"  }, { "ISGN" },
    { "IMIN" }, { "IMAX"  }, { "IABS"  }, { "FADD" },
    { "FSUB" }, { "FMUL"  }, { "FDIV"  }, { "FMOD" },
    { "FSGN" }, { "FMIN"  }, { "FMAX"  }, { "FABS" },
    { "FLR"  }, { "CEIL"  }, { "ROUND" }, { "SIN"  },
    { "ACOS" }, { "ATAN2" }, { "LOG"   }, { "POW"  }
};

static uint8_t  decodeflags         = FLAG_NONE;

////////////////////////////////////////////////////////////////////////////////////////
//
// fetch_instruction(): update IP, IR, IV (and decodeflags) from rom_offset
//
static void  fetch_instruction (void)
{
    REG(IP)                            = rom_offset;
    REG(IR)                            = IMEMGET(REG(IP)); // current instruction
    if ((REG(IR) & IMMVAL_MASK)       == IMMVAL_MASK)
    {
        REG(IV)                        = IMEMGET(REG(IP) + 1);
        decodeflags                    = FLAG_IMMEDIATE;
    }
    else
    {
        REG(IV)                        = 0x00000000;
        decodeflags                    = FLAG_NONE;
    }
}

int32_t   main (int32_t  argc, char **argv)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    linked_l *btmp                  = NULL;
    linked_l *tmp                   = NULL;
    size_t    len                   = 0;
    uint8_t   chk                   = FALSE;
    uint8_t   errorflag             = FLAG_NONE;
    uint8_t   processflag           = FALSE;
    uint32_t  value                 = 0;
    uint32_t  old_count             = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // initialize variables
    //
    biosfile                        = NULL;
    cartfile                        = NULL;
    memcfile                        = NULL;
    commandfile                     = NULL;
    rom_offset                      = BIOS_START_OFFSET;
    modeflag                        = FLAG_ASM;
    sourceflag                      = FALSE;
    branchflag                      = FALSE;
    biosasmdebugflag                = FALSE;
    bioscdebugflag                  = FALSE;
    cartasmdebugflag                = FALSE;
    cartcdebugflag                  = FALSE;
    profileflag                     = FALSE;
    runflag                         = FALSE;
    colorflag                       = FALSE;
    nextflag                        = FALSE;
    seek_word                       = 0xFFFFFFFF;
    watch_word                      = 0xFFFFFFFF;
    wordsize                        = 4;
    derefaddr                       = FALSE;
    haltflag                        = FALSE;
    exitstatus                      = 0;
    exitflag                        = FALSE;
    dumpspec                        = NULL;
    symbolfile                      = NULL;
    consoleflag                     = FALSE;
    kbdinput                        = NULL;
    kbdport                         = -1;
    frame_us                        = 1000000 / 60;
    turboflag                       = FALSE;
    waitflag                        = FALSE;
    errorcheck                      = FALSE;
    sys_error                       = ERROR_NONE;
    sys_reg_show                    = FALSE;
    action                          = INPUT_INIT;
    token_label                     = NULL;
    bpoint                          = NULL;
    dpoint                          = NULL;
    lpoint                          = NULL;
    mpoint                          = NULL;
    ppoint                          = NULL;
    csub                            = NULL;
    tpoint                          = NULL;
    next_tpoint                     = NULL;
    profcount                       = 0;
    profcountsub                    = 0;
    wpoint                          = NULL;  // Watchpoint list

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Set CART starting offset as a break point
    //
    //tmp                             = listnode (LIST_MEM, CART_START_OFFSET);
    //tpoint                          = list_add (tpoint, tmp);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Open /dev/null
    //
    devnull                         = fopen ("/dev/null", "w");
    if (devnull                    == NULL)
    {
        fprintf (stderr, "[error] could not open '/dev/null' for writing!\n");
        exit (2);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // if verbose has not been redirected, point it at devnull
    //
    if (verbose                    == NULL)
    {
        verbose                     = devnull;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // if debug has not been redirected, point it at devnull
    //
    if (debug                      == NULL)
    {
        debug                       = devnull;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Process command-line arguments
    //
    process_args ((int32_t) argc, (int8_t **) argv);
    runctl_init ();             // --batch decides where acknowledgements go

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Open the indicated V32 file
    //
    if (cartfile                   == NULL)
    {
        fprintf (verbose, "[v32sim] started without specifying V32 cartridge file!\n");
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Set up 'biosfile'
    //
    if (biosfile                   == NULL)
    {
        len                         = strlen (DEFAULT_BIOS) + 1;
        biosfile                    = (int8_t *) ralloc (sizeof (int8_t),
                                                         strlen (DEFAULT_BIOS) + 1,
                                                         FLAG_NONE);
        if (biosfile               == NULL)
        {
            fprintf (stderr, "[ERROR] Allocation for '%s' failed\n", "biosfile");
            exit (STRING_ALLOC_FAIL);
        }
        strcpy (biosfile, DEFAULT_BIOS);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Our registers will be in a data_t array
    //
    init_registers ();

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Allocate Vircon32 IOPorts (a 2D array of ports)
    //
    init_ioports ();

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Allocate Vircon32 memory regions (RAM, BIOS, CART, MEMC)
    //
    init_memory ();

    ////////////////////////////////////////////////////////////////////////////////////
    //
    gpu_reset ();
    spu_reset ();

    // Initialize BIOS (load contents into memory), optionally loading any debug data
    //
    chk                             = load_memory (V32_PAGE_BIOS, biosfile);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // load any debug file labels pertaining to page being loaded (the debug file is
    // searched for next to the V32 file, unless -n was given or the file was named
    // explicitly with a --*-debug option)
    //
    if (((biosasmdebugflag != MU) && (biosasmdebug == NULL))  &&
        (chk                       == TRUE)  &&
        (biosfile                  != NULL))
    {
        load_labels (biosfile, V32_PAGE_BIOS, FLAG_SEARCH | FLAG_ASM);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // load any debug file labels pertaining to page being loaded
    //
    if (((bioscdebugflag != MU) && (bioscdebug == NULL))  &&
        (chk                       == TRUE)  &&
        (biosfile                  != NULL))
    {
        load_labels (biosfile, V32_PAGE_BIOS, FLAG_SEARCH | FLAG_C);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Initialize CART (load contents into memory), optionally loading any debug data
    //
    chk                             = load_memory (V32_PAGE_CART, cartfile);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // load any debug file labels pertaining to page being loaded
    //
    if (((cartasmdebugflag != MU) && (cartasmdebug == NULL)) &&
        (chk                       == TRUE) &&
        (cartfile                  != NULL))
    {
        load_labels (cartfile, V32_PAGE_CART, FLAG_SEARCH | FLAG_ASM);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // load any debug file labels pertaining to page being loaded
    //
    if (((cartcdebugflag != MU) && (cartcdebug == NULL)) &&
        (chk                       == TRUE) &&
        (cartfile                  != NULL))
    {
        load_labels (cartfile, V32_PAGE_CART, FLAG_SEARCH | FLAG_C);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // data symbols (the C globals of the cartridge), unless debug files are off (-n)
    //
    if ((cartasmdebugflag != MU) && (chk == TRUE) && (cartfile != NULL))
    {
        sym_load_cart (cartfile);
    }
    if (symbolfile != NULL)
    {
        if (sym_load_file (symbolfile) == 0)
        {
            fprintf (stderr, "[WARNING] --symbol-file: no usable symbols in '%s'\n", symbolfile);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // If a BIOS assembly debug file was specified, process it here
    //
    if (biosasmdebug               != NULL)
    {
        load_labels (biosasmdebug, 0, FLAG_NONE | FLAG_ASM);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // If a BIOS C debug file was specified, process it here
    //
    if (bioscdebug                 != NULL)
    {
        load_labels (bioscdebug,   0, FLAG_NONE | FLAG_C);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // If a CART assembly debug file was specified, process it here
    //
    if (cartasmdebug               != NULL)
    {
        load_labels (cartasmdebug, 0, FLAG_NONE | FLAG_ASM);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // If a CART C debug file was specified, process it here
    //
    if (cartcdebug                 != NULL)
    {
        load_labels (cartcdebug,   0, FLAG_NONE | FLAG_C);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Initialize MEMC (load contents into memory)
    //
    chk                             = load_memory (V32_PAGE_MEMC, memcfile);
    if ((memcfile                  != NULL) &&
        (chk                       == FALSE))
    {
        exit (FILE_POSITION_ERROR);
    }
    memc_power_on ();   // a live v32dev mailbox is synchronized before the first instruction

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // v32io keyboard, on the requested gamepad port, and its host input
    //
    if (kbdport                    >= 0)
    {
        v32io_kbd_attach (kbdport);
    }
    if ((v32io_tty_on () == TRUE) && (kbdinput == NULL))
    {
        if (isatty (STDIN_FILENO) == 0 || isatty (STDOUT_FILENO) == 0)
        {
            fprintf (stderr, "[ERROR] --tty needs a terminal on stdin and stdout\n");
            exit (1);
        }
        kbdinput  = (int8_t *) "stdin";
    }
    if (kbdinput                   != NULL)
    {
        if (kbdport                <  0)
        {
            v32io_kbd_attach (0);
        }
        v32io_kbd_source ((const char *) kbdinput);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Load commands from command-file
    //
    // (Unless running unprompted, the command file is read by prompt().)

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Process breakpoint waitlist (tpoint), if any
    //
    if (tpoint                     != NULL)
    {
        linked_l *nxt               = NULL;
        tmp                         = tpoint;
        while (tmp                 != NULL)
        {
            nxt                     = tmp -> next;
            if (tmp -> label       == NULL) // not a label: offset
            {
                value               = tmp -> RAW;
                btmp                = find_value (bpoint, value);
                if (btmp           == NULL) // not an existing breakpoint
                {
                    btmp            = list_grab (&tpoint, tmp);
                    fprintf (debug, "[main] BREAKing at offset 0x%.8X\n", value);
                    bpoint          = list_add (bpoint, btmp);
                }
            }
            else // label
            {
                btmp                = find_label (lpoint, tmp  -> label);
                if (btmp           != NULL) // yes, match for label in label list
                {
                    value           = btmp -> RAW;
                    btmp            = find_value (bpoint, value); // check offset in breaklist
                    if (btmp       == NULL) // not an existing breakpoint in breaklist
                    {
                        btmp        = list_grab (&tpoint, tmp);
                        btmp -> RAW = value;
                        fprintf (debug, "[main] BREAKing at label: '%s'\n", btmp -> label);
                        fprintf (debug, "[main] BREAKing at offset 0x%.8X\n", btmp -> RAW);
                        bpoint      = list_add (bpoint, btmp);
                    }
                }
            }

            tmp                     = nxt;
        }

        while (tpoint             != NULL)
        {
            tmp                     = tpoint;
            tpoint                 = tpoint -> next;
            if (tmp -> label       != NULL)
            {
                rfree (tmp -> label);
            }
            rfree (tmp);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // arm the signal handler to trap SIGINT signals (useful for getting out of
    // run mode or continues
    //
    signal (SIGINT, sigint);
    signal (SIGSEGV, sigcrash);
    signal (SIGBUS,  sigcrash);
    signal (SIGFPE,  sigcrash);
    signal (SIGABRT, sigcrash);

    addrprof_init ();      // the page sizes are known once the BIOS, cart and card are loaded

    /*
    fprintf (stdout, "rom_offset: %.8X\n", rom_offset);
    */
    if ((runflag                      == FALSE) &&
        (batchflag                    == FALSE))
    {
        fprintf (stdout, "Vircon32 Simulator / Debugger  (v32sim %s)\n", VERSION);
        fprintf (stdout, "=======================================\n");
        fprintf (stdout, "[ OFFSET ] [HEXVALUES] [OPC] [DST]\n");
    }

    if (runflag                       == TRUE)
    {
        v32io_term_resume ();
    }

    while ((action                    != INPUT_QUIT) &&
           (haltflag                  == FALSE))
    {
        ////////////////////////////////////////////////////////////////////////////////
        //
        // Check for breakpoint
        //
        btmp                           = bpoint;
        while (btmp                   != NULL)
        {
            value                      = btmp -> RAW;
            if (rom_offset            == value)
            {
                if (colorflag         == TRUE)
                {
                    fprintf (stdout, "\e[1;31m");
                }
                status_printf ("[breakpoint] triggered on 0x%.8X\n", value);
                if (colorflag         == TRUE)
                {
                    fprintf (stdout, "\e[m");
                }
                runflag                = FALSE;
            }
            btmp                       = btmp -> next;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Check for instruction opcode (watchfor trigger)
        //
        if (watch_word                == REG(IR))
        {
            if (colorflag             == TRUE)
            {
                fprintf (stdout, "\e[1;31m");
            }
            status_printf ("[watchfor] trigger encountered (0x%.8X)\n", watch_word);
            if (colorflag             == TRUE)
            {
                fprintf (stdout, "\e[m");
            }
            runflag                    = FALSE;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Check for delayed break to single-step via seek_word (argument)
        //
        if (rom_offset                == seek_word)
        {
            runflag                    = FALSE;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Update the InstructionPointer and InstructionRegister
        //
        fetch_instruction ();

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Running unprompted (-r): there is no prompt to feed the command file, so run
        // it now that the first instruction is fetched, then fetch again (it may have
        // moved IP)
        //
        if ((commandfile              != NULL) && (runflag == TRUE))
        {
            fprintf (verbose, "LOADING COMMAND-FILE\n");
            load_command ();
            fetch_instruction ();
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // If verbosity is enabled and we are in continuation mode
        //
        if ((verbose                  == stderr) &&
            (runflag                  == TRUE))
        {
            put_word (REG(IR), FLAG_DISPLAY);

            ////////////////////////////////////////////////////////////////////////////
            //
            // If enabled, check for error generating instruction
            //
            if (errorcheck            == TRUE)
            {
                errorflag              = decode (REG(IR), REG(IV), FREG(IV),
                                                 decodeflags    | FLAG_ERROR);

                ////////////////////////////////////////////////////////////////////////
                //
                // If errorflag is false (there is a detected problem), kick back
                // to te prompt, set up error highlighting.
                //
                if (errorflag         == FALSE)
                {
                    runflag            = FALSE;
                    decodeflags        = decodeflags            | FLAG_ERROR;
                }
            }

            ////////////////////////////////////////////////////////////////////////////
            //
            // Display the current instruction
            //
            decode   (REG(IR), REG(IV), FREG(IV), decodeflags | FLAG_DISPLAY);
            if (FLAG_IMMEDIATE        == (decodeflags & FLAG_IMMEDIATE))
            {
                put_word (REG(IV), FLAG_DISPLAY);
                fprintf  (stdout, "\n");
            }
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Single-step mode
        //
        if (runflag                   == FALSE)
        {
            ////////////////////////////////////////////////////////////////////////////
            //
            // if enabled, display the profile count
            //
                /*
            if (profileflag           == TRUE)
            {
                if (colorflag         == TRUE)
                {
                    fprintf (stdout, "\e[1;34m");
                }

                if (IMEMGET(REG(IP)-1) == RET)
                {
                    fprintf (stdout, "[profiling] subroutine instruction count: %u\n", csub -> COUNT);
                }

                if (colorflag         == TRUE)
                {
                    fprintf (stdout, "\e[m");
                }
            }
                */

            ////////////////////////////////////////////////////////////////////////////
            //
            // Display the prompt
            //
            do
            {
                if (batchflag         == FALSE)
                {
                    source_overlay (REG(IP));

                    if (sys_reg_show      == TRUE)
                    {
                        show_sysregs ();
                        switch ((REG(IP) & 0x30000000) >> 28)
                        {
                            case V32_PAGE_RAM:
                                fprintf (stdout, " [RAM]");
                                break;
                            case V32_PAGE_BIOS:
                                fprintf (stdout, "[BIOS]");
                                break;
                            case V32_PAGE_CART:
                                fprintf (stdout, "[CART]");
                                break;
                            case V32_PAGE_MEMC:
                                fprintf (stdout, "[MEMC]");
                                break;
                        }

                        fprintf (stdout, "[%.8X]: ", REG(IP));
                    }
                    else
                    {
                        put_word (REG(IR), FLAG_DISPLAY);
                    }

                    ////////////////////////////////////////////////////////////////////////
                    //
                    // If enabled, check for error generating instruction
                    //
                    if (errorcheck        == TRUE)
                    {
                        errorflag          = decode (REG(IR), REG(IV), FREG(IV),
                                                     decodeflags    | FLAG_ERROR);

                        if (errorflag     == FALSE)
                        {
                            decodeflags    = decodeflags            | FLAG_ERROR;
                        }
                    }
                    decode (REG(IR), REG(IV), FREG(IV), decodeflags | FLAG_DISPLAY);

                    if (FLAG_IMMEDIATE    == (decodeflags & FLAG_IMMEDIATE))
                    {
                        if (sys_reg_show  == TRUE)
                        {
                            switch ((REG(IP) & V32_PAGE_MASK) >> 28)
                            {
                                case V32_PAGE_RAM:
                                    fprintf (stdout, " [RAM]");
                                    break;
                                case V32_PAGE_BIOS:
                                    fprintf (stdout, "[BIOS]");
                                    break;
                                case V32_PAGE_CART:
                                    fprintf (stdout, "[CART]");
                                    break;
                                case V32_PAGE_MEMC:
                                    fprintf (stdout, "[MEMC]");
                                    break;
                            }
                            fprintf  (stdout, "[%.8X]: 0x%.8X", (REG(IP) + 1), REG(IV));
                        }
                        else
                        {
                            put_word (REG(IV), FLAG_DISPLAY | FLAG_IMMEDIATE);
                        }
                        fprintf  (stdout, "\n");
                    }


                }
                processflag            = FALSE;

                if (batchflag         == FALSE)
                {
                    displayshow  (dpoint, 0);
                }

                ////////////////////////////////////////////////////////////////////////
                //
                // Display the prompt and obtain input
                //
                processflag            = prompt (REG(IR));

            }
            while (processflag        == FALSE);

            if (runflag               == TRUE)
            {
                v32io_term_resume ();
            }
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // 'quit' (or the end of the input, or a failed 'expect' in batch mode): do not
        // run one more instruction
        //
        if (action                    == INPUT_QUIT)
        {
            break;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // process the current instruction
        //
        if (ignoreflag                == FALSE)
        {
            ////////////////////////////////////////////////////////////////////////////
            //
            // Clear the system error variable
            //
            sys_error                  = ERROR_NONE;

            ////////////////////////////////////////////////////////////////////////////
            //
            // Execute the current instruction
            //
            addrprof_count (REG(IP));
            decode (REG(IR), REG(IV), FREG(IV), decodeflags | FLAG_PROCESS);

            // Check for watchpoint triggers
            if (wpoint_check ()       == TRUE)
            {
                runflag                = FALSE;
            }
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // check for and react to system error
        //
        if (sys_error                 != ERROR_NONE)
        {
            ////////////////////////////////////////////////////////////////////////////
            //
            // Per Vircon32  specifications, when a system  error occurs,
            // the  system  error   value,  InstructionPointer (REG(IP)),
            // InstructionRegister (REG(IR)), and ImmediateValue REG(IV),
            // are stored in R0, R1, R2,  and R3 respectively, the SP and
            // BP registers wiped, and flow  redirected to the BIOS error
            // handler at 0x10000000.
            //
            // To check: does  the cycle counter of  the instruction that
            // the error occurred on get incremented?
            //
            if (exitflag               == TRUE)
            {
                fprintf (stderr, "[v32sim] system error %d at IP=0x%.8X\n", (int) sys_error, REG(IP));
                exitstatus             = 3;
                haltflag               = TRUE;
                continue;
            }
            REG(R0)                    = sys_error;
            REG(R1)                    = REG(IP) + (((REG(IR) & IMMVAL_MASK) != 0) ? 2 : 1);
            REG(R2)                    = REG(IR);
            REG(R3)                    = REG(IV);

            REG(SP)                    = 0x003FFFFF;   // the last word of RAM, as at reset
            REG(BP)                    = 0x003FFFFF;

            REG(IP)                    = BIOS_ERROR_OFFSET;
            rom_offset                 = BIOS_ERROR_OFFSET;

            sys_error                  = ERROR_NONE;
            runflag                    = FALSE;

            continue; // kick to next iteration
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // update the Cycle Counter
        //
        if (ignoreflag                == FALSE)
        {
            update_cycle ();
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // update profcount if CPU instruction profiling is enabled
        //
        if (profileflag                == TRUE)
        {
            profcount                   = profcount + 1;
            if (csub                   != NULL)
            {
                csub -> COUNT           = csub -> COUNT + 1;
            }
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Increment rom_offset but only if we have not branched
        //
        if (branchflag                == TRUE)
        {
            branchflag                 = FALSE;
            if (retflag               == TRUE)
            {
                retflag                = FALSE;

                // Only stop if the backtrace depth matches the saved depth
                if ((nextflag         == TRUE) &&
                    (tpoint           == next_tpoint))
                {
                    runflag            = FALSE;
                }

                ////////////////////////////////////////////////////////////////////////
                //
                // if profiling is enabled, remove entry from subroutine profile list
                //
                if ((profileflag      == TRUE) &&
                    (runflag          == FALSE))
                {
                    if (colorflag     == TRUE)
                    {
                        fprintf (stdout, "\e[1;34m");
                    }

                    ////////////////////////////////////////////////////////////////////
                    //
                    // display stats of recently returned subroutine
                    //
                    csub -> CYCLES     = csub -> CYCLES + csub -> COUNT;
                    csub -> FRAMES     = csub -> CYCLES / V32_CYCLES_PER_FRAME;
                    csub -> time       = (float) csub -> CYCLES / (float) (V32_FRAMES_PER_SECOND * V32_CYCLES_PER_FRAME);
                    csub -> CYCLES     = csub -> CYCLES % V32_CYCLES_PER_FRAME;
                    if (csub -> label != NULL)
                    {
                        fprintf (display, "[profiling subroutine %s] ", csub -> label);
                    }
                    else
                    {
                        fprintf (display, "[profiling subroutine 0x%.8X] ", csub -> RAW);
                    }
                    fprintf (display, "total cycles: %u, frames: %u, overall runtime: %.3fs\n",    csub -> CYCLES, csub -> FRAMES, csub -> time);

                    if (colorflag     == TRUE)
                    {
                        fprintf (stdout, "\e[m");
                    }
                }

                ////////////////////////////////////////////////////////////////////////
                //
                // csub: revert pointer to previous function (obtain offset from
                // the backtrace list)
                //
                // old_count stores the number of cycles used in the returned
                // subroutine: we add these to the parent subroutine cycles
                //
                if ((tpoint           != NULL) &&
                    (csub             != NULL))
                {
                    csub -> ACTIVE     = FALSE;
                    old_count          = csub -> COUNT;
                    csub               = find_value (ppoint, tpoint -> RAW);
                    csub -> COUNT      = csub -> COUNT + old_count;
                }
            }
        }
        else
        {
            rom_offset                 = rom_offset   + 1;
            if ((REG(IR) & 0x02000000)   >  0)
            {
                rom_offset             = rom_offset   + 1;
            }
            ignoreflag                 = FALSE;
        }
    }

    if ((frame_limit == TRUE) && (exitstatus == 0) && (exitflag == TRUE))
    {
        exitstatus                  = 2;
    }
    if ((check_failures > 0) && (exitstatus == 0))
    {
        exitstatus                  = 1;     // an expect failed, or an until timed out
    }
    if ((script_errors > 0) && (exitstatus == 0) && (batchflag == TRUE))
    {
        exitstatus                  = 4;     // a command of the script could not be understood
    }
    if (quit_status_set              == TRUE)
    {
        exitstatus                  = quit_status;
    }
    addrprof_write ();
    if (consoleflag == TRUE)
    {
        gpu_console_show (stdout);
    }
    v32io_shutdown ();
    memc_save ();   // flush any memory card changes to its file

    if (dumpspec                      != NULL)
    {
        dump_words (dumpspec);
    }
    if (batchflag                    == FALSE)
    {
        fprintf (stdout, "------------------------------------\n");
        fprintf (stdout, "IP: 0x%.8X\n", REG(IP));
        fprintf (stdout, "IR: 0x%.8X\n", REG(IR));
        fprintf (stdout, "IV: 0x%.8X\n", REG(IV));
        fprintf (stdout, "------------------------------------\n");
        if (displayshow  (dpoint, 0)  != 0)
        {
            fprintf (stdout, "------------------------------------\n");
        }
        fprintf (stdout, "SYSTEM HALTED\n");
    }
    fflush (stdout);

    rfree (reg);
    rfree (memory);
    rfree (ioports);

    if ((devnull                      != NULL) &&
        (devnull                      != stderr))
    {
        fclose (devnull);
    }

    if ((verbose                      != NULL) &&
        (verbose                      != stderr) &&
        (verbose                      != devnull))
    {
        fclose (verbose);
    }

    return ((int32_t) exitstatus);
}
