#include "defines.h"

////////////////////////////////////////////////////////////////////////////////////////
//
// global variables - important file and memory/register resources
//
FILE      *display;
FILE      *devnull;
FILE      *verbose;
uint8_t   *data;
uint8_t   *destination;
uint8_t   *source;
int8_t    *biosfile;
int8_t    *cartfile;
int8_t    *commandfile;
data_t    *reg;
int8_t    *token_label;

////////////////////////////////////////////////////////////////////////////////////////
//
// Variables related to IOPorts and memory
//
data_t   **ioports;
mem_t     *memory;
int8_t     sys_error;
uint8_t    sys_force;
uint8_t    sys_reg_show;

////////////////////////////////////////////////////////////////////////////////////////
//
// flags
//
uint8_t    action;
uint8_t    runflag;
uint8_t    branchflag;
uint8_t    indexflag;
uint8_t    haltflag;
uint8_t    waitflag;
uint8_t    wordsize;
uint32_t   rom_offset;
uint32_t   seek_word;
display_l *dpoint;

int32_t    main (int32_t  argc, uint8_t **argv)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    display_l *dtmp                    = NULL;
    FILE      *fptr                    = NULL;
    float      fimmediate              = 0.0;
    int32_t    index                   = 0;
    int32_t    value                   = 0;
    size_t     len                     = 0;
    uint8_t   *arg                     = NULL;
    uint8_t    decodeflags             = FLAG_NONE;
    uint8_t    input[64];
    uint8_t    processflag             = FALSE;
    uint8_t    token_type              = 0;
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
    commandfile                        = NULL;
    rom_offset                         = BIOS_START_OFFSET;
    branchflag                         = FALSE;
    runflag                            = FALSE;
    seek_word                          = 0xFFFFFFFF;
    wordsize                           = 4;
    indexflag                          = FALSE;
    haltflag                           = FALSE;
    waitflag                           = FALSE;
    sys_error                          = ERROR_NONE;
    sys_reg_show                       = FALSE;
    action                             = INPUT_INIT;
    token_label                        = NULL;

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

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Load commands from command-file
    //
    if (commandfile                   != NULL)
    {
        fptr                           = fopen (commandfile, "r");
        if (fptr                      == NULL)
        {
            fprintf (stderr, "[ERROR] Could not open '%s' for reading!\n", commandfile);
            exit (1);
        }

        while (!feof (fptr))
        {
            //if (input                 != NULL)
            //{
             //   free (input);
            //}
            index                      = 0;
            token_label                = NULL;
            while (!feof (fptr))
            {
                input[index]           = fgetc (fptr);
                if (input[index]      == '\n')
                {
                    input[index]       = '\0';
                    break;
                }
                index                  = index + 1;
            }

            if (feof (fptr))
            {
                break;
            }
            token_type                 = tokenize_input (input);

            switch (action)
            {
                case INPUT_DISPLAY:
                    switch (token_type)
                    {
                        case PARSE_NONE:
                            fprintf (stderr, "[ERROR] malformed display value\n");
                            break;

                        case PARSE_MEMORY:
                            arg                = strtok ((input+2), " ");
                            value              = strtol (arg, NULL, 16);
                            dtmp               = newdispnode (LIST_MEM, new_word_i32 (&value, 1), 1);
                            if (token_label   != NULL)
                            {
                                value          = sizeof (int8_t) * strlen (token_label) + 1;
                                dtmp -> label  = (int8_t *) malloc (value);
                                strcpy (dtmp -> label, token_label);
                            }
                            dpoint             = display_add (dpoint, dtmp);
                            break;

                        case PARSE_REGISTERS:
                            for (index         = 0;
                                 index        <= 15;
                                 index         = index + 1)
                            {
                                dtmp           = newdispnode (LIST_REG, new_word_i32 (&index, 1), 1);
                                dpoint         = display_add (dpoint, dtmp);
                            }
                            break;

                        case PARSE_REGISTER: // specific, general register
                            arg                = strtok ((input+2), " ");
                            token_type         = parse_reg (arg);
                            if (token_type    >  15)
                            {
                                sys_reg_show   = TRUE;
                                break;
                            }
                            token_type         = token_type & 0x0000001F;
                            dtmp               = newdispnode (LIST_REG, new_word_i32 ((uint32_t *) &token_type, 1), 1);
                            if (token_label   != NULL)
                            {
                                value          = sizeof (int8_t) * strlen (token_label) + 1;
                                dtmp -> label  = (int8_t *) malloc (value);
                                strcpy (dtmp -> label, token_label);
                            }
                            dpoint             = display_add (dpoint, dtmp);
                            fprintf (stdout, "[cmd] adding R%u to the list\n", token_type);
                            break;

                        case PARSE_IOPORT:
                            arg                = strtok ((input+2), " ");
                            value              = strtol (arg, NULL, 16);
                            dtmp               = newdispnode (LIST_IOP, new_word_i32 (&value, 1), 1);
                            if (token_label   != NULL)
                            {
                                value          = sizeof (int8_t) * strlen (token_label) + 1;
                                dtmp -> label  = (int8_t *) malloc (value);
                                strcpy (dtmp -> label, token_label);
                            }
                            dpoint             = display_add (dpoint, dtmp);
                            break;
                    }
                    break;
            }
        }
    }
    action                                     = INPUT_INIT;

    fprintf (stdout, "rom_offset: %.8X\n", rom_offset);
    fprintf (stdout, "vbinoffset: %.8X\n", vbinoffset);
    fprintf (stdout, "vtexoffset: %.8X\n", vtexoffset);
    fprintf (stdout, "vsndoffset: %.8X\n", vsndoffset);

    while ((action                    != INPUT_QUIT) &&
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
        word                           = IMEMGET (IP_REG);
        IR_REG                         = word;        // current instruction

        immediate                      = word & 0x02000000;
        if (immediate                 == 0x02000000)
        {
            rom_offset                 = rom_offset + 1;
            immediate                  = IMEMGET (IP_REG + 1);
            fimmediate                 = FMEMGET (IP_REG + 1);
            decodeflags                = FLAG_IMMEDIATE;
        }
        else
        {
            immediate                  = 0x00000000;
            fimmediate                 = 0.0;
            decodeflags                = FLAG_NONE;
        }
        IV_REG                         = immediate;   // immediate value

        if (runflag                   == FALSE)
        {
            if (sys_reg_show          == TRUE)
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

            if (FLAG_IMMEDIATE        == (decodeflags & FLAG_IMMEDIATE))
            {
                if (sys_reg_show      == TRUE)
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

            processflag                = FALSE;
            do
            {
                ////////////////////////////////////////////////////////////////////////
                //
                // Display the prompt and obtain input
                //
                processflag            = prompt (word);

            }
            while (processflag        == FALSE);
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
