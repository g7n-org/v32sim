#include "defines.h"

void       displayshow  (linked_l *list, uint8_t    flag)
{
    linked_l *dtmp                = NULL;
    uint32_t   count               = 0;
    uint32_t   value               = 0;
    word_t    *wtmp                = NULL;
    int8_t     entry[24];
    uint8_t    check               = FALSE;

    dtmp                           = list;
    while (dtmp                   != NULL)
    {
        if (colorflag             == TRUE)
        {
            fprintf (stdout, "\e[0;36m");
        }
        fprintf (stdout, "[%2u]  ", count);
        wtmp                       = &(dtmp -> data);
        switch (dtmp -> type)
        {
            case LIST_REG_DEREF:
                value              = wtmp -> i32;
                output_reg (value, dtmp -> fmt, TRUE, dtmp -> label);
                break;

            case LIST_REG:
                value              = wtmp -> i32;
                output_reg (value, dtmp -> fmt, FALSE, dtmp -> label);
                break;

            case LIST_MEM_DEREF:
                value              = ISYSMEMGET (wtmp -> i32);
                check              = memory_chk (value);
                output_mem (value, dtmp -> fmt, TRUE, dtmp -> label);
                /*
                sprintf (entry, "[%.8X>%.8X]", wtmp -> i32, value);
                if (list -> space <  strlen (entry))
                {
                    list -> space  = strlen (entry);
                }
                fprintf (stdout, "%*s: ", list -> space, entry);
                if (colorflag     == TRUE)
                {
                    fprintf (stdout, "\e[1;36m");
                }
                if (dtmp -> label != NULL)
                {
                    if (check     == TRUE)
                    {
                        fprintf (stdout, "0x%.8X \"%s\"\n",
                                IMEMGET(value), dtmp -> label);
                    }
                    else
                    {
                        fprintf (stdout, "<invalid address> \"%s\"\n",
                                dtmp -> label);
                    }
                }
                else
                {
                    if (check     == TRUE)
                    {
                        fprintf (stdout, "0x%.8X\n",
                                IMEMGET(value));
                    }
                    else
                    {
                        fprintf (stdout, "<invalid address>\n");
                    }
                }
                */
                if (colorflag     == TRUE)
                {
                    fprintf (stdout, "\e[m");
                }
                break;

            case LIST_MEM:
                value              = ISYSMEMGET (wtmp -> i32);
                output_mem (value, dtmp -> fmt, FALSE, dtmp -> label);
                /*
                sprintf (entry, "0x%.8X", wtmp -> i32);
                if (list -> space <  strlen (entry))
                {
                    list -> space  = strlen (entry);
                }
                fprintf (stdout, "%*s: ", list -> space, entry);

                if (colorflag     == TRUE)
                {
                    fprintf (stdout, "\e[1;36m");
                }
                if (dtmp -> label != NULL)
                {
                    fprintf (stdout, "0x%.8X \"%s\"\n", value, dtmp -> label);
                }
                else
                {
                    fprintf (stdout, "0x%.8X\n", value);
                }
                if (colorflag     == TRUE)
                {
                    fprintf (stdout, "\e[m");
                }
                */
                break;

            case LIST_IOP:
                value              = ioports_get (wtmp -> i32, TRUE);
                sprintf (entry, "[0x%.3X]", wtmp -> i32);
                fprintf (stdout, "%*s: ", list -> space, entry);
                if (colorflag     == TRUE)
                {
                    fprintf (stdout, "\e[1;36m");
                }
                fprintf (stdout, "0x%.8X\n", value);
                if (colorflag     == TRUE)
                {
                    fprintf (stdout, "\e[m");
                }
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
        fprintf (stdout, "%*s: ", (dpoint -> space + 6), REGNAME(index));
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
