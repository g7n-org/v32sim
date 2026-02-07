#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define  FLAG_NONE  0
#define  FLAG_SHOW  1 

#define  HLT        0x00
#define  MOV        0x13
#define  OUT        0x18

uint8_t  *data;
uint32_t  offset;
uint8_t   wordsize;


uint32_t  get_word (FILE *, uint8_t);

int32_t   main (int8_t  argc,  uint8_t **argv)
{
    FILE     *program          = NULL;
    int32_t   index            = 0;
    uint8_t   param            = 0;
    uint32_t  immediate        = 0x00000000;
    uint8_t   opcode           = 0x00;
    uint8_t   dstreg           = 0x00;
    uint8_t   srcreg           = 0x00;
    uint16_t  port             = 0x0000;
    uint32_t  word             = 0x00000000;
    uint32_t  rom_offset       = 0x00000000;
    uint32_t  vbinoffset       = 0x00000000;
    uint32_t  vtexoffset       = 0x00000000;
    uint32_t  vsndoffset       = 0x00000000;

    wordsize                   = 4;
    data                       = (uint8_t *) malloc (sizeof (uint8_t) * wordsize);
    offset                     = 0x20000000;
    program                    = fopen (argv[1], "r");

    fread (data, sizeof (uint8_t), wordsize, program);
    index                      = strncmp (data, "V32-", 4);
    if (index                 != 0)
    {
        fprintf (stderr, "[ERROR] Provided file is NOT a Vircon32 file\n");
        exit (1);
    }

    fread (data, sizeof (uint8_t), wordsize, program);
    index                      = strncmp (data, "CART", 4);
    if (index                 != 0)
    {
        fprintf (stderr, "[ERROR] Provided file is NOT a Vircon32 cartridge\n");
        exit (2);
    }

    fseek (program, 22 * wordsize, SEEK_CUR);
    offset                     = offset + 23;

    vbinoffset                 = get_word (program, FLAG_NONE) / wordsize;
    word                       = get_word (program, FLAG_NONE);
    vtexoffset                 = get_word (program, FLAG_NONE) / wordsize;
    word                       = get_word (program, FLAG_NONE);
    vsndoffset                 = get_word (program, FLAG_NONE) / wordsize;
    
    for (index                 = 0;
         index                <  6;
         index                 = index + 1)
    {
        word                   = get_word (program, FLAG_NONE);
    }

    rom_offset                 = 0x20000000;

    fprintf (stdout, "rom_offset: %.8X\n", rom_offset);
    fprintf (stdout, "vbinoffset: %.8X\n", vbinoffset);
    fprintf (stdout, "vtexoffset: %.8X\n", vtexoffset);
    fprintf (stdout, "vsndoffset: %.8X\n", vsndoffset);

    while (!feof (program))
    {
        word                   = get_word (program, FLAG_SHOW);
        if (!feof (program))
        {
            fprintf (stdout, "word: %.8X ", word);
            rom_offset         = rom_offset + 1;
            opcode             = (word & 0xFC000000) >> 26;
            switch (opcode)
            {
                case HLT:
                    if (word  == 0x00000000)
                    {
                        fprintf (stdout, "%5s ", "HLT");
                    }
                    param      = 0;
                    break;

                case MOV:
                    fprintf (stdout, "%5s ", "MOV");
                    param      = 2;
                    break;

                case OUT:
                    port           = word & 0x00003FFF;
                    fprintf (stdout, "%5s 0x%.3X, ", "OUT", port);
                    immediate      = word & 0x02000000;
                    if (immediate >  0x00000000)
                    {
                        immediate  = get_word (program, FLAG_NONE);
                        fprintf (stdout, "0x%.8X\n", immediate);
                    }
                    else
                    {
                        dstreg     = (word & 0x01E00000) >> 21;
                        fprintf (stdout, "R%u", dstreg);
                    }

                    param      = 0;
                    break;
            }

            if (param         >= 1)
            {
                dstreg         = (word & 0x01E00000) >> 21;
                fprintf (stdout, "R%u, ", dstreg);

                immediate      = word & 0x02000000;
                if (immediate >  0x00000000)
                {
                    immediate  = get_word (program, FLAG_NONE);
                    fprintf (stdout, "0x%.8X", immediate);
                }
            }

            fprintf (stdout, "\n");
        }
    }
    
    return (0);
}

//////////////////////////////////////////////////////////////////////////////
//
// get_word(): obtain the next word from the program file, optionally
//             displaying information like offsets and byte data; the
//             word is assembled with shifts and iORs from the bytes.
//
// RETURN VAL: the uint32_t of the word just read
//
uint32_t  get_word (FILE *program, uint8_t  flag)
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
        // Increment offset
        //
        offset        = offset + 1;

        //////////////////////////////////////////////////////////////////////
        //
        // Check `flag` to see if we will display the current offset
        //
        if (flag     == FLAG_SHOW)
        {
            fprintf (stdout, "[%.8X] ", offset);
        }

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

            //////////////////////////////////////////////////////////////////
            //
            // Check `flag` to see if we will display the current byte
            //
            if (flag == FLAG_SHOW)
            {
                fprintf (stdout, "%.2hhX ", *(data+index));
            }
        }
    }

    return (word);
}
