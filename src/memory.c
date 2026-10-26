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
                (memory+page) -> size       = 0;     // sized by load_memory() from the ROM header
                (memory+page) -> flag       = FLAG_READ;
                (memory+page) -> data       = NULL;
                break;

            case V32_PAGE_CART:
                (memory+page) -> type       = V32_PAGE_CART;
                (memory+page) -> firstaddr  = CART_FIRST_ADDR;
                (memory+page) -> size       = 0;
                (memory+page) -> flag       = FLAG_READ;
                (memory+page) -> data       = NULL;
                break;

            case V32_PAGE_MEMC:
                (memory+page) -> type       = V32_PAGE_MEMC;
                (memory+page) -> firstaddr  = MEMC_FIRST_ADDR;
                (memory+page) -> size       = (get_filesize (memcfile) > 0) ? MEMC_WORDS : 0;
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
        len                                 = (memory+page) -> size + (memory+page) -> extra;

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
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // The memory bus decodes only bits 28-29 of an address as the device and the low 28
    // bits as the address inside it (V32MemoryBus): bits 30 and 31 are ignored, so
    // 0x40000000 is RAM, 0x90000000 is the BIOS, 0xE0000000 the cartridge and -1 is
    // the last word of the memory card's range. There is no "invalid page"; an access
    // is refused only by the device itself (outside its size, or a write to ROM)
    //
    uint32_t  page             = (address & V32_PAGE_MASK) >> 28;
    uint32_t  offset           = (address & 0x0FFFFFFF);
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

        default:    // V32_PAGE_MEMC
            sprintf (type, "MEMC");
            break;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // report the error, set system error
    //
    attr                       = (memory+page) -> flag;
    if ((offset               >= (memory+page) -> size) ||
        ((sys_force           == FALSE) &&
         (flag                != (flag & attr))))
    {
        if (sys_force         == FALSE)
        {
            fprintf (verbose, "[ERROR] invalid %s %s at 0x%.8X\n",
                              type, action, address);
            sys_error          = error;
        }
        result                 = FALSE;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // memory not loaded (so any access would be an error)
    //
    if ((memory+page) -> data == NULL)
    {
        if ((result           == TRUE) &&
            (sys_force        == FALSE))
        {
            sys_error          = error;
        }
        result                 = FALSE;
    }

    return (result);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// memory_texel(): a pixel of a ROM texture. Pixels are kept after the program ROM in the
// page's storage but are not part of the CPU-visible memory (the program ROM alone is
// what the console maps), so they are reached by this, not by memory_get()
//
word_t *memory_texel (uint32_t  address)
{
    uint32_t  page   = (address & V32_PAGE_MASK) >> 28;
    uint32_t  offset = (address & 0x0FFFFFFF);

    if ((memory+page) -> data == NULL ||
        offset >= (memory+page) -> size + (memory+page) -> extra)
    {
        return (NULL);
    }
    return (&(((memory+page) -> data) + offset) -> value);
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
    uint32_t  page                 = (address & V32_PAGE_MASK) >> 28;
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
    uint32_t  page                  = (address & V32_PAGE_MASK) >> 28;
    uint32_t  offset                = (address & 0x0FFFFFFF);
    uint8_t   check                 = TRUE;

    check                           = memory_chk (address, FLAG_WRITE, sys_force);
    if (check                      == TRUE)
    {
        dptr                        = (memory+page)  -> data;
        (dptr+offset) -> value.i32  = dataword;
        if (page                   == V32_PAGE_MEMC)
        {
            memc_touch (offset);      // the card file is saved at the end of the frame
        }
    }
}

void    fmemory_set (uint32_t  address, float  dataword, uint8_t  sys_force)
{
    data_t   *dptr                  = NULL;
    uint32_t  page                  = (address & V32_PAGE_MASK) >> 28;
    uint32_t  offset                = (address & 0x0FFFFFFF);
    uint8_t   check                 = TRUE;

    check                           = memory_chk (address, FLAG_WRITE, sys_force);
    if (check                      == TRUE)
    {
        dptr                        = (memory+page)  -> data;
        (dptr+offset) -> value.f32  = dataword;
        if (page                   == V32_PAGE_MEMC)
        {
            memc_touch (offset);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// dump_words(): --dump=N[@ADDR|@LABEL]; print N words of memory, one signed
// integer per line, after the run (what tools/vrun -d does). A label that is not
// found as given is also tried as global_LABEL (v32cc's name for a C variable).
//
void  dump_words (int8_t *spec)
{
    char      buf[256];
    char      alt[300];
    char     *at                    = NULL;
    char     *end                   = NULL;
    uint32_t  count                 = 0;
    uint32_t  address               = RAM_FIRST_ADDR;
    uint32_t  idx                   = 0;
    word_t   *wptr                  = NULL;

    strncpy (buf, (char *) spec, sizeof (buf) - 1);
    buf[sizeof (buf) - 1]           = '\0';
    at                              = strchr (buf, '@');
    if ((at == NULL) && ((at = strchr (buf, ':')) != NULL))
    {
        // ADDR:COUNT form: swap into COUNT@ADDR
        *at                         = '\0';
        snprintf (alt, sizeof (alt), "%s@%s", at + 1, buf);
        strncpy (buf, alt, sizeof (buf) - 1);
        at                          = strchr (buf, '@');
    }
    if (at                         != NULL)
    {
        *at                         = '\0';
        at                          = at + 1;
    }
    count                           = (uint32_t) strtoul (buf, &end, 0);
    if ((end == buf) || (*end != '\0'))
    {
        fprintf (stderr, "[ERROR] --dump: expected N[@ADDR|@LABEL]\n");
        return;
    }
    if (at                         != NULL)
    {
        address                     = (uint32_t) strtoul (at, &end, 16);
        if ((end == at) || (*end != '\0'))
        {
            if (sym_lookup (at, &address, NULL) == FALSE)    // a label, or a C global (also as global_NAME)
            {
                fprintf (stderr, "[ERROR] --dump: unknown address or symbol '%s'\n", at);
                return;
            }
        }
    }
    for (idx = 0; idx < count; idx = idx + 1)
    {
        wptr                        = memory_get (address + idx, TRUE);
        if (wptr == NULL)
        {
            break;
        }
        fprintf (stdout, "%d\n", (int) wptr -> i32);
    }
}
