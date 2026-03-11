#include "defines.h"

void  output_reg (uint8_t  id, uint8_t  fmt, uint8_t  flag)
{
    if (fmt        == FORMAT_HEX)
    {
        if (flag   == TRUE)
        {
            fprintf (stdout, "[%3s(0x%.8X)]: 0x%.8X\n",
                    REGNAME(id), REG(id), IMEMGET(REG(id)));
        }
        else
        {
            fprintf (stdout, "%3s: 0x%.8X\n", REGNAME(id), REG(id));
        }
    }
    else if (fmt   == FORMAT_LOWERHEX)
    {
        if (flag   == TRUE)
        {
            fprintf (stdout, "[%3s(0x%.8X)]: 0x%.8x\n",
                    REGNAME(id), REG(id), IMEMGET(REG(id)));
        }
        else
        {
            fprintf (stdout, "%3s: 0x%.8x\n", REGNAME(id), REG(id));
        }
    }
    else if (fmt   == FORMAT_UNSIGNED)
    {
        if (flag   == TRUE)
        {
            fprintf (stdout, "[%3s(0x%.8X)]: %u\n",
                    REGNAME(id), REG(id), IMEMGET(REG(id)));
        }
        else
        {
            fprintf (stdout, "%3s: %u\n",
                    REGNAME(id), REG(id));
        }
    }
    else if (fmt   == FORMAT_OCTAL)
    {
        if (flag   == TRUE)
        {
            fprintf (stdout, "[%3s(0x%.8X)]: 0%o\n",
                    REGNAME(id), REG(id), IMEMGET(REG(id)));
        }
        else
        {
            fprintf (stdout, "%3s: 0%o\n",
                    REGNAME(id), REG(id));
        }
    }
    else if (fmt   == FORMAT_FLOAT)
    {
        if (flag   == TRUE)
        {
            fprintf (stdout, "[%3s(0x%.8X)]: %.2f\n",
                    REGNAME(id), REG(id), FMEMGET(REG(id)));
        }
        else
        {
            fprintf (stdout, "%3s: %f\n",
                    REGNAME(id), FREG(id));
        }
    }
    else if (fmt   == FORMAT_SIGNED)
    {
        if (flag   == TRUE)
        {
            fprintf (stdout, "[%3s(0x%.8X)]: %d\n",
                    REGNAME(id), REG(id), IMEMGET(REG(id)));
        }
        else
        {
            fprintf (stdout, "%3s: %d\n",
                    REGNAME(id), REG(id));
        }
    }
    else if (fmt   == FORMAT_BINARY)
    {
        if (flag   == TRUE)
        {
            fprintf (stdout, "[%3s(0x%.8X)]: %X (not yet)\n",
                    REGNAME(id), REG(id), IMEMGET(REG(id)));
        }
        else
        {
            fprintf (stdout, "%3s: %X (not yet)\n",
                    REGNAME(id), REG(id));
        }
    }
}

void  output_mem (uint32_t  value, uint8_t  fmt,  uint8_t  flag)
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
