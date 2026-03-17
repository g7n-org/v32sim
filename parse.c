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

    fprintf (verbose, "[parse_token] passed token: '%s'\n", token);

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
        fprintf (verbose, "[parse_token] invalid token value\n");
        result               = PARSE_NONE;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // RegEx execution success: obtain the value.
    //
    else if (check          == 0)
    {
        fprintf (verbose, "[parse_token] successful match for token\n");
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

    fprintf (verbose, "[parse_memrange] passed token: '%s'\n", token);

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
        fprintf (verbose, "[parse_imm] invalid memory range\n");
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
    uint8_t    *form0        = "^ *([^ ]+) *[[] *([^ ]+) *[]] *$";
    uint8_t    *form1        = "^ *([^ ]+) *[[] *([^ ]+) *[]] *([^ ]+) *$";

    fprintf (verbose, "[parse_deref] passed token: '%s'\n", input);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // allocate and populate pattern array
    //
    pattern                  = (uint8_t **) malloc (sizeof (uint8_t *) * 2);
    *(pattern+0)             = form0;
    *(pattern+1)             = form1;

    for (index               = 0;
         index              <  2;
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
            fprintf (verbose, "[parse_deref] invalid deref value\n");
            *flag            = FALSE;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx execution success: obtain the value.
        //
        else if (check      == 0)
        {
            fprintf (verbose, "[parse_deref] match on dereference\n");

            ////////////////////////////////////////////////////////////////////////////
            //
            // allocate memory for reconstructed string
            //
            string           = (uint8_t *) malloc (sizeof (uint8_t) * strlen (input));
            snprintf (string, strlen (input) - 1, "%.*s %.*s %.*s",
                      (int) (match[1].rm_eo - match[1].rm_so),
                      (input + match[1].rm_so),
                      (int) (match[2].rm_eo - match[2].rm_so),
                      (input + match[2].rm_so),
                      (int) (match[3].rm_eo - match[3].rm_so),
                      (input + match[3].rm_so));
            *flag            = TRUE;
            break;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx execution error: bail out (shouldn't happen)
        //
        else
        {
            fprintf (stderr, "[parse_token] ERROR: RegEx execution failed\n");
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

uint8_t  parse_imm (uint8_t *token)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    uint8_t     result       = 0;
    int32_t     check        = 0;
    int32_t     index        = 0;
    regex_t     regex;
    regmatch_t  match[2];
    uint8_t   **pattern      = NULL;
    uint8_t    *form0        = "^ *(0x[0-9A-F]{1,8}) *$";             // hex
    uint8_t    *form1        = "^ *[[] *(0x[0-9A-F]{1,8}) *[]] *$";   // hex ptr
    uint8_t    *form2        = "^ *([0-9A-F]{1,8}[hH]) *$";           // hex
    uint8_t    *form3        = "^ *[[] *([0-9A-F]{1,8}[hH]) *[]] *$"; // hex ptr
    uint8_t    *form4        = "^ *(0b[01]{1,32}) *$";                // bin
    uint8_t    *form5        = "^ *[[] *(0b[01]{1,32}) *[]] *$";      // bin ptr
    uint8_t    *form6        = "^ *(0[0-7]{1,10}) *$";                // oct
    uint8_t    *form7        = "^ *[[] *(0[0-7]{1,10}) *[]] *$";      // oct ptr
    uint8_t    *form8        = "^ *([0-9]|[1-9][0-9]*) *$";           // dec
    uint8_t    *form9        = "^ *[[] *([0-9]|[1-9][0-9]*) *[]] *$"; // dec ptr

    fprintf (verbose, "[parse_imm] passed token: '%s'\n", token);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // allocate and populate pattern array
    //
    pattern                  = (uint8_t **) malloc (sizeof (uint8_t *) * 10);
    *(pattern+0)             = form0;
    *(pattern+1)             = form1;
    *(pattern+2)             = form2;
    *(pattern+3)             = form3;
    *(pattern+4)             = form4;
    *(pattern+5)             = form5;
    *(pattern+6)             = form6;
    *(pattern+7)             = form7;
    *(pattern+8)             = form8;
    *(pattern+9)             = form9;

    for (index               = 0;
         index              <  10;
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
        check                = regexec (&regex, token, 2, match, 0);
        if (check           == REG_NOMATCH)
        {
            fprintf (verbose, "[parse_imm] invalid immediate value\n");
            result           = PARSE_NONE;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx execution success: obtain the value.
        //
        else if (check      == 0)
        {
            if (index       <= 1) // hex
            {
                result       = strtol (token, NULL, 16);
                fprintf (verbose, "[parse_imm] imm type: hex (0x%.8X)\n", result);
            }
            else if ((index >= 2) && // hex
                     (index <= 3))
            {
                //trim the suffixing H/h
                result       = strtol (token, NULL, 16);
                fprintf (verbose, "[parse_imm] imm type: hex (%.8XH)\n",  result);
            }
            else if ((index >= 4) &&
                     (index <= 5))   // bin
            {
                //trim the prefixing 0b
                result       = strtol (token, NULL, 2);
                fprintf (verbose, "[parse_imm] imm type: binary\n");
            }
            else if ((index >= 6) &&
                     (index <= 7))   // oct
            {
                result       = strtol (token, NULL, 8);
                fprintf (verbose, "[parse_imm] imm type: octal (0%o)\n",  result);
            }
            else
            {
                result       = strtol (token, NULL, 10);
                fprintf (verbose, "[parse_imm] imm type: decimal (%u)\n", result);
            }

            result           = PARSE_IMMEDIATE;

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

    fprintf (verbose, "[parse_reg] passed token: '%s'\n", token);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // allocate and populate pattern array
    //
    pattern                     = (uint8_t **) malloc (sizeof (uint8_t *) * 9);
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
            fprintf (verbose, "[parse_reg] invalid register\n");
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
                fprintf (verbose, "[parse_reg] identified: R%u\n", result);
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
                        fprintf (verbose, "[parse_reg] impossible situation\n");
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
                        fprintf (verbose, "[parse_reg] impossible situation\n");
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
                        fprintf (verbose, "[parse_reg] impossible situation\n");
                        break;
                }
            }
            else if (index     == 8)
            {
                result          = PARSE_REGISTERS;
            }
            else
            {
                fprintf (verbose, "[parse_reg] impossible situation\n");
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

    fprintf (verbose, "[parse_reg] register numberic ID: %hhu\n", result);

    return (result);
}
