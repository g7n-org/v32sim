#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ioports.h"

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

#define  NUM_MEMORY_PAGES    4

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
#define  IV                  17
#define  PC                  18

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
};
typedef struct memory_type mem_t;

FILE     *display;
FILE     *devnull;
FILE     *program;
uint8_t  *destination;
uint8_t  *source;
mem_t    *memory;
word_t   *reg;

////////////////////////////////////////////////////////////////////////////////////////
//
// Variables related to IOPorts
//
data_t  **ioports;
int32_t   date_day;
int32_t   date_year;
int32_t   time_day;

uint8_t  *data;
uint8_t   haltflag;
uint8_t   waitflag;
uint8_t   wordsize;
uint32_t  rom_offset;

////////////////////////////////////////////////////////////////////////////////////////
//
// Function prototypes
//
uint32_t  get_word     (FILE *);
void      put_word     (uint32_t, uint8_t);
void      decode       (uint32_t, uint32_t, uint8_t);
void      init_ioports (void);                        // initialize IOPorts
int32_t   ioports_get  (uint16_t);                    // get value from port
void      ioports_set  (uint16_t, int32_t);           // set value to port
void      init_memory  (void);                        // initialize memory
word_t   *memory_get   (uint32_t);                    // get value from memory
void      memory_set   (uint32_t, word_t *);          // set value to memory
int32_t   word2int     (word_t *);
float     word2float   (word_t *);

