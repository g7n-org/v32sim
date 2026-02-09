#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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
#define  R12           12
#define  R13           13
#define  R14           14
#define  R15           15
#define  IP            16
#define  IV            17
#define  PC            18

#define  FLAG_NONE     0
#define  FLAG_SHOW     1 

#define  HLT           0x00
#define  JMP           0x02
#define  JT            0x05
#define  JF            0x06
#define  IEQ           0x07
#define  IGT           0x09
#define  MOV           0x13
#define  IN            0x17
#define  OUT           0x18
#define  IADD          0x26
#define  ISUB          0x27
#define  IMUL          0x28
#define  IDIV          0x29
#define  IMOD          0x2A

uint8_t  *data;
uint32_t  offset;
int32_t  *memory;
int32_t  *reg;
uint8_t   wordsize;

uint32_t  get_word (FILE *);
void      put_word (uint32_t, uint8_t);

int32_t   main (int8_t  argc,  uint8_t **argv)
{
    FILE     *program              = NULL;
    int32_t   index                = 0;
    int32_t   value                = 0;
    size_t    len                  = 0;
    uint8_t  *destination          = NULL;
    uint8_t  *source               = NULL;
    uint8_t   immflag              = FALSE;
    uint8_t   opcode               = 0x00;
    uint8_t   dstreg               = 0x00;
    uint8_t   srcreg               = 0x00;
    uint8_t   addr                 = 0x00;
    uint16_t  port                 = 0x0000;
    uint32_t  immediate            = 0x00000000;
    uint32_t  rom_offset           = 0x00000000;
    uint32_t  vbinoffset           = 0x00000000;
    uint32_t  vtexoffset           = 0x00000000;
    uint32_t  vsndoffset           = 0x00000000;
    uint32_t  word                 = 0x00000000;

    wordsize                       = 4;
    len                            = sizeof (uint8_t) * 17;
    destination                    = (uint8_t *) malloc (len);
    source                         = (uint8_t *) malloc (len);
    len                            = sizeof (uint8_t) * wordsize;
    data                           = (uint8_t *) malloc (len);
    len                            = sizeof (int32_t) * 1024 * 1024 * wordsize;
    memory                         = (int32_t *) malloc (len);
    len                            = sizeof (int32_t) * NUM_REGISTERS;
    reg                            = (int32_t *) malloc (len);
    offset                         = 0x20000000;
    program                        = fopen (argv[1], "r");

    fread (data, sizeof (uint8_t), wordsize, program);
    index                          = strncmp (data, "V32-", 4);
    if (index                     != 0)
    {
        fprintf (stderr, "[ERROR] Provided file is NOT a Vircon32 file\n");
        exit (1);
    }

    fread (data, sizeof (uint8_t), wordsize, program);
    index                          = strncmp (data, "CART", 4);
    if (index                     != 0)
    {
        fprintf (stderr, "[ERROR] Provided file is NOT a Vircon32 cartridge\n");
        exit (2);
    }

    fseek (program, 22 * wordsize, SEEK_CUR);

    vbinoffset                     = get_word (program) / wordsize;
    word                           = get_word (program);
    vtexoffset                     = get_word (program) / wordsize;
    word                           = get_word (program);
    vsndoffset                     = get_word (program) / wordsize;
    
    for (index                     = 0;
         index                    <  6;
         index                     = index + 1)
    {
        word                       = get_word (program);
    }

    rom_offset                     = 0x20000000;

    fprintf (stdout, "rom_offset: %.8X\n", rom_offset);
    fprintf (stdout, "vbinoffset: %.8X\n", vbinoffset);
    fprintf (stdout, "vtexoffset: %.8X\n", vtexoffset);
    fprintf (stdout, "vsndoffset: %.8X\n", vsndoffset);

    offset                         = rom_offset;

    while (!feof (program))
    {
        *(reg+PC)                  = offset;
        word                       = get_word (program);
        put_word (word, FLAG_SHOW);
        *(reg+IP)                  = word;
        offset                     = offset + 1;
        rom_offset                 = rom_offset + 1;

        immediate                  = word & 0x02000000;
        if (immediate             == 0x02000000)
        {
            immediate              = get_word (program);
            immflag                = TRUE;
        }
        else
        {
            immediate              = 0x00000000;
            immflag                = FALSE;
        }

        *(reg+IV)                  = immediate;

        //fprintf (stdout, "word: %.8X ", word);

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Obtain instruction parameters from instruction word
        //
        //rom_offset             = rom_offset + 1;
        opcode                 = (word & 0xFC000000) >> 26;
        dstreg                 = (word & 0x01E00000) >> 21;
        srcreg                 = (word & 0x001E0000) >> 17;
        addr                   = (word & 0x0001C000) >> 14;
        port                   = (word & 0x00003FFF);

        switch (opcode)
        {
            case HLT:
                if (word      == 0x00000000)
                {
                    fprintf (stdout, "%-5s ", "HLT");
                }
                break;

            case JMP:
                if (immflag           == TRUE)
                {
                    if (*(reg+dstreg) == TRUE)
                    {
                        *(reg+PC)      = immediate;
                    }
                    sprintf (destination, "0x%.8X", immediate);
                }
                else
                {
                    if (*(reg+dstreg) == TRUE)
                    {
                        *(reg+PC    )  = *(reg+srcreg);
                    }
                    sprintf (destination, "R%u",    srcreg);
                }
                fprintf (stdout,      "%-5s %-16s", "JMP", destination);
                break;

            case JT:
                sprintf (destination, "R%u,", dstreg);
                if (immflag           == TRUE)
                {
                    if (*(reg+dstreg) == TRUE)
                    {
                        *(reg+PC)      = immediate;
                    }
                    sprintf (source, "0x%.8X", immediate);
                }
                else
                {
                    if (*(reg+dstreg) == TRUE)
                    {
                        *(reg+PC    )  = *(reg+srcreg);
                    }
                    sprintf (source, "R%u",    srcreg);
                }
                fprintf (stdout,      "%-5s %-16s %-16s", "JT", destination, source);
                break;

            case JF:
                sprintf (destination, "R%u,", dstreg);
                if (immflag           == TRUE)
                {
                    if (*(reg+dstreg) == FALSE)
                    {
                        *(reg+PC)      = immediate;
                    }
                    sprintf (source, "0x%.8X", immediate);
                }
                else
                {
                    if (*(reg+dstreg) == FALSE)
                    {
                        *(reg+PC    )  = *(reg+srcreg);
                    }
                    sprintf (source, "R%u",    srcreg);
                }
                fprintf (stdout,      "%-5s %-16s %-16s", "JF", destination, source);
                break;

            case IEQ:

                if (immflag           == TRUE)
                {
                    if (*(reg+dstreg) == immediate)
                    {
                        *(reg+dstreg)  = TRUE;
                    }
                    else
                    {
                        *(reg+dstreg)  = FALSE;
                    }
                    sprintf (source, "0x%.8X", immediate);
                }
                else
                {
                    if (*(reg+dstreg) == *(reg+srcreg))
                    {
                        *(reg+dstreg)  = TRUE;
                    }
                    else
                    {
                        *(reg+dstreg)  = FALSE;
                    }
                    sprintf (source, "R%u",    srcreg);
                }
                fprintf (stdout,      "%-5s %-16s %-16s", "IEQ", destination, source);
                break;

            case IGT:

                if (immflag           == TRUE)
                {
                    if (*(reg+dstreg) >  immediate)
                    {
                        *(reg+dstreg)  = TRUE;
                    }
                    else
                    {
                        *(reg+dstreg)  = FALSE;
                    }
                    sprintf (source, "0x%.8X", immediate);
                }
                else
                {
                    if (*(reg+dstreg) >  *(reg+srcreg))
                    {
                        *(reg+dstreg)  = TRUE;
                    }
                    else
                    {
                        *(reg+dstreg)  = FALSE;
                    }
                    sprintf (source, "R%u",    srcreg);
                }
                fprintf (stdout,      "%-5s %-16s %-16s", "IGT", destination, source);
                break;

            case MOV:

                switch (addr)
                {
                    case 00: // MOV DSTREG, Immediate
                        *(reg+dstreg)  = immediate;
                        sprintf (destination, "R%u,",          dstreg);
                        sprintf (source,      "0x%.8X",        immediate);
                        break;

                    case 01: // MOV DSTREG, SRCREG
                        *(reg+dstreg)  = srcreg;
                        sprintf (destination, "R%u,",          dstreg);
                        sprintf (source,      "R%u",           srcreg);
                        break;

                    case 02: // MOV DSTREG, [Immediate]
                        *(reg+dstreg)  = *(memory+immediate);
                        sprintf (destination, "R%u,",          dstreg);
                        sprintf (source,      "[0x%.8X]",      immediate);
                        break;

                    case 03: // MOV DSTREG, [SRCREG]
                        value          = *(reg+dstreg);
                        *(reg+dstreg)  = *(memory+value);
                        sprintf (destination, "R%u,",          dstreg);
                        sprintf (source,      "[R%u]",         srcreg);
                        break;

                    case 04: // MOV DSTREG, [SRCREG+Immediate]
                        *(reg+dstreg)  = *(memory+(*(reg+srcreg)+immediate));
                        sprintf (destination, "R%u,",          dstreg);
                        sprintf (source,      "[R%u+0x%.8X]",  srcreg, immediate);
                        break;

                    case 05: // MOV [Immediate], SRCREG
                        *(memory+immediate)  = *(reg+srcreg);
                        sprintf (destination, "[0x%.8X],",     immediate);
                        sprintf (source,      "R%u",           srcreg);
                        break;

                    case 06: // MOV [DSTREG], SRCREG
                        *(memory+*(reg+dstreg))  = *(reg+srcreg);
                        sprintf (destination, "[R%u],",        dstreg);
                        sprintf (source,      "R%u",           srcreg);
                        break;

                    case 07: // MOV [DSTREG+Immediate], SRCREG
                        *(memory+(*(reg+dstreg)+immediate))  = *(reg+srcreg);
                        sprintf (destination, "[R%u+0x%.8X],", dstreg, immediate);
                        sprintf (source,      "R%u",           srcreg);
                        break;
                }
                fprintf (stdout,      "%-5s %-16s %-16s", "MOV", destination, source);
                break;

            case IN:
                sprintf (destination, "R%u,",         dstreg);
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
                    sprintf (source,  "R%u",          dstreg);
                }
                fprintf (stdout,      "%-5s %-16s %-16s", "OUT", destination, source);
                break;

            case IADD:
                sprintf (destination, "R%u,",         dstreg);
                if (immflag           == TRUE)
                {
                    *(reg+dstreg)      = *(reg+dstreg) + immediate;
                    sprintf (source,  "0x%.8X",       immediate);
                }
                else
                {
                    *(reg+dstreg)      = *(reg+dstreg) + srcreg;
                    sprintf (source,  "R%u",          srcreg);
                }
                fprintf (stdout,      "%-5s %-16s %-16s", "IADD", destination, source);
                break;

            case ISUB:
                sprintf (destination, "R%u,",         dstreg);
                if (immflag           == TRUE)
                {
                    *(reg+dstreg)      = *(reg+dstreg) - immediate;
                    sprintf (source,  "0x%.8X",       immediate);
                }
                else
                {
                    *(reg+dstreg)      = *(reg+dstreg) - srcreg;
                    sprintf (source,  "R%u",          srcreg);
                }
                fprintf (stdout,      "%-5s %-16s %-16s", "ISUB", destination, source);
                break;

            case IMUL:
                sprintf (destination, "R%u,",         dstreg);
                if (immflag           == TRUE)
                {
                    *(reg+dstreg)      = *(reg+dstreg) * immediate;
                    sprintf (source,  "0x%.8X",       immediate);
                }
                else
                {
                    *(reg+dstreg)      = *(reg+dstreg) * srcreg;
                    sprintf (source,  "R%u",          srcreg);
                }
                fprintf (stdout,      "%-5s %-16s %-16s", "IMUL", destination, source);
                break;

            case IDIV:
                sprintf (destination, "R%u,",         dstreg);
                if (immflag           == TRUE)
                {
                    *(reg+dstreg)      = *(reg+dstreg) / immediate;
                    sprintf (source,  "0x%.8X",       immediate);
                }
                else
                {
                    *(reg+dstreg)      = *(reg+dstreg) / srcreg;
                    sprintf (source,  "R%u",          srcreg);
                }
                fprintf (stdout,      "%-5s %-16s %-16s", "IDIV", destination, source);
                break;

            case IMOD:
                sprintf (destination, "R%u,",         dstreg);
                if (immflag           == TRUE)
                {
                    *(reg+dstreg)      = *(reg+dstreg) % immediate;
                    sprintf (source,  "0x%.8X",       immediate);
                }
                else
                {
                    *(reg+dstreg)      = *(reg+dstreg) % srcreg;
                    sprintf (source,  "R%u",          srcreg);
                }
                fprintf (stdout,      "%-5s %-16s %-16s", "IMOD", destination, source);
                break;
        }

        fprintf (stdout, "\n");
        if (immflag           == TRUE)
        {
            put_word (*(reg+IV), FLAG_SHOW);
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
