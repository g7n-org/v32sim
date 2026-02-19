#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <getopt.h>
#include <time.h>
#include "ioports.h"

#define  BIOS_DEFAULT_PATH   "/usr/local/Vircon32/Emulator/Bios/StandardBios.v32"

#define  V32_PAGE_RAM        0
#define  V32_PAGE_BIOS       1
#define  V32_PAGE_CART       2
#define  V32_PAGE_MEMC       3

#define  RAM_LAST_ADDR       0x003FFFFF
#define  BIOS_LAST_ADDR      0x100FFFFF
#define  CART_LAST_ADDR      0x27FFFFFF
#define  MEMC_LAST_ADDR      0x3003FFFF

#define  IOPORTS_ALLOC_FAIL  3
#define  IOPORTS_READ_ERROR  4
#define  IOPORTS_WRITE_ERROR 5
#define  IOPORTS_BAD_PORT    6

#define  MEMORY_ALLOC_FAIL   7
#define  MEMORY_READ_ERROR   8
#define  MEMORY_WRITE_ERROR  9
#define  MEMORY_BAD_ACCESS   10

#define  FILE_OPEN_ERROR     11
#define  FILE_POSITION_ERROR 12

#define  STRING_ALLOC_FAIL   13
#define  DATA_ALLOC_FAIL     14
#define  LIST_ALLOC_FAIL     15

#define  LIST_REG            0
#define  LIST_MEM            1
#define  LIST_IOP            2

#define  NUM_MEMORY_PAGES    4

#define  NUM_PORT_CATEGORIES 7
#define  NUM_TIM_PORTS       4
#define  NUM_RNG_PORTS       1
#define  NUM_GPU_PORTS       18
#define  NUM_SPU_PORTS       14
#define  NUM_INP_PORTS       13
#define  NUM_CAR_PORTS       4
#define  NUM_MEM_PORTS       1

#define  TIM_PORT            0
#define  RNG_PORT            1
#define  GPU_PORT            2
#define  SPU_PORT            3
#define  INP_PORT            4
#define  CAR_PORT            5
#define  MEM_PORT            6

#define  OPCODE_MASK         0xFC000000
#define  DSTREG_MASK         0x01E00000
#define  SRCREG_MASK         0x001E0000
#define  MOVADR_MASK         0x0001C000
#define  IOPORT_MASK         0x00003FFF
#define  OPCODESHIFT         26
#define  DSTREGSHIFT         21
#define  SRCREGSHIFT         17
#define  MOVADRSHIFT         14

#define  TRUE                1
#define  FALSE               0

#define  NUM_REGISTERS       19

#define  R0                  0
#define  R1                  1
#define  R2                  2
#define  R3                  3
#define  R4                  4
#define  R5                  5
#define  R6                  6
#define  R7                  7
#define  R8                  8
#define  R9                  9
#define  R10                 10
#define  R11                 11
#define  CR                  11
#define  R12                 12
#define  SR                  12
#define  R13                 13
#define  DR                  13
#define  R14                 14
#define  BP                  14
#define  R15                 15
#define  SP                  15
#define  IP                  16
#define  IR                  17
#define  IV                  18

#define  DSTREG              (reg+dst) -> i32
#define  SRCREG              (reg+src) -> i32
#define  BP_REG              (reg+BP)  -> i32
#define  SP_REG              (reg+SP)  -> i32
#define  IP_REG              (reg+IP)  -> i32
#define  IR_REG              (reg+IR)  -> i32
#define  IV_REG              (reg+IV)  -> i32

#define  FLAG_NONE           0
#define  FLAG_DISPLAY        1
#define  FLAG_PROCESS        2 
#define  FLAG_IMMEDIATE      4

#define  FLAG_READ           4
#define  FLAG_WRITE          2

#define  HLT                 0x00
#define  WAIT                0x01
#define  JMP                 0x02
#define  CALL                0x03
#define  RET                 0x04
#define  JT                  0x05
#define  JF                  0x06
#define  IEQ                 0x07
#define  INE                 0x08
#define  IGT                 0x09
#define  IGE                 0x0A
#define  ILT                 0x0B
#define  ILE                 0x0C
#define  MOV                 0x13
#define  PUSH                0x15
#define  POP                 0x16
#define  IN                  0x17
#define  OUT                 0x18
#define  CIB                 0x1E
#define  NOT                 0x20
#define  AND                 0x21
#define  OR                  0x22
#define  XOR                 0x23
#define  BNOT                0x24
#define  SHL                 0x25
#define  IADD                0x26
#define  ISUB                0x27
#define  IMUL                0x28
#define  IDIV                0x29
#define  IMOD                0x2A

union  word_type
{
    int32_t  i32;
    float    f32;
};
typedef union word_type word_t;

typedef struct display_list display_l;
struct display_list
{
    uint8_t    type;
    uint8_t    num;
    word_t    *list;
    display_l *next;
};

struct data_type
{
    word_t   value;
    uint8_t  flag;
    int8_t  *name;
};
typedef struct data_type data_t;

struct memory_type
{
    uint8_t   type;
    data_t   *data;
    uint32_t  firstaddr;
    uint32_t  last_addr;
    uint32_t  size;
};
typedef struct memory_type mem_t;

FILE     *display;
FILE     *devnull;
FILE     *program;
uint8_t  *destination;
uint8_t  *source;
int8_t   *biosfile;
int8_t   *cartfile;
mem_t    *memory;
word_t   *reg;

////////////////////////////////////////////////////////////////////////////////////////
//
// Variables related to IOPorts
//
data_t  **ioports;
uint8_t   sys_force;
uint8_t   sys_reg_show;

uint8_t  *data;
uint8_t   runflag;
uint8_t   doneflag;
uint8_t   haltflag;
uint8_t   waitflag;
uint8_t   wordsize;
uint32_t  rom_offset;

////////////////////////////////////////////////////////////////////////////////////////
//
// Function prototypes
//
size_t     get_filesize (int8_t *);
uint32_t   get_word     (FILE *);
void       put_word     (uint32_t,    uint8_t);
void       decode       (uint32_t,    uint32_t,    uint8_t);
void       init_ioports (void);                              // initialize IOPorts
int32_t    ioports_get  (uint16_t);                          // get value from port
void       ioports_set  (uint16_t,    int32_t);              // set value to port
void       init_memory  (void);                              // initialize memory
void       load_memory  (uint32_t,    int8_t *);             // load file into memory
word_t    *memory_get   (uint32_t);                          // get value from memory
void       memory_set   (uint32_t,    uint32_t);             // set value to memory
word_t    *new_word_i32 (uint32_t *,  uint8_t);
void       update_cycle (void);                              // updating CycleCounter
void       update_frame (void);                              // updating FrameCounter
uint32_t   word2int     (word_t *);
float      word2float   (word_t *);
display_l *newdispnode  (uint8_t,     word_t *,    uint8_t);
display_l *display_add  (display_l *, display_l *);
void       displayshow  (display_l *, uint8_t);
void       show_sysregs (void);
void       process_args (int32_t,     int8_t **);
void       usage        (int8_t *);

