#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

uint8_t  *data;
uint32_t  offset;
uint8_t   wordsize;

uint32_t  get_word (FILE *, uint8_t);

int32_t   main (int8_t  argc,  uint8_t **argv)
{
    FILE     *program     = NULL;
    int32_t   index       = 0;
    uint32_t  word        = 0x00000000;
    uint32_t  vbinoffset  = 0x00000000;
    uint32_t  vtexoffset  = 0x00000000;
    uint32_t  vsndoffset  = 0x00000000;

    data                  = (uint8_t *) malloc (sizeof (uint8_t) * 4);
    offset                = 0x20000000;
    program               = fopen (argv[1], "r");
    wordsize              = 4;

    fread (data, sizeof (uint8_t), wordsize, program);
    index                 = strncmp (data, "V32-", 4);
    if (index            != 0)
    {
        fprintf (stderr, "[ERROR] Provided file is NOT a Vircon32 file\n");
        exit (1);
    }

    fread (data, sizeof (uint8_t), wordsize, program);
    index                 = strncmp (data, "CART", 4);
    if (index            != 0)
    {
        fprintf (stderr, "[ERROR] Provided file is NOT a Vircon32 cartridge\n");
        exit (2);
    }

    fseek (program, 22 * 4, SEEK_CUR);
    offset                = offset + 23;

    vbinoffset            = get_word (program, 0) / 4;
    word                  = get_word (program, 0);
    vtexoffset            = get_word (program, 0) / 4;
    word                  = get_word (program, 0);
    vsndoffset            = get_word (program, 0) / 4;
    
    for (index            = 0;
         index           <  6;
         index            = index + 1)
    {
        word              = get_word (program, 0);
    }

    fprintf (stdout, "vbinoffset: %.8X\n", vbinoffset);
    fprintf (stdout, "vtexoffset: %.8X\n", vtexoffset);
    fprintf (stdout, "vsndoffset: %.8X\n", vsndoffset);

    while (!feof (program))
    {
        word              = get_word (program, 1);
        if (!feof (program))
        {
            fprintf (stdout, "word: %.8X\n", word);
        }
    }
    
    return (0);
}

uint32_t  get_word (FILE *program, uint8_t  flag)
{
    int32_t   index   = 0;
    uint32_t  word    = 0x00000000;

    fread (data, sizeof (uint8_t), wordsize, program);
    if (!feof (program))
    {
        offset        = offset + 1;
        word          = 0x00000000;
        fprintf (stdout, "[%.8X] ", offset);

        for (index    = 0;
             index   <  wordsize;
             index    = index + 1)
        {
            word      = word | (*(data+index) << (8*index));
            if (flag == 1)
            {
                fprintf (stdout, "%.2hhX ", *(data+index));
            }
        }
    }

    return (word);
}
