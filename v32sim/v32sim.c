#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define  OPCODE_MASK     0xFC000000
#define  DSTREG_MASK     0x01E00000
#define  SRCREG_MASK     0x001E0000
#define  MOVADR_MASK     0x0001C000
#define  IOPORT_MASK     0x00003FFF
#define  OPCODESHIFT     26
#define  DSTREGSHIFT     21
#define  SRCREGSHIFT     17
#define  MOVADRSHIFT     14

#define  TRUE            1
#define  FALSE           0

#define  NUM_REGISTERS   19

#define  R0              0
#define  R1              1
#define  R2              2
#define  R3              3
#define  R4              4
#define  R5              5
#define  R6              6
#define  R7              7
#define  R8              8
#define  R9              9
#define  R10             10
#define  R11             11
#define  CR              11
#define  R12             12
#define  SR              12
#define  R13             13
#define  DR              13
#define  R14             14
#define  BP              14
#define  R15             15
#define  SP              15
#define  IP              16
#define  IV              17
#define  PC              18

#define  FLAG_NONE       0
#define  FLAG_DISPLAY    1
#define  FLAG_PROCESS    2 
#define  FLAG_IMMEDIATE  4

#define  HLT             0x00
#define  WAIT            0x01
#define  JMP             0x02
#define  CALL            0x03
#define  RET             0x04
#define  JT              0x05
#define  JF              0x06
#define  IEQ             0x07
#define  INE             0x08
#define  IGT             0x09
#define  IGE             0x0A
#define  ILT             0x0B
#define  ILE             0x0C
#define  MOV             0x13
#define  PUSH            0x15
#define  POP             0x16
#define  IN              0x17
#define  OUT             0x18
#define  CIB             0x1E
#define  NOT             0x20
#define  AND             0x21
#define  OR              0x22
#define  XOR             0x23
#define  BNOT            0x24
#define  SHL             0x25
#define  IADD            0x26
#define  ISUB            0x27
#define  IMUL            0x28
#define  IDIV            0x29
#define  IMOD            0x2A

union data_type
{
    int32_t  i32;
    float    f32;
};

typedef union data_type data_t;

FILE     *display;
FILE     *devnull;
FILE     *program;
uint8_t  *destination;
uint8_t  *source;
int32_t  *memory;
int32_t  *portaddr;
data_t   *reg;
int32_t   cyclecounter;
int32_t   framecounter;
int32_t   date_day;
int32_t   date_year;
int32_t   time_day;
uint8_t  *data;
uint8_t   haltflag;
uint8_t   waitflag;
uint8_t   wordsize;
uint32_t  offset;
uint32_t  rom_offset;

uint32_t  get_word (FILE *);
void      put_word (uint32_t, uint8_t);
void      decode   (uint32_t, uint32_t, uint8_t);