int32_t    main     (int32_t  argc, uint8_t **argv)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    display_l *display                 = NULL;
    display_l *dtmp                    = NULL;
    int32_t    index                   = 0;
    int32_t    value                   = 0;
    int32_t    lastaddr                = 0;
    size_t     len                     = 0;
    uint8_t   *input                   = NULL;
    uint8_t   *arg                     = NULL;
    uint8_t    newcommand              = '\0';
    uint8_t    lastcommand             = '\0';
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
    rom_offset                         = 0x00000000;
    doneflag                           = FALSE;
    runflag                            = FALSE;
    wordsize                           = 4;
    haltflag                           = FALSE;
    waitflag                           = FALSE;
    sys_reg_show                       = FALSE;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Process command-line arguments
    //
    process_args ((int32_t) argc, (int8_t **) argv);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Open the indicated V32 file
    //
    if (cartfile                      == NULL)
    {
        fprintf (stderr, "[ERROR] Must specify Vircon32 cartridge file!\n");
        exit (1);
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
    // Open /dev/null
    //
    devnull                            = fopen ("/dev/null", "w");

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
    len                                = sizeof (int32_t) * NUM_REGISTERS;
    reg                                = (word_t *) malloc (len);
    if (reg                           == NULL)
    {
        fprintf (stderr, "[ERROR] Failed to allocate resources for registers\n");
        exit    (DATA_ALLOC_FAIL);
    }

    for (index                         = 0;
         index                        <  NUM_REGISTERS;
         index                         = index + 1)
    {
        (reg+index) -> i32             = 0x00000000;
    }
    BP_REG                             = 0x003FFFFF;
    SP_REG                             = 0x003FFFFF;

    rom_offset                         = 0x10000004;

    fprintf (stdout, "rom_offset: %.8X\n", rom_offset);
    fprintf (stdout, "vbinoffset: %.8X\n", vbinoffset);
    fprintf (stdout, "vtexoffset: %.8X\n", vtexoffset);
    fprintf (stdout, "vsndoffset: %.8X\n", vsndoffset);

    while ((*(input+0)                != EOF) &&
           (*(input+0)                != 'q') &&
           (haltflag                  == FALSE))
    {
        if (doneflag                  == TRUE)
        {
            rom_offset                 = rom_offset   + 1;
            doneflag                   = FALSE;
        }
        IP_REG                         = rom_offset;
        word                           = word2int (memory_get (IP_REG));
        IR_REG                         = word;        // current instruction

        immediate                      = word & 0x02000000;
        if (immediate                 == 0x02000000)
        {
            rom_offset                 = rom_offset + 1;
            immediate                  = word2int (memory_get (IP_REG + 1));
            decodeflags                = FLAG_IMMEDIATE;
        }
        else
        {
            immediate                  = 0x00000000;
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
        decode   (word, immediate, decodeflags | FLAG_DISPLAY);

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
            newcommand                 = '\0';
            processflag                = FALSE;
            do
            {
                displayshow  (display, 0);

                ////////////////////////////////////////////////////////////////////////
                //
                // Display the prompt (filter out trailing newline)
                //
                fprintf (stdout, "v32sim> ");

                index                  = 0;
                do
                {
                    *(input+index)     = fgetc (stdin);
                    index              = index + 1;
                }
                while (*(input+(index-1)) != '\n');
                
                ////////////////////////////////////////////////////////////////////////
                //
                // newcommand will be the character recently input; if newline,
                // repeat lastcommand
                //
                newcommand                 = *(input+0);
                if (*(input+0)            == '\n')
                {
                    *(input+1)             = '\0';
                    newcommand             = lastcommand;
                }
                *(input+index-1)           = '\0';
                
                switch (newcommand)
                {
                    case 'c':
                        processflag        = TRUE;
                        runflag            = TRUE;
                        break;

                    case 'd':
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
                        newcommand         = '\0';
                        break;

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

                    case 'r':
                        if (*(input+1)    == ' ')
                        {
                            arg            = strtok ((input+2), " ");
                            value          = atoi (arg+1);
                            sprintf (source, "R%u:", value);
                            fprintf (stdout, "%-4s 0x%.8X\n", source, (reg+value) -> i32);
                        }
                        else
                        {
                            for (index     = 0;
                                 index    <  16;
                                 index     = index + 1)
                            {
                                sprintf (source, "R%u:", index);
                                fprintf (stdout, "%-4s 0x%.8X\n", source, (reg+index) -> i32);
                            }
                        }
                        break;

                    case 's':
                        processflag        = TRUE;
                        lastcommand        = 's';
                        break;

                    case '?':
                    case 'h':
                        fprintf (stdout, "  c          - resume execution\n");
                        fprintf (stdout, "  d XYZ      - add displaylist item\n");
                        fprintf (stdout, "    R#       - general register\n");
                        fprintf (stdout, "    I(P|R|V) - system register\n");
                        fprintf (stdout, "    0xaddr   - memory address\n");
                        fprintf (stdout, "    0xioaddr - IOPort\n");
                        fprintf (stdout, "  c          - resume execution\n");
                        fprintf (stdout, "  m [addr]   - display memory address\n");
                        fprintf (stdout, "  P ioaddr   - display IOPort content\n");
                        fprintf (stdout, "  r [r#]     - display register(s)\n");
                        fprintf (stdout, "  s          - step to next instruction\n");
                        break;
                }
                lastcommand                = newcommand;
            }
            while (processflag            == FALSE);
        }

        decode (word, immediate, decodeflags | FLAG_PROCESS);

        /*
        IP_REG                             = rom_offset;  // location
        IR_REG                             = word;        // current instruction
        IV_REG                             = immediate;   // immediate value
        */

        ////////////////////////////////////////////////////////////////////////////////
        //
        // update the Cycle Counter
        //
        update_cycle ();

    }
    
    fprintf (stdout, "SYSTEM HALTED\n");

    return (0);
}

//////////////////////////////////////////////////////////////////////////////
//
// get_word(): obtain the next word from the program file,  the  word 
//             is assembled with shifts and iORs from the bytes.
//
// RETURN VAL: the uint32_t of the word just read
//
uint32_t  get_word (FILE *fptr)
{
    //////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize local variables
    //
    int32_t   index   = 0;
    uint32_t  word    = 0x00000000;  // clear word

    //////////////////////////////////////////////////////////////////////////
    //
    // Read a wordsize amount of data from the FILE pointer
    //
    fread (data, sizeof (uint8_t), wordsize, fptr);

    //////////////////////////////////////////////////////////////////////////
    //
    // If we have not encountered the end of the file, proceed with
    // processing
    //
    if (!feof (fptr))
    {
        //////////////////////////////////////////////////////////////////////
        //
        // Assemble the individual bytes into one word
        //
        for (index    = 0;
             index   <  wordsize;
             index    = index + 1)
        {
            //////////////////////////////////////////////////////////////////
            //
            // shifts and iORs to place the byte at the appropriate
            // word byte offset
            //
            word      = word | (*(data+index) << (8 * index));
        }
    }

    return (word);
}

