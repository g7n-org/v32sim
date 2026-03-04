#include "defines.h"

display_l *newdispnode  (uint8_t  type, uint32_t  value)
{
    display_l *newnode          = (display_l *) malloc (sizeof (display_l) * 1);
    if (newnode                == NULL)
    {
        fprintf (stderr, "[ERROR] Could not allocate memory for displaylist node!\n");
        exit (LIST_ALLOC_FAIL);
    }

    newnode -> label            = NULL;
    newnode -> type             = type;
    newnode -> list             = (word_t *) malloc (sizeof (word_t) * 1);
    newnode -> list -> raw      = value;
    newnode -> next             = NULL;

    fprintf (verbose, "[newdispnode] type: %hhu, value: %u\n", newnode -> type, value);

    return (newnode);
}

display_l *display_add  (display_l *list, display_l *node)
{
    display_l *tmp                       = NULL;
    //int32_t    check                     = 0;

    if (node                            != NULL)
    {
        if (list                        != NULL)
        {
            tmp                          = list;
            while (tmp -> next          != NULL)
            {
                /*
                if (list -> type        == node -> type)
                {
                    check                = memcmp (list -> list, node -> list, sizeof (word_t));
                    if (check           == 0)
                    {
                        if (list -> num == node -> num)
                        {
                            break; // identical node found, do not add
                        }
                    }
                }
                */
                tmp                      = tmp -> next;
            }

            tmp -> next                  = node;
        }
        else
        {
            list                         = node;
        }
    }
    return (list);
}

void       displayshow  (display_l *list, uint8_t    flag)
{
    display_l *dtmp                = NULL;
    uint32_t   count               = 0;
    uint32_t   value               = 0;
    word_t    *wtmp                = NULL;
    int8_t     entry[24];
    uint8_t    check               = FALSE;

    dtmp                           = list;
    while (dtmp                   != NULL)
    {
        fprintf (stdout, "[%2u]  ", count);
        wtmp                       = dtmp -> list;
        switch (dtmp -> type)
        {
            case LIST_REG_DEREF:
                value              = wtmp -> i32;
                sprintf (entry,  "[R%u>%.8X)]",  value, REG(value));
                fprintf (stdout, "%19s: ", entry);

                check              = memory_chk (REG(value));
                if (dtmp -> label != NULL)
                {
                    if (check     == TRUE)
                    {
                        fprintf (stdout, "0x%.8X \"%s\"\n",
                                IMEMGET(REG(value)), dtmp -> label);
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
                        fprintf (stdout, "0x%.8X\n",        IMEMGET(REG(value)));
                    }
                    else
                    {
                        fprintf (stdout, "<invalid address>\n");
                    }
                }
                break;

            case LIST_REG:
                value              = wtmp -> i32;
                switch (value)
                {
                    case CR:
                        fprintf (stdout, "%19s: ", "R11(CR)");
                        break;

                    case SR:
                        fprintf (stdout, "%19s: ", "R12(SR)");
                        break;

                    case DR:
                        fprintf (stdout, "%19s: ", "R13(DR)");
                        break;

                    case BP:
                        fprintf (stdout, "%19s: ", "R14(BP)");
                        break;

                    case SP:
                        fprintf (stdout, "%19s: ", "R15(SP)");
                        break;

                    default:
                        sprintf (entry,  "R%u",    value);
                        fprintf (stdout, "%19s: ", entry);
                        break;
                }

                if (dtmp -> label != NULL)
                {
                    fprintf (stdout, "0x%.8X \"%s\"\n", REG(value), dtmp -> label);
                }
                else
                {
                    fprintf (stdout, "0x%.8X\n",        REG(value));
                }
                break;

            case LIST_MEM_DEREF:
                sys_force          = TRUE;
                value              = IMEMGET (wtmp -> i32);
                check              = memory_chk (value);
                sprintf (entry, "[%.8X>%.8X]", wtmp -> i32, IMEMGET(value));
                fprintf (stdout, "%19s: ", entry);
                if (dtmp -> label != NULL)
                {
                    if (check     == TRUE)
                    {
                        fprintf (stdout, "0x%.8X \"%s\"\n",
                                IMEMGET(IMEMGET(value)), dtmp -> label);
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
                                IMEMGET(IMEMGET(value)));
                    }
                    else
                    {
                        fprintf (stdout, "<invalid address>\n");
                    }
                }
                break;

            case LIST_MEM:
                sys_force          = TRUE;
                value              = IMEMGET (wtmp -> i32);
                if (dtmp -> label != NULL)
                {
                    fprintf (stdout, "0x%.8X: 0x%.8X \"%s\"\n",
                            wtmp -> i32, value, dtmp -> label);
                }
                else
                {
                    fprintf (stdout, "0x%.8X: 0x%.8X\n", wtmp -> i32, value);
                }
                break;

            case LIST_IOP:
                sys_force          = TRUE;
                value              = ioports_get (wtmp -> i32);
                fprintf (stdout, "[0x%.3X] 0x%.8X\n", wtmp -> i32, value);
                break;
        }

        count                   = count + 1;
        dtmp                    = dtmp -> next;
    }
}

void  show_sysregs (void)
{
    int32_t  index  = 0;
    for (index      = IP;
         index     <= IV;
         index      = index + 1)
    {
        switch (index)
        {
            case IP:
                fprintf (stdout, "%16s: ", "IP");
                break;

            case IR:
                fprintf (stdout, "%16s: ", "IR");
                break;

            case IV:
                fprintf (stdout, "%16s: ", "IV");
                break;
        }
        fprintf (stdout, "0x%.8X\n", REG(index));
    }
}
