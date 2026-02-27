#include "defines.h"

display_l *newdispnode  (uint8_t  type, word_t *list, uint8_t  num)
{
    display_l *newnode          = (display_l *) malloc (sizeof (display_l) * 1);
    if (newnode                == NULL)
    {
        fprintf (stderr, "[ERROR] Could not allocate memory for displaylist node!\n");
        exit (LIST_ALLOC_FAIL);
    }

	newnode -> label            = NULL;
	newnode -> addr             = 0;
    newnode -> type             = type;
    newnode -> num              = num;
    newnode -> list             = list;
    newnode -> next             = NULL;

    return (newnode);
}

display_l *display_add  (display_l *list, display_l *node)
{
    display_l *tmp              = NULL;

    if (node                   != NULL)
    {
        if (list               != NULL)
        {
            tmp                 = list;
            while (tmp -> next != NULL)
            {
                tmp             = tmp -> next;
            }

            tmp -> next         = node;
        }
        else
        {
            list                = node;
        }
    }
    return (list);
}

void       displayshow  (display_l *list, uint8_t    flag)
{
    display_l *dtmp             = NULL;
    int32_t    index            = 0;
    uint32_t   count            = 0;
    uint32_t   value            = 0;
    word_t    *wtmp             = NULL;
    int8_t     entry[16];

    dtmp                        = list;
    while (dtmp                != NULL)
    {
        fprintf (stdout, "[%2u]  ", count);
        wtmp                    = dtmp -> list;
        switch (dtmp -> type)
        {
            case LIST_REG:

                value           = wtmp -> i32;
                switch (value)
                {
                    case CR:
                        fprintf (stdout, "%10s: ", "R11(CR)");
                        break;

                    case SR:
                        fprintf (stdout, "%10s: ", "R12(SR)");
                        break;

                    case DR:
                        fprintf (stdout, "%10s: ", "R13(DR)");
                        break;

                    case BP:
                        fprintf (stdout, "%10s: ", "R14(BP)");
                        break;

                    case SP:
                        fprintf (stdout, "%10s: ", "R15(SP)");
                        break;

                    default:
                        sprintf (entry,  "R%u",    value);
                        fprintf (stdout, "%10s: ", entry);
                        break;
                }
                fprintf (stdout, "0x%.8X\n", REG(value));
                break;

            case LIST_MEM:
                for (index      = 0;
                     index     <  list -> num;
                     index      = index + 1)
                {
                    sys_force  == TRUE;
                    value       = word2int (memory_get ((wtmp+index) -> i32));
					if (dtmp -> label != NULL)
					{
						fprintf (stdout, "0x%.8X: 0x%.8X \"%s\"\n", (wtmp+index) -> i32, value, dtmp -> label);
					}
					else
					{
						fprintf (stdout, "0x%.8X: 0x%.8X\n", (wtmp+index) -> i32, value);
					}
                }
                break;

            case LIST_IOP:
                for (index      = 0;
                     index     <  list -> num;
                     index      = index + 1)
                {
                    sys_force  == TRUE;
                    value       = ioports_get ((wtmp+index) -> i32);
                    fprintf (stdout, "[0x%.3X] 0x%.8X\n", (wtmp+index) -> i32, value);
                }
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