void      put_word (uint32_t  word, uint8_t  flag)
{
    //////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize local variables
    //
    int32_t   index   = 0;
    uint32_t  mask    = 0xFF000000;

    //////////////////////////////////////////////////////////////////////////
    //
    // Check `flag` to see if we will display the current offset
    //
    if (flag         == FLAG_DISPLAY)
    {
        fprintf (stdout, "[%.8X] ", IP_REG);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Check `flag` to see if we will display the current byte
    //
    if (flag         == FLAG_DISPLAY)
    {
        for (index    = (wordsize - 1);
             index   >= 0;
             index    = index - 1)
        {
            fprintf (stdout, "%.2hhX ", ((word & mask) >> (8 * index)));
            mask      = mask >> 8;
        }
    }
}

void  decode (uint32_t  instruction, uint32_t  immediate, uint8_t  flags)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    int32_t   value                = 0;
    uint8_t   displayflag          = (flags & FLAG_DISPLAY)   ? TRUE : FALSE;
    uint8_t   immflag              = (flags & FLAG_IMMEDIATE) ? TRUE : FALSE;
    uint8_t   processflag          = (flags & FLAG_PROCESS)   ? TRUE : FALSE;
    uint8_t   opcode               = (instruction & OPCODE_MASK) >> OPCODESHIFT;
    uint32_t  dst                  = (instruction & DSTREG_MASK) >> DSTREGSHIFT;
    uint32_t  src                  = (instruction & SRCREG_MASK) >> SRCREGSHIFT;
    uint8_t   addr                 = (instruction & MOVADR_MASK) >> MOVADRSHIFT;
    uint16_t  port                 = (instruction & IOPORT_MASK);

    ////////////////////////////////////////////////////////////////////////////////
    //
    // Set display FILE pointer to appropriate destination (based on flags)
    //
    display                        = (displayflag == TRUE) ? stdout : devnull;

    ////////////////////////////////////////////////////////////////////////////////
    //
    // Determine opcode and perform needed processing, based on flags
    //
    switch (opcode)
    {
        case HLT:
            if (instruction       == 0x00000000)
            {
                fprintf (display, "%-5s ", "HLT");
            }

            if (processflag       == TRUE)
            {
                haltflag           = TRUE;
            }
            break;

        case WAIT:
            fprintf (display,     "%-5s ", "WAIT");

            if (processflag       == TRUE)
            {
                ////////////////////////////////////////////////////////////////////////
                //
                // waitflag will trigger TIM_CycleCounter to reset
                //
                waitflag           = TRUE;

                ////////////////////////////////////////////////////////////////////////
                //
                // update the Frame Counter (forces reset of Cycle Counter)
                //
                update_frame ();
            }
            break;

        case JMP:
            if (immflag             == TRUE)
            {
                if (processflag     == TRUE)
                {
                    IP_REG           = immediate;
                    rom_offset       = IP_REG;
                }
                sprintf (destination, "0x%.8X", immediate);
            }
            else
            {
                if (processflag     == TRUE)
                {
                    IP_REG           = DSTREG;
                    rom_offset       = IP_REG;
                }
                sprintf (destination, "R%u",    dst);
            }
            fprintf (display,      "%-5s %-16s", "JMP", destination);
            break;

        case CALL:

            ////////////////////////////////////////////////////////////////////////
            //
            // push current value in the Program Counter onto the stack
            //
            if (processflag           == TRUE)
            {
                SP_REG                 = SP_REG - 1;
                memory_set (SP_REG, (IP_REG + 1));
            }

            if (immflag               == TRUE)
            {
                if (processflag       == TRUE)
                {
                    IP_REG             = immediate;
                    rom_offset         = IP_REG;
                }
                sprintf (destination, "0x%.8X", immediate);
            }
            else
            {
                if (processflag       == TRUE)
                {
                    IP_REG             = DSTREG;
                    rom_offset         = IP_REG;
                }
                sprintf (destination, "R%u",    dst);
            }
            fprintf (display,      "%-5s %-16s", "CALL", destination);
            break;

        case RET:

            ////////////////////////////////////////////////////////////////////////
            //
            // pop current value in the stack off back into the Program Counter
            //
            if (processflag           == TRUE)
            {
                IP_REG                 = word2int (memory_get (SP_REG));
                rom_offset             = IP_REG;
                SP_REG                 = SP_REG + 1;
            }
            fprintf (display,     "%-5s ", "RET");
            break;

        case JT:
            sprintf (destination, "R%u,", dst);
            if (immflag               == TRUE)
            {
                if ((processflag      == TRUE)  &&
                    (DSTREG           == TRUE))
                {
                    IP_REG             = immediate;
                    rom_offset         = IP_REG;
                }
                sprintf (source, "0x%.8X", immediate);
            }
            else
            {
                if ((processflag      == TRUE)  &&
                    (DSTREG           == TRUE))
                {
                    IP_REG             = SRCREG;
                    rom_offset         = IP_REG;
                }
                sprintf (source, "R%u",    src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "JT", destination, source);
            break;

        case JF:
            sprintf (destination, "R%u,", dst);
            if (immflag               == TRUE)
            {
                if ((processflag      == TRUE)  &&
                    (DSTREG           == FALSE))
                {
                    IP_REG             = immediate;
                    rom_offset         = IP_REG;
                }
                sprintf (source, "0x%.8X", immediate);
            }
            else
            {
                if ((processflag      == TRUE)  &&
                    (DSTREG           == FALSE))
                {
                    IP_REG             = SRCREG;
                    rom_offset         = IP_REG;
                }
                sprintf (source, "R%u",    src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "JF", destination, source);
            break;

        case IEQ:

            if (immflag               == TRUE)
            {
                if ((processflag      == TRUE)  &&
                    (DSTREG           == immediate))
                {
                    DSTREG            = TRUE;
                }
                else if (processflag  == TRUE)
                {
                    DSTREG             = FALSE;
                }
                sprintf (source, "0x%.8X", immediate);
            }
            else
            {
                if ((processflag      == TRUE)  &&
                    (DSTREG           == SRCREG))
                {
                    (reg+dst)  -> i32  = TRUE;
                }
                else if (processflag  == TRUE)
                {
                    (reg+dst)  -> i32  = FALSE;
                }
                sprintf (source, "R%u",    src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "IEQ", destination, source);
            break;

        case INE:

            if (immflag              == TRUE)
            {
                if (DSTREG           != immediate)
                {
                    DSTREG            = TRUE;
                }
                else
                {
                    DSTREG            = FALSE;
                }
                sprintf (source, "0x%.8X", immediate);
            }
            else
            {
                if (DSTREG           != SRCREG)
                {
                    DSTREG            = TRUE;
                }
                else
                {
                    DSTREG            = FALSE;
                }
                sprintf (source, "R%u",    src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "INE", destination, source);
            break;

        case IGT:

            if (immflag              == TRUE)
            {
                if (DSTREG           >  immediate)
                {
                    DSTREG            = TRUE;
                }
                else
                {
                    DSTREG            = FALSE;
                }
                sprintf (source, "0x%.8X", immediate);
            }
            else
            {
                if (DSTREG           >  SRCREG)
                {
                    DSTREG            = TRUE;
                }
                else
                {
                    DSTREG            = FALSE;
                }
                sprintf (source, "R%u",    src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "IGT", destination, source);
            break;

        case IGE:

            if (immflag              == TRUE)
            {
                if (DSTREG           >= immediate)
                {
                    DSTREG            = TRUE;
                }
                else
                {
                    DSTREG            = FALSE;
                }
                sprintf (source, "0x%.8X", immediate);
            }
            else
            {
                if (DSTREG           >= SRCREG)
                {
                    DSTREG            = TRUE;
                }
                else
                {
                    DSTREG            = FALSE;
                }
                sprintf (source, "R%u",    src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "IGE", destination, source);
            break;

        case ILT:

            if (immflag              == TRUE)
            {
                if (DSTREG           <  immediate)
                {
                    DSTREG            = TRUE;
                }
                else
                {
                    DSTREG            = FALSE;
                }
                sprintf (source, "0x%.8X", immediate);
            }
            else
            {
                if (DSTREG           <  SRCREG)
                {
                    DSTREG            = TRUE;
                }
                else
                {
                    DSTREG            = FALSE;
                }
                sprintf (source, "R%u",    src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "ILT", destination, source);
            break;

        case ILE:

            if (immflag              == TRUE)
            {
                if (DSTREG           <= immediate)
                {
                    DSTREG            = TRUE;
                }
                else
                {
                    DSTREG            = FALSE;
                }
                sprintf (source, "0x%.8X", immediate);
            }
            else
            {
                if (DSTREG           <= SRCREG)
                {
                    DSTREG            = TRUE;
                }
                else
                {
                    DSTREG            = FALSE;
                }
                sprintf (source, "R%u",    src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "ILE", destination, source);
            break;

        case MOV:

            switch (addr)
            {
                case 00: // MOV DSTREG, Immediate
                    if (processflag  == TRUE)
                    {
                        DSTREG            = immediate;
                    }
                    sprintf (destination, "R%u,",          dst);
                    sprintf (source,      "0x%.8X",        immediate);
                    break;

                case 01: // MOV DSTREG, SRCREG
                    if (processflag      == TRUE)
                    {
                        DSTREG            = SRCREG;
                    }
                    sprintf (destination, "R%u,",          dst);
                    sprintf (source,      "R%u",           src);
                    break;

                case 02: // MOV DSTREG, [Immediate]
                    if (processflag      == TRUE)
                    {
                        DSTREG            = word2int (memory_get (immediate));
                    }
                    sprintf (destination, "R%u,",          dst);
                    sprintf (source,      "[0x%.8X]",      immediate);
                    break;

                case 03: // MOV DSTREG, [SRCREG]
                    if (processflag      == TRUE)
                    {
                        value             = DSTREG;
                        DSTREG            = word2int (memory_get (value));
                    }
                    sprintf (destination, "R%u,",          dst);
                    sprintf (source,      "[R%u]",         src);
                    break;

                case 04: // MOV DSTREG, [SRCREG+Immediate]
                    if (processflag  == TRUE)
                    {
                        value             = word2int (memory_get (SRCREG+immediate));
                        DSTREG            = value;
                    }
                    sprintf (destination, "R%u,",          dst);
                    sprintf (source,      "[R%u+0x%.8X]",  src, immediate);
                    break;

                case 05: // MOV [Immediate], SRCREG
                    if (processflag     == TRUE)
                    {
                        memory_set (immediate, SRCREG);
                    }
                    sprintf (destination, "[0x%.8X],",     immediate);
                    sprintf (source,      "R%u",           src);
                    break;

                case 06: // MOV [DSTREG], SRCREG
                    if (processflag     == TRUE)
                    {
                        memory_set (DSTREG, SRCREG);
                    }
                    sprintf (destination, "[R%u],",        dst);
                    sprintf (source,      "R%u",           src);
                    break;

                case 07: // MOV [DSTREG+Immediate], SRCREG
                    if (processflag     == TRUE)
                    {
                        value=DSTREG + immediate;
                        memory_set (value, SRCREG);
                    }
                    sprintf (destination, "[R%u+0x%.8X],", dst, immediate);
                    sprintf (source,      "R%u",           src);
                    break;
            }
            fprintf (display,      "%-5s %-16s %-16s", "MOV", destination, source);
            break;

        case PUSH:

            ////////////////////////////////////////////////////////////////////////
            //
            // push value in register onto the stack
            //
            if (processflag             == TRUE)
            {
                SP_REG                   = SP_REG - 1;
                memory_set (SP_REG, DSTREG);
            }

            sprintf (destination, "R%u",    dst);
            fprintf (display,      "%-5s %-16s", "PUSH", destination);
            break;
            break;

        case POP:

            ////////////////////////////////////////////////////////////////////////
            //
            // pop current value in the stack off into the indicated register
            //
            if (processflag             == TRUE)
            {
                value                    = word2int (memory_get (SP_REG));
                SP_REG                   = SP_REG + 1;
                DSTREG                   = value;
            }
            sprintf (destination, "R%u",    dst);
            fprintf (display,      "%-5s %-16s", "POP", destination);
            break;

        case IN:
            if (processflag             == TRUE)
            {
                DSTREG                   = ioports_get (port);
            }
            sprintf (destination, "R%u,",         dst);
            sprintf (source,      "0x%.3X",       port);
            fprintf (display,      "%-5s %-16s %-16s", "IN", destination, source);
            break;

        case OUT:
            sprintf (destination, "0x%.3X,",      port);
            if (immflag                 == TRUE)
            {
                if (processflag         == TRUE)
                {
                    ioports_set (port, immediate);
                }
                sprintf (source,  "0x%.3X",       immediate);
            }
            else
            {
                if (processflag         == TRUE)
                {
                    ioports_set (port, DSTREG);
                }
                sprintf (source,  "R%u",          dst);
            }
            fprintf (display,      "%-5s %-16s %-16s", "OUT", destination, source);
            break;

        case CIB:
            if ((processflag            == TRUE) &&
                (DSTREG                 != FALSE))
            {
                DSTREG                   = TRUE;
            }
            else if (processflag        == TRUE)
            {
                DSTREG                   = FALSE;
            }
            sprintf (destination, "R%u",    dst);
            fprintf (display,      "%-5s %-16s", "CIB", destination);
            break;

        case NOT:
            if (processflag             == TRUE)
            {
                DSTREG                   = ~(DSTREG);
            }
            sprintf (destination, "R%u",    dst);
            fprintf (display,      "%-5s %-16s", "NOT", destination);
            break;

        case AND:
            sprintf (destination, "R%u",    dst);
            if (immflag                 == TRUE)
            {
                DSTREG                  &= immediate;
                sprintf (source,  "0x%.8X",       immediate);
            }
            else
            {
                DSTREG                  &= src;
                sprintf (source,  "R%u",          src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "AND", destination, source);
            break;

        case OR:
            sprintf (destination, "R%u",    dst);
            if (immflag                 == TRUE)
            {
                DSTREG                  |= immediate;
                sprintf (source,  "0x%.8X",       immediate);
            }
            else
            {
                DSTREG                  |= src;
                sprintf (source,  "R%u",          src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "OR", destination, source);
            break;

        case XOR:
            sprintf (destination, "R%u",    dst);
            if (immflag                 == TRUE)
            {
                DSTREG                  ^= immediate;
                sprintf (source,  "0x%.8X",       immediate);
            }
            else
            {
                DSTREG                  ^= src;
                sprintf (source,  "R%u",          src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "XOR", destination, source);
            break;

        case BNOT:
            if (DSTREG                  == FALSE)
            {
                DSTREG                   = TRUE;
            }
            else
            {
                DSTREG                   = FALSE;
            }
            sprintf (destination, "R%u",    dst);
            fprintf (display,      "%-5s %-16s", "BNOT", destination);
            break;

        case SHL:
            sprintf (destination, "R%u",    dst);
            if (immflag                 == TRUE)
            {
                if (DSTREG              >= 0)
                {
                    DSTREG             <<= immediate;
                }
                else
                {
                    DSTREG             >>= immediate;
                }
                sprintf (source,  "0x%.8X",       immediate);
            }
            else
            {
                if (DSTREG              >= 0)
                {
                    DSTREG             <<= src;
                }
                else
                {
                    DSTREG             >>= src;
                }
                sprintf (source,  "R%u",          src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "SHL", destination, source);
            break;

        case IADD:
            sprintf (destination, "R%u,",         dst);
            if (immflag                 == TRUE)
            {
                if (processflag         == TRUE)
                {
                    DSTREG              += immediate;
                }
                sprintf (source,  "0x%.8X",       immediate);
            }
            else
            {
                if (processflag         == TRUE)
                {
                    DSTREG              += src;
                }
                sprintf (source,  "R%u",          src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "IADD", destination, source);
            break;

        case ISUB:
            sprintf (destination, "R%u,",         dst);
            if (immflag                 == TRUE)
            {
                if (processflag         == TRUE)
                {
                    DSTREG              -= immediate;
                }
                sprintf (source,  "0x%.8X",       immediate);
            }
            else
            {
                if (processflag         == TRUE)
                {
                    DSTREG              -= src;
                }
                sprintf (source,  "R%u",          src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "ISUB", destination, source);
            break;

        case IMUL:
            sprintf (destination, "R%u,",         dst);
            if (immflag              == TRUE)
            {
                DSTREG            *= immediate;
                sprintf (source,  "0x%.8X",       immediate);
            }
            else
            {
                DSTREG            *= src;
                sprintf (source,  "R%u",          src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "IMUL", destination, source);
            break;

        case IDIV:
            sprintf (destination, "R%u,",         dst);
            if (immflag              == TRUE)
            {
                DSTREG            /= immediate;
                sprintf (source,  "0x%.8X",       immediate);
            }
            else
            {
                DSTREG            /= src;
                sprintf (source,  "R%u",          src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "IDIV", destination, source);
            break;

        case IMOD:
            sprintf (destination, "R%u,",         dst);
            if (immflag           == TRUE)
            {
                DSTREG            %= immediate;
                sprintf (source,  "0x%.8X",       immediate);
            }
            else
            {
                DSTREG                = DSTREG % src;
                sprintf (source,  "R%u",          src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "IMOD", destination, source);
            break;
    }

    if (processflag         == TRUE)
    {
        doneflag             = TRUE;
    }
    fprintf (stdout, "\n");
}

void  init_ioports  (void)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    int32_t    index                       = 0;
    int32_t    value                       = 0;
    int8_t    *nptr                        = NULL;
    data_t    *pptr                        = NULL;
    size_t     len                         = 0;
    struct tm *current_time_tm;
    time_t     current_time_raw;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // ioports is the top-level (double pointer) nexus, each category is a single-
    // pointer array hanging off of each element of ioports.
    //
    len                                    = sizeof (data_t *) * NUM_PORT_CATEGORIES;
    ioports                                = (data_t **) malloc (len);
    if (ioports                           == NULL)
    {
        fprintf (stderr, "[error] failed to allocate memory for 'ioports'\n");
        exit (IOPORTS_ALLOC_FAIL);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // TIM ports: allocate and initialize
    //
    len                                    = sizeof (data_t)   * NUM_TIM_PORTS;
    *(ioports+TIM_PORT)                    = (data_t *) malloc (len);
    pptr                                   = *(ioports+TIM_PORT);

    current_time_raw                       = time (NULL); // obtain current time (raw)
    current_time_tm                        = localtime (&current_time_raw);

    for (index                             = 0;
         index                            <  NUM_TIM_PORTS;
         index                             = index + 1)
    {
        (pptr+index) -> value.i32          = 0x00000000;
        (pptr+index) -> flag               = FLAG_READ;
        (pptr+index) -> name               = (int8_t *) malloc (sizeof (int8_t) * 32);
        nptr                               = (pptr+index) -> name;

        switch (TIM_CurrentDate | index)
        {
            case TIM_CurrentDate:
                value                      = current_time_tm -> tm_year + 1900;
                value                      = value << 16;
                value                     |= current_time_tm -> tm_yday + 1;
                (pptr+index) -> value.i32  = value;
                sprintf (nptr, "TIM_CurrentDate");
                break;

            case TIM_CurrentTime:
                value                      = current_time_tm -> tm_hour * 3600;
                value                     += current_time_tm -> tm_min  * 60;
                value                     += current_time_tm -> tm_sec  * 60;
                (pptr+index) -> value.i32  = value;
                sprintf (nptr, "TIM_CurrentTime");
                break;

            case TIM_FrameCounter:
                sprintf (nptr, "TIM_FrameCounter");
                break;

            case TIM_CycleCounter:
                sprintf (nptr, "TIM_CycleCounter");
                break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // RNG ports: allocate and initialize
    //
    len                               = sizeof (data_t)   * NUM_RNG_PORTS;
    *(ioports+RNG_PORT)               = (data_t *) malloc (len);
    pptr                              = *(ioports+RNG_PORT);

    (pptr+0) -> value.i32             = 0x00000000; //rand ();
    (pptr+0) -> flag                  = FLAG_READ | FLAG_WRITE;
    (pptr+0) -> name                  = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                              = (pptr+0) -> name;
    sprintf (nptr, "RNG_CurrentValue");

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // GPU ports: allocate and initialize
    //
    len                               = sizeof (data_t)   * NUM_GPU_PORTS;
    *(ioports+GPU_PORT)               = (data_t *) malloc (len);
    pptr                              = *(ioports+GPU_PORT);

    for (index                        = 0;
         index                       <  NUM_GPU_PORTS;
         index                        = index + 1)
    {
        (pptr+index) -> value.i32     = 0x00000000;
        (pptr+index) -> flag          = FLAG_READ | FLAG_WRITE;
        (pptr+index) -> name          = (int8_t *) malloc (sizeof (int8_t) * 32);
        nptr                          = (pptr+index) -> name;

        switch (GPU_Command | index)
        {
            case GPU_Command:
                (pptr+index) -> flag  = FLAG_WRITE;
                sprintf (nptr, "GPU_Command");
                break;

            case GPU_RemainingPixels:
                (pptr+index) -> flag  = FLAG_READ;
                sprintf (nptr, "GPU_RemainingPixels");
                break;

            case GPU_ClearColor:
                sprintf (nptr, "GPU_ClearColor");
                break;

            case GPU_MultiplyColor:
                sprintf (nptr, "GPU_MultiplyColor");
                break;

            case GPU_ActiveBlending:
                sprintf (nptr, "GPU_ActiveBlending");
                break;

            case GPU_SelectedTexture:
                sprintf (nptr, "GPU_SelectedTexture");
                break;

            case GPU_SelectedRegion:
                sprintf (nptr, "GPU_SelectedRegion");

            case GPU_DrawingPointX:
                sprintf (nptr, "GPU_DrawingPointX");
                break;

            case GPU_DrawingPointY:
                sprintf (nptr, "GPU_DrawingPointY");
                break;

            case GPU_DrawingScaleX:
                sprintf (nptr, "GPU_DrawingScaleX");
                break;

            case GPU_DrawingScaleY:
                sprintf (nptr, "GPU_DrawingScaleY");
                break;

            case GPU_DrawingAngle:
                sprintf (nptr, "GPU_DrawingAngle");
                break;

            case GPU_RegionMinX:
                sprintf (nptr, "GPU_RegionMinX");
                break;

            case GPU_RegionMinY:
                sprintf (nptr, "GPU_RegionMinY");
                break;

            case GPU_RegionMaxX:
                sprintf (nptr, "GPU_RegionMaxX");
                break;

            case GPU_RegionMaxY:
                sprintf (nptr, "GPU_RegionMaxY");
                break;

            case GPU_RegionHotspotX:
                sprintf (nptr, "GPU_RegionHotspotX");
                break;

            case GPU_RegionHotspotY:
                sprintf (nptr, "GPU_RegionHotspotY");
                break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // INP ports: allocate and initialize
    //
    len                               = sizeof (data_t)   * NUM_INP_PORTS;
    *(ioports+INP_PORT)               = (data_t *) malloc (len);
    pptr                              = *(ioports+INP_PORT);

    for (index                        = 0;
         index                       <  NUM_INP_PORTS;
         index                        = index + 1)
    {
        (pptr+index) -> value.i32     = 0x00000000;
        (pptr+index) -> flag          = FLAG_READ;
        (pptr+index) -> name          = (int8_t *) malloc (sizeof (int8_t) * 32);
        nptr                          = (pptr+index) -> name;

        switch (INP_SelectedGamepad | index)
        {
            case INP_SelectedGamepad:
                (pptr+index) -> flag  = FLAG_READ | FLAG_WRITE;
                sprintf (nptr, "INP_SelectedGamepad");
                break;

            case INP_GamepadConnected:
                sprintf (nptr, "INP_GamepadConnected");
                break;

            case INP_GamepadLeft:
                sprintf (nptr, "INP_GamepadLeft");
                break;

            case INP_GamepadRight:
                sprintf (nptr, "INP_GamepadRight");
                break;

            case INP_GamepadUp:
                sprintf (nptr, "INP_GamepadUp");
                break;

            case INP_GamepadDown:
                sprintf (nptr, "INP_GamepadDown");
                break;

            case INP_GamepadButtonStart:
                sprintf (nptr, "INP_GamepadButtonStart");
                break;

            case INP_GamepadButtonA:
                sprintf (nptr, "INP_GamepadButtonA");
                break;

            case INP_GamepadButtonB:
                sprintf (nptr, "INP_GamepadButtonB");
                break;

            case INP_GamepadButtonX:
                sprintf (nptr, "INP_GamepadButtonX");
                break;

            case INP_GamepadButtonY:
                sprintf (nptr, "INP_GamepadButtonY");
                break;

            case INP_GamepadButtonL:
                sprintf (nptr, "INP_GamepadButtonL");
                break;

            case INP_GamepadButtonR:
                sprintf (nptr, "INP_GamepadButtonR");
                break;
        }
    }
}

int32_t  ioports_get  (uint16_t  portaddr)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize local variables
    //
    int32_t   value         = 0x00000000;                // obtained value
    uint16_t  type          = (portaddr & 0x0700) >> 8;  // port category
    uint16_t  attr          = (portaddr & 0x00FF);       // item within category
    uint8_t   flag          = FLAG_NONE;                 // short form access
    data_t   *pptr          = *(ioports+type);           // pointer for sanity
    int32_t  *dptr          = (pptr+attr) -> value.i32;  // pointer to port data

    flag                    = (pptr+attr) -> flag;
    if ((flag & FLAG_READ) != FLAG_READ)
    {
        if (sys_force      == FALSE)
        {
            fprintf (stderr, "[ERROR] port '%s' not accessible via READ!\n",
                             (pptr+attr) -> name);
            exit (IOPORTS_READ_ERROR);
        }
        sys_force           = FALSE;
    }

    switch (portaddr)
    {
        case RNG_CurrentValue: // obtain pseudorandom value, place in port
            *dptr           = rand ();
            value           = (pptr+attr) -> value.i32;
            break;

        default:
            value           = (pptr+attr) -> value.i32;
            break;
    }

    return (value);
}

void      ioports_set  (uint16_t  portaddr, int32_t  value)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize local variables
    //
    uint16_t  type           = (portaddr & 0x0700) >> 8;    // port category
    uint16_t  attr           = (portaddr & 0x00FF);         // item within category
    uint8_t   flag           = FLAG_NONE;                   // short form access
    data_t   *pptr           = *(ioports+type);             // pointer for sanity

    flag                     = (pptr+attr) -> flag;
    if ((flag & FLAG_WRITE) != FLAG_WRITE)
    {
        if (sys_force       == FALSE)
        {
            fprintf (stderr, "[ERROR] port '%s' not accessible via WRITE!\n",
                             (pptr+attr) -> name);
            exit (IOPORTS_WRITE_ERROR);
        }
        sys_force            = FALSE;
    }

    switch (type)
    {
        case TIM_PORT:
            if (attr        >= NUM_TIM_PORTS)
            {
                fprintf (stderr, "[ERROR] invalid TIM port 0x%.3hX\n", portaddr);
                exit (IOPORTS_BAD_PORT);
            }
            break;

        case RNG_PORT:
            if (attr        >= NUM_RNG_PORTS)
            {
                fprintf (stderr, "[ERROR] invalid RNG port 0x%.3hX\n", portaddr);
                exit (IOPORTS_BAD_PORT);
            }
            break;

        case GPU_PORT:
            if (attr        >= NUM_GPU_PORTS)
            {
                fprintf (stderr, "[ERROR] invalid GPU port 0x%.3hX\n", portaddr);
                exit (IOPORTS_BAD_PORT);
            }
            break;

        case SPU_PORT:
            if (attr        >= NUM_SPU_PORTS)
            {
                fprintf (stderr, "[ERROR] invalid SPU port 0x%.3hX\n", portaddr);
                exit (IOPORTS_BAD_PORT);
            }
            break;

        case INP_PORT:
            if (attr        >= NUM_INP_PORTS)
            {
                fprintf (stderr, "[ERROR] invalid INP port 0x%.3hX\n", portaddr);
                exit (IOPORTS_BAD_PORT);
            }
            break;

        case CAR_PORT:
            if (attr        >= NUM_CAR_PORTS)
            {
                fprintf (stderr, "[ERROR] invalid CAR port 0x%.3hX\n", portaddr);
                exit (IOPORTS_BAD_PORT);
            }
            break;

        case MEM_PORT:
            if (attr        >= NUM_MEM_PORTS)
            {
                fprintf (stderr, "[ERROR] invalid MEM port 0x%.3hX\n", portaddr);
                exit (IOPORTS_BAD_PORT);
            }
            break;
    }

    switch (portaddr)
    {
        case RNG_CurrentValue:
            srand (value);
            break;

        default: // catch all- the standard transaction for external setting
            (pptr+attr) -> value.i32  = value;
            break;
    }
}

void    init_memory (void)
{
    int32_t  offset                         = 0;
    int32_t  page                           = 0;
    data_t  *dptr                           = NULL;
    size_t   len                            = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Allocate the memory device
    //
    len                                     = sizeof (mem_t) * NUM_MEMORY_PAGES;
    memory                                  = (mem_t *) malloc (len);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Verify malloc() was successful
    //
    if (memory                             == NULL)
    {
        fprintf (stderr, "[error] failed to allocate for memory resource\n");
        exit (MEMORY_ALLOC_FAIL);
    }

    for (page                               = 0;
         page                              <  NUM_MEMORY_PAGES;
         page                               = page  + 1)
    {
        switch (page)
        {
            case V32_PAGE_RAM:  // Vircon32 RAM is 16MB / 4MW
                (memory+page) -> type       = V32_PAGE_RAM;
                (memory+page) -> firstaddr  = 0x00000000;
                (memory+page) -> last_addr  = 0x003FFFFF;
                (memory+page) -> size       = 1024 * 1024 * wordsize;
                break;

            case V32_PAGE_BIOS:
                (memory+page) -> type       = V32_PAGE_BIOS;
                (memory+page) -> firstaddr  = 0x10000000;
                (memory+page) -> last_addr  = 0x100FFFFF;
                (memory+page) -> size       = get_filesize (BIOS_DEFAULT_PATH);
                break;

            case V32_PAGE_CART:
                (memory+page) -> type       = V32_PAGE_CART;
                (memory+page) -> firstaddr  = 0x20000000;
                (memory+page) -> last_addr  = 0x27FFFFFF;
                (memory+page) -> size       = get_filesize (cartfile);
                break;

            case V32_PAGE_MEMC:
                (memory+page) -> type       = V32_PAGE_MEMC;
                (memory+page) -> firstaddr  = 0x30000000;
                (memory+page) -> last_addr  = 0x3003FFFF;
                (memory+page) -> size       = 1024 * 256;
                break;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // allocate memory for the page
        //
        len                                 = sizeof (data_t) * (memory+page) -> size;
        (memory+page)     -> data           = (data_t *) malloc (len);
        if ((memory+page) -> data          == NULL)
        {
            fprintf (stderr, "[error] failed to allocate for memory page\n");
            exit (MEMORY_ALLOC_FAIL);
        }

        dptr                                = (memory+page) -> data;
        for (offset                         = 0;
             offset                        <  (memory+page) -> size;
             offset                         = offset + 1)
        {
            switch (page)
            {
                case V32_PAGE_RAM:
                case V32_PAGE_MEMC:
                    (dptr+offset) -> flag   = FLAG_READ | FLAG_WRITE;
                    break;

                case V32_PAGE_BIOS:
                case V32_PAGE_CART:
                    (dptr+offset) -> flag   = FLAG_READ;
                    break;
            }
            (dptr+offset) -> value.i32      = 0x00000000;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Mark unused BIOS page words as neither read nor write
        //
        if (page                           == V32_PAGE_BIOS)
        {
            for (offset                     = (memory+page) -> size;
                 offset                    <  (memory+page) -> last_addr - (memory+page) -> firstaddr;
                 offset                     = offset + 1)
            {
                (dptr+offset) -> flag       = FLAG_NONE;
            }
        }            
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// load_memory(): load data files from disk into page-appropriate location in memory
//
void    load_memory (uint32_t  page, int8_t *filename)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    FILE     *fptr    = NULL;
    uint32_t  offset  = 0x00000000;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Adjust offset to be at the start of the indicated page
    //
    offset            = offset | (page << 28);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Open indicated filename for reading
    //
    fptr              = fopen (filename, "rb");
    if (fptr         == NULL)
    {
        fprintf (stderr, "[ERROR] Could not open '%s' for reading\n", filename);
        exit    (FILE_OPEN_ERROR);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Adjust position in file to move beyond header data, based on page
    //
    switch (page)
    {
        case V32_PAGE_BIOS: // we need to skip ahead to word 0x23 (both BIOS and CART)
        case V32_PAGE_CART:
            fseek (fptr, (35 * wordsize), SEEK_CUR);
            break;

        case V32_PAGE_MEMC: // we need to skip ahead to word ?? (check for value on MEMC)
            break;
    }
    
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Continually read in words from the file until we reach EOF, placing each
    // read word in memory at the appropriate offset.
    //
    while (!feof (fptr))
    {
        sys_force     = TRUE;
        memory_set (offset, get_word (fptr));
        if (!feof (fptr))
        {
            offset    = offset + 1;
        }
    }

    fclose (fptr);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// memory_get(): retrieve word_t located at requested memory address
//
// returns a word_t pointer to the requested content
//
word_t *memory_get (uint32_t  address)
{
    data_t   *dptr             = NULL;
    uint32_t  page             = (address & 0xF0000000) >> 28;
    uint32_t  offset           = (address & 0x0FFFFFFF);
    uint8_t   flag             = FLAG_NONE;
    word_t   *wptr             = NULL;

    switch (page)
    {
        case V32_PAGE_RAM:
            if (address       >  RAM_LAST_ADDR)
            {
                fprintf (stderr, "[ERROR] invalid RAM access at 0x%.8X\n", address);
                exit (MEMORY_BAD_ACCESS);
            }

            dptr               = (memory+page)  -> data;
            flag               = ((dptr+offset) -> flag) & FLAG_READ;
            if (flag          != FLAG_READ)
            {
                if (sys_force == FALSE)
                {
                    fprintf (stderr, "[ERROR] RAM address 0x%.8X not readable!\n", address);
                    exit (MEMORY_READ_ERROR);
                }
                sys_force      = FALSE;
            }
            break;

        case V32_PAGE_BIOS:
            if (address       >  BIOS_LAST_ADDR)
            {
                fprintf (stderr, "[ERROR] invalid BIOS access at 0x%.8X\n", address);
                exit (MEMORY_BAD_ACCESS);
            }

            dptr               = (memory+page)  -> data;
            flag               = ((dptr+offset) -> flag) & FLAG_READ;
            if (flag          != FLAG_READ)
            {
                if (sys_force == FALSE)
                {
                    fprintf (stderr, "[ERROR] BIOS address 0x%.8X not readable!\n", address);
                    exit (MEMORY_READ_ERROR);
                }
                sys_force      = FALSE;
            }
            break;

        case V32_PAGE_CART:
            if (address       >  CART_LAST_ADDR)
            {
                fprintf (stderr, "[ERROR] invalid CART access at 0x%.8X\n", address);
                exit (MEMORY_BAD_ACCESS);
            }

            dptr               = (memory+page)  -> data;
            flag               = ((dptr+offset) -> flag) & FLAG_READ;
            if (flag          != FLAG_READ)
            {
                if (sys_force == FALSE)
                {
                    fprintf (stderr, "[ERROR] CART address 0x%.8X not readable!\n", address);
                    exit (MEMORY_READ_ERROR);
                }
                sys_force      = FALSE;
            }
            break;

        case V32_PAGE_MEMC:
            if (address       >  MEMC_LAST_ADDR)
            {
                fprintf (stderr, "[ERROR] invalid MEMC access at 0x%.8X\n", address);
                exit (MEMORY_BAD_ACCESS);
            }

            dptr               = (memory+page)  -> data;
            flag               = ((dptr+offset) -> flag) & FLAG_READ;
            if (flag          != FLAG_READ)
            {
                if (sys_force == FALSE)
                {
                    fprintf (stderr, "[ERROR] MEMC address 0x%.8X not readable!\n", address);
                    exit (MEMORY_READ_ERROR);
                }
                sys_force      = FALSE;
            }
            break;
    }

    dptr                       = (memory+page)  -> data;
    wptr                       = &(dptr+offset) -> value;
    return (wptr);
}

void    memory_set (uint32_t  address, uint32_t  dataword)
{
    data_t   *dptr              = NULL;
    uint32_t  page              = (address & 0xF0000000) >> 28;
    uint32_t  offset            = (address & 0x0FFFFFFF);
    uint8_t   flag              = FLAG_NONE;

    switch (page)
    {
        case V32_PAGE_RAM:
            if (address        >  RAM_LAST_ADDR)
            {
                fprintf (stderr, "[ERROR] invalid RAM access at 0x%.3hX\n", address);
                exit (MEMORY_BAD_ACCESS);
            }

            dptr                = (memory+page)  -> data;
            flag                = ((dptr+offset) -> flag) & FLAG_WRITE;
            if (flag           != FLAG_WRITE)
            {
                if (sys_force  == FALSE)
                {
                    fprintf (stderr, "[ERROR] RAM address 0x%.8X not writable!\n", address);
                    exit (MEMORY_WRITE_ERROR);
                }
                sys_force       = FALSE;
            }
            break;

        case V32_PAGE_BIOS:
            if (address        >  BIOS_LAST_ADDR)
            {
                fprintf (stderr, "[ERROR] invalid BIOS access at 0x%.3hX\n", address);
                exit (MEMORY_BAD_ACCESS);
            }

            dptr                = (memory+page)  -> data;
            flag                = ((dptr+offset) -> flag) & FLAG_WRITE;
            if (flag           != FLAG_WRITE)
            {
                if (sys_force  == FALSE)
                {
                    fprintf (stderr, "[ERROR] BIOS address 0x%.8X not writable!\n", address);
                    exit (MEMORY_WRITE_ERROR);
                }
                sys_force       = FALSE;
            }
            break;

        case V32_PAGE_CART:
            if (address        >  CART_LAST_ADDR)
            {
                fprintf (stderr, "[ERROR] invalid CART access at 0x%.3hX\n", address);
                exit (MEMORY_BAD_ACCESS);
            }

            dptr                = (memory+page)  -> data;
            flag                = ((dptr+offset) -> flag) & FLAG_WRITE;
            if (flag           != FLAG_WRITE)
            {
                if (sys_force  == FALSE)
                {
                    fprintf (stderr, "[ERROR] CART address 0x%.8X not writable!\n", address);
                    exit (MEMORY_WRITE_ERROR);
                }
                sys_force       = FALSE;
            }
            break;

        case V32_PAGE_MEMC:
            if (address        >  MEMC_LAST_ADDR)
            {
                fprintf (stderr, "[ERROR] invalid MEMC access at 0x%.3hX\n", address);
                exit (MEMORY_BAD_ACCESS);
            }

            dptr                = (memory+page)  -> data;
            flag                = ((dptr+offset) -> flag) & FLAG_WRITE;
            if (flag           != FLAG_WRITE)
            {
                if (sys_force  == FALSE)
                {
                    fprintf (stderr, "[ERROR] MEMC address 0x%.8X not writable!\n", address);
                    exit (MEMORY_WRITE_ERROR);
                }
                sys_force       = FALSE;
            }
            break;
    }

    dptr                        = (memory+page)  -> data;
    (dptr+offset) -> value.i32  = dataword;
}

uint32_t  word2int     (word_t *info)
{
    return (info -> i32);
}

float     word2float   (word_t *info)
{
    return (info -> f32);
}

void      update_cycle (void)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    uint32_t  value    = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // If we are not WAITing, increment TIM_CycleCounter
    //
    if (waitflag      == FALSE)
    {
        value          = ioports_get (TIM_CycleCounter);
        value          = value + 1;
    }
    else
    {
        waitflag       = FALSE;  // reset waitflag
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Check for frame roll-over
    //
    if (value         >= 250000)
    {
        update_frame ();
        value          = 0;
    }

    sys_force          = TRUE;
    ioports_set (TIM_CycleCounter, value);
}

void      update_frame (void)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    uint32_t  value    = 0;
    uint32_t  upper    = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // increment frame counter
    //
    value              = ioports_get (TIM_FrameCounter);
    value              = value + 1;
    sys_force          = TRUE; // system override on a read-only port
    ioports_set (TIM_FrameCounter, value);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // adjust TIM_CurrentTime, if enough frames have elapsed
    //
    if ((value % 60)  == 0)
    {
        value          = ioports_get (TIM_CurrentTime);
        value          = value + 1;

        ////////////////////////////////////////////////////////////////////////////////
        //
        // adjust TIM_CurrentDate, if enough seconds have elapsed in TIM_CurrentTime
        //
        if (value     >= 86400)
        {
            value      = ioports_get (TIM_CurrentDate);
            upper      = value & 0xFFFF0000; // isolate year from TIM_CurrentDate
            value      = value & 0x0000FFFF; // isolate day  from TIM_CurrentDate
            value      = value + 1;          // increment the day

            ////////////////////////////////////////////////////////////////////////////
            //
            // larger adjustment of TIM_CurrentDate if year needs incrementing
            //
            if (value >= 365)                // TODO: compensate for leap years
            {
                value  = 0;                  // new year, reset the day to 0
                upper  = upper + 0x00010000; // increment the year
                value  = upper;              // recombine YEAR and DAY
            }
            sys_force  = TRUE;
            ioports_set (TIM_CurrentDate, value);

            value      = 0;                  // TIM_CurrentTime resets to 0
        }

        sys_force      = TRUE;
        ioports_set (TIM_CurrentTime, value);
    }
}

size_t    get_filesize (int8_t *filename)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    int32_t  offset  = 0;
    size_t   size    = 0;
    FILE    *fptr    = fopen (filename, "rb"); // open in (binary) read mode

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // open indicated file for reading (in binary mode, if pertinent)
    //
    if (fptr        == NULL)
    {
        fprintf (stderr, "[ERROR] Unable to open file '%s' for reading\n", filename);
        exit    (FILE_OPEN_ERROR);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // adjust FILE pointer to end of file
    //
    offset           = fseek (fptr, 0, SEEK_END);
    if (offset      == 0)
    {
        ////////////////////////////////////////////////////////////////////////////////
        //
        // Get the current position (file size in bytes)
        //
        size         = ftell (fptr);
        if (size    == -1)
        {
            fprintf (stderr, "[ERROR] Unable to obtain file position\n");
            exit    (FILE_POSITION_ERROR);
        }
    }
    else
    {
        fprintf (stderr, "[ERROR] Unable to seek to end of file\n");
        exit    (FILE_POSITION_ERROR);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Close the file
    //
    fclose (fptr);

    return (size);
}

display_l *newdispnode  (uint8_t  type, word_t *list, uint8_t  num)
{
    display_l *newnode          = (display_l *) malloc (sizeof (display_l) * 1);
    if (newnode                == NULL)
    {
        fprintf (stderr, "[ERROR] Could not allocate memory for displaylist node!\n");
        exit (LIST_ALLOC_FAIL);
    }

    newnode -> type             = type;
    newnode -> num              = num;
    newnode -> list             = list;
    newnode -> next             = NULL;

    return (newnode);
}

display_l *display_add  (display_l *list, display_l *node)
{
    display_l *tmp              = NULL;

    if (node                   != NULL)
    {
        if (list               != NULL)
        {
            tmp                 = list;
            while (tmp -> next != NULL)
            {
                tmp             = tmp -> next;
            }

            tmp -> next         = node;
        }
        else
        {
            list                = node;
        }
    }
    return (list);
}

void       displayshow  (display_l *list, uint8_t    flag)
{
    display_l *dtmp             = NULL;
    int32_t    index            = 0;
    uint32_t   count            = 0;
    uint32_t   value            = 0;
    word_t    *wtmp             = NULL;
    int8_t     entry[16];

    dtmp                        = list;
    while (dtmp                != NULL)
    {
        fprintf (stdout, "[%2u]  ", count);
        wtmp                    = dtmp -> list;
        switch (dtmp -> type)
        {
            case LIST_REG:

                value           = wtmp -> i32;
                switch (value)
                {
                    case CR:
                        fprintf (stdout, "%10s: ", "R11(CR)");
                        break;

                    case SR:
                        fprintf (stdout, "%10s: ", "R12(SR)");
                        break;

                    case DR:
                        fprintf (stdout, "%10s: ", "R13(DR)");
                        break;

                    case BP:
                        fprintf (stdout, "%10s: ", "R14(BP)");
                        break;

                    case SP:
                        fprintf (stdout, "%10s: ", "R15(SP)");
                        break;

                    default:
                        sprintf (entry,  "R%u",    value);
                        fprintf (stdout, "%10s: ", entry);
                        break;
                }
                fprintf (stdout, "0x%.8X\n", (reg+value) -> i32);
                break;

            case LIST_MEM:
                for (index      = 0;
                     index     <  list -> num;
                     index      = index + 1)
                {
                    sys_force  == TRUE;
                    value       = word2int (memory_get ((wtmp+index) -> i32));
                    fprintf (stdout, "0x%.8X: 0x%.8X\n", (wtmp+index) -> i32, value);
                }
                break;

            case LIST_IOP:
                for (index      = 0;
                     index     <  list -> num;
                     index      = index + 1)
                {
                    sys_force  == TRUE;
                    value       = ioports_get ((wtmp+index) -> i32);
                    fprintf (stdout, "[0x%.3X] 0x%.8X\n", (wtmp+index) -> i32, value);
                }
                break;
        }

        count                   = count + 1;
        dtmp                    = dtmp -> next;
    }
}

word_t    *new_word_i32 (uint32_t *value, uint8_t  num)
{
    int32_t  index               = 0;
    word_t  *new_word            = (word_t *) malloc (sizeof (word_t) * num);

    for (index                   = 0;
         index                  <  num;
         index                   = index + 1)
    {
        (new_word+index) -> i32  = *(value+index);
    }

    return (new_word);
}

void       process_args (int32_t  argc, int8_t **argv)
{
    int32_t   opt                  = 0;
    int32_t   option_index         = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // getopt(3) long options and mapping to short options
    //
    struct option long_options[]   = {
       { "biosfile",       required_argument, 0, 'B' },
       { "binary",         no_argument,       0, 'b' },
       { "cartfile",       required_argument, 0, 'C' },
       { "colors",         no_argument,       0, 'c' },
       { "fullstep",       no_argument,       0, 'F' },
       { "stop-at",        required_argument, 0, 's' },
       { "verbose",        no_argument,       0, 'v' },
       { "help",           no_argument,       0, 'h' },
       { 0,                0,                 0,  0  }
    };

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Process command-line arguments, via getopt(3)
    //
    opt                            = getopt_long ((int) argc, (char **) argv,
                                                  "B:bC:cFs:vh", long_options,
                                                  &option_index);
    while (opt                    != -1)
    {
        switch (opt)
        {
            case 'B':
                biosfile           = optarg;
                break;

            case 'b':
                //binaryflag         = 1;
                break;

            case 'C':
                cartfile           = optarg;
                break;

            case 'c':
                //fancyflag          = FANCY_COLORS;
                break;

            case 'F':
                runflag            = TRUE;
                break;

            case 's':
                //start              = strtol (optarg, NULL, 16);
                break;

            case 'h':
                usage ((int8_t *) argv[0]);
                break;
        }
        opt                        = getopt_long ((int) argc, (char **) argv,
                                                  "B:bC:cFs:vh", long_options,
                                                  &option_index);
    }
}

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

void  show_sysregs (void)
{
    int32_t  index  = 0;
    for (index      = IP;
         index     <= IV;
         index      = index + 1)
    {
        switch (index)
        {
            case IP:
                fprintf (stdout, "%16s: ", "IP");
                break;

            case IR:
                fprintf (stdout, "%16s: ", "IR");
                break;

            case IV:
                fprintf (stdout, "%16s: ", "IV");
                break;
        }
        fprintf (stdout, "0x%.8X\n", (reg+index) -> i32);
    }
}
