#include "defines.h"

void  init_memory (void)
{
    int32_t  page                           = 0;
    size_t   len                            = 0;
    uint8_t  chk                            = FALSE;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Allocate the memory device
    //
    len                                     = NUM_MEMORY_PAGES;
    memory                                  = (mem_t *) calloc (sizeof (mem_t), len);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Verify malloc() was successful
    //
    if (memory                             == NULL)
    {
        fprintf (stderr, "[error] failed to allocate for memory resource\n");
        exit (MEMORY_ALLOC_FAIL);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Initialize each page
    //
    for (page                               = 0;
         page                              <  NUM_MEMORY_PAGES;
         page                               = page  + 1)
    {
        switch (page)
        {
            case V32_PAGE_RAM:  // Vircon32 RAM is 16MB / 4MW
                (memory+page) -> type       = V32_PAGE_RAM;
                (memory+page) -> firstaddr  = RAM_FIRST_ADDR;
                (memory+page) -> last_addr  = RAM_FINAL_ADDR;
                (memory+page) -> size       = 1024 * 1024 * wordsize;
                (memory+page) -> flag       = FLAG_READ | FLAG_WRITE;
                (memory+page) -> data       = NULL;
                break;

            case V32_PAGE_BIOS:
                (memory+page) -> type       = V32_PAGE_BIOS;
                (memory+page) -> firstaddr  = BIOS_FIRST_ADDR;
                (memory+page) -> size       = get_filesize (biosfile);
                (memory+page) -> last_addr  = (memory+page) -> firstaddr + (memory+page) -> size;
                (memory+page) -> flag       = FLAG_READ;
                (memory+page) -> data       = NULL;
                break;

            case V32_PAGE_CART:
                (memory+page) -> type       = V32_PAGE_CART;
                (memory+page) -> firstaddr  = CART_FIRST_ADDR;
                (memory+page) -> size       = get_filesize (cartfile);
                (memory+page) -> last_addr  = (memory+page) -> firstaddr + (memory+page) -> size;
                (memory+page) -> flag       = FLAG_READ;
                (memory+page) -> data       = NULL;
                break;

            case V32_PAGE_MEMC:
                (memory+page) -> type       = V32_PAGE_MEMC;
                (memory+page) -> firstaddr  = MEMC_FIRST_ADDR;
                (memory+page) -> size       = 1024 * 256;
                (memory+page) -> size       = get_filesize (memcfile);
                (memory+page) -> last_addr  = (memory+page) -> firstaddr + (memory+page) -> size;
                (memory+page) -> flag       = FLAG_READ | FLAG_WRITE;
                (memory+page) -> data       = NULL;
                break;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Check that memory has been allocated for the page
        //
        chk                                 = alloc_memory (page);
        if (chk                            == FALSE)
        {
            continue;
        }
    }
}

uint8_t  alloc_memory (int32_t  page)
{
    size_t   len                            = 0;
    uint8_t  result                         = FALSE;

    if ((page                              >= V32_PAGE_RAM) &&
        (page                              <= V32_PAGE_MEMC))
    {
        ////////////////////////////////////////////////////////////////////////////////
        //
        // determine size of page
        //
        len                                 = (memory+page) -> size;

        ////////////////////////////////////////////////////////////////////////////////
        //
        // if pagesize is 0, skip
        //
        if (len                            >  0)
        {
            ////////////////////////////////////////////////////////////////////////////
            //
            // dallocate memory for the page, if it exists
            //
            if ((memory+page) -> data      != NULL)
            {
                free ((memory+page) -> data);
                (memory+page) -> data       = NULL;
            }

            ////////////////////////////////////////////////////////////////////////////
            //
            // allocate memory for the page
            //
            (memory+page)     -> data       = (data_t *) calloc (len, sizeof (data_t));
            if ((memory+page) -> data      == NULL)
            {
                fprintf (stderr, "[error] failed to allocate for memory page\n");
                exit (MEMORY_ALLOC_FAIL);
            }

            result                          = TRUE;
        }
    }

    return (result);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// memory_chk(): check that the address being transacted is valid
//
// returns a TRUE or FALSE
//
uint8_t  memory_chk (uint32_t  address, uint8_t  sys_force)
{
    uint32_t  page             = (address & 0xF0000000) >> 28;
    uint8_t   result           = TRUE;

    switch (page)
    {
        case V32_PAGE_RAM:
            if (address       >  RAM_FINAL_ADDR)
            {
                if (sys_force == FALSE)
                {
                    fprintf (verbose, "[ERROR] invalid RAM access at 0x%.8X\n", address);
                    sys_error  = ERROR_MEMORY_READ;
                }
                result         = FALSE;
            }
            break;

        case V32_PAGE_BIOS:
            if (address       >  (memory+page) -> last_addr)
            {
                if (sys_force == FALSE)
                {
                    fprintf (verbose, "[ERROR] invalid BIOS access at 0x%.8X\n", address);
                    sys_error  = ERROR_MEMORY_READ;
                }
                result         = FALSE;
            }
            break;

        case V32_PAGE_CART:
            if (address       >  (memory+page) -> last_addr)
            {
                if (sys_force == FALSE)
                {
                    fprintf (verbose, "[ERROR] invalid CART access at 0x%.8X\n", address);
                    sys_error  = ERROR_MEMORY_READ;
                }
                result         = FALSE;
            }
            break;

        case V32_PAGE_MEMC:
            if (address       >  MEMC_FINAL_ADDR)
            {
                if (sys_force == FALSE)
                {
                    fprintf (verbose, "[ERROR] invalid MEMC access at 0x%.8X\n", address);
                    sys_error  = ERROR_MEMORY_READ;
                }
                result         = FALSE;
            }
            break;

        default:
            if (sys_force     == FALSE)
            {
                fprintf (verbose,     "[ERROR] invalid memory access at 0x%.8X\n", address);
                sys_error      = ERROR_MEMORY_READ;
            }
            result             = FALSE;
            break;
    }

    if ((memory+page) -> data == NULL)
    {
        result                 = FALSE;
    }

    return (result);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// memory_get(): retrieve word_t located at requested memory address
//
// returns a word_t pointer to the requested content
//
word_t *memory_get (uint32_t  address, uint8_t  sys_force)
{
    data_t   *dptr                 = NULL;
    uint32_t  page                 = (address & 0xF0000000) >> 28;
    uint32_t  offset               = (address & 0x0FFFFFFF);
    uint8_t   check                = FLAG_NONE;
    uint8_t   flag                 = FLAG_NONE;
    word_t   *wptr                 = NULL;

    check                          = memory_chk (address, sys_force);
    if (check                     == TRUE)
    {
        flag                       = ((memory+page) -> flag) & FLAG_READ;
        switch (page)
        {
            case V32_PAGE_RAM:
                if (flag          != FLAG_READ)
                {
                    if (sys_force == FALSE)
                    {
                        fprintf (verbose, "[ERROR] RAM address 0x%.8X not readable!\n",
                                address);
                        sys_error  = ERROR_MEMORY_READ;
                    }
                }
                break;

            case V32_PAGE_BIOS:
                if (flag          != FLAG_READ)
                {
                    if (sys_force == FALSE)
                    {
                        fprintf (verbose, "[ERROR] BIOS address 0x%.8X not readable!\n",
                                address);
                        sys_error  = ERROR_MEMORY_READ;
                    }
                }
                break;

            case V32_PAGE_CART:
                if (flag          != FLAG_READ)
                {
                    if (sys_force == FALSE)
                    {
                        fprintf (verbose, "[ERROR] CART address 0x%.8X not readable!\n",
                                address);
                        sys_error  = ERROR_MEMORY_READ;
                    }
                }
                break;

            case V32_PAGE_MEMC:
                if (flag          != FLAG_READ)
                {
                    if (sys_force == FALSE)
                    {
                        fprintf (verbose, "[ERROR] MEMC address 0x%.8X not readable!\n",
                                address);
                        sys_error  = ERROR_MEMORY_READ;
                    }
                }
                break;

            default:
                if (sys_force     == FALSE)
                {
                    fprintf (verbose, "[ERROR] memory address 0x%.8X not readable!\n",
                            address);
                    sys_error      = ERROR_MEMORY_READ;
                }
                break;
        }

        dptr                       = (memory+page)  -> data;
        wptr                       = &(dptr+offset) -> value;
    }

    return (wptr);
}

void    memory_set (uint32_t  address, uint32_t  dataword, uint8_t  sys_force)
{
    data_t   *dptr              = NULL;
    uint32_t  page              = (address & 0xF0000000) >> 28;
    uint32_t  offset            = (address & 0x0FFFFFFF);
    uint8_t   flag              = FLAG_NONE;

    flag                        = ((memory+page) -> flag) & FLAG_WRITE;
    switch (page)
    {
        case V32_PAGE_RAM:
            if (address        >  RAM_FINAL_ADDR)
            {
                fprintf (stderr, "[ERROR] invalid RAM access at 0x%.3hX\n", address);
                exit (MEMORY_BAD_ACCESS);
            }

            if (flag           != FLAG_WRITE)
            {
                if (sys_force  == FALSE)
                {
                    fprintf (stderr, "[ERROR] RAM address 0x%.8X not writable!\n", address);
                    exit (MEMORY_WRITE_ERROR);
                }
            }
            break;

        case V32_PAGE_BIOS:
            if (address        >  (memory+page) -> last_addr)
            {
                fprintf (stderr, "[ERROR] invalid BIOS access at 0x%.3hX\n", address);
                exit (MEMORY_BAD_ACCESS);
            }

            if (flag           != FLAG_WRITE)
            {
                if (sys_force  == FALSE)
                {
                    fprintf (stderr, "[ERROR] BIOS address 0x%.8X not writable!\n", address);
                    exit (MEMORY_WRITE_ERROR);
                }
            }
            break;

        case V32_PAGE_CART:
            if (address        >  (memory+page) -> last_addr)
            {
                fprintf (stderr, "[ERROR] invalid CART access at 0x%.3hX\n", address);
                exit (MEMORY_BAD_ACCESS);
            }

            if (flag           != FLAG_WRITE)
            {
                if (sys_force  == FALSE)
                {
                    fprintf (stderr, "[ERROR] CART address 0x%.8X not writable!\n", address);
                    exit (MEMORY_WRITE_ERROR);
                }
            }
            break;

        case V32_PAGE_MEMC:
            if (address        >  MEMC_FINAL_ADDR)
            {
                fprintf (stderr, "[ERROR] invalid MEMC access at 0x%.3hX\n", address);
                exit (MEMORY_BAD_ACCESS);
            }

            if (flag           != FLAG_WRITE)
            {
                if (sys_force  == FALSE)
                {
                    fprintf (stderr, "[ERROR] MEMC address 0x%.8X not writable!\n", address);
                    exit (MEMORY_WRITE_ERROR);
                }
            }
            break;

        default:
            if (sys_force      == FALSE)
            {
                fprintf (stderr, "[ERROR] invalid address 0x%.8X!\n", address);
                exit (MEMORY_BAD_ACCESS);
            }
            break;
    }

    dptr                        = (memory+page)  -> data;
    (dptr+offset) -> value.i32  = dataword;
}
