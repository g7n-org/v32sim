#include "defines.h"

void  output_reg (uint8_t  id, uint8_t  fmt, uint8_t  flag, uint8_t *label)
{
    int32_t   check          = TRUE;
    int32_t   index          = 0;
    uint32_t  data           = 0;
    uint8_t   entry[64];
    uint8_t   escape         = 0;

    if (flag                == TRUE)
    {
        if (colorflag       == TRUE)
        {
            sprintf (entry, "\e[m\e[0;31m[\e[m\e[0;34m%s(0x%.8X)\e[m\e[0;31m]",
                    REGNAME(id), REG(id));
            escape           = 24;
        }
        else
        {
            sprintf (entry, "[%s(0x%.8X)]", REGNAME(id), REG(id));
        }
        check                = memory_chk (REG(id), TRUE);
    }
    else
    {
        if (colorflag       == TRUE)
        {
            sprintf (entry, "\e[m\e[0;34m%s", REGNAME(id));
            escape           = 12;
        }
        else
        {
            sprintf (entry, "%s", REGNAME(id));
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Recalibrate spacing if needed (ie encounter a wider displaylist member)
    //
    if (dpoint -> space     <  (strlen (entry) - escape))
    {
        dpoint -> space      = strlen (entry) - escape;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Display the display list header
    //
    fprintf (stdout, "%*s", dpoint -> space, entry);

    if (check               == FALSE)
    {
        if (colorflag       == TRUE)
        {
            fprintf (stdout, "\e[m: \e[1;37m<\e[m\e[1;31minvalid address\e[m\e[1;37m>\e[m");
        }
        else
        {
            fprintf (stdout, ": <invalid address>");
        }
    }
    else
    {
        switch (fmt)
        {
            case FORMAT_DEFAULT:
                if (flag    == TRUE)
                {
                    if (colorflag == TRUE)
                    {
                        fprintf (stdout, "\e[m: \e[1;36m0x%.8X",  IMEMGET(REG(id)));
                    }
                    else
                    {
                        fprintf (stdout, ": 0x%.8X",              IMEMGET(REG(id)));
                    }
                }
                else
                {
                    if (colorflag == TRUE)
                    {
                        fprintf (stdout, "\e[m: \e[1;36m0x%.8X",  REG(id));
                    }
                    else
                    {
                        fprintf (stdout, ": 0x%.8X",              REG(id));
                    }
                }
                break;

            case FORMAT_HEX:
                if (flag    == TRUE)
                {
                    if (colorflag == TRUE)
                    {
                        fprintf (stdout, "/X\e[m: \e[1;36m0x%.8X",  IMEMGET(REG(id)));
                    }
                    else
                    {
                        fprintf (stdout, "/X: 0x%.8X",              IMEMGET(REG(id)));
                    }
                }
                else
                {
                    if (colorflag == TRUE)
                    {
                        fprintf (stdout, "/X\e[m: \e[1;36m0x%.8X",  REG(id));
                    }
                    else
                    {
                        fprintf (stdout, "/X: 0x%.8X",              REG(id));
                    }
                }
                break;

            case FORMAT_LOWERHEX:
                if (flag    == TRUE)
                {
                    if (colorflag == TRUE)
                    {
                        fprintf (stdout, "/x\e[m: \e[1;36m0x%.8X",  IMEMGET(REG(id)));
                    }
                    else
                    {
                        fprintf (stdout, "/x: 0x%.8X",              IMEMGET(REG(id)));
                    }
                }
                else
                {
                    if (colorflag == TRUE)
                    {
                        fprintf (stdout, "/x\e[m: \e[1;36m0x%.8x",  REG(id));
                    }
                    else
                    {
                        fprintf (stdout, "/x: 0x%.8x",              REG(id));
                    }
                }
                break;

            case FORMAT_UNSIGNED:
                if (flag    == TRUE)
                {
                    if (colorflag == TRUE)
                    {
                        fprintf (stdout, "/u\e[m: \e[1;36m%u",      IMEMGET(REG(id)));
                    }
                    else
                    {
                        fprintf (stdout, "/u: %u",                  IMEMGET(REG(id)));
                    }
                }
                else
                {
                    if (colorflag == TRUE)
                    {
                        fprintf (stdout, "/u\e[m: \e[1;36m%u",      REG(id));
                    }
                    else
                    {
                        fprintf (stdout, "/u: %u",                  REG(id));
                    }
                }
                break;

            case FORMAT_OCTAL:
                if (flag    == TRUE)
                {
                    if (colorflag == TRUE)
                    {
                        fprintf (stdout, "/o\e[m: \e[1;36m0%o",     IMEMGET(REG(id)));
                    }
                    else
                    {
                        fprintf (stdout, "/o: 0%o",                 IMEMGET(REG(id)));
                    }
                }
                else
                {
                    if (colorflag == TRUE)
                    {
                        fprintf (stdout, "/o\e[m: \e[1;36m0%o",     REG(id));
                    }
                    else
                    {
                        fprintf (stdout, "/o: 0%o",                 REG(id));
                    }
                }
                break;

            case FORMAT_FLOAT:
                if (flag    == TRUE)
                {
                    if (colorflag == TRUE)
                    {
                        fprintf (stdout, "/f\e[m: \e[1;36m%.2f",    FMEMGET(REG(id)));
                    }
                    else
                    {
                        fprintf (stdout, "/f: %.2f",                FMEMGET(REG(id)));
                    }
                }
                else
                {
                    if (colorflag == TRUE)
                    {
                        fprintf (stdout, "/f\e[m: \e[1;36m%.2f",    FREG(id));
                    }
                    else
                    {
                        fprintf (stdout, "/f: %.2f",                FREG(id));
                    }
                }
                break;

            case FORMAT_SIGNED:
                if (flag    == TRUE)
                {
                    if (colorflag == TRUE)
                    {
                        fprintf (stdout, "/d\e[m: \e[1;36m%d",      IMEMGET(REG(id)));
                    }
                    else
                    {
                        fprintf (stdout, "/d: %d",                  IMEMGET(REG(id)));
                    }
                }
                else
                {
                    if (colorflag == TRUE)
                    {
                        fprintf (stdout, "/d\e[m: \e[1;36m%d",      REG(id));
                    }
                    else
                    {
                        fprintf (stdout, "/d: %d",                  REG(id));
                    }
                }
                break;

            case FORMAT_BINARY:
                if (flag          == TRUE)
                {
                    data           = IMEMGET(REG(id));
                    if (colorflag == TRUE)
                    {
                        fprintf (stdout, "/b\e[m: \e[1;36m");
                    }
                    else
                    {
                        fprintf (stdout, "/b: ");
                    }
                }
                else
                {
                    data           = REG(id);
                    if (colorflag == TRUE)
                    {
                        fprintf (stdout, "/b\e[m: \e[1;36m");
                    }
                    else
                    {
                        fprintf (stdout, "/b: ");
                    }
                }

                for (index         = 31;
                     index        >= 0;
                     index         = index - 1)
                {
                    entry[index]   = (data % 2) + 0x30;
                    data           = data / 2;
                }
                entry[32]          = '\0';
                fprintf (stdout, "%s", entry);
                break;
        }
        if (colorflag       == TRUE)
        {
            fprintf (stdout, "\e[m");
        }
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
    int32_t   check          = TRUE;
    int32_t   index          = 0;
    uint32_t  data           = 0;
    uint8_t   entry[64];
    uint8_t   escape         = 0;

    if (flag                == TRUE)
    {
        data                 = ISYSMEMGET(value);
        if (colorflag       == TRUE)
        {
            sprintf (entry, "\e[m\e[0;31m[\e[m\e[0;34m0x%.8X(0x%.8X)\e[0;31m]\e[m",
                    value, data);
            escape           = 17;
        }
        else
        {
            sprintf (entry, "[0x%.8X(0x%.8X)]", value, data);
        }
        check                = memory_chk (value, TRUE) & memory_chk (data, TRUE);
    }
    else
    {
        if (colorflag       == TRUE)
        {
            sprintf (entry, "\e[m\e[0;34m0x%.8X\e[m", value);
            escape           = 8;
        }
        else
        {
            sprintf (entry, "0x%.8X", value);
        }
        check                = memory_chk (value, TRUE);
    }

    if (dpoint -> space     <  (strlen (entry) - escape))
    {
        dpoint -> space      = strlen (entry) - escape;
    }

    fprintf (stdout, "%*s: ", dpoint -> space, entry);

    if (check               == FALSE)
    {
        if (colorflag       == TRUE)
        {
            fprintf (stdout, "\e[1;37m<\e[m\e[1;31minvalid address\e[m\e[1;37m>\e[m");
        }
        else
        {
            fprintf (stdout, "<invalid address>");
        }
    }
    else
    {
        if (colorflag       == TRUE)
        {
            fprintf (stdout, "\e[1;36m");
        }
        switch (fmt)
        {
            case FORMAT_HEX:
                if (flag    == TRUE)
                {
                    fprintf (stdout, "0x%.8X",
                            IMEMGET(IMEMGET(value)));
                }
                else
                {
                    fprintf (stdout, "0x%.8X",
                            IMEMGET(value));
                }
                break;

            case FORMAT_LOWERHEX:
                if (flag    == TRUE)
                {
                    fprintf (stdout, "0x%.8x",
                            IMEMGET(IMEMGET(value)));
                }
                else
                {
                    fprintf (stdout, "0x%.8x",
                            IMEMGET(value));
                }
                break;

            case FORMAT_UNSIGNED:
                if (flag    == TRUE)
                {
                    fprintf (stdout, "%u",
                            IMEMGET(IMEMGET(value)));
                }
                else
                {
                    fprintf (stdout, "%u",
                            IMEMGET(value));
                }
                break;

            case FORMAT_OCTAL:
                if (flag    == TRUE)
                {
                    fprintf (stdout, "0%o",
                            IMEMGET(IMEMGET(value)));
                }
                else
                {
                    fprintf (stdout, "0%o",
                            IMEMGET(value));
                }
                break;

            case FORMAT_FLOAT:
                if (flag    == TRUE)
                {
                    fprintf (stdout, "%.2f",
                            FMEMGET(IMEMGET(value)));
                }
                else
                {
                    fprintf (stdout, "%.2f",
                            FMEMGET(value));
                }
                break;

            case FORMAT_SIGNED:
                if (flag    == TRUE)
                {
                    fprintf (stdout, "%d",
                            IMEMGET(IMEMGET(value)));
                }
                else
                {
                    fprintf (stdout, "%d",
                            IMEMGET(value));
                }
                break;

            case FORMAT_BINARY:
                if (flag    == TRUE)
                {
                    data     = IMEMGET(IMEMGET(value));
                }
                else
                {
                    data     = IMEMGET(value);
                }

                for (index   = 0;
                     index  <  32;
                     index   = index + 1)
                {
                    fprintf (stdout, "%u", (data % 2));
                    data     = data / 2;
                }
                break;
        }
        if (colorflag       == TRUE)
        {
            fprintf (stdout, "\e[m");
        }
    }

    if (label               != NULL)
    {
        if (colorflag       == TRUE)
        {
            fprintf (stdout, "\e[0;33m");
        }
        fprintf (stdout, " \"%s\"", label);
        if (colorflag       == TRUE)
        {
            fprintf (stdout, "\e[m");
        }
    }
    fprintf (stdout, "\n");
}
