#include "defines.h"

void  init_memory (void)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    int32_t  page                           = 0;
    size_t   len                            = 0;
    uint8_t  chk                            = FALSE;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Allocate the memory device
    //
    len                                     = NUM_MEMORY_PAGES;
    memory                                  = (mem_t *) ralloc (sizeof (mem_t),
                                                                len, FLAG_RETERR);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Verify ralloc() was successful
    //
    if (memory                             == NULL)
    {
        fprintf (stderr, "[init_memory] ERROR: failed to allocate memory!\n");
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
                (memory+page) -> size       = 1024 * 1024 * wordsize;
                (memory+page) -> flag       = FLAG_READ | FLAG_WRITE;
                (memory+page) -> data       = NULL;
                break;

            case V32_PAGE_BIOS:
                (memory+page) -> type       = V32_PAGE_BIOS;
                (memory+page) -> firstaddr  = BIOS_FIRST_ADDR;
                (memory+page) -> size       = get_filesize (biosfile);
                (memory+page) -> flag       = FLAG_READ;
                (memory+page) -> data       = NULL;
                break;

            case V32_PAGE_CART:
                (memory+page) -> type       = V32_PAGE_CART;
                (memory+page) -> firstaddr  = CART_FIRST_ADDR;
                (memory+page) -> size       = get_filesize (cartfile);
                (memory+page) -> flag       = FLAG_READ;
                (memory+page) -> data       = NULL;
                break;

            case V32_PAGE_MEMC:
                (memory+page) -> type       = V32_PAGE_MEMC;
                (memory+page) -> firstaddr  = MEMC_FIRST_ADDR;
                (memory+page) -> size       = get_filesize (memcfile);
                (memory+page) -> flag       = FLAG_READ | FLAG_WRITE;
                (memory+page) -> data       = NULL;
                break;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Calculate the memory page last_addr
        //
        (memory+page) -> last_addr          = (memory+page) -> firstaddr;
        (memory+page) -> last_addr         += (memory+page) -> size;

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
            (memory+page)     -> data       = (data_t *) ralloc (sizeof (data_t),
                                                                 len,
                                                                 FLAG_RETERR);
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
uint8_t  memory_chk (uint32_t  address, uint8_t  flag, uint8_t  sys_force)
{
    uint32_t  page             = (address & 0xF0000000) >> 28;
    uint8_t   type[5];
    uint8_t   action[6];
    uint8_t   attr             = 0;
    uint8_t   result           = TRUE;
    int8_t    error            = ERROR_NONE;

    if ((flag & FLAG_READ)    == FLAG_READ)
    {
        sprintf (action, "read");
        error                  = ERROR_MEMORY_READ;
    }
    else
    {
        sprintf (action, "write");
        error                  = ERROR_MEMORY_WRITE;
    }

    switch (page)
    {
        case V32_PAGE_RAM:
            sprintf (type, "RAM ");
            break;

        case V32_PAGE_BIOS:
            sprintf (type, "BIOS");
            break;

        case V32_PAGE_CART:
            sprintf (type, "CART");
            break;

        case V32_PAGE_MEMC:
            sprintf (type, "MEMC");
            break;

        default:
            type[0]            = '\0';
            if (sys_force     == FALSE)
            {
                fprintf (verbose, "[ERROR] invalid memory %s at 0x%.8X\n",
                                  action, address);
                sys_error      = error;
            }
            result             = FALSE;
            break;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // report the error, set system error
    //
    if (result                == TRUE)
    {
        attr                   = (memory+page) -> flag;
        if ((address          >  (memory+page) -> last_addr) ||
            ((sys_force       == FALSE) &&
             (flag            != (flag & attr))))
        {
            if (sys_force     == FALSE)
            {
                fprintf (verbose, "[ERROR] invalid %s %s at 0x%.8X\n",
                                  type, action, address);
                sys_error      = error;
            }
            result             = FALSE;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // memory not loaded (so any access would be an error)
    //
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
    word_t   *wptr                 = NULL;

    check                          = memory_chk (address, FLAG_READ, sys_force);
    if (check                     == TRUE)
    {
        dptr                       = (memory+page)  -> data;
        wptr                       = &(dptr+offset) -> value;
    }

    return (wptr);
}

void    memory_set (uint32_t  address, uint32_t  dataword, uint8_t  sys_force)
{
    data_t   *dptr                  = NULL;
    uint32_t  page                  = (address & 0xF0000000) >> 28;
    uint32_t  offset                = (address & 0x0FFFFFFF);
    uint8_t   check                 = TRUE;

    check                           = memory_chk (address, FLAG_WRITE, sys_force);
    if (check                      == TRUE)
    {
        dptr                        = (memory+page)  -> data;
        (dptr+offset) -> value.i32  = dataword;
    }
}

void    fmemory_set (uint32_t  address, float  dataword, uint8_t  sys_force)
{
    data_t   *dptr                  = NULL;
    uint32_t  page                  = (address & 0xF0000000) >> 28;
    uint32_t  offset                = (address & 0x0FFFFFFF);
    uint8_t   check                 = TRUE;

    check                           = memory_chk (address, FLAG_WRITE, sys_force);
    if (check                      == TRUE)
    {
        dptr                        = (memory+page)  -> data;
        (dptr+offset) -> value.f32  = dataword;
    }
}
