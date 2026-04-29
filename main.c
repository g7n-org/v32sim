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
uint8_t   runflag;
uint8_t   colorflag;
uint8_t   branchflag;
uint8_t   ignoreflag;
uint8_t   derefaddr;
uint8_t   errorcheck;
uint8_t   haltflag;
uint8_t   waitflag;
uint8_t   wordsize;
uint32_t  rom_offset;
uint32_t  seek_word;
uint32_t  watch_word;
linked_l *bpoint;
linked_l *dpoint;
linked_l *lpoint;
linked_l *mpoint; // tracking allocated memory
linked_l *tpoint;

int32_t   main (int32_t  argc, char **argv)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    FILE     *fptr                  = NULL;
    linked_l *btmp                  = NULL;
    linked_l *tmp                   = NULL;
    size_t    len                   = 0;
    slli      elapsed_ns            = 0;
    TimeSpec  delay;
    TimeSpec  start;
    TimeSpec  end;
    uint8_t   chk                   = FALSE;
    uint8_t   decodeflags           = FLAG_NONE;
    uint8_t   errorflag             = FLAG_NONE;
    uint8_t   processflag           = FALSE;
    uint8_t  *filename              = NULL;
    uint32_t  value                 = 0;
    uint32_t  line_number           = 0;
    uint8_t  *line_input            = NULL;
    int32_t   index                 = 0;
    size_t    buffer_size           = 0;

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
    branchflag                      = FALSE;
    biosasmdebugflag                = FALSE;
    bioscdebugflag                  = FALSE;
    cartasmdebugflag                = FALSE;
    cartcdebugflag                  = FALSE;
    runflag                         = FALSE;
    colorflag                       = FALSE;
    seek_word                       = 0xFFFFFFFF;
    watch_word                      = 0x00000000;
    wordsize                        = 4;
    derefaddr                       = FALSE;
    haltflag                        = FALSE;
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
    tpoint                          = NULL;

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
    // Initialize BIOS (load contents into memory), optionally loading any debug data
    //
    chk                             = load_memory (V32_PAGE_BIOS, biosfile);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // load any debug file labels pertaining to page being loaded
    //
    if ((biosasmdebugflag          == TRUE)  &&
        (chk                       == TRUE)  &&
        (biosfile                  != NULL))
    {
        load_labels (biosfile, V32_PAGE_BIOS, FLAG_SEARCH | FLAG_ASM);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // load any debug file labels pertaining to page being loaded
    //
    if ((bioscdebugflag            == TRUE)  &&
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
    if ((cartasmdebugflag          == FALSE) &&
        (chk                       == TRUE) &&
        (cartfile                  != NULL))
    {
        load_labels (cartfile, V32_PAGE_CART, FLAG_SEARCH | FLAG_ASM);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // load any debug file labels pertaining to page being loaded
    //
    if ((cartcdebugflag            == FALSE) &&
        (chk                       == TRUE) &&
        (cartfile                  != NULL))
    {
        load_labels (cartfile, V32_PAGE_CART, FLAG_SEARCH | FLAG_C);
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

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Load commands from command-file
    //
    if (commandfile                != NULL)
    {
        fprintf (verbose, "LOADING COMMAND-FILE\n");
        load_command ();
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Process breakpoint waitlist (tpoint), if any
    //
    if (tpoint                     != NULL)
    {
        tmp                         = tpoint;
        while (tmp                 != NULL)
        {
            if (tmp -> label       == NULL) // not a label: offset
            {
                value               = tmp -> data.raw;
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
                    value           = btmp -> data.raw;
                    btmp            = find_value (bpoint, value); // check offset in breaklist
                    if (btmp       == NULL) // not an existing breakpoint in breaklist
                    {
                        btmp        = list_grab (&tpoint, tmp);
                        btmp -> data.raw  = value;
                        fprintf (debug, "[main] BREAKing at label: '%s'\n", btmp -> label);
                        fprintf (debug, "[main] BREAKing at offset 0x%.8X\n", btmp -> data.raw);
                        bpoint      = list_add (bpoint, btmp);
                    }
                }
            }

            tmp                     = tmp -> next;
        }
    }

    /*
    fprintf (stdout, "rom_offset: %.8X\n", rom_offset);
    */
    fprintf (stdout, "Vircon32 Simulator / Debugger  (v32sim)\n");
    fprintf (stdout, "=======================================\n");
    fprintf (stdout, "[ OFFSET ] [HEXVALUES] [OPC] [DST]\n");

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
            value                      = btmp -> data.raw;
            if (rom_offset            == value)
            {
                if (colorflag         == TRUE)
                {
                    fprintf (stdout, "\e[1;31m");
                }
                fprintf (stdout, "[breakpoint] triggered on 0x%.8X\n", value);
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
            fprintf (stdout, "[watchfor] trigger encountered (0x%.8X)\n", watch_word);
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
        REG(IP)                        = rom_offset;
        REG(IR)                        = IMEMGET(REG(IP)); // current instruction

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Update the ImmediateValue system register (based on instruction in IR)
        //
        if ((REG(IR) & IMMVAL_MASK)   == IMMVAL_MASK)
        {
            REG(IV)                    = IMEMGET(REG(IP) + 1);
            decodeflags                = FLAG_IMMEDIATE;
        }
        else
        {
            REG(IV)                    = 0x00000000;
            decodeflags                = FLAG_NONE;
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
            // Display the prompt
            //
            do
            {

				if (modeflag          == FLAG_C)
				{
					//fprintf (stdout, "** MODEFLAG FLAG_C\n");
					tmp                = find_value (lpoint, REG(IP));
					if (tmp           != NULL)
					{
						if (filename  == NULL)
						{
							fptr       = fopen (tmp -> cname, "r");
							line_number  = 1;
						}
						else if (strcmp (filename, tmp -> cname) == 0)
						{
							fclose (fptr);
							fptr       = fopen (tmp -> cname, "r");
							line_number  = 1;
						}

						if (tmp -> line <  line_number)
						{
							if (fptr != NULL)
								fclose (fptr);
							fptr       = fopen (tmp -> cname, "r");
							line_number  = 1;
						}

						for (index     = 1;
							 index    <  tmp -> line;
							 index     = index + 1)
						{
							len        = getline (&line_input, &buffer_size, fptr);
						}
						line_number    = index;

						/*
						if (len       >  0)
							fprintf (stdout, "%4u: %s\n", tmp -> line, line_input);
						else
							fprintf (stdout, "empty (line_number: %u, tmp->line: %u\n", line_number, tmp->line);
							*/
					}
				}
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
                        put_word (REG(IV), FLAG_DISPLAY);
                    }
                    fprintf  (stdout, "\n");
                }

                processflag            = FALSE;

                displayshow  (dpoint, 0);

                ////////////////////////////////////////////////////////////////////////
                //
                // Display the prompt and obtain input
                //
                processflag            = prompt (REG(IR));

            }
            while (processflag        == FALSE);
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // process the current instruction
        //
        if (ignoreflag                == FALSE)
        {
            ////////////////////////////////////////////////////////////////////////////
            //
            // Obtain the current time, for instruction timing purposes
            // (pre-execution)
            //
            timespec_get (&start, TIME_UTC);

            ////////////////////////////////////////////////////////////////////////////
            //
            // Clear the system error variable
            //
            sys_error                  = ERROR_NONE;

            ////////////////////////////////////////////////////////////////////////////
            //
            // Execute the current instruction
            //
            decode (REG(IR), REG(IV), FREG(IV), decodeflags | FLAG_PROCESS);

            ////////////////////////////////////////////////////////////////////////////
            //
            // Obtain the current time (post execution), for instruction timing
            // purposes
            //
            timespec_get (&end, TIME_UTC);

            ////////////////////////////////////////////////////////////////////////////
            //
            // Calculate the time elapsed
            //
            elapsed_ns                 = timediff_ns (&start, &end);

            ////////////////////////////////////////////////////////////////////////////
            //
            // If the instruction took less than 66 nanoseconds, we need to
            // delay to keep sync with the 15MHz clock speed
            //
            fprintf (debug, "[main] instruction took %llu nanoseconds\n", elapsed_ns);
            if (elapsed_ns            <  66)
            {
                delay.tv_sec           = 0;
                delay.tv_nsec          = 66 - elapsed_ns;

                nanosleep (&delay, NULL);
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
            REG(R0)                    = sys_error;
            REG(R1)                    = REG(IP);
            REG(R2)                    = REG(IR);
            REG(R3)                    = REG(IV);

            REG(SP)                    = 0x00000000;
            REG(BP)                    = 0x00000000;

            REG(IP)                    = BIOS_ERROR_OFFSET;
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
        // Increment rom_offset but only if we have not branched
        //
        if (branchflag                == TRUE)
        {
            branchflag                 = FALSE;
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

    rfree (reg);
    rfree (memory);
    rfree (ioports);

    if ((devnull                      != NULL) &&
        (devnull                      != stderr))
    {
        fclose (devnull);
    }

    if ((verbose                      != NULL) &&
        (verbose                      != stderr))
    {
        fclose (verbose);
    }

    fprintf (stdout, "SYSTEM HALTED\n");

    return (0);
}
