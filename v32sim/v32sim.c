#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define  TRUE          1
#define  FALSE         0

#define  NUM_REGISTERS 19

#define  R0            0
#define  R1            1
#define  R2            2
#define  R3            3
#define  R4            4
#define  R5            5
#define  R6            6
#define  R7            7
#define  R8            8
#define  R9            9
#define  R10           10
#define  R11           11
#define  CR            11
#define  R12           12
#define  SR            12
#define  R13           13
#define  DR            13
#define  R14           14
#define  BP            14
#define  R15           15
#define  SP            15
#define  IP            16
#define  IV            17
#define  PC            18

#define  FLAG_NONE     0
#define  FLAG_SHOW     1 

#define  HLT           0x00
#define  WAIT          0x01
#define  JMP           0x02
#define  CALL          0x03
#define  RET           0x04
#define  JT            0x05
#define  JF            0x06
#define  IEQ           0x07
#define  INE           0x08
#define  IGT           0x09
#define  IGE           0x0A
#define  ILT           0x0B
#define  ILE           0x0C
#define  MOV           0x13
#define  PUSH          0x15
#define  POP           0x16
#define  IN            0x17
#define  OUT           0x18
#define  CIB           0x1E
#define  NOT           0x20
#define  AND           0x21
#define  OR            0x22
#define  XOR           0x23
#define  BNOT          0x24
#define  SHL           0x25
#define  IADD          0x26
#define  ISUB          0x27
#define  IMUL          0x28
#define  IDIV          0x29
#define  IMOD          0x2A

union data_type
{
    int32_t  i32;
    float    f32;
};
typedef union data_type data_t;

int32_t  *memory;
data_t    *reg;
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

uint32_t  get_word (FILE *);
void      put_word (uint32_t, uint8_t);

