#include "defines.h"

void  output_reg (uint8_t  id, uint8_t  fmt, uint8_t  flag, uint8_t *label)
{
    uint8_t  entry[64];
	uint8_t  escape      = 0;

    if (flag            == TRUE)
    {
        sprintf (entry, "\e[1;31m[\e[m\e[0;34m%s(0x%.8X)\e[1;31m]\e[m",
                REGNAME(id), REG(id));
		escape           = 20;
    }
    else
    {
        sprintf (entry, "\e[0;34m%s",
                  REGNAME(id));
		escape           = 8;
    }

    if (strlen (entry) - escape >  dpoint -> space)
    {
        dpoint -> space  = strlen (entry) - escape;
    }

    fprintf (stdout, "%*s: ", dpoint -> space, entry);

    if (colorflag       == TRUE)
    {
        fprintf (stdout, "\e[1;36m");
    }
    switch (fmt)
    {
        case FORMAT_HEX:
            if (flag    == TRUE)
            {
                fprintf (stdout, "0x%.8X",  IMEMGET(REG(id)));
            }
            else
            {
                fprintf (stdout, "0x%.8X",  REG(id));
            }
            break;

        case FORMAT_LOWERHEX:
            if (flag    == TRUE)
            {
                fprintf (stdout, "0x%.8x",  IMEMGET(REG(id)));
            }
            else
            {
                fprintf (stdout, "0x%.8x",  REG(id));
            }
            break;

        case FORMAT_UNSIGNED:
            if (flag    == TRUE)
            {
                fprintf (stdout, "%u",      IMEMGET(REG(id)));
            }
            else
            {
                fprintf (stdout, "%u",      REG(id));
            }
            break;

        case FORMAT_OCTAL:
            if (flag    == TRUE)
            {
                fprintf (stdout, "0%o",     IMEMGET(REG(id)));
            }
            else
            {
                fprintf (stdout, "0%o",     REG(id));
            }
            break;

        case FORMAT_FLOAT:
            if (flag    == TRUE)
            {
                fprintf (stdout, "%.2f",    FMEMGET(REG(id)));
            }
            else
            {
                fprintf (stdout, "%.2f",    FREG(id));
            }
            break;

        case FORMAT_SIGNED:
            if (flag    == TRUE)
            {
                fprintf (stdout, "%d",      IMEMGET(REG(id)));
            }
            else
            {
                fprintf (stdout, "%d",      REG(id));
            }
            break;

        case FORMAT_BINARY:
            if (flag    == TRUE)
            {
                fprintf (stdout, "%X (not yet)",   IMEMGET(REG(id)));
            }
            else
            {
                fprintf (stdout, "%X (not yet)", REG(id));
            }
            break;
    }
    if (colorflag       == TRUE)
    {
        fprintf (stdout, "\e[m");
    }

    if (label           != NULL)
    {
        if (colorflag   == TRUE)
        {
            fprintf (stdout, "\e[0;33m");
        }
        fprintf (stdout, " \"%s\"", label);
        if (colorflag   == TRUE)
        {
            fprintf (stdout, "\e[m");
        }
    }
    fprintf (stdout, "\n");
}

void  output_mem (uint32_t  value, uint8_t  fmt,  uint8_t  flag, uint8_t *label)
{
    if (fmt      == FORMAT_HEX)
    {
        if (flag == TRUE)
        {
            fprintf (stdout, "[0x%.8X(0x%.8X)]: 0x%.8X\n",
                    value, IMEMGET(value),
                    IMEMGET(IMEMGET(value)));
        }
        else
        {
            fprintf (stdout, "[0x%.8X]: 0x%.8X\n",
                    value, IMEMGET (value));
        }
    }
    else if (fmt == FORMAT_LOWERHEX)
    {
        if (flag == TRUE)
        {
            fprintf (stdout, "[0x%.8X(0x%.8X)]: 0x%.8x\n",
                    value, IMEMGET(value),
                    IMEMGET(IMEMGET(value)));
        }
        else
        {
            fprintf (stdout, "[0x%.8X]: 0x%.8x\n",
                    value, IMEMGET (value));
        }
    }
    else if (fmt == FORMAT_UNSIGNED)
    {
        if (flag == TRUE)
        {
            fprintf (stdout, "[0x%.8X(0x%.8X)]: %u\n",
                    value, IMEMGET(value),
                    IMEMGET(IMEMGET(value)));
        }
        else
        {
            fprintf (stdout, "[0x%.8X]: %u\n",
                    value, IMEMGET (value));
        }
    }
    else if (fmt == FORMAT_OCTAL)
    {
        if (flag == TRUE)
        {
            fprintf (stdout, "[0x%.8X(0x%.8X)]: 0%o\n",
                    value, IMEMGET(value),
                    IMEMGET(IMEMGET(value)));
        }
        else
        {
            fprintf (stdout, "[0x%.8X]: 0%o\n",
                    value, IMEMGET (value));
        }
    }
    else if (fmt == FORMAT_FLOAT)
    {
        if (flag == TRUE)
        {
            fprintf (stdout, "[0x%.8X(0x%.8X)]: %.2f\n",
                    value, IMEMGET(value),
                    FMEMGET(IMEMGET(value)));
        }
        else
        {
            fprintf (stdout, "[0x%.8X]: %.2f\n",
                    value, FMEMGET (value));
        }
    }
    else if (fmt == FORMAT_SIGNED)
    {
        if (flag == TRUE)
        {
            fprintf (stdout, "[0x%.8X(0x%.8X)]: %d\n",
                    value, IMEMGET(value),
                    IMEMGET(IMEMGET(value)));
        }
        else
        {
            fprintf (stdout, "[0x%.8X]: %d\n",
                    value, IMEMGET (value));
        }
    }
    else if (fmt == FORMAT_BINARY)
    {
        if (flag == TRUE)
        {
            fprintf (stdout, "[0x%.8X(0x%.8X)]: 0x%.8X (binary not yet implemented)\n",
                    value, IMEMGET(value),
                    IMEMGET(IMEMGET(value)));
        }
        else
        {
            fprintf (stdout, "[0x%.8X]: 0x%.8x (binary not yet implemented)\n",
                    value, IMEMGET (value));
        }
    }
}
