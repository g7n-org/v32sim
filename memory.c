#include "defines.h"

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
                (memory+page) -> size       = 1024 * 1024 * wordsize;
                break;

            case V32_PAGE_BIOS:
                (memory+page) -> type       = V32_PAGE_BIOS;
                (memory+page) -> firstaddr  = 0x10000000;
                (memory+page) -> last_addr  = 0x100FFFFF;
                (memory+page) -> size       = get_filesize (BIOS_DEFAULT_PATH);
                break;

            case V32_PAGE_CART:
                (memory+page) -> type       = V32_PAGE_CART;
                (memory+page) -> firstaddr  = 0x20000000;
                (memory+page) -> last_addr  = 0x27FFFFFF;
                (memory+page) -> size       = get_filesize (cartfile);
                break;

            case V32_PAGE_MEMC:
                (memory+page) -> type       = V32_PAGE_MEMC;
                (memory+page) -> firstaddr  = 0x30000000;
                (memory+page) -> last_addr  = 0x3003FFFF;
                (memory+page) -> size       = 1024 * 256;
                break;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // allocate memory for the page
        //
        len                                 = sizeof (data_t) * (memory+page) -> size;
        (memory+page)     -> data           = (data_t *) malloc (len);
        if ((memory+page) -> data          == NULL)
        {
            fprintf (stderr, "[error] failed to allocate for memory page\n");
            exit (MEMORY_ALLOC_FAIL);
        }

        dptr                                = (memory+page) -> data;
        for (offset                         = 0;
             offset                        <  (memory+page) -> size;
             offset                         = offset + 1)
        {
            switch (page)
            {
                case V32_PAGE_RAM:
                case V32_PAGE_MEMC:
                    (dptr+offset) -> flag   = FLAG_READ | FLAG_WRITE;
                    break;

                case V32_PAGE_BIOS:
                case V32_PAGE_CART:
                    (dptr+offset) -> flag   = FLAG_READ;
                    break;
            }
            (dptr+offset) -> value.i32      = 0x00000000;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Mark unused BIOS page words as neither read nor write
        //
        if (page                           == V32_PAGE_BIOS)
        {
            for (offset                     = (memory+page) -> size;
                 offset                    <  (memory+page) -> last_addr - (memory+page) -> firstaddr;
                 offset                     = offset + 1)
            {
                (dptr+offset) -> flag       = FLAG_NONE;
            }
        }            
    }
}

