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
    uint8_t  result                 = TRUE;

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
            // write MEMC data back to memcfile
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
    FILE     *fptr                    = NULL;
    int32_t   count                   = 0;
    int32_t   index                   = 0;
    uint32_t  offset                  = 0x00000000;
    uint32_t  num_vtex                = 0x00000000;
    uint32_t  num_vsnd                = 0x00000000;
    uint32_t  data_size               = 0x00000000;
    uint32_t  data                    = 0x00000000;
    uint32_t *checksum                = NULL;
    size_t    size                    = 0;
    uint32_t  len                     = 0;
    uint8_t   chk                     = FALSE;
    uint8_t   result                  = FALSE;
	region_t *rptr                    = NULL;
    vtex_t   *vptr                    = NULL;

    if (filename                     != NULL)
    {
        ////////////////////////////////////////////////////////////////////////////////
        //
        // Adjust offset to be at the start of the indicated page
        //
        offset                        = offset | (page << 28);

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Open indicated filename for reading
        //
        fptr                          = fopen (filename, "rb");
        if (fptr                     == NULL)
        {
            fprintf (stderr, "[ERROR] Could not open '%s' for reading\n", filename);
            exit    (FILE_OPEN_ERROR);
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Adjust size of region of memory
        //
        (memory+page) -> size         = get_filesize (filename);
        (memory+page) -> last_addr    = (memory+page) -> firstaddr;
        (memory+page) -> last_addr   += (memory+page) -> size;
        chk                           = alloc_memory (page);
        if (chk                      == TRUE)
        {
            ////////////////////////////////////////////////////////////////////////////
            //
            // Adjust position in file to move beyond header data, based on page
            //
            switch (page)
            {
                case V32_PAGE_BIOS:
                case V32_PAGE_CART:
                    fseek (fptr, (0x16 * wordsize), SEEK_SET);

                    ////////////////////////////////////////////////////////////////////
                    //
                    // Here we  obtain the number of  textures and sounds
                    // within the CART, for memory allocation of the CART
                    // cart_regions array (and likely whatever soundarray
                    // is eventually concocted)
                    //
                    num_vtex          = get_word (fptr);
                    num_vsnd          = get_word (fptr);

                    ////////////////////////////////////////////////////////////////////
                    //
                    // While we are in the neighborhood, obtain the  vtex
                    // and vsnd offsets
                    //
                    get_word (fptr); // skipping
                    get_word (fptr); // skipping
                    get_word (fptr); // VTEX offset
                    get_word (fptr); // VSND offset

                    ////////////////////////////////////////////////////////////////////
                    //
                    // CART texture/regions  is a 2D array  of region_t's
                    // for   managing   the   regions  present   in   the
                    // potentially many CART  textures. Allocation of the
                    // second dimension is done  after the CART is loaded
                    // (so that resources are  only allocated for what is
                    // needed based on the CART that is being run).
                    //
                    if (page         == V32_PAGE_CART)
                    {
                        size          = sizeof (vtex_t);
                        len           = num_vtex;
                        cart_vtex     = (vtex_t *) calloc (size, len); 
                        vptr          = cart_vtex;
                    }
                    else
                    {
                        vptr          = bios_vtex;
                    }

                    vptr -> qty       = num_vtex;

                    ////////////////////////////////////////////////////////////////////
                    //
                    // Allocate memory for each texture  (to  have up  to
                    // 4096 regions)
                    //
                    for (index        = 0;
                         index       <  num_vtex;
                         index        = index + 1)
                    {
                        if (page     == V32_PAGE_CART)
                        {
                            rptr      = (cart_vtex+index) -> region;
                        }
                        else // BIOS
                        {
                            rptr      = bios_vtex         -> region;
                        }

                        size          = sizeof (region_t);
                        len           = V32_REGIONS_PER_TEXTURE;
                        rptr          = (region_t *) calloc (size, len);
                    }

                    ////////////////////////////////////////////////////////////////////
                    //
                    // we need to skip ahead to word 0x22 (BIOS and CART), this will
                    // contain the length of the VBIN section
                    //
                    fseek (fptr, (0x22 * wordsize), SEEK_SET);
                    data_size         = get_word (fptr);

                    if (page         == V32_PAGE_CART)
                    {
                        SYSPORTSET(CAR_NumberOfTextures, num_vtex);
                        SYSPORTSET(CAR_NumberOfSounds,   num_vsnd);
                        SYSPORTSET(CAR_Connected,        TRUE);
                        SYSPORTSET(CAR_ProgramROMSize,   data_size);
                        fprintf (debug, "[LOAD:CART] ");
                    }
                    else
                    {
                        fprintf (debug, "[LOAD:BIOS] ");
                    }
                    fprintf (debug, "#vtex: %u, #vsnd: %u, romsize: 0x%.8X\n",
                                    num_vtex, num_vsnd, data_size);
                    break;

                case V32_PAGE_MEMC: // we need to skip ahead to word ?? (check for value)
                    fprintf (debug, "[LOAD:MEMC]\n");
                    fseek (fptr, (0x02 * wordsize), SEEK_SET);
                    SYSPORTSET(MEM_Connected,        TRUE);
                    data_size         = 262144;
                    break;
            }

            ////////////////////////////////////////////////////////////////////////////
            //
            // Continually read in words from the file until we reach EOF, placing
            // each read word in memory at the appropriate offset.
            //
            checksum                  = &(memory+page) -> checksum;
            for (index                = 0;
                 index               <  data_size;
                 index                = index + 1)
            {
                data                  = get_word (fptr);
                SYSMEMSET(offset, data);
                if (!feof (fptr))
                {
                    *checksum         = *checksum + data;
                    offset            = offset + 1;
                }
                else
                {
                    if (page         == V32_PAGE_MEMC)
                    {
                        fprintf (stderr, "[ERROR:MEMC] ");
                    }
                    else
                    {
                        fprintf (stderr, "[ERROR:VBIN] ");
                    }
                    fprintf (stderr, "Unexpected end of file reading '%s' at offset 0x%.8X!\n",
                                     filename, offset);
                    exit (FILE_POSITION_ERROR);
                }
            }

            ////////////////////////////////////////////////////////////////////////////
            //
            // Now at VTEX
            //
            if ((page                == V32_PAGE_BIOS) ||
                (page                == V32_PAGE_CART))
            {
                fprintf (debug, "[LOAD:VBIN] final offset is: 0x%.8X\n", offset);
                for (count            = 0;
                     count           <  num_vtex;
                     count            = count + 1)
                {
					if (page         == V32_PAGE_CART)
					{
						vptr          = (cart_vtex+count);
					}
					else // BIOS
					{
						vptr          = bios_vtex;
					}

                    ////////////////////////////////////////////////////////////////////
                    //
                    // populate VTEX attributes
                    //
                    get_word (fptr); // "V32_"
                    get_word (fptr); // "VTEX"
                    vptr -> wide      = get_word (fptr); // width
                    vptr -> high      = get_word (fptr); // height
                    vptr -> size      = vptr -> wide * vptr -> high;
                    vptr -> offset    = offset;

                    for (index        = 0;
                         index       <  vptr -> size;
                         index        = index + 1)
                    {
                        data          = get_word (fptr);
                        SYSMEMSET(offset, data);
                        if (!feof (fptr))
                        {
                            *checksum = *checksum + data;
                            offset    = offset + 1;
                        }
                        else
                        {
                            fprintf (stderr, "[ERROR:VTEX] Unexpected end of file reading '%s' at offset 0x%.8X!\n",
                                             filename, offset);
                            exit (FILE_POSITION_ERROR);
                        }
                    }

                    fprintf (debug, "[LOAD:VBIN] final offset of vtex #%d is: 0x%.8X\n", count, offset);
                }

                // TODO: add section for VSND data, will look much like the above
            }
            else
            {
                fprintf (debug, "[LOAD:MEMC] final offset is: 0x%.8X (%u)\n", offset, offset);
            }
            
            fclose (fptr);

            ////////////////////////////////////////////////////////////////////////////
            //
            // Operation deemed successful
            //
            result                    = TRUE;
        }
    }

    return (result);
}
