#include "defines.h"
#include <regex.h>

uint8_t  parse_token (uint8_t *token, uint8_t *pattern, uint8_t  parse_success)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    uint8_t     result       = PARSE_NONE;
    int32_t     check        = 0;
    regex_t     regex;
    regmatch_t  match[2];

    fprintf (debug, "[parse_token] passed token: '%s'\n", token);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // RegEx compilation: compile current pattern into our regex for processing
    //
    check                    = regcomp (&regex, pattern, REG_EXTENDED | REG_ICASE);
    if (check               != 0)
    {
        fprintf (stderr, "[parse_token] ERROR: RegEx compilation failed\n");
        exit    (REGEX_COMPILE_ERROR);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // RegEx execution: make sure input conforms to provided pattern
    //
    check                    = regexec (&regex, token, 2, match, 0);
    if (check               == REG_NOMATCH)
    {
        fprintf (debug, "[parse_token] invalid token value\n");
        result               = PARSE_NONE;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // RegEx execution success: obtain the value.
    //
    else if (check          == 0)
    {
        fprintf (debug, "[parse_token] successful match for token\n");
        result               = parse_success;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // RegEx execution error: bail out (shouldn't happen)
    //
    else
    {
        fprintf (stderr, "[parse_token] ERROR: RegEx execution failed\n");
        exit    (REGEX_EXECUTE_ERROR);
    }

    regfree (&regex);

    return  (result);
}

uint8_t  parse_memrange (uint8_t *token)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    uint8_t     result       = PARSE_NONE;
    int32_t     check        = 0;
    regex_t     regex;
    regmatch_t  match[2];
    uint8_t    *pattern      = "^ *(0x[0-9A-F]{8}) *- *(0x[0-9A-F]{8}) *$";

    fprintf (debug, "[parse_memrange] passed token: '%s'\n", token);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // RegEx compilation: compile current pattern into our regex for processing
    //
    check                    = regcomp (&regex, pattern, REG_EXTENDED | REG_ICASE);
    if (check               != 0)
    {
        fprintf (stderr, "[parse_memrange] ERROR: RegEx compilation failed\n");
        exit    (REGEX_COMPILE_ERROR);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // RegEx execution: make sure input conforms to provided pattern
    //
    check                    = regexec (&regex, token, 2, match, 0);
    if (check               == REG_NOMATCH)
    {
        fprintf (debug, "[parse_memrange] invalid memory range\n");
        result               = PARSE_NONE;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // RegEx execution success: obtain the value.
    //
    else if (check          == 0)
    {
        result               = PARSE_MEMRANGE;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // RegEx execution error: bail out (shouldn't happen)
    //
    else
    {
        fprintf (stderr, "[parse_memrange] ERROR: RegEx execution failed\n");
        exit    (REGEX_EXECUTE_ERROR);
    }

    regfree (&regex);

    return  (result);
}

uint8_t *parse_deref (uint8_t *input, uint8_t *flag)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    int32_t     check        = 0;
    int32_t     index        = 0;
    regex_t     regex;
    regmatch_t  match[4];
    uint8_t    *string       = NULL;
    uint8_t   **pattern      = NULL;
    uint8_t    *form0        = "^ *[[] *([^ ]+) *[]] *$";
    uint8_t    *form1        = "^ *([^ ]+) *[[] *([^ ]+) *[]] *$";
    uint8_t    *form2        = "^ *([^ ]+) *[[] *([^ ]+) *[]] *([^ ]+) *$";

    fprintf (debug, "[parse_deref] passed token: '%s'\n", input);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // allocate and populate pattern array
    //
    pattern                  = (uint8_t **) ralloc (sizeof (uint8_t *), 3, FLAG_NONE);
    *(pattern+0)             = form0;
    *(pattern+1)             = form1;
    *(pattern+2)             = form2;

    for (index               = 0;
         index              <  3;
         index               = index + 1)
    {
        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx compilation: compile current pattern into our regex for processing
        //
        check                = regcomp (&regex,
                                        *(pattern+index),
                                        REG_EXTENDED | REG_ICASE);
        if (check           != 0)
        {
            fprintf (stderr, "[ERROR] RegEx compilation failed\n");
            exit    (REGEX_COMPILE_ERROR);
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx execution: make sure input conforms to provided pattern
        //
        check                = regexec (&regex, input, 4, match, 0);
        if (check           == REG_NOMATCH)
        {
            fprintf (debug, "[parse_deref] invalid deref value\n");
            *flag            = FALSE;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx execution success: obtain the value.
        //
        else if (check      == 0)
        {
            fprintf (debug, "[parse_deref] match on dereference\n");

            ////////////////////////////////////////////////////////////////////////////
            //
            // allocate memory for reconstructed string
            //
            string           = (uint8_t *) ralloc (sizeof (uint8_t), strlen (input), FLAG_NONE);
            snprintf (string, strlen (input) - 1, "%.*s %.*s %.*s",
                      (int) (match[1].rm_eo - match[1].rm_so),
                      (input + match[1].rm_so),
                      (int) (match[2].rm_eo - match[2].rm_so),
                      (input + match[2].rm_so),
                      (int) (match[3].rm_eo - match[3].rm_so),
                      (input + match[3].rm_so));
            *flag            = TRUE;
            fprintf (debug, "[parse_deref] resulting string: '%s'\n", string);
            break;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx execution error: bail out (shouldn't happen)
        //
        else
        {
            fprintf (stderr, "[parse_deref] ERROR: RegEx execution failed\n");
            exit    (REGEX_EXECUTE_ERROR);
        }
    }

    regfree (&regex);
    free    (pattern);

    if (string              == NULL)
    {
        string               = input;
    }

    return  (string);
}

uint8_t  parse_imm (uint8_t *token, data_t *data)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    uint8_t     result        = 0;
    int32_t     check         = 0;
    int32_t     count         = 0;
    int32_t     index         = 0;
    regex_t     regex;
    regmatch_t  match[4];
    uint8_t   **pattern       = NULL;
    uint8_t    *form0         = "^ *0b([01]{1,32}) *$";                // bin
    uint8_t    *form1         = "^ *(0[0-7]{1,10}) *$";                // oct
    uint8_t    *form2         = "^ *([0-9]|[1-9][0-9]*) *$";           // unsigned dec
    uint8_t    *form3         = "^ *([+-]?[0-9]|[+-]?[1-9][0-9]*) *$"; // signed dec
    uint8_t    *form4         = "^ *([+-]?[0-9]*[.][0-9]*) *$";        // float
    uint8_t    *form5         = "^ *(0x[0-9A-F]{1,8})[hH]? *$";        // hex
    //uint8_t    *form1        = "^ *[[] *(0x[0-9A-F]{1,8}) *[]] *$";   // hex ptr

    fprintf (debug, "[parse_imm] passed token: '%s'\n", token);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // allocate and populate pattern array
    //
    pattern                   = (uint8_t **) ralloc (sizeof (uint8_t *), 6, FLAG_NONE);
    *(pattern+0)              = form0;
    *(pattern+1)              = form1;
    *(pattern+2)              = form2;
    *(pattern+3)              = form3;
    *(pattern+4)              = form4;
    *(pattern+5)              = form5;

    for (index                = 0;
         index               <  6;
         index                = index + 1)
    {
        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx compilation: compile current pattern into our regex for processing
        //
        check                 = regcomp (&regex,
                                         *(pattern+index),
                                         REG_EXTENDED | REG_ICASE);
        if (check            != 0)
        {
            fprintf (stderr, "[ERROR] RegEx compilation failed\n");
            exit    (REGEX_COMPILE_ERROR);
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx execution: make sure input conforms to provided pattern
        //
        check                 = regexec (&regex, token, 4, match, 0);
        if (check            == REG_NOMATCH)
        {
            fprintf (debug, "[parse_imm] invalid immediate value\n");
            result            = PARSE_NONE;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx execution success: obtain the value.
        //
        else if (check       == 0)
        {
            for (count        = 0;
                 count       <  4;
                 count        = count + 1)
            {
                fprintf (debug, "[parse_imm] match[%d]: '%s'\n",
                                count, (token+match[count].rm_so));
            }

            fprintf (debug, "[parse_imm] imm type: ");
            data -> fmt        = index;
            switch (index)
            {
                case FORMAT_BINARY:
                    data -> value.i32  = strtol ((token+match[1].rm_so), NULL, 2);
                    fprintf (debug, "binary (0x%.8X)\n", data -> value.i32);
                    break;

                case FORMAT_STRING:
                    data -> value.i32  = strtol ((token+match[1].rm_so), NULL, 2);
                    fprintf (debug, "string '%s' (0x%.8X)\n", (token+match[1].rm_so), data -> value.i32);
                    break;

                case FORMAT_OCTAL:
                    data -> value.i32  = strtol ((token+match[1].rm_so), NULL, 8);
                    fprintf (debug, "octal (0%o)\n", data -> value.i32);
                    break;

                case FORMAT_UNSIGNED:
                case FORMAT_SIGNED:
                    data -> value.i32  = strtol ((token+match[1].rm_so), NULL, 10);
                    fprintf (debug, "decimal (%d)\n", data -> value.i32);
                    break;

                case FORMAT_FLOAT:
                    data -> value.f32  = strtof ((token+match[1].rm_so), NULL);
                    fprintf (debug, "float (%.2f)\n", data -> value.f32);
                    break;

                case FORMAT_HEX:
                    data -> value.i32  = strtol ((token+match[1].rm_so), NULL, 16);
                    fprintf (debug, "hex (0x%.8X)\n", data -> value.i32);
                    break;
            }

            result             = PARSE_IMMEDIATE;

            regfree (&regex);
            break;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx execution error: bail out (shouldn't happen)
        //
        else
        {
            fprintf (stderr, "[parse_imm] ERROR: RegEx execution failed\n");
            exit    (REGEX_EXECUTE_ERROR);
        }

        regfree (&regex);
    }

    free    (pattern);

    return  (result);
}

uint8_t   parse_reg (uint8_t *token)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    int32_t     check           = 0;
    int32_t     index           = 0;
    int32_t     value           = 0;
    uint8_t     byte            = 0;
    uint8_t     result          = 0xFF;
    regex_t     regex;
    regmatch_t  match[2];
    int8_t     *source          = NULL;
    uint8_t   **pattern         = NULL;
    uint8_t    *form0           = "^ *(R[0-9]|R1[0-5]) *$";           // reg
    uint8_t    *form1           = "^ *[[] *(R[0-9]|R1[0-5]) *[]] *$"; // reg ptr
    uint8_t    *form2           = "^ *([SB]P) *$";                    // stack alias
    uint8_t    *form3           = "^ *[[] ([SB]P) *[]] *$";           // stack alias ptr
    uint8_t    *form4           = "^ *([CSD]R) *$";                   // str alias
    uint8_t    *form5           = "^ *[[] *([CSD]R) *[]] *$";         // str alias ptr
    uint8_t    *form6           = "^ *(I[PRV]) *$";                   // sys reg
    uint8_t    *form7           = "^ *[[] *(I[PRV]) *[]] *$";         // sys reg ptr
    uint8_t    *form8           = "^ *(reg|regs|registers) *$";       // register map

    fprintf (debug, "[parse_reg] passed token: '%s'\n", token);

    source                          = (uint8_t *) ralloc (sizeof (uint8_t),
                                                          18,
                                                          FLAG_NONE);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // allocate and populate pattern array
    //
    pattern                     = (uint8_t **) ralloc (sizeof (uint8_t *), 9, FLAG_NONE);
    *(pattern+0)                = form0;
    *(pattern+1)                = form1;
    *(pattern+2)                = form2;
    *(pattern+3)                = form3;
    *(pattern+4)                = form4;
    *(pattern+5)                = form5;
    *(pattern+6)                = form6;
    *(pattern+7)                = form7;
    *(pattern+8)                = form8;

    for (index                  = 0;
         index                 <  9;
         index                  = index + 1)
    {
        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx compilation: compile pattern into our regex for processing
        //
        check                   = regcomp (&regex,
                                           *(pattern+index),
                                           REG_EXTENDED | REG_ICASE);
        if (check              != 0)
        {
            fprintf (stderr, "[ERROR] RegEx compilation failed\n");
            exit    (REGEX_COMPILE_ERROR);
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx execution: make sure input conforms to provided pattern
        //
        check                   = regexec (&regex, token, 2, match, 0);
        if (check              == REG_NOMATCH)
        {
            fprintf (debug, "[parse_reg] invalid register\n");
            result              = PARSE_NONE;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx execution success! Display the results
        //
        else if (check         == 0)
        {
            sprintf (source, "%.*s", (int) (match[1].rm_eo - match[1].rm_so),
                                     (token + match[1].rm_so));

            if (index          <= 1) // regular register
            {
                value           = atoi ((source+1));
                result          = value & 0x000000FF;
                fprintf (debug, "[parse_reg] identified: R%u\n", result);
            }
            else if ((index    >= 2) && // stack register
                     (index    <= 3))
            {
                byte            = *(source);
                switch (byte)
                {
                    case 'b':
                    case 'B':
                        result  = BP; // BP (R14)
                        break;

                    case 's':
                    case 'S':
                        result  = SP; // SP (R15)
                        break;

                    default:
                        fprintf (debug, "[parse_reg] impossible situation\n");
                        break;
                }
            }
            else if ((index    >= 4) && // string register
                     (index    <= 5))
            {
                byte            = *(source);
                switch (byte)
                {
                    case 'c':
                    case 'C':
                        result  = CR; // CR (R11)
                        break;

                    case 's':
                    case 'S':
                        result  = SR; // SR (R12)
                        break;

                    case 'd':
                    case 'D':
                        result  = DR; // DR (R13)
                        break;

                    default:
                        fprintf (debug, "[parse_reg] impossible situation\n");
                        break;
                }
            }
            else if ((index    >= 6) && // system register
                     (index    <= 7))
            {
                byte            = *(source+1);
                switch (byte)
                {
                    case 'p':
                    case 'P':
                        result  = IP; // InstructionPointer
                        break;

                    case 'r':
                    case 'R':
                        result  = IR; // InstructionRegister
                        break;

                    case 'v':
                    case 'V':
                        result  = IV; // ImmediateValue
                        break;

                    default:
                        fprintf (debug, "[parse_reg] impossible situation\n");
                        break;
                }
            }
            else if (index     == 8)
            {
                result          = PARSE_REGISTERS;
            }
            else
            {
                fprintf (debug, "[parse_reg] impossible situation\n");
            }

            regfree (&regex);
            break;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx execution error: bail out
        //
        else
        {
            fprintf (stderr, "[parse_reg] ERROR: RegEx execution failed\n");
            exit    (REGEX_EXECUTE_ERROR);
        }

        regfree (&regex);
    }

    fprintf (debug, "[parse_reg] register numberic ID: %hhu\n", result);

    rfree (source);

    return (result);
}