size_t    get_filesize (int8_t *filename)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    int32_t  offset  = 0;
    size_t   size    = 0;
    FILE    *fptr    = fopen (filename, "rb"); // open in (binary) read mode

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // open indicated file for reading (in binary mode, if pertinent)
    //
    if (fptr        == NULL)
    {
        fprintf (stderr, "[ERROR] Unable to open file '%s' for reading\n", filename);
        exit    (FILE_OPEN_ERROR);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // adjust FILE pointer to end of file
    //
    offset           = fseek (fptr, 0, SEEK_END);
    if (offset      == 0)
    {
        ////////////////////////////////////////////////////////////////////////////////
        //
        // Get the current position (file size in bytes)
        //
        size         = ftell (fptr);
        if (size    == -1)
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

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Close the file
    //
    fclose (fptr);

    return (size);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// load_memory(): load data files from disk into page-appropriate location in memory
//
void    load_memory (uint32_t  page, int8_t *filename)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    FILE     *fptr     = NULL;
    uint32_t  offset   = 0x00000000;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Adjust offset to be at the start of the indicated page
    //
    offset             = offset | (page << 28);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Open indicated filename for reading
    //
    fptr               = fopen (filename, "rb");
    if (fptr          == NULL)
    {
        fprintf (stderr, "[ERROR] Could not open '%s' for reading\n", filename);
        exit    (FILE_OPEN_ERROR);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Adjust position in file to move beyond header data, based on page
    //
    switch (page)
    {
        case V32_PAGE_CART:
            fseek (fptr, (22 * wordsize), SEEK_SET);
			sys_force  = TRUE;
			ioports_set (CAR_NumberOfTextures, get_word (fptr));
			sys_force  = TRUE;
			ioports_set (CAR_NumberOfSounds,   get_word (fptr));
			get_word (fptr);
			sys_force  = TRUE;
			ioports_set (CAR_ProgramROMSize,   get_word (fptr));
			sys_force  = TRUE;
			ioports_set (CAR_Connected,        TRUE);
        case V32_PAGE_BIOS: // we need to skip ahead to word 0x23 (both BIOS and CART)
            fseek (fptr, (35 * wordsize), SEEK_SET);
            break;

        case V32_PAGE_MEMC: // we need to skip ahead to word ?? (check for value on MEMC)
            break;
    }
    
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Continually read in words from the file until we reach EOF, placing each
    // read word in memory at the appropriate offset.
    //
    while (!feof (fptr))
    {
        sys_force      = TRUE;
        memory_set (offset, get_word (fptr));
        if (!feof (fptr))
        {
            offset     = offset + 1;
        }
    }

    fclose (fptr);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// memory_get(): retrieve word_t located at requested memory address
//
// returns a word_t pointer to the requested content
//
word_t *memory_get (uint32_t  address)
{
    data_t   *dptr             = NULL;
    uint32_t  page             = (address & 0xF0000000) >> 28;
    uint32_t  offset           = (address & 0x0FFFFFFF);
    uint8_t   flag             = FLAG_NONE;
    word_t   *wptr             = NULL;

    switch (page)
    {
        case V32_PAGE_RAM:
            if (address       >  RAM_LAST_ADDR)
            {
                fprintf (verbose, "[ERROR] invalid RAM access at 0x%.8X\n", address);
                sys_error      = ERROR_MEMORY_READ;
            }

            dptr               = (memory+page)  -> data;
            flag               = ((dptr+offset) -> flag) & FLAG_READ;
            if (flag          != FLAG_READ)
            {
                if (sys_force == FALSE)
                {
                    fprintf (verbose, "[ERROR] RAM address 0x%.8X not readable!\n", address);
                    sys_error  = ERROR_MEMORY_READ;
             //       exit (MEMORY_READ_ERROR);
                }
                sys_force      = FALSE;
            }
            break;

        case V32_PAGE_BIOS:
            if (address       >  BIOS_LAST_ADDR)
            {
                fprintf (verbose, "[ERROR] invalid BIOS access at 0x%.8X\n", address);
                sys_error      = ERROR_MEMORY_READ;
            //    exit (MEMORY_BAD_ACCESS);
            }

            dptr               = (memory+page)  -> data;
            flag               = ((dptr+offset) -> flag) & FLAG_READ;
            if (flag          != FLAG_READ)
            {
                if (sys_force == FALSE)
                {
                    fprintf (verbose, "[ERROR] BIOS address 0x%.8X not readable!\n", address);
                    sys_error  = ERROR_MEMORY_READ;
                //    exit (MEMORY_READ_ERROR);
                }
                sys_force      = FALSE;
            }
            break;

        case V32_PAGE_CART:
            if (address       >  CART_LAST_ADDR)
            {
                fprintf (verbose, "[ERROR] invalid CART access at 0x%.8X\n", address);
                sys_error      = ERROR_MEMORY_READ;
                //exit (MEMORY_BAD_ACCESS);
            }

            dptr               = (memory+page)  -> data;
            flag               = ((dptr+offset) -> flag) & FLAG_READ;
            if (flag          != FLAG_READ)
            {
                if (sys_force == FALSE)
                {
                    fprintf (verbose, "[ERROR] CART address 0x%.8X not readable!\n", address);
                    sys_error  = ERROR_MEMORY_READ;
                    //exit (MEMORY_READ_ERROR);
                }
                sys_force      = FALSE;
            }
            break;

        case V32_PAGE_MEMC:
            if (address       >  MEMC_LAST_ADDR)
            {
                fprintf (verbose, "[ERROR] invalid MEMC access at 0x%.8X\n", address);
                sys_error      = ERROR_MEMORY_READ;
                //exit (MEMORY_BAD_ACCESS);
            }

            dptr               = (memory+page)  -> data;
            flag               = ((dptr+offset) -> flag) & FLAG_READ;
            if (flag          != FLAG_READ)
            {
                if (sys_force == FALSE)
                {
                    fprintf (verbose, "[ERROR] MEMC address 0x%.8X not readable!\n", address);
                    sys_error  = ERROR_MEMORY_READ;
                    //exit (MEMORY_READ_ERROR);
                }
                sys_force      = FALSE;
            }
            break;
    }

    if (sys_error             == ERROR_NONE)
    {
        dptr                   = (memory+page)  -> data;
        wptr                   = &(dptr+offset) -> value;
    }
    return (wptr);
}

// define  ERROR_MEMORY_READ       0
// define  ERROR_MEMORY_WRITE      1
void    memory_set (uint32_t  address, uint32_t  dataword)
{
    data_t   *dptr              = NULL;
    uint32_t  page              = (address & 0xF0000000) >> 28;
    uint32_t  offset            = (address & 0x0FFFFFFF);
    uint8_t   flag              = FLAG_NONE;

    switch (page)
    {
        case V32_PAGE_RAM:
            if (address        >  RAM_LAST_ADDR)
            {
                fprintf (stderr, "[ERROR] invalid RAM access at 0x%.3hX\n", address);
                exit (MEMORY_BAD_ACCESS);
            }

            dptr                = (memory+page)  -> data;
            flag                = ((dptr+offset) -> flag) & FLAG_WRITE;
            if (flag           != FLAG_WRITE)
            {
                if (sys_force  == FALSE)
                {
                    fprintf (stderr, "[ERROR] RAM address 0x%.8X not writable!\n", address);
                    exit (MEMORY_WRITE_ERROR);
                }
                sys_force       = FALSE;
            }
            break;

        case V32_PAGE_BIOS:
            if (address        >  BIOS_LAST_ADDR)
            {
                fprintf (stderr, "[ERROR] invalid BIOS access at 0x%.3hX\n", address);
                exit (MEMORY_BAD_ACCESS);
            }

            dptr                = (memory+page)  -> data;
            flag                = ((dptr+offset) -> flag) & FLAG_WRITE;
            if (flag           != FLAG_WRITE)
            {
                if (sys_force  == FALSE)
                {
                    fprintf (stderr, "[ERROR] BIOS address 0x%.8X not writable!\n", address);
                    exit (MEMORY_WRITE_ERROR);
                }
                sys_force       = FALSE;
            }
            break;

        case V32_PAGE_CART:
            if (address        >  CART_LAST_ADDR)
            {
                fprintf (stderr, "[ERROR] invalid CART access at 0x%.3hX\n", address);
                exit (MEMORY_BAD_ACCESS);
            }

            dptr                = (memory+page)  -> data;
            flag                = ((dptr+offset) -> flag) & FLAG_WRITE;
            if (flag           != FLAG_WRITE)
            {
                if (sys_force  == FALSE)
                {
                    fprintf (stderr, "[ERROR] CART address 0x%.8X not writable!\n", address);
                    exit (MEMORY_WRITE_ERROR);
                }
                sys_force       = FALSE;
            }
            break;

        case V32_PAGE_MEMC:
            if (address        >  MEMC_LAST_ADDR)
            {
                fprintf (stderr, "[ERROR] invalid MEMC access at 0x%.3hX\n", address);
                exit (MEMORY_BAD_ACCESS);
            }

            dptr                = (memory+page)  -> data;
            flag                = ((dptr+offset) -> flag) & FLAG_WRITE;
            if (flag           != FLAG_WRITE)
            {
                if (sys_force  == FALSE)
                {
                    fprintf (stderr, "[ERROR] MEMC address 0x%.8X not writable!\n", address);
                    exit (MEMORY_WRITE_ERROR);
                }
                sys_force       = FALSE;
            }
            break;
    }

    dptr                        = (memory+page)  -> data;
    (dptr+offset) -> value.i32  = dataword;
}
