#include "defines.h"

void  output_reg (uint8_t  id, uint8_t  fmt, uint8_t  flag, uint8_t *label)
{
    int32_t   check               = TRUE;
    int32_t   index               = 0;
    uint32_t  data                = 0;
    uint8_t   addr[19];
    uint8_t   entry[33];

    if (flag                     == TRUE)
    {
        if (REGALIAS(id)         != NULL)
        {
            sprintf (addr, "[%s(%s)>0x%.8X]", REGNAME(id), REGALIAS(id), REG(id));
        }
        else
        {
            sprintf (addr, "[%s>0x%.8X]", REGNAME(id), REG(id));
        }
        check                     = memory_chk (REG(id), TRUE);
    }
    else
    {
        if (REGALIAS(id)         != NULL)
        {
            sprintf (addr, "%s(%s)", REGNAME(id), REGALIAS(id));
        }
        else
        {
            sprintf (addr, "%s", REGNAME(id));
        }
    }

    switch (fmt)
    {
        case FORMAT_HEX:
            sprintf (entry, "%s/X", addr);
            break;

        case FORMAT_LOWERHEX:
            sprintf (entry, "%s/x", addr);
            break;

        case FORMAT_UNSIGNED:
            sprintf (entry, "%s/u", addr);
            break;

        case FORMAT_OCTAL:
            sprintf (entry, "%s/o", addr);
            break;

        case FORMAT_FLOAT:
            sprintf (entry, "%s/f", addr);
            break;

        case FORMAT_SIGNED:
            sprintf (entry, "%s/d", addr);
            break;

        case FORMAT_BINARY:
            sprintf (entry, "%s/b", addr);
            break;

        default:
            sprintf (entry, "%s",   addr);
            break;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Recalibrate spacing if needed (ie encounter a wider displaylist member)
    //
    if (dpoint -> space          <  strlen (entry))
    {
        dpoint -> space           = strlen (entry);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Display the display list header
    //
    fprintf (stdout, "%*s: ", dpoint -> space, entry);

    if (colorflag                == TRUE)
    {
        fprintf (stdout, "\e[1;36m");
    }

    if (check                    == FALSE)
    {
        fprintf (stdout, "<invalid address>");
    }
    else
    {
        switch (fmt)
        {
            case FORMAT_DEFAULT:
            case FORMAT_HEX:
                if (flag         == TRUE)
                {
                    fprintf (stdout, "0x%.8X",   IMEMGET(REG(id)));
                }
                else
                {
                    fprintf (stdout, "0x%.8X",   REG(id));
                }
                break;

            case FORMAT_LOWERHEX:
                if (flag         == TRUE)
                {
                    fprintf (stdout, "0x%.8X", IMEMGET(REG(id)));
                }
                else
                {
                    fprintf (stdout, "0x%.8x", REG(id));
                }
                break;

            case FORMAT_UNSIGNED:
                if (flag         == TRUE)
                {
                    fprintf (stdout, "%u",     IMEMGET(REG(id)));
                }
                else
                {
                    fprintf (stdout, "%u",     REG(id));
                }
                break;

            case FORMAT_OCTAL:
                if (flag         == TRUE)
                {
                    fprintf (stdout, "0%o",    IMEMGET(REG(id)));
                }
                else
                {
                    fprintf (stdout, "0%o",    REG(id));
                }
                break;

            case FORMAT_FLOAT:
                if (flag         == TRUE)
                {
                    fprintf (stdout, "%.2f",   FMEMGET(REG(id)));
                }
                else
                {
                    fprintf (stdout, "%.2f",   FREG(id));
                }
                break;

            case FORMAT_SIGNED:
                if (flag         == TRUE)
                {
                    fprintf (stdout, "%d",     IMEMGET(REG(id)));
                }
                else
                {
                    fprintf (stdout, "%d",     REG(id));
                }
                break;

            case FORMAT_BINARY:
                if (flag         == TRUE)
                {
                    data          = IMEMGET(REG(id));
                }
                else
                {
                    data          = REG(id);
                }

                for (index        = 31;
                     index       >= 0;
                     index        = index - 1)
                {
                    entry[index]  = (data % 2) + 0x30;
                    data          = data / 2;
                }
                entry[32]         = '\0';
                fprintf (stdout, "%s", entry);
                break;
        }
    }

    if (label                    != NULL)
    {
        fprintf (stdout, " \"%s\"", label);
    }
    fprintf (stdout, "\n");
}

void  output_mem (uint32_t  value, uint8_t  fmt,  uint8_t  flag, uint8_t *label)
{
    int32_t   check          = TRUE;
    int32_t   index          = 0;
    uint32_t  data           = 0;
    uint8_t   addr[26];
    uint8_t   entry[33];

    if (flag                == TRUE)
    {
        data                 = ISYSMEMGET(value);
        sprintf (addr, "[0x%.8X(0x%.8X)]", value, data);
        check                = memory_chk (value, TRUE) & memory_chk (data, TRUE);
    }
    else
    {
        sprintf (addr, "0x%.8X", value);
        check                = memory_chk (value, TRUE);
    }

    switch (fmt)
    {
        case FORMAT_HEX:
            sprintf (entry, "%s/X", addr);
            break;

        case FORMAT_LOWERHEX:
            sprintf (entry, "%s/x", addr);
            break;

        case FORMAT_UNSIGNED:
            sprintf (entry, "%s/u", addr);
            break;

        case FORMAT_OCTAL:
            sprintf (entry, "%s/o", addr);
            break;

        case FORMAT_FLOAT:
            sprintf (entry, "%s/f", addr);
            break;

        case FORMAT_SIGNED:
            sprintf (entry, "%s/d", addr);
            break;

        case FORMAT_BINARY:
            sprintf (entry, "%s/b", addr);
            break;

        default:
            sprintf (entry, "%s",   addr);
            break;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Recalibrate spacing if needed (ie encounter a wider displaylist member)
    //
    if (dpoint -> space          <  strlen (entry))
    {
        dpoint -> space           = strlen (entry);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Display the display list header
    //
    fprintf (stdout, "%*s: ", dpoint -> space, entry);

    if (colorflag                == TRUE)
    {
        fprintf (stdout, "\e[1;36m");
    }

    if (check                    == FALSE)
    {
        fprintf (stdout, "<invalid address>");
    }
    else
    {
        switch (fmt)
        {
            case FORMAT_DEFAULT:
            case FORMAT_HEX:
                if (flag         == TRUE)
                {
                    fprintf (stdout, "0x%.8X", IMEMGET(IMEMGET(value)));
                }
                else
                {
                    fprintf (stdout, "0x%.8X", IMEMGET(value));
                }
                break;

            case FORMAT_LOWERHEX:
                if (flag         == TRUE)
                {
                    fprintf (stdout, "0x%.8x", IMEMGET(IMEMGET(value)));
                }
                else
                {
                    fprintf (stdout, "0x%.8x", IMEMGET(value));
                }
                break;

            case FORMAT_UNSIGNED:
                if (flag         == TRUE)
                {
                    fprintf (stdout, "%u",     IMEMGET(IMEMGET(value)));
                }
                else
                {
                    fprintf (stdout, "%u",     IMEMGET(value));
                }
                break;

            case FORMAT_OCTAL:
                if (flag         == TRUE)
                {
                    fprintf (stdout, "0%o",    IMEMGET(IMEMGET(value)));
                }
                else
                {
                    fprintf (stdout, "0%o",    IMEMGET(value));
                }
                break;

            case FORMAT_FLOAT:
                if (flag         == TRUE)
                {
                    fprintf (stdout, "%.2f",   FMEMGET(IMEMGET(value)));
                }
                else
                {
                    fprintf (stdout, "%.2f",   FMEMGET(value));
                }
                break;

            case FORMAT_SIGNED:
                if (flag         == TRUE)
                {
                    fprintf (stdout, "%d",     IMEMGET(IMEMGET(value)));
                }
                else
                {
                    fprintf (stdout, "%d",     IMEMGET(value));
                }
                break;

            case FORMAT_BINARY:
                if (flag         == TRUE)
                {
                    data          = IMEMGET(IMEMGET(value));
                }
                else
                {
                    data          = IMEMGET(value);
                }

                for (index        = 31;
                     index       >= 0;
                     index        = index - 1)
                {
                    entry[index]  = (data % 2) + 0x30;
                    data          = data / 2;
                }
                entry[32]         = '\0';
                fprintf (stdout, "%s", entry);
                break;
        }
    }

    if (label                    != NULL)
    {
        fprintf (stdout, " \"%s\"", label);
    }
    fprintf (stdout, "\n");
}

void  output_iop (uint32_t  value, uint8_t  fmt, uint8_t *label)
{
    int32_t   check               = TRUE;
    int32_t   index               = 0;
    uint32_t  data                = 0;
    uint8_t   addr[6];
    uint8_t   entry[33];

    sprintf (addr, "0x%.3X", value);
    check                         = ioports_chk (value, FLAG_READ, TRUE);

    switch (fmt)
    {
        case FORMAT_HEX:
            sprintf (entry, "%s/X", addr);
            break;

        case FORMAT_LOWERHEX:
            sprintf (entry, "%s/x", addr);
            break;

        case FORMAT_UNSIGNED:
            sprintf (entry, "%s/u", addr);
            break;

        case FORMAT_OCTAL:
            sprintf (entry, "%s/o", addr);
            break;

        case FORMAT_FLOAT:
            sprintf (entry, "%s/f", addr);
            break;

        case FORMAT_SIGNED:
            sprintf (entry, "%s/d", addr);
            break;

        case FORMAT_BINARY:
            sprintf (entry, "%s/b", addr);
            break;

        default:
            sprintf (entry, "%s",   addr);
            break;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Recalibrate spacing if needed (ie encounter a wider displaylist member)
    //
    if (dpoint -> space          <  strlen (entry))
    {
        dpoint -> space           = strlen (entry);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Display the display list header
    //
    fprintf (stdout, "%*s: ", dpoint -> space, entry);

    if (colorflag                == TRUE)
    {
        fprintf (stdout, "\e[1;36m");
    }

    if (check                    == FALSE)
    {
        fprintf (stdout, "<invalid port>");
    }
    else
    {
        switch (fmt)
        {
            case FORMAT_DEFAULT:
            case FORMAT_HEX:
                fprintf (stdout, "0x%.8X", ISYSPORTGET(value));
                break;

            case FORMAT_LOWERHEX:
                fprintf (stdout, "0x%.8x", ISYSPORTGET(value));
                break;

            case FORMAT_UNSIGNED:
                fprintf (stdout, "%u",     ISYSPORTGET(value));
                break;

            case FORMAT_OCTAL:
                fprintf (stdout, "0%o",    ISYSPORTGET(value));
                break;

            case FORMAT_FLOAT:
                fprintf (stdout, "%.2f",   FPORTGET(value));
                break;

            case FORMAT_SIGNED:
                fprintf (stdout, "%d",     ISYSPORTGET(value));
                break;

            case FORMAT_BINARY:
                data              = ISYSPORTGET(value);

                for (index        = 31;
                     index       >= 0;
                     index        = index - 1)
                {
                    entry[index]  = (data % 2) + 0x30;
                    data          = data / 2;
                }
                entry[32]         = '\0';
                fprintf (stdout, "%s", entry);
                break;
        }
    }

    if (label                    != NULL)
    {
        fprintf (stdout, " (%s)", label);
    }
    fprintf (stdout, "\n");
}
