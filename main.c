#include "defines.h"

////////////////////////////////////////////////////////////////////////////////////////
//
// global variables - important file and memory/register resources
//
FILE      *display;
FILE      *devnull;
FILE      *debug;
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
uint8_t    colorflag;
uint8_t    branchflag;
uint8_t    derefaddr;
uint8_t    haltflag;
uint8_t    waitflag;
uint8_t    wordsize;
uint32_t   rom_offset;
uint32_t   seek_word;
uint32_t   watch_word;
display_l *dpoint;

int32_t    main (int32_t  argc, uint8_t **argv)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    float      fimmediate              = 0.0;
    size_t     len                     = 0;
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
    commandfile                        = NULL;
    rom_offset                         = BIOS_START_OFFSET;
    branchflag                         = FALSE;
    runflag                            = FALSE;
    colorflag                          = FALSE;
    seek_word                          = 0xFFFFFFFF;
    watch_word                         = 0x00000000;
    wordsize                           = 4;
    derefaddr                          = FALSE;
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
    if (devnull                       == NULL)
    {
        fprintf (stderr, "[error] could not open '/dev/null' for writing!\n");
        exit (2);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // if verbose has not been redirected, point it at devnull
    //
    if (verbose                       == NULL)
    {
        verbose                        = devnull;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // if debug has not been redirected, point it at devnull
    //
    if (debug                         == NULL)
    {
        debug                          = devnull;
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

    /*
    fprintf (stdout, "rom_offset: %.8X\n", rom_offset);
    fprintf (stdout, "vbinoffset: %.8X\n", vbinoffset);
    fprintf (stdout, "vtexoffset: %.8X\n", vtexoffset);
    fprintf (stdout, "vsndoffset: %.8X\n", vsndoffset);
    */
    fprintf (stdout, "Vircon32 Simulator / Debugger  (v32sim)\n");
    fprintf (stdout, "=======================================\n");
    fprintf (stdout, "[ OFFSET ] [HEXVALUES] [OPC] [DST]\n");

    while ((action                    != INPUT_QUIT) &&
           (haltflag                  == FALSE))
    {
        ////////////////////////////////////////////////////////////////////////////////
        //
        // Check for delayed break to single-step via seek_word (argument)
        //
        if (rom_offset                == seek_word)
        {
            if (colorflag             == TRUE)
            {
                fprintf (stdout, "\e[1;31m");
            }
            fprintf (stdout, "[seekword] trigger encountered (0x%.8X)\n", seek_word);
            if (colorflag             == TRUE)
            {
                fprintf (stdout, "\e[m");
            }
            runflag                    = FALSE;
        }

        REG(IP)                        = rom_offset;
        word                           = IMEMGET(REG(IP));
        REG(IR)                        = word;        // current instruction

        if (watch_word                == word)
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

        immediate                      = word & 0x02000000;
        if (immediate                 == 0x02000000)
        {
            //rom_offset                 = rom_offset + 1;
            immediate                  = IMEMGET(REG(IP) + 1);
            fimmediate                 = FMEMGET(REG(IP) + 1);
            decodeflags                = FLAG_IMMEDIATE;
        }
        else
        {
            immediate                  = 0x00000000;
            fimmediate                 = 0.0;
            decodeflags                = FLAG_NONE;
        }
        REG(IV)                        = immediate;   // immediate value

        if ((debug                    != NULL) &&
            (runflag                  == TRUE))
        {
            put_word (word, FLAG_DISPLAY);
            decode   (word, immediate, fimmediate, decodeflags | FLAG_DISPLAY);
            if (FLAG_IMMEDIATE        == (decodeflags & FLAG_IMMEDIATE))
            {
                put_word (immediate, FLAG_DISPLAY);
            }
            fprintf  (stdout, "\n");
            displayshow  (dpoint, 0);
        }

        if (runflag                   == FALSE)
        {
            if (sys_reg_show          == TRUE)
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
                put_word (word, FLAG_DISPLAY);
            }

            decode   (word, immediate, fimmediate, decodeflags | FLAG_DISPLAY);

            if (FLAG_IMMEDIATE        == (decodeflags & FLAG_IMMEDIATE))
            {
                if (sys_reg_show      == TRUE)
                {
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
                    fprintf  (stdout, "[%.8X]: 0x%.8X", (REG(IP) + 1), REG(IV));
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
            if ((word & 0x02000000)   >  0)
            {
                rom_offset             = rom_offset   + 1;
            }
        }
    }

    fprintf (stdout, "SYSTEM HALTED\n");

    return (0);
}
