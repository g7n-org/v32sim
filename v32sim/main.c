#include "defines.h"

////////////////////////////////////////////////////////////////////////////////////////
//
// global variables - important file and memory/register resources
//
FILE     *display;
FILE     *devnull;
FILE     *program;
FILE     *verbose;
uint8_t  *data;
uint8_t  *destination;
uint8_t  *source;
int8_t   *biosfile;
int8_t   *cartfile;
data_t   *reg;

////////////////////////////////////////////////////////////////////////////////////////
//
// Variables related to IOPorts and memory
//
data_t  **ioports;
mem_t    *memory;
int8_t    sys_error;
uint8_t   sys_force;
uint8_t   sys_reg_show;

////////////////////////////////////////////////////////////////////////////////////////
//
// flags
//
uint8_t   action;
uint8_t   runflag;
uint8_t   branchflag;
uint8_t   haltflag;
uint8_t   waitflag;
uint8_t   wordsize;
uint32_t  rom_offset;
uint32_t  seek_word;

int32_t    main (int32_t  argc, uint8_t **argv)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    display_l *display                 = NULL;
    display_l *dtmp                    = NULL;
    float      fimmediate              = 0.0;
    int32_t    index                   = 0;
    int32_t    value                   = 0;
    int32_t    lastaddr                = 0;
    size_t     len                     = 0;
    uint8_t   *input                   = NULL;
    uint8_t   *arg                     = NULL;
    //uint8_t    newcommand              = '\0';
    uint8_t    lastaction              = INPUT_INIT;
    uint8_t    decodeflags             = FLAG_NONE;
    uint8_t    processflag             = FALSE;
    uint32_t   immediate               = 0x00000000;
    uint32_t   vbinoffset              = 0x00000000;
    uint32_t   vtexoffset              = 0x00000000;
    uint32_t   vsndoffset              = 0x00000000;
    uint32_t   word                    = 0x00000000;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // initialize variables
    //
    biosfile                           = NULL;
    cartfile                           = NULL;
    rom_offset                         = BIOS_START_OFFSET;
    branchflag                         = FALSE;
    runflag                            = FALSE;
    seek_word                          = 0xFFFFFFFF;
    wordsize                           = 4;
    haltflag                           = FALSE;
    waitflag                           = FALSE;
    sys_error                          = ERROR_NONE;
    sys_reg_show                       = FALSE;
	action                             = INPUT_INIT;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Process command-line arguments
    //
    process_args ((int32_t) argc, (int8_t **) argv);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Open /dev/null
    //
    devnull                            = fopen ("/dev/null", "w");
    if (verbose                       == NULL)
    {
        verbose                        = devnull;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Open the indicated V32 file
    //
    if (cartfile                      == NULL)
    {
        fprintf (stderr, "[ERROR] Must specify Vircon32 cartridge file!\n");
        exit (NO_CART_ERROR);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Set up 'biosfile'
    //
    if (biosfile                      == NULL)
    {
        len                            = strlen (BIOS_DEFAULT_PATH) + 1;
        biosfile                       = (int8_t *) malloc (len);
        if (biosfile                  == NULL)
        {
            fprintf (stderr, "[ERROR] Allocation for '%s' failed\n", "biosfile");
            exit (STRING_ALLOC_FAIL);
        }
        strcpy (biosfile, BIOS_DEFAULT_PATH);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // 18 is the maximum length of potential columnar output of an operand:
    //
    // "[RXX+0x12345678],\0" <- 17 bytes of string data + 1 NULL terminator
    // "0123456789ABCDEF10"
    //
    len                                = sizeof (uint8_t) * 18;
    destination                        = (uint8_t *) malloc (len);
    source                             = (uint8_t *) malloc (len);
    if ((destination                  == NULL) ||
        (source                       == NULL))
    {
        fprintf (stderr, "[ERROR] Allocation of string failed\n");
        exit (STRING_ALLOC_FAIL);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Unless I use some better input mechanism, input can be up to 256 bytes in 
    // length (should be more than enough)
    //
    len                                = sizeof (uint8_t) * 256;
    input                              = (uint8_t *) malloc (len);
    arg                                = (uint8_t *) malloc (len);
    if ((input                        == NULL) ||
        (arg                          == NULL))
    {
        fprintf (stderr, "[ERROR] Allocation of string failed\n");
        exit (STRING_ALLOC_FAIL);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // This is merely the word of data being read in, so wordsize bytes
    //
    len                                = sizeof (uint8_t) * wordsize;
    data                               = (uint8_t *) malloc (len);
    if (data                          == NULL)
    {
        fprintf (stderr, "[ERROR] Allocation of string '%s' failed\n", data);
        exit (STRING_ALLOC_FAIL);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Allocate Vircon32 IOPorts (a 2D array of ports)
    //
    sys_force                          = FALSE;
    init_ioports ();

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Allocate Vircon32 memory regions (RAM, BIOS, CART, MEMC)
    //
    init_memory ();
    load_memory (V32_PAGE_BIOS, biosfile); // load BIOS file contents into memory
    load_memory (V32_PAGE_CART, cartfile); // load CART file contents into memory
    //load_memory (V32_PAGE_MEMC, memcfile); // load MEMC file contents into memory

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Our registers will be in a word_t array
    //
    init_registers ();

    fprintf (stdout, "rom_offset: %.8X\n", rom_offset);
    fprintf (stdout, "vbinoffset: %.8X\n", vbinoffset);
    fprintf (stdout, "vtexoffset: %.8X\n", vtexoffset);
    fprintf (stdout, "vsndoffset: %.8X\n", vsndoffset);

    while ((*(input+0)                != EOF) &&
           (*(input+0)                != 'q') &&
           (haltflag                  == FALSE))
    {
        ////////////////////////////////////////////////////////////////////////////////
        //
        // Check for delayed break to single-step via seek_word (argument)
        //
        if (rom_offset                == seek_word)
        {
            runflag                    = FALSE;
        }

        IP_REG                         = rom_offset;
        word                           = word2int (memory_get (IP_REG));
        IR_REG                         = word;        // current instruction

        immediate                      = word & 0x02000000;
        if (immediate                 == 0x02000000)
        {
            rom_offset                 = rom_offset + 1;
            immediate                  = word2int   (memory_get (IP_REG + 1));
            fimmediate                 = word2float (memory_get (IP_REG + 1));
            decodeflags                = FLAG_IMMEDIATE;
        }
        else
        {
            immediate                  = 0x00000000;
            fimmediate                 = 0.0;
            decodeflags                = FLAG_NONE;
        }
        IV_REG                         = immediate;   // immediate value

        if (sys_reg_show              == TRUE)
        {
            show_sysregs ();
            switch ((IP_REG & 0x30000000) >> 28)
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

            fprintf (stdout, "[%.8X]: ", IP_REG);
        }
        else
        {
            put_word (word, FLAG_DISPLAY);
        }
        decode   (word, immediate, fimmediate, decodeflags | FLAG_DISPLAY);

        if (FLAG_IMMEDIATE            == (decodeflags & FLAG_IMMEDIATE))
        {
            if (sys_reg_show          == TRUE)
            {
                switch ((IP_REG & 0x30000000) >> 28)
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
                fprintf  (stdout, "[%.8X]: 0x%.8X", (IP_REG + 1), IV_REG);
            }
            else
            {
                put_word (immediate, FLAG_DISPLAY);
            }
            fprintf  (stdout, "\n");
        }

        if (runflag                   == FALSE)
        {
            //newcommand                 = '\0';
			//action                     = INPUT_NONE;
            processflag                = FALSE;
            do
            {
                displayshow  (display, 0);

                ////////////////////////////////////////////////////////////////////////
                //
                // Display the prompt and obtain input
                //
                if (input             != NULL)
                {
                    free (input);
                }
				action                 = INPUT_INIT;
                input                  = get_input (stdin, "v32sim> ");
				if (input             == '\0')
				{
					action             = INPUT_NONE;
				}
                //tokenize_asm   (input);
				fprintf (stdout, "action: %u\n", action);
				if (action            != INPUT_NONE)
				{
					tokenize_input (input);
				}
                /*
                fprintf (stdout, "v32sim> ");

                index                  = 0;
                do
                {
uint8_t *get_input (FILE *fptr, const uint8_t *prompt)
                    *(input+index)     = fgetc (stdin);
                    index              = index + 1;
                }
                while (*(input+(index-1)) != '\n');
                */
                
                ////////////////////////////////////////////////////////////////////////
                //
                // newcommand will be the character recently input; if newline,
                // repeat lastcommand
                //
                //newcommand                 = *(input+0);
				if (action                == INPUT_NONE)
				{
					action                 = lastaction;
				}
					/*
                if (*(input+0)            == '\0')
                {
                    *(input+1)             = '\0';
                    newcommand             = lastcommand;
                }
                *(input+index-1)           = '\0';
				*/
                
                switch (action)
                {
                    case INPUT_BREAK:
                        fprintf (stdout, "BREAK\n");
                        break;

                    case INPUT_CONTINUE:
                        fprintf (stdout, "CONTINUE\n");
                        processflag        = TRUE;
                        runflag            = TRUE;
                        break;

                    case INPUT_DISPLAY:
                        fprintf (stdout, "DISPLAY\n");
                        arg                = strtok ((input+2), " ");
                        value              = strlen (arg);
                        fprintf (stdout, "arg: %s, '%c'\n", arg, *(arg+1));
                        if ((*(arg+0)     == 'R') ||
                            (*(arg+0)     == 'r'))
                        {
                            value          = atoi ((arg+1));
                            dtmp           = newdispnode (LIST_REG, new_word_i32 (&value, 1), 1);
                        }
                        else if ((*(arg)  == 'I') ||
                                 (*(arg)  == 'i'))
                        {
                            sys_reg_show   = TRUE;
                            break;
                        }
                        else if (value    == 10) // memory address
                        {
                            value          = strtol (arg, NULL, 16);
                            dtmp           = newdispnode (LIST_MEM, new_word_i32 (&value, 1), 1);
                        }
                        else
                        {
                            value          = strtol (arg, NULL, 16);
                            dtmp           = newdispnode (LIST_IOP, new_word_i32 (&value, 1), 1);
                        }
                        display            = display_add (display, dtmp);
                        //newcommand         = '\0';
						action             = INPUT_NONE;
                        break;

                        /*
                    case 'm':
                        if (*(input+1)    == ' ')
                        {
                            arg            = strtok ((input+2), " ");
                            value          = strtol (arg, NULL, 16);
                            sprintf (source, "[%.8X]:", value);
                            fprintf (stdout, "%-4s 0x%.8X\n",
                                             source, word2int (memory_get (value)));
                            lastaddr       = value;
                        }
                        else
                        {
                            value          = lastaddr & 0x0FFFFFFF;
                            if (value     <  0x0000004)
                            {
                                lastaddr   = lastaddr & 0xF0000000;
                                lastaddr   = lastaddr | 0x00000004;
                            }

                            for (index     = lastaddr - 4;
                                 index    <  lastaddr + 4;
                                 index     = index    + 1)
                            {
                                sprintf (source, "[%.8X]:", index);
                                fprintf (stdout, "%-11s 0x%.8X\n",
                                                 source, word2int (memory_get (index)));
                            }
                            lastaddr       = index;
                        }
                        break;

                    case 'P':
                        if (*(input+1)    == ' ')
                        {
                            arg            = strtok ((input+2), " ");
                            sys_force      = TRUE;
                            index          = strtol (arg, NULL, 16);
                            value          = ioports_get (index);
                            fprintf (stdout, "[%s]: 0x%.8X\n", arg, value);
                        }
                        break;
*/
                    case INPUT_QUIT:
                        processflag        = 2;
                        break;

                        /*
                    case 'r':
                        if (*(input+1)    == ' ')
                        {
                            arg            = strtok ((input+2), " ");
                            value          = atoi (arg+1);
                            sprintf (source, "R%u:", value);
                            fprintf (stdout, "%-4s 0x%.8X\n", source, REG(value));
                        }
                        else
                        {
                            for (index     = 0;
                                 index    <  16;
                                 index     = index + 1)
                            {
                                sprintf (source, "R%u:", index);
                                fprintf (stdout, "%-4s 0x%.8X\n", source, REG(index));
                            }
                        }
                        break;*/

                    case INPUT_STEP:
                        fprintf (stdout, "STEP\n");
                        processflag        = TRUE;
                        //lastcommand        = 's';
						lastaction         = INPUT_STEP;
                        break;

					case INPUT_NEXT:
						fprintf (stdout, "NEXT\n");
						processflag        = TRUE;
						lastaction         = INPUT_NEXT;

						////////////////////////////////////////////////////////////////
						//
						// if the instruction is a CALL, set seek_word and then
						// set runflag to TRUE; otherwise, should behave like step
						//
						if (((word & 0xFC000000) >> 26) == 0x03)
						{
							runflag        = TRUE;
							//seek_word      = rom_offset + 1; // next instruction
							seek_word      = rom_offset;
							if ((word & 0x02000000)     >  0)
							{
								seek_word  = seek_word  + 1; // if immediate value
							}
							fprintf (stdout, "seek_word: 0x%.8X\n", seek_word);
						}
						break;

                    case INPUT_HELP:
                        fprintf (stdout, "  c          - resume execution\n");
                        fprintf (stdout, "  d XYZ      - add displaylist item\n");
                        fprintf (stdout, "    R#       - general register\n");
                        fprintf (stdout, "    I(P|R|V) - system register\n");
                        fprintf (stdout, "    0xaddr   - memory address\n");
                        fprintf (stdout, "    0xioaddr - IOPort\n");
                        fprintf (stdout, "  m [addr]   - display memory address\n");
                        fprintf (stdout, "  P ioaddr   - display IOPort content\n");
                        fprintf (stdout, "  r [r#]     - display register(s)\n");
                        fprintf (stdout, "  n          - next (skip over subroutines)\n");
                        fprintf (stdout, "  s          - step to next instruction\n");
                        break;
                }
                lastaction                 = action;
            }
            while (processflag            == FALSE);
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // process the current instruction
        //
        decode (word, immediate, fimmediate, decodeflags | FLAG_PROCESS);

        ////////////////////////////////////////////////////////////////////////////////
        //
        // check for and react to system error
        //
        if (sys_error                     != ERROR_NONE)
        {
            ////////////////////////////////////////////////////////////////////////////
            //
            // Per Vircon32  specifications, when a system  error occurs,
            // the  system  error   value,  InstructionPointer  (IP_REG),
            // InstructionRegister (IR_REG),  and ImmediateValue (IV_REG)
            // are stored in R0, R1, R2,  and R3 respectively, the SP and
            // BP registers wiped, and flow  redirected to the BIOS error
            // handler at 0x10000000.
            //
            // To check: does  the cycle counter of  the instruction that
            // the error occurred on get incremented?
            //
            REG(R0)                        = sys_error;
            REG(R1)                        = IP_REG;
            REG(R2)                        = IR_REG;
            REG(R3)                        = IV_REG;

            SP_REG                         = 0x00000000;
            BP_REG                         = 0x00000000;

            IP_REG                         = BIOS_ERROR_OFFSET;
            continue; // kick to next iteration
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // update the Cycle Counter
        //
        update_cycle ();

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
        }
    }
    
    fprintf (stdout, "SYSTEM HALTED\n");

    return (0);
}