int32_t   main (int32_t  argc,  uint8_t **argv)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    FILE      *program                 = NULL;
    int32_t    index                   = 0;
    int32_t    value                   = 0;
    size_t     len                     = 0;
    struct tm *current_time_tm;
    time_t     current_time_raw;
    uint8_t   *destination             = NULL;
    uint8_t   *source                  = NULL;
    uint8_t   *input                   = NULL;
    uint8_t    immflag                 = FALSE;
    uint8_t    runflag                 = FALSE;
    uint8_t    opcode                  = 0x00;
    uint8_t    dst                     = 0x00;
    uint8_t    src                     = 0x00;
    uint8_t    addr                    = 0x00;
    uint16_t   port                    = 0x0000;
    uint32_t   immediate               = 0x00000000;
    uint32_t   rom_offset              = 0x00000000;
    uint32_t   vbinoffset              = 0x00000000;
    uint32_t   vtexoffset              = 0x00000000;
    uint32_t   vsndoffset              = 0x00000000;
    uint32_t   word                    = 0x00000000;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // initialize variables
    //
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
    len                                = sizeof (uint8_t) * 18;
    destination                        = (uint8_t *) malloc (len);
    source                             = (uint8_t *) malloc (len);
    len                                = sizeof (uint8_t) * 256;
    input                              = (uint8_t *) malloc (len);
    len                                = sizeof (uint8_t) * wordsize;
    data                               = (uint8_t *) malloc (len);
    len                                = sizeof (int32_t) * 1024 * 1024 * wordsize;
    memory                             = (int32_t *) malloc (len);
    len                                = sizeof (int32_t) * NUM_REGISTERS;
    reg                                = (data_t *) malloc (len);
    offset                             = 0x20000000;
    program                            = fopen (argv[1], "r");
    haltflag                           = FALSE;
    waitflag                           = FALSE;

    fread (data, sizeof (uint8_t), wordsize, program);
    index                              = strncmp (data, "V32-", 4);
    if (index                         != 0)
    {
        fprintf (stderr, "[ERROR] Provided file is NOT a Vircon32 file\n");
        exit (1);
    }

    fread (data, sizeof (uint8_t), wordsize, program);
    index                              = strncmp (data, "CART", 4);
    value                              = strncmp (data, "BIOS", 4);
    if ((index                        != 0) &&
        (value                        != 0))
    {
        fprintf (stderr, "[ERROR] Provided file is NOT a Vircon32 cartridge\n");
        exit (2);
    }

    if (index                         == 0) // CART
    {
        rom_offset                     = 0x20000000;
    }
    else if (value                    == 0) // BIOS
    {
        rom_offset                     = 0x10000000;
    }

    fseek (program, 22 * wordsize, SEEK_CUR);

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

    while (!feof (program))
    {
        if (runflag                   == FALSE)
        {
            do
            {
                fprintf (stdout, "v32sim> ");
                fscanf  (stdin,  "%s", input);
                if (strncmp (input, "r", 1) == 0)
                {
                    for (index = 0; index < 16; index++)
                    {
                        sprintf (source, "R%u:", index);
                        fprintf (stdout, "%-4s 0x%.8X\n", source, (reg+index) -> i32);
                    }
                }
                else if (strncmp (input, "c", 1) == 0)
                {
                    runflag            = TRUE;
                }
            }
            while (strncmp (input, "s", 1) != 0);
        }

        word                           = get_word (program);
        put_word (word, FLAG_SHOW);

        (reg+PC) -> i32                = offset; // location
        (reg+IP) -> i32                = word; // current instruction
        offset                         = offset       + 1;
        rom_offset                     = rom_offset   + 1;
        cyclecounter                   = cyclecounter + 1;
        framecounter                   = framecounter + 1;

        immediate                      = word & 0x02000000;
        if (immediate                 == 0x02000000)
        {
            immediate                  = get_word (program);
            immflag                    = TRUE;
        }
        else
        {
            immediate                  = 0x00000000;
            immflag                    = FALSE;
        }

        (reg+IV) -> i32                = immediate; // immediate value

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Obtain instruction parameters from instruction word
        //
        opcode                         = (word & 0xFC000000) >> 26;
        dst                            = (word & 0x01E00000) >> 21;
        src                            = (word & 0x001E0000) >> 17;
        addr                           = (word & 0x0001C000) >> 14;
        port                           = (word & 0x00003FFF);

        switch (opcode)
        {
            case HLT:
                if (word              == 0x00000000)
                {
                    fprintf (stdout, "%-5s ", "HLT");
                }
                haltflag               = TRUE;
                break;

            case WAIT:
                fprintf (stdout,     "%-5s ", "WAIT");
                waitflag               = TRUE;
                framecounter           = framecounter + 1;
                value                  = framecounter;
                if ((value % 60)      == 0)
                {
                    time_day           = time_day   + 1;
                }
                cyclecounter           = 0;
                break;

            case JMP:
                if (immflag             == TRUE)
                {
                    value                = (reg+dst) -> i32;
                    if (value           == TRUE)
                    {
                        (reg+PC) -> i32  = immediate;
                    }
                    sprintf (destination, "0x%.8X", immediate);
                }
                else
                {
                    value                = (reg+dst) -> i32;
                    if (value           == TRUE)
                    {
                        (reg+PC) -> i32  = (reg+dst) -> i32;
                    }
                    sprintf (destination, "R%u",    dst);
                }
                fprintf (stdout,      "%-5s %-16s", "JMP", destination);
                break;

            case CALL:

                ////////////////////////////////////////////////////////////////////////
                //
                // push current value in the Program Counter onto the stack
                //
                (reg+SP) -> i32        = (reg+SP) -> i32 - 1;
                value                  = (reg+SP) -> i32;
                *(memory+value)        = (reg+PC) -> i32;

                if (immflag           == TRUE)
                {
                    (reg+PC) -> i32    = immediate;
                    sprintf (destination, "0x%.8X", immediate);
                }
                else
                {
                    (reg+PC) -> i32    = (reg+dst) -> i32;
                    sprintf (destination, "R%u",    dst);
                }
                fprintf (stdout,      "%-5s %-16s", "CALL", destination);
                break;

            case RET:

                ////////////////////////////////////////////////////////////////////////
                //
                // pop current value in the stack off back into the Program Counter
                //
                value                  = (reg+SP) -> i32;
                (reg+PC) -> i32        = *(memory+value);
                (reg+SP) -> i32        = (reg+SP) -> i32 + 1;
                fprintf (stdout,     "%-5s ", "RET");
                break;

            case JT:
                sprintf (destination, "R%u,", dst);
                if (immflag           == TRUE)
                {
                    if ((reg+dst) -> i32 == TRUE)
                    {
                        (reg+PC) -> i32   = immediate;
                    }
                    sprintf (source, "0x%.8X", immediate);
                }
                else
                {
                    if ((reg+dst) -> i32 == TRUE)
                    {
                        (reg+PC) -> i32  = (reg+src) -> i32;
                    }
                    sprintf (source, "R%u",    src);
                }
                fprintf (stdout,      "%-5s %-16s %-16s", "JT", destination, source);
                break;

            case JF:
                sprintf (destination, "R%u,", dst);
                if (immflag           == TRUE)
                {
                    if ((reg+dst) -> i32 == FALSE)
                    {
                        (reg+PC) -> i32   = immediate;
                    }
                    sprintf (source, "0x%.8X", immediate);
                }
                else
                {
                    if ((reg+dst) -> i32 == FALSE)
                    {
                        (reg+PC)  -> i32  = (reg+src) -> i32;
                    }
                    sprintf (source, "R%u",    src);
                }
                fprintf (stdout,      "%-5s %-16s %-16s", "JF", destination, source);
                break;

            case IEQ:

                if (immflag           == TRUE)
                {
                    if ((reg+dst) -> i32 == immediate)
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
                    if ((reg+dst) -> i32 == (reg+src) -> i32)
                    {
                        (reg+dst) -> i32  = TRUE;
                    }
                    else
                    {
                        (reg+dst) -> i32  = FALSE;
                    }
                    sprintf (source, "R%u",    src);
                }
                fprintf (stdout,      "%-5s %-16s %-16s", "IEQ", destination, source);
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
                fprintf (stdout,      "%-5s %-16s %-16s", "INE", destination, source);
                break;

            case IGT:

                if (immflag           == TRUE)
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
                fprintf (stdout,      "%-5s %-16s %-16s", "IGT", destination, source);
                break;

            case IGE:

                if (immflag           == TRUE)
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
                fprintf (stdout,      "%-5s %-16s %-16s", "IGE", destination, source);
                break;

            case ILT:

                if (immflag           == TRUE)
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
                fprintf (stdout,      "%-5s %-16s %-16s", "ILT", destination, source);
                break;

            case ILE:

                if (immflag           == TRUE)
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
                fprintf (stdout,      "%-5s %-16s %-16s", "ILE", destination, source);
                break;

            case MOV:

                switch (addr)
                {
                    case 00: // MOV DSTREG, Immediate
                        (reg+dst) -> i32  = immediate;
                        sprintf (destination, "R%u,",          dst);
                        sprintf (source,      "0x%.8X",        immediate);
                        break;

                    case 01: // MOV DSTREG, SRCREG
                        (reg+dst) -> i32  = src;
                        sprintf (destination, "R%u,",          dst);
                        sprintf (source,      "R%u",           src);
                        break;

                    case 02: // MOV DSTREG, [Immediate]
                        (reg+dst) -> i32  = *(memory+immediate);
                        sprintf (destination, "R%u,",          dst);
                        sprintf (source,      "[0x%.8X]",      immediate);
                        break;

                    case 03: // MOV DSTREG, [SRCREG]
                        value          = (reg+dst) -> i32;
                        (reg+dst) -> i32  = *(memory+value);
                        sprintf (destination, "R%u,",          dst);
                        sprintf (source,      "[R%u]",         src);
                        break;

                    case 04: // MOV DSTREG, [SRCREG+Immediate]
                        (reg+dst) -> i32  = *(memory+((reg+src) -> i32 + immediate));
                        sprintf (destination, "R%u,",          dst);
                        sprintf (source,      "[R%u+0x%.8X]",  src, immediate);
                        break;

                    case 05: // MOV [Immediate], SRCREG
                        *(memory+immediate)  = (reg+src) -> i32;
                        sprintf (destination, "[0x%.8X],",     immediate);
                        sprintf (source,      "R%u",           src);
                        break;

                    case 06: // MOV [DSTREG], SRCREG
                        *(memory+(reg+dst) -> i32)  = (reg+src) -> i32;
                        sprintf (destination, "[R%u],",        dst);
                        sprintf (source,      "R%u",           src);
                        break;

                    case 07: // MOV [DSTREG+Immediate], SRCREG
                        *(memory+((reg+dst) -> i32+immediate))  = (reg+src) -> i32;
                        sprintf (destination, "[R%u+0x%.8X],", dst, immediate);
                        sprintf (source,      "R%u",           src);
                        break;
                }
                fprintf (stdout,      "%-5s %-16s %-16s", "MOV", destination, source);
                break;

            case PUSH:

                ////////////////////////////////////////////////////////////////////////
                //
                // push value in register onto the stack
                //
                (reg+SP) -> i32           = (reg+SP) -> i32 - 1;
                value                     = (reg+SP) -> i32;
                *(memory+value)           = (reg+dst) -> i32;

                sprintf (destination, "R%u",    dst);
                fprintf (stdout,      "%-5s %-16s", "PUSH", destination);
                break;
                break;

            case POP:

                ////////////////////////////////////////////////////////////////////////
                //
                // pop current value in the stack off into the indicated register
                //
                value                  = (reg+SP) -> i32;
                (reg+dst) -> i32          = *(memory+value);
                (reg+SP) -> i32           = (reg+SP) -> i32 + 1;
                sprintf (destination, "R%u",    dst);
                fprintf (stdout,      "%-5s %-16s", "POP", destination);
                break;

            case IN:
                sprintf (destination, "R%u,",         dst);
                sprintf (source,      "0x%.3X",       port);
                fprintf (stdout,      "%-5s %-16s %-16s", "IN", destination, source);
                break;

            case OUT:
                sprintf (destination, "0x%.3X,",      port);
                if (immflag   == TRUE)
                {
                    sprintf (source,  "0x%.3X",       port);
                }
                else
                {
                    sprintf (source,  "R%u",          dst);
                }
                fprintf (stdout,      "%-5s %-16s %-16s", "OUT", destination, source);
                break;

            case CIB:
                if ((reg+dst) -> i32     != FALSE)
                {
                    (reg+dst) -> i32      = TRUE;
                }
                else
                {
                    (reg+dst) -> i32      = FALSE;
                }
                sprintf (destination, "R%u",    dst);
                fprintf (stdout,      "%-5s %-16s", "CIB", destination);
                break;

            case NOT:
                (reg+dst) -> i32          = ~((reg+dst) -> i32);
                sprintf (destination, "R%u",    dst);
                fprintf (stdout,      "%-5s %-16s", "NOT", destination);
                break;

            case AND:
                sprintf (destination, "R%u",    dst);
                if (immflag           == TRUE)
                {
                    (reg+dst) -> i32      &= immediate;
                    sprintf (source,  "0x%.8X",       immediate);
                }
                else
                {
                    (reg+dst) -> i32      &= src;
                    sprintf (source,  "R%u",          src);
                }
                fprintf (stdout,      "%-5s %-16s %-16s", "AND", destination, source);
                break;

            case OR:
                sprintf (destination, "R%u",    dst);
                if (immflag           == TRUE)
                {
                    (reg+dst) -> i32      |= immediate;
                    sprintf (source,  "0x%.8X",       immediate);
                }
                else
                {
                    (reg+dst) -> i32      |= src;
                    sprintf (source,  "R%u",          src);
                }
                fprintf (stdout,      "%-5s %-16s %-16s", "OR", destination, source);
                break;

            case XOR:
                sprintf (destination, "R%u",    dst);
                if (immflag           == TRUE)
                {
                    (reg+dst) -> i32      ^= immediate;
                    sprintf (source,  "0x%.8X",       immediate);
                }
                else
                {
                    (reg+dst) -> i32      ^= src;
                    sprintf (source,  "R%u",          src);
                }
                fprintf (stdout,      "%-5s %-16s %-16s", "XOR", destination, source);
                break;

            case BNOT:
                if ((reg+dst) -> i32      == FALSE)
                {
                    (reg+dst) -> i32       = TRUE;
                }
                else
                {
                    (reg+dst) -> i32       = FALSE;
                }
                sprintf (destination, "R%u",    dst);
                fprintf (stdout,      "%-5s %-16s", "BNOT", destination);
                break;

            case SHL:
                sprintf (destination, "R%u",    dst);
                if (immflag           == TRUE)
                {
                    if ((reg+dst) -> i32 >= 0)
                    {
                        (reg+dst) -> i32 <<= immediate;
                    }
                    else
                    {
                        (reg+dst) -> i32 >>= immediate;
                    }
                    sprintf (source,  "0x%.8X",       immediate);
                }
                else
                {
                    if ((reg+dst) -> i32 >= 0)
                    {
                        (reg+dst) -> i32    <<= src;
                    }
                    else
                    {
                        (reg+dst) -> i32    >>= src;
                    }
                    sprintf (source,  "R%u",          src);
                }
                fprintf (stdout,      "%-5s %-16s %-16s", "SHL", destination, source);
                break;

            case IADD:
                sprintf (destination, "R%u,",         dst);
                if (immflag              == TRUE)
                {
                    (reg+dst) -> i32  += immediate;
                    sprintf (source,  "0x%.8X",       immediate);
                }
                else
                {
                    (reg+dst) -> i32  += src;
                    sprintf (source,  "R%u",          src);
                }
                fprintf (stdout,      "%-5s %-16s %-16s", "IADD", destination, source);
                break;

            case ISUB:
                sprintf (destination, "R%u,",         dst);
                if (immflag              == TRUE)
                {
                    (reg+dst) -> i32  -= immediate;
                    sprintf (source,  "0x%.8X",       immediate);
                }
                else
                {
                    (reg+dst) -> i32  -= src;
                    sprintf (source,  "R%u",          src);
                }
                fprintf (stdout,      "%-5s %-16s %-16s", "ISUB", destination, source);
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
                fprintf (stdout,      "%-5s %-16s %-16s", "IMUL", destination, source);
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
                fprintf (stdout,      "%-5s %-16s %-16s", "IDIV", destination, source);
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
                fprintf (stdout,      "%-5s %-16s %-16s", "IMOD", destination, source);
                break;
        }

        fprintf (stdout, "\n");
        if (immflag           == TRUE)
        {
            put_word ((reg+IV) -> i32, FLAG_SHOW);
            offset             = offset + 1;
            rom_offset         = rom_offset + 1;
            fprintf (stdout, "\n");
        }
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
    if (flag         == FLAG_SHOW)
    {
        fprintf (stdout, "[%.8X] ", offset);
    }

    //////////////////////////////////////////////////////////////////////////
    //
    // Check `flag` to see if we will display the current byte
    //
    if (flag         == FLAG_SHOW)
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
