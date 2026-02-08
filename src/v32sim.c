#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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
#define  IGT           0x09
#define  MOV           0x13
#define  IN            0x17
#define  OUT           0x18

uint8_t  *data;
uint32_t  offset;
int32_t  *memory;
int32_t  *reg;
uint8_t   wordsize;

uint32_t  get_word (FILE *);
void      showword (uint32_t, uint8_t);

int32_t   main (int8_t  argc,  uint8_t **argv)
{
    FILE     *program              = NULL;
    int32_t   index                = 0;
    int32_t   value                = 0;
    size_t    len                  = 0;
    uint8_t   immflag              = 0;
    uint8_t   param                = 0;
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
    offset                         = offset + 23;

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
        showword (word, FLAG_SHOW);
        *(reg+IP)                  = word;
        offset                     = offset + 1;

        immediate                  = word & 0x02000000;
        if (immediate             == 0x02000000)
        {
            immediate              = get_word (program);
            immflag                = 1;
        }
        else
        {
            immediate              = 0x00000000;
            immflag                = 0;
        }

        *(reg+IV)                  = immediate;

        //fprintf (stdout, "word: %.8X ", word);

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Obtain instruction parameters from instruction word
        //
        rom_offset             = rom_offset + 1;
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
                param          = 0;
                break;

            case IGT:

                ////////////////////////////////////////////////////////////////////
                //
                // Identify instruction
                //
                fprintf (stdout, "%-5s R%u, ", "IGT", dstreg);
                if (immflag           == 1)
                {
                    if (*(reg+dstreg) >  immediate)
                    {
                        *(reg+dstreg)  = 1;
                    }
                    else
                    {
                        *(reg+dstreg)  = 0;
                    }
                    fprintf (stdout, "0x%.8X", immediate);
                }
                else
                {
                    if (*(reg+dstreg) >  *(reg+srcreg))
                    {
                        *(reg+dstreg)  = 1;
                    }
                    else
                    {
                        *(reg+dstreg)  = 0;
                    }
                    fprintf (stdout, "R%u",    srcreg);
                }
                break;

            case MOV:

                ////////////////////////////////////////////////////////////////////
                //
                // Identify instruction
                //
                fprintf (stdout, "%-5s ", "MOV");

                switch (addr)
                {
                    case 00: // MOV DSTREG, Immediate
                        *(reg+dstreg)  = immediate;
                        fprintf (stdout, "R%u, 0x%.8X", dstreg, immediate);
                        break;

                    case 01: // MOV DSTREG, SRCREG
                        *(reg+dstreg)  = srcreg;
                        fprintf (stdout, "R%u, R%u", dstreg, srcreg);
                        break;

                    case 02: // MOV DSTREG, [Immediate]
                        *(reg+dstreg)  = *(memory+immediate);
                        fprintf (stdout, "R%u, [0x%.8X]", dstreg, immediate);
                        break;

                    case 03: // MOV DSTREG, [SRCREG]
                        value          = *(reg+dstreg);
                        *(reg+dstreg)  = *(memory+value);
                        fprintf (stdout, "R%u, [R%u]", dstreg, srcreg);
                        break;

                    case 04: // MOV DSTREG, [SRCREG+Immediate]
                        *(reg+dstreg)  = *(memory+(*(reg+srcreg)+immediate));
                        fprintf (stdout, "R%u, [R%u+0x%.8X]", dstreg, srcreg, immediate);
                        break;

                    case 05: // MOV [Immediate], SRCREG
                        *(memory+immediate)  = *(reg+srcreg);
                        fprintf (stdout, "[0x%.8X], R%u", immediate, srcreg);
                        break;

                    case 06: // MOV [DSTREG], SRCREG
                        *(memory+*(reg+dstreg))  = *(reg+srcreg);
                        fprintf (stdout, "[R%u], R%u", dstreg, srcreg);
                        break;

                    case 07: // MOV [DSTREG+Immediate], SRCREG
                        *(memory+(*(reg+dstreg)+immediate))  = *(reg+srcreg);
                        fprintf (stdout, "[R%u+0x%.8X], R%u", dstreg, immediate, srcreg);
                        break;
                }
                param          = 0;
                break;

            case IN:
                fprintf (stdout, "%-5s R%u, 0x%.8X", "IN", dstreg, port);
                param          = 0;
                break;

            case OUT:
                fprintf (stdout, "%-5s 0x%.3X, ", "OUT", port);
                if (immflag   == 1)
                {
                    fprintf (stdout, "0x%.8X", immediate);
                }
                else
                {
                    fprintf (stdout, "R%u", dstreg);
                }

                param          = 0;
                break;
        }

        if (param             >= 1)
        {
            dstreg             = (word & 0x01E00000) >> 21;
            fprintf (stdout, "R%u, ", dstreg);

            immediate          = word & 0x02000000;
            if (immflag       == 1)
            {
                fprintf (stdout, "0x%.8X", immediate);
                *(reg+dstreg)  = immediate;
            }
        }

        fprintf (stdout, "\n");
        if (immflag           == 1)
        {
            showword (*(reg+IV), FLAG_SHOW);
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

void      showword (uint32_t  word, uint8_t  flag)
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