int32_t   main     (int32_t  argc, uint8_t **argv)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    int32_t    index                   = 0;
    int32_t    value                   = 0;
    size_t     len                     = 0;
    struct tm *current_time_tm;
    time_t     current_time_raw;
    uint8_t   *input                   = NULL;
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
    cyclecounter                       = 0;
    framecounter                       = 0;
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

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // This is merely the word of data being read in, so wordsize bytes
    //
    len                                = sizeof (uint8_t) * wordsize;
    data                               = (uint8_t *) malloc (len);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Allocate Vircon32 IOPorts
    //
    len                                = sizeof (int32_t) * 1024 * 8;
    portaddr                           = (int32_t *) malloc (len);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Vircon32 has 16MB of RAM, or 4MW of memory
    //
    len                                = sizeof (int32_t) * 1024 * 1024 * wordsize;
    memory                             = (int32_t *) malloc (len);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Our registers will be in a data_t array
    //
    len                                = sizeof (int32_t) * NUM_REGISTERS;
    reg                                = (data_t *) malloc (len);
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

    offset                             = rom_offset;

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
        offset                         = offset       + 1;
        rom_offset                     = rom_offset   + 1;

        if (FLAG_IMMEDIATE            == (decodeflags & FLAG_IMMEDIATE))
        {
            put_word (immediate, FLAG_DISPLAY);
            fprintf (stdout, "\n");
            offset                     = offset       + 1;
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
                *(input+0)             = fgetc (stdin);
                if (*(input+0)        != '\n')
                {
                    fgetc (stdin);
                }
                
                ////////////////////////////////////////////////////////////////////////
                //
                // newcommand will be the character recently input; if newline,
                // repeat lastcommand
                //
                newcommand             = *(input+0);
                if (*(input+0)        == '\n')
                {
                    newcommand         = lastcommand;
                }
                
                switch (newcommand)
                {
                    case 'c':
                        processflag    = TRUE;
                        runflag        = TRUE;
                        break;

                    case 'r':
                        for (index     = 0;
                             index    <  16;
                             index     = index + 1)
                        {
                            sprintf (source, "R%u:", index);
                            fprintf (stdout, "%-4s 0x%.8X\n", source, (reg+index) -> i32);
                        }
                        break;

                    case 's':
                        processflag    = TRUE;
                        lastcommand    = 's';
                        break;
                }
                lastcommand            = newcommand;
            }
            while (processflag        == FALSE);
        }

        decode (word, immediate, decodeflags | FLAG_PROCESS);

        (reg+PC) -> i32                = offset;    // location
        (reg+IP) -> i32                = word;      // current instruction
        cyclecounter                   = cyclecounter + 1;
        framecounter                   = framecounter + 1;

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
        fprintf (stdout, "[%.8X] ", offset);
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
    uint8_t   port                 = (instruction & IOPORT_MASK);

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
                framecounter       = framecounter + 1;
                value              = framecounter;
                if ((value % 60)  == 0)
                {
                    time_day       = time_day   + 1;
                }
                cyclecounter       = 0;
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
                *(memory+value)        = (reg+PC) -> i32;
            }

            if (immflag               == TRUE)
            {
                if (processflag       == TRUE)
                {
                    (reg+PC) -> i32               = immediate;
                    offset             = (reg+PC) -> i32;
                    fseek (program, offset + 0x24, SEEK_SET);
                }
                sprintf (destination, "0x%.8X", immediate);
            }
            else
            {
                if (processflag       == TRUE)
                {
                    (reg+PC) -> i32               = (reg+dst) -> i32;
                    offset             = (reg+PC) -> i32;
                    fseek (program, offset + 0x24, SEEK_SET);
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
                (reg+PC) -> i32        = *(memory+value);
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
                    (reg+PC) -> i32               = immediate;
                }
                sprintf (source, "0x%.8X", immediate);
            }
            else
            {
                if ((processflag      == TRUE)  &&
                    ((reg+dst) -> i32 == TRUE))
                {
                    (reg+PC) -> i32               = (reg+src) -> i32;
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
                    (reg+PC) -> i32               = immediate;
                }
                sprintf (source, "0x%.8X", immediate);
            }
            else
            {
                if ((processflag      == TRUE)  &&
                    ((reg+dst) -> i32 == FALSE))
                {
                    (reg+PC) -> i32               = (reg+src) -> i32;
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
                    (reg+dst) -> i32  = FALSE;
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

            if (immflag           == TRUE)
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
                    if (processflag  == TRUE)
                    {
                        (reg+dst) -> i32  = src;
                    }
                    sprintf (destination, "R%u,",          dst);
                    sprintf (source,      "R%u",           src);
                    break;

                case 02: // MOV DSTREG, [Immediate]
                    if (processflag  == TRUE)
                    {
                        (reg+dst) -> i32  = *(memory+immediate);
                    }
                    sprintf (destination, "R%u,",          dst);
                    sprintf (source,      "[0x%.8X]",      immediate);
                    break;

                case 03: // MOV DSTREG, [SRCREG]
                    if (processflag  == TRUE)
                    {
                        value         = (reg+dst) -> i32;
                        (reg+dst) -> i32  = *(memory+value);
                    }
                    sprintf (destination, "R%u,",          dst);
                    sprintf (source,      "[R%u]",         src);
                    break;

                case 04: // MOV DSTREG, [SRCREG+Immediate]
                    if (processflag  == TRUE)
                    {
                        (reg+dst) -> i32  = *(memory+((reg+src) -> i32 + immediate));
                    }
                    sprintf (destination, "R%u,",          dst);
                    sprintf (source,      "[R%u+0x%.8X]",  src, immediate);
                    break;

                case 05: // MOV [Immediate], SRCREG
                    if (processflag  == TRUE)
                    {
                        *(memory+immediate)  = (reg+src) -> i32;
                    }
                    sprintf (destination, "[0x%.8X],",     immediate);
                    sprintf (source,      "R%u",           src);
                    break;

                case 06: // MOV [DSTREG], SRCREG
                    if (processflag == TRUE)
                    {
                        *(memory+(reg+dst) -> i32)  = (reg+src) -> i32;
                    }
                    sprintf (destination, "[R%u],",        dst);
                    sprintf (source,      "R%u",           src);
                    break;

                case 07: // MOV [DSTREG+Immediate], SRCREG
                    if (processflag     == TRUE)
                    {
                        value=(((reg+dst) -> i32)+immediate);
                        *(memory+value)  = (reg+src) -> i32;
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
                *(memory+value)          = (reg+dst) -> i32;
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
                (reg+dst) -> i32         = *(memory+value);
                (reg+SP) -> i32          = (reg+SP) -> i32 + 1;
            }
            sprintf (destination, "R%u",    dst);
            fprintf (display,      "%-5s %-16s", "POP", destination);
            break;

        case IN:
            if (processflag             == TRUE)
            {
                (reg+dst) -> i32         = *(portaddr+port);
            }
            sprintf (destination, "R%u,",         dst);
            sprintf (source,      "0x%.3X",       port);
            fprintf (display,      "%-5s %-16s %-16s", "IN", destination, source);
            break;

        case OUT:
            sprintf (destination, "0x%.3X,",      port);
            if (immflag                 == TRUE)
            {
                sprintf (source,  "0x%.3X",       port);
            }
            else
            {
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