int32_t   main     (int32_t  argc, uint8_t **argv)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    int32_t    index                   = 0;
    int32_t    value                   = 0;
    int32_t    lastaddr                = 0;
    //data_t    *pptr                    = NULL;
    size_t     len                     = 0;
    struct tm *current_time_tm;
    time_t     current_time_raw;
    uint8_t   *input                   = NULL;
    uint8_t   *arg                     = NULL;
    uint8_t    newcommand              = '\0';
    uint8_t    lastcommand             = '\0';
    uint8_t    decodeflags             = FLAG_NONE;
    uint8_t    runflag                 = FALSE;
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
    rom_offset                         = 0x00000000;
    current_time_raw                   = time (NULL); // obtain current time (raw)
    current_time_tm                    = localtime (&current_time_raw);
    date_day                           = current_time_tm -> tm_yday + 1;
    date_year                          = current_time_tm -> tm_year + 1900;
    time_day                           = current_time_tm -> tm_hour * 3600;
    time_day                          += current_time_tm -> tm_min  * 60;
    time_day                          += current_time_tm -> tm_sec  * 60;
    wordsize                           = 4;
    haltflag                           = FALSE;
    waitflag                           = FALSE;

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

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Unless I use some better input mechanism, input can be up to 256 bytes in 
    // length (should be more than enough)
    //
    len                                = sizeof (uint8_t) * 256;
    input                              = (uint8_t *) malloc (len);
    arg                                = (uint8_t *) malloc (len);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // This is merely the word of data being read in, so wordsize bytes
    //
    len                                = sizeof (uint8_t) * wordsize;
    data                               = (uint8_t *) malloc (len);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Allocate Vircon32 IOPorts (a 2D array of ports)
    //
    init_ioports ();

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Allocate Vircon32 RAM (a 16MB/4MW 1D array of word_t)
    //
    init_memory ();

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Our registers will be in a word_t array
    //
    len                                = sizeof (int32_t) * NUM_REGISTERS;
    reg                                = (word_t *) malloc (len);
    for (index                         = 0;
         index                        <  NUM_REGISTERS;
         index                         = index + 1)
    {
        (reg+index) -> i32             = 0x00000000;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Open the indicated V32 file
    //
    program                            = fopen (argv[1], "r");

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Open /dev/null
    //
    devnull                            = fopen ("/dev/null", "w");

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Verify we have a valid Vircon32 binary file (first two words)
    //
    fread (data, sizeof (uint8_t), wordsize, program);
    index                              = strncmp (data, "V32-", 4);
    if (index                         != 0)
    {
        fprintf (stderr, "[ERROR] Provided file is NOT a Vircon32 file\n");
        exit (1);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Valid V32 files are CARTs and BIOS files
    //
    fread (data, sizeof (uint8_t), wordsize, program);
    index                              = strncmp (data, "CART", 4);
    value                              = strncmp (data, "BIOS", 4);
    if ((index                        != 0) &&
        (value                        != 0))
    {
        fprintf (stderr, "[ERROR] Provided file is NOT a Vircon32 cartridge\n");
        exit (2);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Start our rom_offset at the appropriate location (CART or BIOS file)
    //
    if (index                         == 0) // CART
    {
        ////////////////////////////////////////////////////////////////////////////////
        //
        // 0x20000000 is the CART memory "page"
        //
        // 0x20000000-0x27FFFFFF is the CART memory range
        //
        rom_offset                     = 0x20000000;

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Skip past the V32-CART header to the V32-VBIN section
        //
        fseek (program, 22 * wordsize, SEEK_CUR);
    }

    else if (value                    == 0) // BIOS
    {
        ////////////////////////////////////////////////////////////////////////////////
        //
        // 0x10000000 is the BIOS memory "page"
        //
        // 0x10000000-0x100FFFFF is the BIOS memory range
        //
        rom_offset                     = 0x10000000;

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Skip past the error handling section to the start of BIOS code
        //
        fseek (program, 5 * wordsize, SEEK_CUR);
    }

    vbinoffset                         = get_word (program) / wordsize;
    word                               = get_word (program);
    vtexoffset                         = get_word (program) / wordsize;
    word                               = get_word (program);
    vsndoffset                         = get_word (program) / wordsize;
    
    for (index                         = 0;
         index                        <  6;
         index                         = index + 1)
    {
        word                           = get_word (program);
    }

    fprintf (stdout, "rom_offset: %.8X\n", rom_offset);
    fprintf (stdout, "vbinoffset: %.8X\n", vbinoffset);
    fprintf (stdout, "vtexoffset: %.8X\n", vtexoffset);
    fprintf (stdout, "vsndoffset: %.8X\n", vsndoffset);

    while ((!feof (program)) &&
           (*(input+0)                != EOF))
    {
        word                           = get_word (program);

        immediate                      = word & 0x02000000;
        if (immediate                 == 0x02000000)
        {
            immediate                  = get_word (program);
            decodeflags                = FLAG_IMMEDIATE;
        }
        else
        {
            immediate                  = 0x00000000;
            decodeflags                = FLAG_NONE;
        }

        put_word (word, FLAG_DISPLAY);
        decode   (word, immediate, decodeflags | FLAG_DISPLAY);
        rom_offset                     = rom_offset   + 1;

        if (FLAG_IMMEDIATE            == (decodeflags & FLAG_IMMEDIATE))
        {
            put_word (immediate, FLAG_DISPLAY);
            fprintf (stdout, "\n");
            rom_offset                 = rom_offset   + 1;
        }

        if (runflag                   == FALSE)
        {
            newcommand                 = '\0';
            processflag                = FALSE;
            do
            {
                ////////////////////////////////////////////////////////////////////////
                //
                // Display the prompt (filter out trailing newline)
                //
                fprintf (stdout, "v32sim> ");
                //fscanf  (stdin,  "%s", input);
                /*
                *(input+0)             = fgetc (stdin);
                if (*(input+0)        != '\n')
                {
                    *(input+1)         = fgetc (stdin);
                }*/

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
                newcommand             = *(input+0);
                if (*(input+0)        == '\n')
                {
                    *(input+1)         = '\0';
                    newcommand         = lastcommand;
                }
                *(input+index-1)           = '\0';
                fprintf (stdout, "input >%s<\n", input);
                
                switch (newcommand)
                {
                    case 'c':
                        processflag    = TRUE;
                        runflag        = TRUE;
                        break;

                    case 'm':
                        if (*(input+1) == ' ')
                        {
                            arg         = strtok ((input+2), " ");
                            value       = atoi (arg+1);
                            sprintf (source, "[%.8X]:", value);
                            fprintf (stdout, "%-4s 0x%.8X\n", source, word2int (memory_get (value)));
                            lastaddr    = value;
                        }
                        else
                        {
                            if (lastaddr  <  0x0000004)
                            {
                                lastaddr   = 0x0000004;
                            }

                            for (index     = lastaddr - 4;
                                 index    <  lastaddr + 4;
                                 index     = index + 1)
                            {
                                sprintf (source, "[%.8X]:", index);
                                fprintf (stdout, "%-11s 0x%.8X\n", source, word2int (memory_get (index)));
                            }
                            lastaddr       = index;
                        }
                        break;

                    case 'r':
                        if (*(input+1) == ' ')
                        {
                            arg         = strtok ((input+2), " ");
                            value       = atoi (arg+1);
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
                        processflag    = TRUE;
                        lastcommand    = 's';
                        break;

                    case '?':
                        fprintf (stdout, "  c        - continue normal execution\n");
                        fprintf (stdout, "  m [addr] - display memory address(es)\n");
                        fprintf (stdout, "  r [r#]   - display register(s)\n");
                        fprintf (stdout, "  s        - step to next instruction\n");
                        break;
                }
                lastcommand            = newcommand;
            }
            while (processflag        == FALSE);
        }

        decode (word, immediate, decodeflags | FLAG_PROCESS);

        (reg+PC) -> i32                = rom_offset;  // location
        (reg+IP) -> i32                = word;        // current instruction

        ////////////////////////////////////////////////////////////////////////////////
        //
        // update the Cycle Counter
        //
        value                          = ioports_get (TIM_CycleCounter);
        value                          = value + 1;
        ioports_set (TIM_CycleCounter, value);

        ////////////////////////////////////////////////////////////////////////////////
        //
        // update the Frame Counter
        //
        value                          = ioports_get (TIM_FrameCounter);
        value                          = value + 1;
        ioports_set (TIM_FrameCounter, value);

        (reg+IV) -> i32                = immediate; // immediate value
    }
    
    return (0);
}

//////////////////////////////////////////////////////////////////////////////
//
// get_word(): obtain the next word from the program file,  the  word 
//             is assembled with shifts and iORs from the bytes.
//
// RETURN VAL: the uint32_t of the word just read
//
uint32_t  get_word (FILE *program)
{
    //////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize local variables
    //
    int32_t   index   = 0;
    uint32_t  word    = 0x00000000;  // clear word

    //////////////////////////////////////////////////////////////////////////
    //
    // Read a wordsize amount of data from the program file
    //
    fread (data, sizeof (uint8_t), wordsize, program);

    //////////////////////////////////////////////////////////////////////////
    //
    // If we have not encountered the end of the file, proceed with
    // processing
    //
    if (!feof (program))
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
        fprintf (stdout, "[%.8X] ", rom_offset);
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
                waitflag           = TRUE;

                ////////////////////////////////////////////////////////////////////////
                //
                // reset the Cycle Counter to 0
                //
                value                          = ioports_get (TIM_CycleCounter);
                value                          = value + 1;
                ioports_set (TIM_CycleCounter, 0);

                ////////////////////////////////////////////////////////////////////////
                //
                // update the Frame Counter
                //
                value                          = ioports_get (TIM_FrameCounter);
                value                          = value + 1;
                ioports_set (TIM_FrameCounter, value);
                if ((value % 60)  == 0)
                {
                    time_day       = time_day   + 1;
                }
            }
            break;

        case JMP:
            if (immflag             == TRUE)
            {
                if (processflag     == TRUE)
                {
                    value            = (reg+dst) -> i32;
                    if (value       == TRUE)
                    {
                        (reg+PC) -> i32         = immediate;
                    }
                }
                sprintf (destination, "0x%.8X", immediate);
            }
            else
            {
                if (processflag     == TRUE)
                {
                    value            = (reg+dst) -> i32;
                    if (value       == TRUE)
                    {
                        (reg+PC) -> i32         = (reg+dst) -> i32;
                    }
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
                (reg+SP) -> i32        = (reg+SP) -> i32 - 1;
                value                  = (reg+SP) -> i32;
                memory_set (value, (reg+PC) -> i32);
            }

            if (immflag               == TRUE)
            {
                if (processflag       == TRUE)
                {
                    (reg+PC) -> i32    = immediate;
                    rom_offset         = (reg+PC) -> i32;
                    fseek (program, rom_offset + 0x24, SEEK_SET);
                }
                sprintf (destination, "0x%.8X", immediate);
            }
            else
            {
                if (processflag       == TRUE)
                {
                    (reg+PC) -> i32    = (reg+dst) -> i32;
                    rom_offset         = (reg+PC) -> i32;
                    fseek (program, rom_offset + 0x24, SEEK_SET);
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
                value                  = (reg+SP) -> i32;
                (reg+PC) -> i32        = memory_get (value);
                (reg+SP) -> i32        = (reg+SP) -> i32 + 1;
            }
            fprintf (display,     "%-5s ", "RET");
            break;

        case JT:
            sprintf (destination, "R%u,", dst);
            if (immflag               == TRUE)
            {
                if ((processflag      == TRUE)  &&
                    ((reg+dst) -> i32 == TRUE))
                {
                    (reg+PC) -> i32    = immediate;
                }
                sprintf (source, "0x%.8X", immediate);
            }
            else
            {
                if ((processflag      == TRUE)  &&
                    ((reg+dst) -> i32 == TRUE))
                {
                    (reg+PC) -> i32    = (reg+src) -> i32;
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
                    ((reg+dst) -> i32 == FALSE))
                {
                    (reg+PC) -> i32    = immediate;
                }
                sprintf (source, "0x%.8X", immediate);
            }
            else
            {
                if ((processflag      == TRUE)  &&
                    ((reg+dst) -> i32 == FALSE))
                {
                    (reg+PC) -> i32    = (reg+src) -> i32;
                }
                sprintf (source, "R%u",    src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "JF", destination, source);
            break;

        case IEQ:

            if (immflag               == TRUE)
            {
                if ((processflag      == TRUE)  &&
                    ((reg+dst) -> i32 == immediate))
                {
                    (reg+dst) -> i32  = TRUE;
                }
                else if (processflag  == TRUE)
                {
                    (reg+dst) -> i32   = FALSE;
                }
                sprintf (source, "0x%.8X", immediate);
            }
            else
            {
                if ((processflag      == TRUE)  &&
                    ((reg+dst) -> i32 == (reg+src) -> i32))
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
                if ((reg+dst) -> i32 != immediate)
                {
                    (reg+dst) -> i32  = TRUE;
                }
                else
                {
                    (reg+dst) -> i32  = FALSE;
                }
                sprintf (source, "0x%.8X", immediate);
            }
            else
            {
                if ((reg+dst) -> i32 != (reg+src) -> i32)
                {
                    (reg+dst) -> i32  = TRUE;
                }
                else
                {
                    (reg+dst) -> i32  = FALSE;
                }
                sprintf (source, "R%u",    src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "INE", destination, source);
            break;

        case IGT:

            if (immflag              == TRUE)
            {
                if ((reg+dst) -> i32 >  immediate)
                {
                    (reg+dst) -> i32  = TRUE;
                }
                else
                {
                    (reg+dst) -> i32  = FALSE;
                }
                sprintf (source, "0x%.8X", immediate);
            }
            else
            {
                if ((reg+dst) -> i32 >  (reg+src) -> i32)
                {
                    (reg+dst) -> i32  = TRUE;
                }
                else
                {
                    (reg+dst) -> i32  = FALSE;
                }
                sprintf (source, "R%u",    src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "IGT", destination, source);
            break;

        case IGE:

            if (immflag              == TRUE)
            {
                if ((reg+dst) -> i32 >= immediate)
                {
                    (reg+dst) -> i32  = TRUE;
                }
                else
                {
                    (reg+dst) -> i32  = FALSE;
                }
                sprintf (source, "0x%.8X", immediate);
            }
            else
            {
                if ((reg+dst) -> i32 >= (reg+src) -> i32)
                {
                    (reg+dst) -> i32  = TRUE;
                }
                else
                {
                    (reg+dst) -> i32  = FALSE;
                }
                sprintf (source, "R%u",    src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "IGE", destination, source);
            break;

        case ILT:

            if (immflag              == TRUE)
            {
                if ((reg+dst) -> i32 <  immediate)
                {
                    (reg+dst) -> i32  = TRUE;
                }
                else
                {
                    (reg+dst) -> i32  = FALSE;
                }
                sprintf (source, "0x%.8X", immediate);
            }
            else
            {
                if ((reg+dst) -> i32 <  (reg+src) -> i32)
                {
                    (reg+dst) -> i32  = TRUE;
                }
                else
                {
                    (reg+dst) -> i32  = FALSE;
                }
                sprintf (source, "R%u",    src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "ILT", destination, source);
            break;

        case ILE:

            if (immflag              == TRUE)
            {
                if ((reg+dst) -> i32 <= immediate)
                {
                    (reg+dst) -> i32  = TRUE;
                }
                else
                {
                    (reg+dst) -> i32  = FALSE;
                }
                sprintf (source, "0x%.8X", immediate);
            }
            else
            {
                if ((reg+dst) -> i32 <= (reg+src) -> i32)
                {
                    (reg+dst) -> i32  = TRUE;
                }
                else
                {
                    (reg+dst) -> i32  = FALSE;
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
                        (reg+dst) -> i32  = immediate;
                    }
                    sprintf (destination, "R%u,",          dst);
                    sprintf (source,      "0x%.8X",        immediate);
                    break;

                case 01: // MOV DSTREG, SRCREG
                    if (processflag      == TRUE)
                    {
                        (reg+dst) -> i32  = src;
                    }
                    sprintf (destination, "R%u,",          dst);
                    sprintf (source,      "R%u",           src);
                    break;

                case 02: // MOV DSTREG, [Immediate]
                    if (processflag      == TRUE)
                    {
                        (reg+dst) -> i32  = word2int (memory_get (immediate));
                    }
                    sprintf (destination, "R%u,",          dst);
                    sprintf (source,      "[0x%.8X]",      immediate);
                    break;

                case 03: // MOV DSTREG, [SRCREG]
                    if (processflag      == TRUE)
                    {
                        value             = (reg+dst) -> i32;
                        (reg+dst) -> i32  = word2int (memory_get (value));
                    }
                    sprintf (destination, "R%u,",          dst);
                    sprintf (source,      "[R%u]",         src);
                    break;

                case 04: // MOV DSTREG, [SRCREG+Immediate]
                    if (processflag  == TRUE)
                    {
                        value             = memory_get ((reg+src) -> i32 + immediate);
                        (reg+dst) -> i32  = value;
                    }
                    sprintf (destination, "R%u,",          dst);
                    sprintf (source,      "[R%u+0x%.8X]",  src, immediate);
                    break;

                case 05: // MOV [Immediate], SRCREG
                    if (processflag  == TRUE)
                    {
                        memory_set (immediate, (reg+src) -> i32);
                    }
                    sprintf (destination, "[0x%.8X],",     immediate);
                    sprintf (source,      "R%u",           src);
                    break;

                case 06: // MOV [DSTREG], SRCREG
                    if (processflag == TRUE)
                    {
                        memory_set ((reg+dst) -> i32, (reg+src) -> i32);
                    }
                    sprintf (destination, "[R%u],",        dst);
                    sprintf (source,      "R%u",           src);
                    break;

                case 07: // MOV [DSTREG+Immediate], SRCREG
                    if (processflag     == TRUE)
                    {
                        value=((reg+dst) -> i32) + immediate;
                        memory_set (value, (reg+src));
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
                (reg+SP) -> i32          = (reg+SP) -> i32 - 1;
                value                    = (reg+SP) -> i32;
                memory_set (value, (reg+dst) -> i32);
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
                value                    = (reg+SP) -> i32;
                (reg+dst) -> i32         = word2int (memory_get (value));
                (reg+SP) -> i32          = (reg+SP) -> i32 + 1;
            }
            sprintf (destination, "R%u",    dst);
            fprintf (display,      "%-5s %-16s", "POP", destination);
            break;

        case IN:
            if (processflag             == TRUE)
            {
                (reg+dst) -> i32         = ioports_get (port);
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
                    ioports_set (port, (reg+dst) -> i32);
                }
                sprintf (source,  "R%u",          dst);
            }
            fprintf (display,      "%-5s %-16s %-16s", "OUT", destination, source);
            break;

        case CIB:
            if ((processflag            == TRUE) &&
                ((reg+dst) -> i32       != FALSE))
            {
                (reg+dst) -> i32         = TRUE;
            }
            else if (processflag        == TRUE)
            {
                (reg+dst) -> i32         = FALSE;
            }
            sprintf (destination, "R%u",    dst);
            fprintf (display,      "%-5s %-16s", "CIB", destination);
            break;

        case NOT:
            if (processflag             == TRUE)
            {
                (reg+dst) -> i32         = ~((reg+dst) -> i32);
            }
            sprintf (destination, "R%u",    dst);
            fprintf (display,      "%-5s %-16s", "NOT", destination);
            break;

        case AND:
            sprintf (destination, "R%u",    dst);
            if (immflag                 == TRUE)
            {
                (reg+dst) -> i32        &= immediate;
                sprintf (source,  "0x%.8X",       immediate);
            }
            else
            {
                (reg+dst) -> i32        &= src;
                sprintf (source,  "R%u",          src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "AND", destination, source);
            break;

        case OR:
            sprintf (destination, "R%u",    dst);
            if (immflag                 == TRUE)
            {
                (reg+dst) -> i32        |= immediate;
                sprintf (source,  "0x%.8X",       immediate);
            }
            else
            {
                (reg+dst) -> i32        |= src;
                sprintf (source,  "R%u",          src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "OR", destination, source);
            break;

        case XOR:
            sprintf (destination, "R%u",    dst);
            if (immflag                 == TRUE)
            {
                (reg+dst) -> i32        ^= immediate;
                sprintf (source,  "0x%.8X",       immediate);
            }
            else
            {
                (reg+dst) -> i32        ^= src;
                sprintf (source,  "R%u",          src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "XOR", destination, source);
            break;

        case BNOT:
            if ((reg+dst) -> i32        == FALSE)
            {
                (reg+dst) -> i32         = TRUE;
            }
            else
            {
                (reg+dst) -> i32         = FALSE;
            }
            sprintf (destination, "R%u",    dst);
            fprintf (display,      "%-5s %-16s", "BNOT", destination);
            break;

        case SHL:
            sprintf (destination, "R%u",    dst);
            if (immflag                 == TRUE)
            {
                if ((reg+dst) -> i32    >= 0)
                {
                    (reg+dst) -> i32   <<= immediate;
                }
                else
                {
                    (reg+dst) -> i32   >>= immediate;
                }
                sprintf (source,  "0x%.8X",       immediate);
            }
            else
            {
                if ((reg+dst) -> i32    >= 0)
                {
                    (reg+dst) -> i32   <<= src;
                }
                else
                {
                    (reg+dst) -> i32   >>= src;
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
                    (reg+dst) -> i32    += immediate;
                }
                sprintf (source,  "0x%.8X",       immediate);
            }
            else
            {
                if (processflag         == TRUE)
                {
                    (reg+dst) -> i32    += src;
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
                    (reg+dst) -> i32    -= immediate;
                }
                sprintf (source,  "0x%.8X",       immediate);
            }
            else
            {
                if (processflag         == TRUE)
                {
                    (reg+dst) -> i32    -= src;
                }
                sprintf (source,  "R%u",          src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "ISUB", destination, source);
            break;

        case IMUL:
            sprintf (destination, "R%u,",         dst);
            if (immflag              == TRUE)
            {
                (reg+dst) -> i32  *= immediate;
                sprintf (source,  "0x%.8X",       immediate);
            }
            else
            {
                (reg+dst) -> i32  *= src;
                sprintf (source,  "R%u",          src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "IMUL", destination, source);
            break;

        case IDIV:
            sprintf (destination, "R%u,",         dst);
            if (immflag              == TRUE)
            {
                (reg+dst) -> i32  /= immediate;
                sprintf (source,  "0x%.8X",       immediate);
            }
            else
            {
                (reg+dst) -> i32  /= src;
                sprintf (source,  "R%u",          src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "IDIV", destination, source);
            break;

        case IMOD:
            sprintf (destination, "R%u,",         dst);
            if (immflag           == TRUE)
            {
                (reg+dst) -> i32  %= immediate;
                sprintf (source,  "0x%.8X",       immediate);
            }
            else
            {
                (reg+dst) -> i32      = ((reg+dst) -> i32) % src;
                sprintf (source,  "R%u",          src);
            }
            fprintf (display,      "%-5s %-16s %-16s", "IMOD", destination, source);
            break;
    }

    fprintf (stdout, "\n");
}

void  init_ioports  (void)
{
    int8_t  *nptr                 = NULL;
    //int32_t  index                = 0;
    data_t  *pptr                 = NULL;
    size_t   len                  = 0;

    len                           = sizeof (data_t *) * 8;
    ioports                       = (data_t **) malloc (len);
    if (ioports                  == NULL)
    {
        fprintf (stderr, "[error] failed to allocate memory for 'ioports'\n");
        exit (IOPORTS_ALLOC_FAIL);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // TIM ports: allocate and initialize
    //
    len                           = sizeof (data_t)   * NUM_TIM_PORTS;
    *(ioports+TIM_PORT)           = (data_t *) malloc (len);
    pptr                          = *(ioports+TIM_PORT);

    (pptr+0) -> value.i32         = 0x00000000;
    (pptr+0) -> flag              = FLAG_READ;
    (pptr+0) -> name              = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                          = (pptr+0) -> name;
    sprintf (nptr, "TIM_CurrentDate");

    (pptr+1) -> value.i32         = 0x00000000;
    (pptr+1) -> flag              = FLAG_READ;
    (pptr+1) -> name              = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                          = (pptr+1) -> name;
    sprintf (nptr, "TIM_CurrentTime");

    (pptr+2) -> value.i32         = 0x00000000;
    (pptr+2) -> flag              = FLAG_READ;
    (pptr+2) -> name              = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                          = (pptr+2) -> name;
    sprintf (nptr, "TIM_FrameCounter");

    (pptr+3) -> value.i32         = 0x00000000;
    (pptr+3) -> flag              = FLAG_READ;
    (pptr+3) -> name              = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                          = (pptr+3) -> name;
    sprintf (nptr, "TIM_CycleCounter");

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // RNG ports: allocate and initialize
    //
    len                           = sizeof (data_t)   * NUM_RNG_PORTS;
    *(ioports+RNG_PORT)           = (data_t *) malloc (len);
    pptr                          = *(ioports+RNG_PORT);

    (pptr+0) -> value.i32         = 0x00000000;
    (pptr+0) -> flag              = FLAG_READ | FLAG_WRITE;
    (pptr+0) -> name              = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                          = (pptr+0) -> name;
    sprintf (nptr, "RNG_CurrentValue");

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // GPU ports: allocate and initialize
    //
    len                           = sizeof (data_t)   * NUM_GPU_PORTS;
    *(ioports+GPU_PORT)           = (data_t *) malloc (len);
    pptr                          = *(ioports+GPU_PORT);

    (pptr+0) -> value.i32         = 0x00000000;
    (pptr+0) -> flag              = FLAG_WRITE;
    (pptr+0) -> name              = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                          = (pptr+0) -> name;
    sprintf (nptr, "GPU_Command");

    (pptr+1) -> value.i32         = 0x00000000;
    (pptr+1) -> flag              = FLAG_READ;
    (pptr+1) -> name              = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                          = (pptr+1) -> name;
    sprintf (nptr, "GPU_RemainingPixels");

    (pptr+2) -> value.i32         = 0x00000000;
    (pptr+2) -> flag              = FLAG_READ | FLAG_WRITE;
    (pptr+2) -> name              = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                          = (pptr+2) -> name;
    sprintf (nptr, "GPU_ClearColor");

    (pptr+3) -> value.i32         = 0x00000000;
    (pptr+3) -> flag              = FLAG_READ | FLAG_WRITE;
    (pptr+3) -> name              = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                          = (pptr+3) -> name;
    sprintf (nptr, "GPU_MultiplyColor");

    (pptr+4) -> value.i32         = 0x00000000;
    (pptr+4) -> flag              = FLAG_READ | FLAG_WRITE;
    (pptr+4) -> name              = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                          = (pptr+4) -> name;
    sprintf (nptr, "GPU_ActiveBlending");

    (pptr+5) -> value.i32         = 0x00000000;
    (pptr+5) -> flag              = FLAG_READ | FLAG_WRITE;
    (pptr+5) -> name              = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                          = (pptr+5) -> name;
    sprintf (nptr, "GPU_SelectedTexture");

    (pptr+6) -> value.i32         = 0x00000000;
    (pptr+6) -> flag              = FLAG_READ | FLAG_WRITE;
    (pptr+6) -> name              = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                          = (pptr+6) -> name;
    sprintf (nptr, "GPU_SelectedRegion");

    (pptr+7) -> value.i32         = 0x00000000;
    (pptr+7) -> flag              = FLAG_READ | FLAG_WRITE;
    (pptr+7) -> name              = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                          = (pptr+7) -> name;
    sprintf (nptr, "GPU_DrawingPointX");

    (pptr+8) -> value.i32         = 0x00000000;
    (pptr+8) -> flag              = FLAG_READ | FLAG_WRITE;
    (pptr+8) -> name              = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                          = (pptr+8) -> name;
    sprintf (nptr, "GPU_DrawingPointY");

    (pptr+9) -> value.i32         = 0x00000000;
    (pptr+9) -> flag              = FLAG_READ | FLAG_WRITE;
    (pptr+9) -> name              = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                          = (pptr+9) -> name;
    sprintf (nptr, "GPU_DrawingScaleX");

    (pptr+10) -> value.i32        = 0x00000000;
    (pptr+10) -> flag             = FLAG_READ | FLAG_WRITE;
    (pptr+10) -> name             = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                          = (pptr+10) -> name;
    sprintf (nptr, "GPU_DrawingScaleY");

    (pptr+11) -> value.i32        = 0x00000000;
    (pptr+11) -> flag             = FLAG_READ | FLAG_WRITE;
    (pptr+11) -> name             = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                          = (pptr+11) -> name;
    sprintf (nptr, "GPU_DrawingAngle");

    (pptr+12) -> value.i32        = 0x00000000;
    (pptr+12) -> flag             = FLAG_READ | FLAG_WRITE;
    (pptr+12) -> name             = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                          = (pptr+12) -> name;
    sprintf (nptr, "GPU_RegionMinX");

    (pptr+13) -> value.i32        = 0x00000000;
    (pptr+13) -> flag             = FLAG_READ | FLAG_WRITE;
    (pptr+13) -> name             = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                          = (pptr+13) -> name;
    sprintf (nptr, "GPU_RegionMinY");

    (pptr+14) -> value.i32        = 0x00000000;
    (pptr+14) -> flag             = FLAG_READ | FLAG_WRITE;
    (pptr+14) -> name             = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                          = (pptr+14) -> name;
    sprintf (nptr, "GPU_RegionMaxX");

    (pptr+15) -> value.i32        = 0x00000000;
    (pptr+15) -> flag             = FLAG_READ | FLAG_WRITE;
    (pptr+15) -> name             = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                          = (pptr+15) -> name;
    sprintf (nptr, "GPU_RegionMaxY");

    (pptr+16) -> value.i32        = 0x00000000;
    (pptr+16) -> flag             = FLAG_READ | FLAG_WRITE;
    (pptr+16) -> name             = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                          = (pptr+16) -> name;
    sprintf (nptr, "GPU_RegionHotspotX");

    (pptr+17) -> value.i32        = 0x00000000;
    (pptr+17) -> flag             = FLAG_READ | FLAG_WRITE;
    (pptr+17) -> name             = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                          = (pptr+17) -> name;
    sprintf (nptr, "GPU_RegionHotspotX");

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // INP ports: allocate and initialize
    //
    len                           = sizeof (data_t)   * NUM_INP_PORTS;
    *(ioports+INP_PORT)           = (data_t *) malloc (len);
    pptr                          = *(ioports+INP_PORT);

    (pptr+0) -> value.i32         = 0x00000000;
    (pptr+0) -> flag              = FLAG_READ | FLAG_WRITE;
    (pptr+0) -> name              = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                          = (pptr+0) -> name;
    sprintf (nptr, "INP_SelectedGamepad");
}

int32_t  ioports_get  (uint16_t  portaddr)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize local variables
    //
    int32_t   value         = 0x00000000;                // obtained value
    uint16_t  type          = (portaddr & 0x0700) >> 12; // port category
    uint16_t  attr          = (portaddr & 0x00FF);       // item within category
    uint8_t   flag          = FLAG_NONE;                 // short form access
    data_t   *pptr          = *(ioports+type);           // pointer for sanity
    int32_t  *dptr          = (pptr+attr) -> value.i32;  // pointer to port data

    flag                    = (pptr+attr) -> flag;
    if ((flag & FLAG_READ) != FLAG_READ)
    {
        fprintf (stderr, "[ERROR] port '%s' not accessible via READ!\n",
                         (pptr+attr) -> name);
        exit (IOPORTS_READ_ERROR);
    }

    switch (portaddr)
    {
        case TIM_CurrentDate:
        case TIM_CurrentTime:
        case TIM_FrameCounter:
        case TIM_CycleCounter:
            value           = (pptr+attr) -> value.i32;
            break;

        case RNG_CurrentValue: // obtain pseudorandom value, place in port
            *dptr           = rand ();
            value           = (pptr+attr) -> value.i32;
            break;

        case GPU_RemainingPixels:
        case GPU_ClearColor:
        case GPU_MultiplyColor:
        case GPU_ActiveBlending:
        case GPU_SelectedTexture:
        case GPU_SelectedRegion:
        case GPU_DrawingPointX:
        case GPU_DrawingPointY:
        case GPU_DrawingScaleX:
        case GPU_DrawingScaleY:
        case GPU_DrawingAngle:
        case GPU_RegionMinX:
        case GPU_RegionMinY:
        case GPU_RegionMaxX:
        case GPU_RegionMaxY:
        case GPU_RegionHotspotX:
        case GPU_RegionHotspotY:
            value           = (pptr+attr) -> value.i32;
            break;

        case SPU_GlobalVolume:
            break;

        case SPU_SelectedSound:
            break;

        case SPU_SelectedChannel:
            break;

        case SPU_SoundLength:
            break;

        case SPU_SoundPlayWithLoop:
            break;

        case SPU_SoundLoopStart:
            break;

        case SPU_SoundLoopEnd:
            break;

        case SPU_ChannelState:
            break;

        case SPU_ChannelAssignedSound:
            break;

        case SPU_ChannelVolume:
            break;

        case SPU_ChannelSpeed:
            break;

        case SPU_ChannelLoopEnabled:
            break;

        case SPU_ChannelPosition:
            break;

        case INP_SelectedGamepad:
        case INP_GamepadConnected:
        case INP_GamepadLeft:
        case INP_GamepadRight:
        case INP_GamepadUp:
        case INP_GamepadDown:
        case INP_GamepadButtonStart:
        case INP_GamepadButtonA:
        case INP_GamepadButtonB:
        case INP_GamepadButtonX:
        case INP_GamepadButtonY:
        case INP_GamepadButtonL:
        case INP_GamepadButtonR:
            value           = (pptr+attr) -> value.i32;
            break;

        case CAR_Connected:
            break;

        case CAR_ProgramROMSize:
            break;

        case CAR_NumberOfTextures:
            break;

        case CAR_NumberOfSounds:
            break;

        case MEM_Connected:
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

    //fprintf (stdout, "type: %hu, attr:      %hu\n", type, attr);
    //fprintf (stdout, "dptr: %p,  dptr->i32: %.8X\n", dptr, (pptr+attr) -> value.i32);

    flag                     = (pptr+attr) -> flag;
    if ((flag & FLAG_WRITE) != FLAG_WRITE)
    {
        fprintf (stderr, "[ERROR] port '%s' not accessible via WRITE!\n",
                         (pptr+attr) -> name);
        exit (IOPORTS_WRITE_ERROR);
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

        case GPU_Command:
        case GPU_ClearColor:
        case GPU_MultiplyColor:
        case GPU_ActiveBlending:
        case GPU_SelectedTexture:
        case GPU_SelectedRegion:
        case GPU_DrawingPointX:
        case GPU_DrawingPointY:
            (pptr+attr) -> value.i32  = value;
            break;

        case GPU_DrawingScaleX:
            break;

        case GPU_DrawingScaleY:
            break;

        case GPU_DrawingAngle:
            break;

        case GPU_RegionMinX:
            break;

        case GPU_RegionMinY:
            break;

        case GPU_RegionMaxX:
            break;

        case GPU_RegionMaxY:
            break;

        case GPU_RegionHotspotX:
            break;

        case GPU_RegionHotspotY:
            break;

        case SPU_Command:
            break;

        case SPU_GlobalVolume:
            break;

        case SPU_SelectedSound:
            break;

        case SPU_SelectedChannel:
            break;

        case SPU_SoundLength:
            break;

        case SPU_SoundPlayWithLoop:
            break;

        case SPU_SoundLoopStart:
            break;

        case SPU_SoundLoopEnd:
            break;

        case SPU_ChannelState:
            break;

        case SPU_ChannelAssignedSound:
            break;

        case SPU_ChannelVolume:
            break;

        case SPU_ChannelSpeed:
            break;

        case SPU_ChannelLoopEnabled:
            break;

        case SPU_ChannelPosition:
            break;

        case INP_SelectedGamepad:
            (pptr+attr) -> value.i32  = value;
            break;

        case CAR_Connected:
            break;

        case CAR_ProgramROMSize:
            break;

        case CAR_NumberOfTextures:
            break;

        case CAR_NumberOfSounds:
            break;

        case MEM_Connected:
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
                len                         = sizeof (data_t) * 1024 * 1024 * wordsize;
                break;

            case V32_PAGE_BIOS:
                (memory+page) -> type       = V32_PAGE_BIOS;
                (memory+page) -> firstaddr  = 0x10000000;
                (memory+page) -> last_addr  = 0x100FFFFF;
                len                         = sizeof (data_t) * 1024 * 1024 * 1;
                break;

            case V32_PAGE_CART:
                (memory+page) -> type       = V32_PAGE_CART;
                (memory+page) -> firstaddr  = 0x20000000;
                (memory+page) -> last_addr  = 0x27FFFFFF;
                break;

            case V32_PAGE_MEMC:
                (memory+page) -> type       = V32_PAGE_MEMC;
                (memory+page) -> firstaddr  = 0x30000000;
                (memory+page) -> last_addr  = 0x3003FFFF;
                len                         = sizeof (data_t) * 1024 * 256;
                break;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Vircon32 has 16MB of RAM, or 4MW of memory; declare as an array of word_t
        //
        (memory+page) -> data               = (data_t *) malloc (len);

        if ((memory+page) -> data          == NULL)
        {
            fprintf (stderr, "[error] failed to allocate for memory page\n");
            exit (MEMORY_ALLOC_FAIL);
        }

        for (offset                         = 0;
             offset                        <  len;
             offset                         = offset + 1)
        {
            dptr                            = (memory+page) -> data;
            (dptr+offset) -> value.i32      = 0x00000000;
        }
    }
}

word_t *memory_get (uint32_t  address)
{
    data_t   *dptr       = NULL;
    uint32_t  page       = (address & 0xF0000000) >> 28;
    uint32_t  offset     = (address & 0x0FFFFFFF);
    uint8_t   flag       = FLAG_NONE;
    word_t   *wptr       = NULL;

    switch (page)
    {
        case V32_PAGE_RAM:
            if (address >  RAM_LAST_ADDR)
            {
                fprintf (stderr, "[ERROR] invalid RAM access at 0x%.3hX\n", address);
                exit (MEMORY_BAD_ACCESS);
            }

            dptr         = (memory+page) -> data;
            flag         = ((dptr+offset) -> flag) & FLAG_READ;
            if (flag    != FLAG_READ)
            {
                fprintf (stderr, "[ERROR] RAM address 0x%.8X not readable!\n", address);
                exit (MEMORY_READ_ERROR);
            }
            break;

        case V32_PAGE_BIOS:
            if (address >  BIOS_LAST_ADDR)
            {
                fprintf (stderr, "[ERROR] invalid BIOS access at 0x%.3hX\n", address);
                exit (MEMORY_BAD_ACCESS);
            }

            dptr         = (memory+page) -> data;
            flag         = ((dptr+offset) -> flag) & FLAG_READ;
            if (flag    != FLAG_READ)
            {
                fprintf (stderr, "[ERROR] BIOS address 0x%.8X not readable!\n", address);
                exit (MEMORY_READ_ERROR);
            }
            break;

        case V32_PAGE_CART:
            if (address >  CART_LAST_ADDR)
            {
                fprintf (stderr, "[ERROR] invalid CART access at 0x%.3hX\n", address);
                exit (MEMORY_BAD_ACCESS);
            }

            dptr         = (memory+page) -> data;
            flag         = ((dptr+offset) -> flag) & FLAG_READ;
            if (flag    != FLAG_READ)
            {
                fprintf (stderr, "[ERROR] CART address 0x%.8X not readable!\n", address);
                exit (MEMORY_READ_ERROR);
            }
            break;

        case V32_PAGE_MEMC:
            if (address >  MEMC_LAST_ADDR)
            {
                fprintf (stderr, "[ERROR] invalid MEMC access at 0x%.3hX\n", address);
                exit (MEMORY_BAD_ACCESS);
            }

            dptr         = (memory+page) -> data;
            flag         = ((dptr+offset) -> flag) & FLAG_READ;
            if (flag    != FLAG_READ)
            {
                fprintf (stderr, "[ERROR] MEMC address 0x%.8X not readable!\n", address);
                exit (MEMORY_READ_ERROR);
            }
            break;
    }

    dptr                 = (memory+page) -> data;
    wptr                 = &(dptr+offset) -> value;
    return (wptr);
}

void    memory_set (uint32_t  address, word_t *dataword)
{
    uint32_t  page       = (address & 0xF0000000) >> 28;
    uint32_t  offset     = (address & 0x0FFFFFFF);
    uint8_t   flag       = FLAG_NONE;

}

int32_t   word2int     (word_t *info)
{
    return (info -> i32);
}

float     word2float   (word_t *info)
{
    return (info -> f32);
}
