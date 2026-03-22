#include "defines.h"

size_t    get_filesize (int8_t *filename)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    int32_t  offset   = 0;
    size_t   size     = 0;
    FILE    *fptr     = NULL;

    if (filename     != NULL)
    {
        fptr          = fopen (filename, "rb"); // open in (binary) read mode

        ////////////////////////////////////////////////////////////////////////////////
        //
        // open indicated file for reading (in binary mode, if pertinent)
        //
        if (fptr     == NULL)
        {
            fprintf (stderr, "[ERROR] Unable to open '%s' for reading\n", filename);
            exit    (FILE_OPEN_ERROR);
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // adjust FILE pointer to end of file
        //
        offset        = fseek (fptr, 0, SEEK_END);
        if (offset   == 0)
        {
            ////////////////////////////////////////////////////////////////////////////
            //
            // Get the current position (file size in bytes)
            //
            size      = ftell (fptr);
            if (size == -1)
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

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Close the file
        //
        fclose (fptr);
    }

    return (size);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// unload_memory(): unload data files from memory
//
uint8_t  unload_memory (uint32_t  page)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    uint8_t   result                = TRUE;

    switch (page)
    {
        case V32_PAGE_BIOS:
            break;

        case V32_PAGE_CART:
            SYSPORTSET(CAR_Connected,        FALSE);
            SYSPORTSET(CAR_NumberOfTextures, 0);
            SYSPORTSET(CAR_NumberOfSounds,   0);
            SYSPORTSET(CAR_ProgramROMSize,   0);
            break;

        case V32_PAGE_MEMC:
            SYSPORTSET(MEM_Connected,        FALSE);
            break;

        default:
            fprintf (verbose, "[unload_memory] invalid page %u specified\n", page);
            result                  = FALSE;
            break;
    }

    if (result                     == TRUE)
    {
        free ((memory+page) -> data);
        (memory+page) -> data       = NULL;
        (memory+page) -> size       = 0;
        (memory+page) -> last_addr  = (memory+page) -> firstaddr;
    }

    return (result);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// load_memory(): load data files from disk into page-appropriate location in memory
//
uint8_t  load_memory (uint32_t  page, int8_t *filename)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    FILE     *fptr                  = NULL;
    int32_t   index                 = 0;
    uint32_t  offset                = 0x00000000;
    uint32_t  data                  = 0x00000000;
    uint32_t *checksum              = NULL;
    uint8_t   chk                   = FALSE;
    uint8_t   result                = FALSE;

    if (filename                   != NULL)
    {
        ////////////////////////////////////////////////////////////////////////////////
        //
        // Adjust offset to be at the start of the indicated page
        //
        offset                      = offset | (page << 28);

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Open indicated filename for reading
        //
        fptr                        = fopen (filename, "rb");
        if (fptr                   == NULL)
        {
            fprintf (stderr, "[ERROR] Could not open '%s' for reading\n", filename);
            exit    (FILE_OPEN_ERROR);
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Adjust size of region of memory
        //
        (memory+page) -> size       = get_filesize (filename);
        (memory+page) -> last_addr  = (memory+page) -> firstaddr + (memory+page) -> size;
        chk                         = alloc_memory (page);
        if (chk                    == TRUE)
        {
            ////////////////////////////////////////////////////////////////////////////
            //
            // Adjust position in file to move beyond header data, based on page
            //
            switch (page)
            {
                case V32_PAGE_CART:
                    fseek (fptr, (22 * wordsize), SEEK_SET);

                    ////////////////////////////////////////////////////////////////////
                    //
                    // Here we  obtain the number of  textures within the
                    // CART,  so  memory  can  be  allocated  within  the
                    // cart_regions array
                    //
                    data            = get_word (fptr);

                    ////////////////////////////////////////////////////////////////////
                    //
                    // CART texture/regions  is a 2D array  of region_t's
                    // for   managing   the   regions  present   in   the
                    // potentially many CART  textures. Allocation of the
                    // second dimension is done  after the CART is loaded
                    // (so that resources are  only allocated for what is
                    // needed based on the CART that is being run).
                    //
                    cart_regions               = (region_t **) calloc (sizeof (region_t *),
                                                                       V32_CART_TEXTURES); 

                    ////////////////////////////////////////////////////////////////////
                    //
                    // Allocate memory for each texture  (to  have up  to
                    // 4096 regions)
                    //
                    for (index      = 0;
                         index     <  data;
                         index      = index + 1)
                    {
                        *(cart_regions+index)  = (region_t *) calloc (sizeof (region_t),
                                                       V32_REGIONS_PER_TEXTURE);
                    }
                    SYSPORTSET(CAR_NumberOfTextures, data);

                    SYSPORTSET(CAR_NumberOfSounds,   get_word (fptr));
                    get_word (fptr);
                    SYSPORTSET(CAR_ProgramROMSize,   get_word (fptr));
                    SYSPORTSET(CAR_Connected,        TRUE);
                case V32_PAGE_BIOS: // we need to skip ahead to word 0x23 (BIOS and CART)
                    fseek (fptr, (35 * wordsize), SEEK_SET);
                    break;

                case V32_PAGE_MEMC: // we need to skip ahead to word ?? (check for value)
                    SYSPORTSET(MEM_Connected,        TRUE);
                    break;
            }

            ////////////////////////////////////////////////////////////////////////////
            //
            // Continually read in words from the file until we reach EOF, placing
            // each read word in memory at the appropriate offset.
            //
            checksum                = &(memory+page) -> checksum;
            while (!feof (fptr))
            {
                data                = get_word (fptr);
                SYSMEMSET(offset, data);
                if (!feof (fptr))
                {
                    *checksum       = *checksum + data;
                    offset          = offset + 1;
                }
            }

            fclose (fptr);

            ////////////////////////////////////////////////////////////////////////////
            //
            // Operation deemed successful
            //
            result                  = TRUE;
        }
    }

    return (result);
}
