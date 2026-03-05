#include "defines.h"

////////////////////////////////////////////////////////////////////////////////////////
//
// get_word(): obtain the next word from the program file,  the  word 
//             is assembled with shifts and iORs from the bytes.
//
// RETURN VAL: the uint32_t of the word just read
//
uint32_t  get_word (FILE *fptr)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize local variables
    //
    int32_t   index   = 0;
    uint32_t  word    = 0x00000000;  // clear word

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Read a wordsize amount of data from the FILE pointer
    //
    fread (data, sizeof (uint8_t), wordsize, fptr);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // If we have not encountered the end of the file, proceed with
    // processing
    //
    if (!feof (fptr))
    {
        ////////////////////////////////////////////////////////////////////////////////
        //
        // Assemble the individual bytes into one word
        //
        for (index    = 0;
             index   <  wordsize;
             index    = index + 1)
        {
            ////////////////////////////////////////////////////////////////////////////
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
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize local variables
    //
    int32_t   index   = 0;
    uint32_t  mask    = 0xFF000000;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Check `flag` to see if we will display the current offset
    //
    if (flag         == FLAG_DISPLAY)
    {
        fprintf (stdout, "[%.8X] ", REG(IP));
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

////////////////////////////////////////////////////////////////////////////////////////
//
// word2int() - extract i32 element of provided `word_t` and return it
//
// in the event that the passed-in word_t is NULL, return 0.
//
uint32_t  word2int     (word_t *info)
{
    uint32_t  result  = 0;
    if (info         != NULL)
    {
        result        = info -> i32;
    }
    else
    {
        result        = 0;
    }
    return (result);
}

float     word2float   (word_t *info)
{
    return (info -> f32);
}

uint32_t  word2raw     (word_t *info)
{
    return (info -> raw);
}

word_t   *new_word_i32 (uint32_t *value, uint8_t  num)
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
