#include "defines.h"

void  displayshow (linked_l *list, uint8_t  flag)
{
    linked_l *dtmp                 = NULL;
    uint32_t  count                = 0;
    word_t   *wtmp                 = NULL;

    dtmp                           = list;
    while (dtmp                   != NULL)
    {
        if (colorflag             == TRUE)
        {
            fprintf (stdout, "\e[0;36m");
        }
        fprintf (stdout, "[%2u] ", count);
        wtmp                       = &(dtmp -> data);
        switch (dtmp -> type)
        {
            case LIST_REG_DEREF:
                output_reg (wtmp -> i32, dtmp -> fmt, TRUE,  dtmp -> label);
                break;

            case LIST_REG:
                output_reg (wtmp -> i32, dtmp -> fmt, FALSE, dtmp -> label);
                break;

            case LIST_MEM_DEREF:
                output_mem (wtmp -> i32, dtmp -> fmt, TRUE,  dtmp -> label);
                break;

            case LIST_MEM:
                output_mem (wtmp -> i32, dtmp -> fmt, FALSE, dtmp -> label);
                break;

            case LIST_IOP:
                output_iop (wtmp -> i32, dtmp -> fmt, dtmp -> label);
                break;
        }

        count                      = count + 1;
        dtmp                       = dtmp -> next;

        if (colorflag             == TRUE)
        {
            fprintf (stdout, "\e[m");
        }
    }
}

void  show_sysregs (void)
{
    int32_t  index         = 0;
    for (index             = IP;
         index            <= IV;
         index             = index + 1)
    {
        if (colorflag     == TRUE)
        {
            fprintf (stdout, "\e[0;36m");
        }
        fprintf (stdout, "%*s: ", (dpoint -> space + 5), REGNAME(index));
        if (colorflag     == TRUE)
        {
            fprintf (stdout, "\e[m");
        }
        
        if (colorflag     == TRUE)
        {
            fprintf (stdout, "\e[1;36m");
        }
        fprintf (stdout, "0x%.8X\n", REG(index));
        if (colorflag     == TRUE)
        {
            fprintf (stdout, "\e[m");
        }
    }
}

void  display_config (uint8_t  item)
{
    int32_t   index  = 0;
    data_t   *dptr   = NULL;

    switch (item)
    {
        case INPUT_INVENTORY:
            fprintf (stdout, "v32sim system inventory\n");
            fprintf (stdout, "===================================\n");
            fprintf (stdout, "RAM:  %s\n", show_size (V32_PAGE_RAM));
            fprintf (stdout, "BIOS: %s\n", show_size (V32_PAGE_BIOS));
            fprintf (stdout, "  > VTEX #%d (%hux%hu) at 0x%.8X\n",
                     index,
                     (bios_vtex) -> wide,
                     (bios_vtex) -> high,
                     (bios_vtex) -> offset);
            fprintf (stdout, "CART: %s\n", show_size (V32_PAGE_CART));
            for (index       = 0;
                 index      <  (cart_vtex+0) -> qty;
                 index       = index + 1)
            {
                fprintf (stdout, "  > VTEX #%d (%hux%hu) at 0x%.8X\n",
                         index,
                         (cart_vtex+index) -> wide,
                         (cart_vtex+index) -> high,
                         (cart_vtex+index) -> offset);
            }
            fprintf (stdout, "MEMC: %s\n", show_size (V32_PAGE_MEMC));
            fprintf (stdout, "\n");
            break;

        case INPUT_GAMEPAD:
            fprintf (stdout, "v32sim gamepad\n");
            fprintf (stdout, "=============================\n");
            fprintf (stdout, "%22s: %d\n",
                             "INP_SelectedGamepad",
                             IPORTGET(INP_SelectedGamepad));
            fprintf (stdout, "%22s: %s\n",
                             "INP_GamepadConnected",
                             (IPORTGET(INP_GamepadConnected) == 1) ? "true" : "false");
            for (index       = INP_GamepadLeft;
                 index      <= INP_GamepadButtonR;
                 index       = index + 1)
            {
                dptr         = ioports_ptr ((uint16_t) index);
                fprintf (stdout, "%22s: %d\n",
                                 dptr -> name,
                                 IPORTGET((uint16_t) index));
            }
            fprintf (stdout, "\n");
            break;
    }
}

uint8_t *show_size (uint32_t  page)
{
    uint8_t *result    = NULL;
    size_t   size      = 0;

    if ((page         >= V32_PAGE_RAM) &&
        (page         <= V32_PAGE_MEMC))
    {
        size           = (memory+page) -> size;
        result         = (uint8_t *) malloc (sizeof (uint8_t) * 12);
        if (size      >= 1073741824)
        {
            sprintf (result, "%4luGiW",
                    (((size / 1024) / 1024) / 1024));
        }
        if (size      >= 1572864)
        {
            sprintf (result, "%4luMiW",
                    ((size  / 1024) / 1024));
        }
        else if (size >= 1024)
        {
            sprintf (result, "%4lukiW",
                    (size   / 1024));
        }
        else if (size >  0)
        {
            sprintf (result, "%4luW", size);
        }
        else
        {
            sprintf (result, "NOT LOADED");
        }
    }

    return (result);
}
