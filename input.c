#include "defines.h"
#include <readline/readline.h>
#include <readline/history.h>
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
    //int32_t     value        = 0;
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
        /*
        snprintf (token, strlen (), "%.*s-%.*s",
                  (int) (match[1].rm_eo - match[1].rm_so),
                  (token + match[1].rm_so),
                  (int) (match[2].rm_eo - match[2].rm_so),
                  (token + match[2].rm_so));
                  */
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
            //memmove (input, string, strlen (string));
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

            ////////////////////////////////////////////////////////////////////////////
            //
            // for odd values of index, we have a dereference case; set the MSB
            // of result (a 64-bit unsigned) high.
            //
            //if ((index % 1) == 1)
            //{
            //    result       = result | 0x80;
           // }

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

            ////////////////////////////////////////////////////////////////////////////
            //
            // for odd values of index, we have a dereference case; set the MSB
            // of result high.
            //
            /*
            if ((index         <  8) &&
                ((index % 1)   == 1))
            {
                result       = result | 0x80;
            }*/

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

uint32_t  tokenize_asm (uint8_t *string)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    int32_t     check            = 0;
    int32_t     index            = 0;
    uint64_t    result           = 0x0000000000000000;
    uint32_t    instruction      = 0x00000000;
    uint8_t     opcode           = 0x00;
    uint8_t     immflag          = 0x00;
    uint8_t     dstreg           = 0x00;
    uint8_t     srcreg           = 0x00;
    uint8_t     addr             = 0x00;
    uint16_t    port             = 0x0000;
    uint8_t     dderef           = FALSE;
    uint8_t     sderef           = FALSE;
    uint8_t     dindex           = FALSE;
    uint8_t     sindex           = FALSE;
    regex_t     regex;
    regmatch_t  match[4];
    uint8_t    *pattern          = "^ *([a-z][a-z2]{1,4}) +(R[0-9]|R1[0-5]|[SBCDS][PR]|0x[0-9A-F]{1,8}|[0-9]|[1-9][0-9]+) *, +(R[0-9]|R1[0-5]|[SBCDS][PR]|0x[0-9A-F]{1,8}|[0-9]|[1-9][0-9]+) *$";
    //uint8_t   **pattern         = NULL;
    //uint8_t    *form0           = "^ *(HLT|WAIT|RET|MOVS|SETS) *$";   // 0 operand
    //uint8_t    *form0           = "^ *([rR][0-9]|[rR]1[0-5]) *$";   // 1 operand
    //uint8_t    *form0           = "^ *([rR][0-9]|[rR]1[0-5]) *$";   // 2 operand

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // initialize our opcodes array with the available instructions
    //
    opcode_t  lookup[64]         =
    {
        { "HLT"  }, { "WAIT"  }, { "JMP"   }, { "CALL" },
        { "RET"  }, { "JT"    }, { "JF"    }, { "IEQ"  },
        { "INE"  }, { "IGT"   }, { "IGE"   }, { "ILT"  },
        { "ILE"  }, { "FEQ"   }, { "FNE"   }, { "FGT"  },
        { "FGE"  }, { "FLT"   }, { "FLE"   }, { "MOV"  },
        { "LEA"  }, { "PUSH"  }, { "POP"   }, { "IN"   },
        { "OUT"  }, { "MOVS"  }, { "SETS"  }, { "CMPS" },
        { "CIF"  }, { "CFI"   }, { "CIB"   }, { "CFB"  },
        { "NOT"  }, { "AND"   }, { "OR"    }, { "XOR"  },
        { "BNOT" }, { "SHL"   }, { "IADD"  }, { "ISUB" },
        { "IMUL" }, { "IDIV"  }, { "IMOD"  }, { "ISGN" },
        { "IMIN" }, { "IMAX"  }, { "IABS"  }, { "FADD" },
        { "FSUB" }, { "FMUL"  }, { "FDIV"  }, { "FMOD" },
        { "FSGN" }, { "FMIN"  }, { "FMAX"  }, { "FABS" },
        { "FLR"  }, { "CEIL"  }, { "ROUND" }, { "SIN"  },
        { "ACOS" }, { "ATAN2" }, { "LOG"   }, { "POW"  }
    };

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // RegEx compilation: compile pattern into our regex for processing
    //
    check                        = regcomp (&regex,
                                            pattern,
                                            REG_EXTENDED | REG_ICASE);
    if (check                   != 0)
    {
        fprintf (stderr, "[tokenize_asm] ERROR: RegEx compilation failed\n");
        exit    (REGEX_COMPILE_ERROR);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // RegEx execution: make sure input conforms to provided pattern
    //
    check                        = regexec (&regex, string, 4, match, 0);
    if (check                   == REG_NOMATCH)
    {
        fprintf (stderr, "[tokenize_asm] ERROR: malformed input on ASM\n");
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // RegEx execution success! Display the results
    //
    else if (check              == 0)
    {
        for (index               = 0;
             index              <  4;
             index               = index + 1)
        {
            fprintf (debug, "[tokenize_asm] match[%d]: %.*s (%lld - %lld)\n",
                             index,
                             (int) (match[index].rm_eo - match[index].rm_so),
                             (string + match[index].rm_so),
                             (long long int) match[index].rm_so,
                             (long long int) match[index].rm_eo);
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // determine the opcode
        //
        result                   = 0x0000000000000000;
        sprintf (source, "%.*s", (int) (match[1].rm_eo-match[1].rm_so),
                                 (string + match[1].rm_so));
        for (index               = 0;
             index              <  64;
             index               = index + 1)
        {
            check                = strcasecmp (source, lookup[index].name);
            fprintf (debug, "[tokenize_asm] check: %d, token: '%s', opcode: '%s'\n",
                            check, source, lookup[index].name);
            if (check           == 0)
            {
                opcode           = index;
                break;
            }
        }
        fprintf (verbose, "[tokenize_asm] opcode identified as: 0x%.2hhX\n", opcode);

        ////////////////////////////////////////////////////////////////////////////////
        //
        // parse the instruction tokens: process the operands
        //
        sprintf (destination, "%.*s", (int) (match[2].rm_eo - match[2].rm_so),
                                      (string + match[2].rm_so));
        switch (opcode)
        {
            case HLT:
            case WAIT:
            case RET:
                break;

            case CALL:
            case JMP:
                dstreg           = parse_reg (destination);
                if (dstreg      == 0xFF) // was not a valid register, try immediate
                {
                    result       = parse_imm (destination);
                    immflag      = 1;
                }
                else
                {
                    dderef       = ((dstreg & 0x80) >  0)  ? TRUE : FALSE;
                    dstreg       = dstreg & 0x1F;
                    //sysflag      = ((dstreg >  0x0F) >  0) ? TRUE : FALSE;
                }
                break;

            case MOV:
                dstreg           = parse_reg (destination);
                if (dstreg      == 0xFF) // was not a valid register, try immediate
                {
                    result       = parse_imm (destination);
                    dderef       = ((dstreg & 0x8000000000000000) >  0) ? TRUE : FALSE;
                    result       = result & 0x00000000FFFFFFFF;
                }
                else
                {
                    fprintf (verbose, "dstreg: R%u\n", dstreg);
                    dderef       = ((dstreg & 0x80)  >  0) ? TRUE : FALSE;
                    dstreg       = dstreg & 0x1F;
                    //sysflag      = ((dstreg >  0x0F) >  0) ? TRUE : FALSE;
                }

                sprintf (source,      "%.*s", (int) (match[3].rm_eo - match[3].rm_so),
                                              (string + match[3].rm_so));

                srcreg           = parse_reg (source);
                if (srcreg      == 0xFF) // was not a valid register, try immediate
                {
                    result       = parse_imm (source);
                    sderef       = ((srcreg & 0x8000000000000000) >  0) ? TRUE : FALSE;
                    result       = result & 0x00000000FFFFFFFF;
                }
                else
                {
                    fprintf (verbose, "srcreg: R%u\n", srcreg);
                    sderef       = ((srcreg & 0x80)  >  0) ? TRUE : FALSE;
                    srcreg       = srcreg & 0x1F;
                    /*
                    if (sysflag == FALSE)
                    {
                        sysflag  = ((srcreg >  0x0F) >  0) ? TRUE : FALSE;
                    }
                    */
                }

                if ((dstreg     != 0xFF)   && // MOV DSTREG, Immediate
                    (srcreg     == 0xFF)   &&
                    (dderef     == FALSE)  &&
                    (sderef     == FALSE)  &&
                    (dindex     == FALSE)  &&
                    (sindex     == FALSE))
                {
                    addr        == 0x0;
                }
                
                if ((dstreg     != 0xFF)   && // MOV DSTREG, SRCREG
                    (srcreg     != 0xFF)   &&
                    (dderef     == FALSE)  &&
                    (sderef     == FALSE)  &&
                    (dindex     == FALSE)  &&
                    (sindex     == FALSE))
                {
                    addr        == 0x1;
                }

                if ((dstreg     != 0xFF)   && // MOV DSTREG, [Immediate]
                    (srcreg     == 0xFF)   &&
                    (dderef     == FALSE)  &&
                    (sderef     == TRUE)   &&
                    (dindex     == FALSE)  &&
                    (sindex     == FALSE))
                {
                    addr        == 0x2;
                }
                
                if ((dstreg     != 0xFF)   && // MOV DSTREG, [SRCREG]
                    (srcreg     != 0xFF)   &&
                    (dderef     == FALSE)  &&
                    (sderef     == TRUE)   &&
                    (dindex     == FALSE)  &&
                    (sindex     == FALSE))
                {
                    addr        == 0x3;
                }
                
                if ((dstreg     != 0xFF)   && // MOV DSTREG, [SRCREG+Immediate]
                    (srcreg     != 0xFF)   &&
                    (dderef     == FALSE)  &&
                    (sderef     == TRUE)   &&
                    (dindex     == FALSE)  &&
                    (sindex     == TRUE))
                {
                    addr        == 0x4;
                }
                
                if ((dstreg     == 0xFF)   && // MOV [Immediate], SRCREG
                    (srcreg     != 0xFF)   &&
                    (dderef     == TRUE)   &&
                    (sderef     == FALSE)  &&
                    (dindex     == FALSE)  &&
                    (sindex     == FALSE))
                {
                    addr        == 0x5;
                }
                
                if ((dstreg     != 0xFF)   && // MOV [DSTREG], SRCREG
                    (srcreg     != 0xFF)   &&
                    (dderef     == TRUE)   &&
                    (sderef     == FALSE)  &&
                    (dindex     == FALSE)  &&
                    (sindex     == FALSE))
                {
                    addr        == 0x6;
                }

                if ((dstreg     != 0xFF)   && // MOV [DSTREG+Immediate], SRCREG
                    (srcreg     != 0xFF)   &&
                    (dderef     == TRUE)   &&
                    (sderef     == FALSE)  &&
                    (dindex     == TRUE)   &&
                    (sindex     == FALSE))
                {
                    addr        == 0x7;
                }

                // possibility: use bits 13 and 12 of port field to record
                // system register bits (none, 1=IP, 2=IR, 3=IV)
                port             = 0x0000;
                break;
        }

        instruction              = (opcode  << OPCODESHIFT);
        instruction             |= (immflag << IMMED_SHIFT);
        instruction             |= (dstreg  << DSTREGSHIFT);
        instruction             |= (srcreg  << SRCREGSHIFT);
        instruction             |= (addr    << MOVADRSHIFT);
        instruction             |= port;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // RegEx execution error: bail out
    //
    else
    {
        fprintf (stderr, "[ERROR] RegEx execution failed\n");
        exit    (REGEX_EXECUTE_ERROR);
    }

    regfree (&regex);

    fprintf (verbose, "[tokenize_asm] assembling '%s' to: 0x%.8X\n",
                      string, instruction);

    return  (instruction);
}

uint8_t  tokenize_input (uint8_t *input, uint8_t *flag)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    data_t     *dtmp               = NULL;
    linked_l  **list               = NULL;
    linked_l   *ltmp               = NULL;
    linked_l   *tmp                = NULL;
    int32_t     check              = 0;
    int32_t     count              = 0;
    int32_t     value              = 0;
    int32_t     index              = 0;
    int32_t     len                = 0;
    int32_t     offset             = REG(IP);
    int32_t     word               = 0xFFFFFFFF;
    int32_t     immv               = 0xFFFFFFFF;
    regex_t     regex;
    regmatch_t  match[5];
    uint8_t     byte               = 0;
    uint8_t     entry[24];
    uint8_t     lval[24];
    uint8_t     fmt                = FORMAT_DEFAULT;
    int8_t     *pos                = NULL;
    int8_t     *string             = NULL;
    int8_t     *token              = NULL;
    uint8_t   **form               = NULL;
    uint8_t    *form0              = "^ *([a-z?]+) *$";
    uint8_t    *form1              = "^ *([a-z]+) *(IP:0x[0-9A-F]{8})? *(IR:0x[0-9A-F]{8})? *(IV:0x[0-9A-F]{8})? *$";
    uint8_t    *form2              = "^ *([a-z]+) *([^ ]+) *= *([^ ]+) *$";
    uint8_t    *form3              = "^ *([a-z/]+) *([^ ]+) *([A-Z_][A-Z0-9_+-]*)? *$";

    uint8_t   **pattern            = NULL;
    uint8_t    *pattern0           = "^ *(R[0-9]|R1[0-5]|[BCDS][PR]|I[PRV]) *$";  // register
    uint8_t    *pattern1           = "^ *(reg|regs|register|registers|r[*]) *$";  // registers
    uint8_t    *pattern2           = "^ *(0x[0-9A-F]{8}) *$";                     // memory
    uint8_t    *pattern3           = "^ *(0x[0-9A-F]{8}) *- *(0x[0-9A-F]{8}) *$"; // memrange
    uint8_t    *pattern4           = "^ *(0x[0-7][01][0-9A-F]) *$";               // ioport
    uint8_t     result             = 0;

    fprintf (verbose, "[tokenize_input] passed string: '%s'\n", input);

    string                         = parse_deref (input, flag);

    fprintf (verbose, "[tokenize_input] processed string: '%s'\n", string);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // allocate and populate form array
    //
    form                           = (uint8_t **) malloc (sizeof (uint8_t *) * 4);
    *(form+0)                      = form0;
    *(form+1)                      = form1;
    *(form+2)                      = form2;
    *(form+3)                      = form3;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // allocate and populate pattern array
    //
    pattern                        = (uint8_t **) malloc (sizeof (uint8_t *) * 5);
    *(pattern+0)                   = pattern0;
    *(pattern+1)                   = pattern1;
    *(pattern+2)                   = pattern2;
    *(pattern+3)                   = pattern3;
    *(pattern+4)                   = pattern4;

    for (index                             = 0;
         index                            <  4;
         index                             = index + 1)
    {
        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx compilation: compile form pattern into our regex for processing
        //
        check                              = regcomp (&regex,
                                                     *(form+index),
                                                     REG_EXTENDED | REG_ICASE);
        if (check                         != 0)
        {
            fprintf (stderr, "[ERROR] Invalid pattern: RegEx compilation failed\n");
            exit    (REGEX_COMPILE_ERROR);
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx execution: make sure input conforms to provided pattern
        //
        check                              = regexec (&regex, string, 5, match, 0);
        if (check                         == REG_NOMATCH)
        {
            continue;
            fprintf (stderr, "ERROR: malformed input\n");
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx execution success! Display the results
        //
        else if (check                    == 0)
        {
            fprintf (verbose, "[tokenize_input] MATCH on form %u\n", index);
            if (index                     == 0) // single-word command
            {
                byte                       = *(string + match[1].rm_so);
                if (byte                  == 'b')   // break
                {
                    tmp                    = bpoint;
                    count                  = 0;
                    while (tmp            != NULL)
                    {
                        fprintf (stdout, "[%u] 0x%.8X\n", count, tmp -> data.raw);
                        tmp                = tmp -> next;
                        count              = count + 1;
                    }
                    action                 = INPUT_BREAK;
                }
                else if (byte             == 'c')   // continue
                {
                    action                 = INPUT_CONTINUE;
                }
                else if (byte             == 'd')   // display
                {
                    action                 = INPUT_DISPLAY;
                }
                else if (byte             == 'i')   // ignore
                {
                    action                 = INPUT_IGNORE;
                }
                else if (byte             == 'l')   // labels
                {
                    ltmp                   = lpoint;
                    count                  = 0;
                    while (ltmp           != NULL)
                    {
                        if (ltmp -> label != NULL)
                        {
                            fprintf (stdout, "[%u] %s -> 0x%.8X\n",
                                    count, ltmp -> label, ltmp -> data.raw);
                        }
                        ltmp               = ltmp -> next;
                        count              = count + 1;
                    }
                    action                 = INPUT_LABEL;
                }
                else if (byte             == 'n')   // next
                {
                    action                 = INPUT_NEXT;
                }
                else if (byte             == 's')   // step
                {
                    action                 = INPUT_STEP;
                }
                else if (byte             == 'h')   // help
                {
                    action                 = INPUT_HELP;
                }
                else if (byte             == '?') // help
                {
                    action                 = INPUT_HELP;
                }
                else if (byte             == 'q') // quit
                {
                    action                 = INPUT_QUIT;
                }
            }
            else if (index                == 1) // replace-style command
            {
                byte                       = *(string + match[1].rm_so);
                if (byte                  == 'r') // replace
                {
                    action                 = INPUT_REPLACE;
                    fprintf (verbose, "[tokenize_input] replacing ");
                    for (count             = 2;
                         count            <  5;
                         count             = count + 1)
                    {
                        if (-1            == match[count].rm_so)
                        {
                            break;
                        }

                        len                = (match[count].rm_eo-match[count].rm_so);
                        pos                = (string + match[count].rm_so + 1);
                        snprintf (entry, len, "%.*s", len, pos);
                        byte               = *(pos);
                        token              = strtok (entry,  ":");
                        token              = strtok (NULL,   ":");
                        if (byte          == 'P')
                        {
                            offset         = strtol (token, NULL, 16);
                            fprintf (verbose, "IP:0x%.8X ", REG(IP));
                        }
                        else if (byte     == 'R')
                        {
                            fprintf (verbose, "IR:0x%.8X ", IMEMGET(offset));
                            word           = strtol (token, NULL, 16);
                            fprintf (verbose, "with IR:0x%.8X ", word);
                            SYSMEMSET(offset, word);
                        }
                        else if (byte     == 'V')
                        {
                            if (0         <  (word & IMMVAL_MASK))
                            {
                                fprintf (verbose, "IV:0x%.8X ", IMEMGET(offset+1));
                                immv       = strtol (token, NULL, 16);
                                fprintf (verbose, "with IV:0x%.8X ", immv);
                                SYSMEMSET(offset+1, immv);
                            }
                        }
                    }
                    fprintf (verbose, "\n");
                }
            }
            else if (index                == 2) // assign-style command
            {
                action                     = INPUT_SET;
                len                        = (match[2].rm_eo - match[2].rm_so);
                pos                        = (string + match[2].rm_so);
                snprintf (lval, (len+1), "%.*s", (len+1), pos);

                len                        = (match[3].rm_eo - match[3].rm_so);
                pos                        = (string + match[3].rm_so);
                snprintf (entry, (len+1), "%.*s", (len+1), pos);
                fprintf (debug, "[lval]  '%s'\n", lval);
                fprintf (debug, "[entry] '%s'\n", entry);
                fprintf (debug, "[token] '%s'\n", token);

                ////////////////////////////////////////////////////////////////////////
                //
                // Check if we are setting a register of any sort
                //
                result                     = parse_token (lval,
                                                          *(pattern+0),
                                                          PARSE_REGISTER);
                if (result                == PARSE_REGISTER)
                {
                    result                 = parse_reg (lval);
                    value                  = strtol (entry, NULL, 16);
                    fprintf (debug, "[%s] setting to 0x%.8X\n",
                                    REGNAME(result), value);
                    REG(result)            = value;

                    ////////////////////////////////////////////////////////////////////
                    //
                    // If setting IP, adjust IR and (if needed) IV as well.
                    //
                    if (result            == IP)
                    {
                        REG(IR)            = IMEMGET(REG(IP));
                        if (0             <  (REG(IR) & IMMVAL_MASK))
                        {
                            REG(IV)        = IMEMGET(REG(IP)+1);
                        }
                        else
                        {
                            REG(IV)        = 0;
                        }
                    }
                }

                ////////////////////////////////////////////////////////////////////////
                //
                // Check if we are setting a memory address
                //
                result                     = parse_token (lval,
                                                          *(pattern+2),
                                                          PARSE_MEMORY);
                if (result                == PARSE_MEMORY)
                {
                    count                  = strtol (lval,  NULL, 16);
                    value                  = strtol (entry, NULL, 16);
                    fprintf (debug, "[0x%.8X] setting memory to 0x%.8X\n",
                                    count , value);
                    SYSMEMSET(count, value);
                }

                ////////////////////////////////////////////////////////////////////////
                //
                // Check if we are setting an IOPort
                //
                result                     = parse_token (lval,
                                                          *(pattern+4),
                                                          PARSE_IOPORT);
                if (result                == PARSE_IOPORT)
                {
                    count                  = strtol (lval,  NULL, 16);
                    value                  = strtol (entry, NULL, 16);
                    fprintf (debug, "[0x%.3X] setting ioport to 0x%.8X\n",
                                    count, value);
                    SYSPORTSET(count, value);
                }

                else if ((lval[0]         == 'C') ||
                         (lval[0]         == 'c'))
                {
                    check                  = strncasecmp (entry, "true", 4);
                    fprintf (debug, "[color]: %s\n", entry);
                    if (check             == 0)
                    {    
                        colorflag          = TRUE;
                    }
                    else
                    {
                        colorflag          = FALSE;
                    }
                }
                else if ((lval[0]         == 'D') ||
                         (lval[0]         == 'd'))
                {
                    check                  = strncasecmp (lval,  "debug", 5);
                    if (check             == 0)
                    {
                        check              = strncasecmp (entry, "true", 4);
                        if (check         == 0)
                        {    
                            debug          = stderr;
                            fprintf (debug, "[set] debugging ENABLED!\n");
                        }
                        else
                        {
                            fprintf (debug, "[set] disabling debugging!\n");
                            debug          = devnull;
                        }
                    }

                    check                  = strncasecmp (lval,  "deref", 5);
                    if (check             == 0)
                    {
                        check              = strncasecmp (entry, "true", 4);
                        if (check         == 0)
                        {    
                            derefaddr      = TRUE;
                            fprintf (debug, "[set] derefaddr ENABLED!\n");
                        }
                        else
                        {
                            derefaddr      = FALSE;
                            fprintf (debug, "[set] derefaddr DISABLED!\n");
                        }
                    }
                }
                else if ((lval[0]         == 'V') ||
                         (lval[0]         == 'v'))
                {
                    check                  = strncasecmp (lval,  "verbo", 5);
                    if (check             == 0)
                    {
                        check              = strncasecmp (entry, "true", 4);
                        if (check         == 0)
                        {    
                            verbose        = stderr;
                            fprintf (debug, "[set] verbosity ENABLED!\n");
                        }
                        else
                        {
                            fprintf (debug, "[set] disabling verbosity!\n");
                            debug          = devnull;
                        }
                    }
                }
            }
            else if (index                == 3) // parametered command
            {
                byte                       = *(string + match[1].rm_so);
                fprintf (debug, "[debug] data is: '%s', byte is: '%c'\n", (string + match[1].rm_so), byte);
                if (byte                  == 'b') // break
                {
                    fprintf (debug, "BREAK encountered\n");
                    action                 = INPUT_BREAK;
                    token                  = strtok ((string + match[2].rm_so), " ");
                    result                 = parse_token (token, *(pattern+2), PARSE_MEMORY);
                    if (result            != PARSE_NONE) // memory address
                    {
                        fprintf (debug, "BREAK adding the offset '%s'\n", token);
                        value              = strtol (token, NULL, 16);
                        tmp                = listnode (LIST_MEM, value);
                        bpoint             = list_add (bpoint, tmp);
                    }
                    else // not a memory address (assuming label)
                    {
                        ltmp               = lpoint;
                        while (ltmp       != NULL)
                        {
                            if (ltmp -> label != NULL)
                            {
                                check          = strncasecmp (ltmp -> label, token, strlen (ltmp -> label));
                                if (check     == 0) // existing label found in list
                                {
                                    fprintf (debug, "BREAK adding the label '%s'\n", ltmp -> label);
                                    fprintf (debug, "adding 0x%.8X to the list\n", ltmp -> data.i32);
                                    tmp        = listnode (LIST_MEM, ltmp -> data.i32);
                                    bpoint     = list_add (bpoint, tmp);
                                }
                            }
                            ltmp       = ltmp -> next;
                        }
                    }
                }
                else if (byte         == 'd') // display
                {
                    action             = INPUT_DISPLAY;
                    fprintf (debug, "(string+match[1].rm_so): %s\n", (string+match[1].rm_so));
                    strcpy (entry, (string+match[1].rm_so));
                    fprintf (debug, "[before] entry: %s\n", entry);
                    token              = strtok (entry, " ");
                    fprintf (debug, "[after]  entry: %s\n", entry);
                    token              = strtok (entry,  "/");
                    fprintf (debug, "[after]  token: %s\n", token);
                    pos                = strtok (NULL,   "/");
                    fprintf (debug, "pos:            %s\n", pos);
                    if ((pos          != NULL) &&
                        (*(pos+0)     != '\0'))
                    {
                        if (*(pos+0)  == 'b')
                        {
                            fmt        = FORMAT_BINARY;
                        }
                        else if (*pos == 'd')
                        {
                            fmt        = FORMAT_SIGNED;
                        }
                        else if (*pos == 'f')
                        {
                            fmt        = FORMAT_FLOAT;
                        }
                        else if (*pos == 'o')
                        {
                            fmt        = FORMAT_OCTAL;
                        }
                        else if (*pos == 'u')
                        {
                            fmt        = FORMAT_UNSIGNED;
                        }
                        else if (*pos == 'x')
                        {
                            fmt        = FORMAT_LOWERHEX;
                        }
                        else if (*pos == 'X')
                        {
                            fmt        = FORMAT_HEX;
                        }
                    }
                    token              = strtok ((string + match[2].rm_so), " ");
                    for (count         = PARSE_REGISTER;
                         count        <= PARSE_IOPORT;
                         count         = count + 1)
                    {
                        fprintf (verbose,  "[tokenize_input] count: 0x%.2X, pattern: 0x%.2X\n", count, (count-0x7B));
                        ////////////////////////////////////////////////////////////////
                        //
                        // PARSE_REGISTER          0x7A
                        // PARSE_REGISTERS         0x7B
                        // PARSE_MEMORY            0x7C
                        // PARSE_MEMRANGE          0x7D
                        // PARSE_IOPORT            0x7E
                        // PARSE_NONE              0x7F
                        //
                        result         = parse_token (token,
                                                      *(pattern+(count-0x7A)),
                                                      count);
                        if (result    != PARSE_NONE)
                        {
                            break;
                        }
                    }
                    token_label                = strtok (NULL, " ");

                    switch (result)
                    {
                        case PARSE_NONE:
                            fprintf (stderr, "[ERROR] malformed display value\n");
                            break;

                        case PARSE_MEMRANGE:
                            pos                    = strtok (token, "-");
                            value                  = strtol (pos,   NULL, 16);
                            pos                    = strtok (NULL,  "-");
                            check                  = strtol (pos,   NULL, 16);
                            for (index             = value;
                                 index            <= check;
                                 index             = index + 1)
                            {
                                if (*flag         == TRUE)
                                {
                                    tmp            = listnode (LIST_MEM_DEREF, index);
                                }
                                else
                                {
                                    tmp            = listnode (LIST_MEM,       index);
                                }

                                if (token_label   != NULL)
                                {
                                    value          = sizeof (int8_t) * strlen (token_label) + 1;
                                    tmp -> label   = (int8_t *) malloc (value);
                                    strcpy (tmp -> label, token_label);
                                }
                                tmp -> fmt         = fmt;
                                dpoint             = list_add (dpoint, tmp);
                            }
                            break;

                        case PARSE_MEMORY:
                            value              = strtol (token, NULL, 16);
                            if (*flag         == TRUE)
                            {
                                tmp            = listnode (LIST_MEM_DEREF, value);
                            }
                            else
                            {
                                tmp            = listnode (LIST_MEM,       value);
                            }

                            if (token_label   != NULL)
                            {
                                value          = sizeof (int8_t) * strlen (token_label) + 1;
                                tmp -> label   = (int8_t *) malloc (value);
                                strcpy (tmp -> label, token_label);
                            }
                            tmp -> fmt         = fmt;
                            dpoint             = list_add (dpoint, tmp);
                            break;

                        case PARSE_REGISTERS:
                            for (index         = 0;
                                 index        <= 15;
                                 index         = index + 1)
                            {
                                tmp            = listnode (LIST_REG, index);
                                tmp -> fmt     = fmt;
                                dpoint         = list_add (dpoint, tmp);
                            }
                            break;

                        case PARSE_REGISTER: // specific, general register
                            result             = parse_reg (token);
                            if (result        >  15)
                            {
                                sys_reg_show   = TRUE;
                                break;
                            }

                            if (*flag         == TRUE)
                            {
                                tmp            = listnode (LIST_REG_DEREF, result);
                            }
                            else
                            {
                                tmp            = listnode (LIST_REG,       result);
                            }

                            if (token_label   != NULL)
                            {
                                value          = sizeof (int8_t) * strlen (token_label) + 1;
                                tmp -> label   = (int8_t *) malloc (value);
                                strcpy (tmp -> label, token_label);
                            }
                            tmp -> fmt         = fmt;
                            dpoint             = list_add (dpoint, tmp);
                            break;

                        case PARSE_IOPORT:
                            value              = strtol (token, NULL, 16);
                            tmp                = listnode (LIST_IOP, value);

                            if (token_label   != NULL)
                            {
                                value          = sizeof (int8_t) * strlen (token_label) + 1;
                                tmp -> label   = (int8_t *) malloc (value);
                            }
                            else
                            {
                                dtmp           = ioports_ptr (value);
                                token_label    = dtmp -> name;
                                value          = sizeof (int8_t) * strlen (token_label) + 1;
                                tmp -> label   = (int8_t *) malloc (value);
                                if (fmt       != FORMAT_DEFAULT)
                                {
                                    tmp -> fmt = fmt;
                                }
                                else
                                {
                                    tmp -> fmt = dtmp -> fmt;
                                }
                            }
                            strcpy (tmp -> label, token_label);
                            dpoint             = list_add (dpoint, tmp);
                            break;
                    }
                }
                else if (byte         == 'l') // label
                {
                    action             = INPUT_LABEL;
                    token              = strtok ((string + match[2].rm_so), " ");
                    result             = parse_token (token, *(pattern+2), PARSE_MEMORY);
                    fprintf (debug, "[label] token: '%s', result: %X\n", token, result);
                    value              = strtol (token, NULL, 16);
                    ltmp               = listnode (LIST_MEM, value);
                    token_label        = strtok ((string + match[3].rm_so), " ");
                    fprintf (debug, "[label] token_label: '%s'\n", token_label);
                    value              = sizeof (int8_t) * strlen (token_label) + 1;
                    ltmp -> label      = (int8_t *) malloc (value);
                    strcpy (ltmp -> label, token_label);
                    lpoint             = list_add (lpoint, ltmp);
                }
                else if (byte         == 'p') // print
                {
                    action             = INPUT_PRINT;
                    fprintf (debug, "(string+match[1].rm_so): %s\n", (string+match[1].rm_so));
                    strcpy (entry, (string+match[1].rm_so));
                    fprintf (debug, "[before] entry: %s\n", entry);
                    token              = strtok (entry, " ");
                    fprintf (debug, "[after]  entry: %s\n", entry);
                    token              = strtok (entry,  "/");
                    fprintf (debug, "[after]  token: %s\n", token);
                    pos                = strtok (NULL,   "/");
                    fprintf (debug, "pos:            %s\n", pos);
                    if ((pos          != NULL) &&
                        (*(pos+0)          != '\0'))
                    {
                        if (*(pos+0)      == 'b')
                        {
                            fmt            = FORMAT_BINARY;
                        }
                        else if (*pos     == 'd')
                        {
                            fmt            = FORMAT_SIGNED;
                        }
                        else if (*pos     == 'f')
                        {
                            fmt            = FORMAT_FLOAT;
                        }
                        else if (*pos     == 'o')
                        {
                            fmt            = FORMAT_OCTAL;
                        }
                        else if (*pos     == 'u')
                        {
                            fmt            = FORMAT_UNSIGNED;
                        }
                        else if (*pos     == 'x')
                        {
                            fmt            = FORMAT_LOWERHEX;
                        }
                        else if (*pos     == 'X')
                        {
                            fmt            = FORMAT_HEX;
                        }
                    }
                    token              = strtok ((string + match[2].rm_so), " "); // move to next token

                    fprintf (debug, "[print] parsing: token: %s, fmt: %u\n", token, fmt);
                    for (count      = PARSE_REGISTER;
                         count     <= PARSE_IOPORT;
                         count      = count + 1)
                    {
                        ////////////////////////////////////////////////////////////////
                        //
                        // PARSE_REGISTER          0x7A
                        // PARSE_REGISTERS         0x7B
                        // PARSE_MEMORY            0x7C
                        // PARSE_MEMRANGE          0x7D
                        // PARSE_IOPORT            0x7E
                        // PARSE_NONE              0x7F
                        //
                        result      = parse_token (token,
                                                   *(pattern+(count-0x7A)),
                                                   count);
                        if (result != PARSE_NONE)
                        {
                            break;
                        }
                    }

                    switch (result)
                    {
                        case PARSE_NONE:
                            fprintf (stderr, "[ERROR] malformed display value\n");
                            break;

                        case PARSE_MEMRANGE:
                            pos                 = strtok (token, "-");
                            value               = strtol (pos,   NULL, 16);
                            pos                 = strtok (NULL,  "-");
                            check               = strtol (pos,   NULL, 16);
                            for (index          = value;
                                 index         <= check;
                                 index          = index + 1)
                            {
                                output_mem (index, fmt, *flag, NULL);
                            }
                            break;

                        case PARSE_MEMORY:
                            value               = strtol (token, NULL, 16);
                            fprintf (debug, "[input/print] token: '%s', value: 0x%.8X\n", token, value);
                            output_mem (value, fmt, *flag, NULL);
                            break;

                        case PARSE_REGISTERS:
                            for (index          = 0;
                                 index         <= 15;
                                 index          = index + 1)
                            {
                                output_reg (index, fmt, *flag, NULL);
                            }
                            break;

                        case PARSE_REGISTER: // specific, general register
                            result              = parse_reg (token);
                            if (result         <  NUM_REGISTERS)
                            {
                                output_reg (result, fmt, *flag, NULL);
                            }
                            break;

                        case PARSE_IOPORT:
                            value               = strtol (token, NULL, 16);
                            dtmp                = ioports_ptr (value);
                            output_iop (value, dtmp -> fmt, dtmp -> name);
                            break;
                    }
                }
                else if (byte      == 'u') // un-something
                {
                    action          = INPUT_UNDO;
                    token           = strtok ((string + match[2].rm_so), " ");

                    value           = strtol (token, NULL, 10);
                    byte            = *(string + match[1].rm_so + 2);
                    if (byte       == 'b')
                    {
                        list        = &bpoint;
                    }
                    else if (byte  == 'd')
                    {
                        list        = &dpoint;
                    }
                    else if (byte  == 'l')
                    {
                        list        = &lpoint;
                    }

                    count           = 0;
                    tmp             = *list;
                    while (tmp     != NULL)
                    {
                        if (count  == value)
                        {
                            break;
                        }
                        count       = count + 1;
                        tmp         = tmp -> next;
                    }

                    tmp             = list_grab (list, tmp);
                    if (tmp        != NULL)
                    {
                        free (tmp);
                        tmp         = NULL;
                    }
                }
            }

            for (count          = 0;
                 count         <  5;
                 count          = count + 1)
            {
                fprintf (debug, "match[%d]: %.*s (%lld - %lld)\n",
                                count,
                                (int) (match[count].rm_eo - match[count].rm_so),
                                (string + match[count].rm_so),
                                (long long int) match[count].rm_so,
                                (long long int) match[count].rm_eo);
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
            fprintf (stderr, "[ERROR] RegEx execution failed\n");
            exit    (REGEX_EXECUTE_ERROR);
        }

        regfree (&regex);
    }

    free    (form);
    free    (pattern);

    return  (result);
}

uint8_t *get_input (FILE *fptr, const uint8_t *prompt)
{
    ////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    size_t   input_length  = 0;
    uint8_t *input_string  = NULL;

    ////////////////////////////////////////////////////////////////////////////////
    //
    // If colors are enabled, highlight the decoded instruction
    //
    if (colorflag         == TRUE)
    {
        fprintf (stdout, "\e[1;32m");
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // obtain input (via readline)
    //
    input_string           = readline (prompt);

    ////////////////////////////////////////////////////////////////////////////////
    //
    // If colors are enabled, complete the highlight the decoded instruction
    //
    if (colorflag         == TRUE)
    {
        fprintf (stdout, "\e[m");
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // if the line is not empty, add it to the readline history
    //
    input_length           = strlen (input_string);
    if (input_length      >  0)
    {
        add_history (input_string);
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // display what was input
    //
    fprintf (debug, "[get_input] input_string: \"%s\" (input_length: %lu)\n",
                    input_string, input_length);

    if (input_length      == 0)
    {
        action             = INPUT_NONE;
    }

    return (input_string);
}

uint8_t  prompt (uint32_t  word)
{
    int32_t    value                    = 0;
    static uint8_t   *input             = NULL;
    static uint8_t    lastaction        = INPUT_INIT;
    uint8_t    deref_flag               = FALSE;
    uint8_t    processflag              = FALSE;
    //uint8_t    token_type               = PARSE_NONE;

    if ((action                        != INPUT_NONE) &&
        (action                        != INPUT_INIT))
    {
        displayshow  (dpoint, 0);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Display the prompt and obtain input
    //
    if (input                          != NULL)
    {
        free (input);
    }

    input                               = get_input (stdin, "v32sim> ");
    
    if (*input                         == '\0')
    {
        action                          = lastaction;
    }

    //tokenize_asm   (input);

    if (action                         != INPUT_NONE)
    {
        //token_type                      = tokenize_input (input, &deref_flag);
        tokenize_input (input, &deref_flag);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // newcommand will be the character recently input; if newline,
    // repeat lastcommand
    //
    else
    {
        action                          = lastaction;
    }

    switch (action)
    {
        case INPUT_BREAK:
        case INPUT_LABEL:
        case INPUT_REPLACE:
        case INPUT_SET:
        case INPUT_UNDO:
            processflag                 = FALSE;
            action                      = INPUT_INIT;
            break;

        case INPUT_CONTINUE:
        case INPUT_IGNORE:
            processflag                 = TRUE;
            runflag                     = TRUE;
            break;

        case INPUT_PRINT:
            lastaction                  = INPUT_NONE;
            break;

        case INPUT_DISPLAY:
            lastaction                 = INPUT_NONE;
            break;

        case INPUT_QUIT:
            processflag                = 2;
            break;

        case INPUT_STEP:
            processflag                = TRUE;
            break;

        case INPUT_NEXT:
            ////////////////////////////////////////////////////////////////////////////
            //
            // if the instruction is a CALL, set seek_word and then
            // set runflag to TRUE; otherwise, should behave like step
            //
            value                      = (word & OPCODE_MASK) >> OPCODESHIFT;
            if (value                 == 0x03)
            {
                runflag                = TRUE;
                seek_word              = REG(IP) + 1;
                value                  = (word & IMMVAL_MASK);
                if (value             >  0)
                {
                    seek_word          = seek_word  + 1; // if immediate value
                }
            }
            processflag                = TRUE;
            break;

        case INPUT_HELP:
            fprintf (stdout, "  (b)reak 0xMEM|LABEL   - set breakpoint\n");
            fprintf (stdout, "  (c)ontinue            - resume execution\n");
            fprintf (stdout, "  (p)rint XYZ           - one-time display of XYZ\n");
            fprintf (stdout, "  (d)isplay XYZ LABEL   - add displaylist item:\n");
            fprintf (stdout, "    R#                  -   general register\n");
            fprintf (stdout, "    [R#]                -   dereferenced register\n");
            fprintf (stdout, "    I(P|R|V)            -   system register\n");
            fprintf (stdout, "    0xMEM_ADDR          -   4-byte memory address\n");
            fprintf (stdout, "    [0xMEM_ADDR]        -   dereferenced address\n");
            fprintf (stdout, "    0xMEM-0xADDR        -   memory range\n");
            fprintf (stdout, "    [0xMEM-0xADDR]      -   deref memory range\n");
            fprintf (stdout, "    0xIOP               -   IOPort address\n");
            fprintf (stdout, "  (l)abel 0xMEM LABEL   - add label list item\n");
            fprintf (stdout, "  (n)ext                - next (skip subroutines)\n");
            fprintf (stdout, "  (s)tep                - step to next instruction\n");
            fprintf (stdout, "  (i)gnore              - ignore this instruction\n");
            fprintf (stdout, "  (r)eplace X Y Z       - replace:\n");
            fprintf (stdout, "    IP:0xMEM_ADDR       -   with this IP value\n");
            fprintf (stdout, "    IR:0xINSTRUCT       -   with this IR value\n");
            fprintf (stdout, "    IV:0xIMMEDIAT       -   with this IV value\n");
            fprintf (stdout, "  set NAME = VALUE      - set system feature\n");
            fprintf (stdout, "    color:  true/false  -   set color output\n");
            fprintf (stdout, "    deref:  true/false  -   set deref addr\n");
            fprintf (stdout, "    I[PRV]: 0x0ADDRESS  -   set system register\n");
            fprintf (stdout, "    R#:     0xTHEVALUE  -   set register to value\n");
            fprintf (stdout, "    0xMEM:  0xTHEVALUE  -   set memory to value\n");
            fprintf (stdout, "    0xIOP:  0xTHEVALUE  -   set ioport to value\n");
            fprintf (stdout, "  (u)nXYZ               - remove item from list\n");
            fprintf (stdout, "    break               -   remove breakpoint #\n");
            fprintf (stdout, "    display             -   remove displaypoint #\n");
            fprintf (stdout, "    label               -   remove label #\n");
            fprintf (stdout, "  (h)elp/(?)            - display this help\n");
            fprintf (stdout, "  (q)uit                - exit the simulator\n");
            action                     = INPUT_INIT;
            break;
    }
    lastaction                         = action;

    return (processflag);
}

uint32_t  load_labels (uint8_t *filename)
{
    linked_l *ltmp                     = NULL;
    FILE     *fptr                     = NULL;
    int32_t   index                    = 0;
    int32_t   value                    = 0;
    uint32_t  offset                   = 0;
    uint32_t  line_number              = 0;
    uint32_t  tally                    = 0;
    uint8_t   input[128];
    uint8_t  *input_string             = NULL;
    uint8_t   token[128];

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Generate debug filename
    //
    strncpy (token, filename, strlen (filename));
    fprintf (debug, "[load_labels] filename:     %s\n", filename);
    fprintf (debug, "[load_labels] token:        %s\n", token);

    input_string                       = strtok (token, ".");
    fprintf (debug, "[load_labels] token:        %s\n", token);
    fprintf (debug, "[load_labels] input_string: %s\n", input_string);
    sprintf (input, "%s.vbin.debug", input_string);
    fprintf (debug, "[load_labels] input:        %s\n", input);
    
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Load labels from debug file (if it exists)
    //
    if (filename                      != NULL)
    {
        fptr                           = fopen (input, "r");
        if (fptr                      == NULL)
        {
            fprintf (verbose, "No debug file for '%s' found. Skipping...\n", input);
        }
        else
        {
            fprintf (verbose, "LOADING DEBUGGING DATA FOR: %s\n", filename);
            while (!feof (fptr))
            {
                index                  = 0;
                input[index]           = ' ';
                while (input[index]   != '\0')
                {
                    input[index]       = fgetc (fptr);
                    if (input[index]  == '\n')
                    {
                        input[index]   = '\0';
                        break;
                    }

                    if (feof (fptr))
                    {
                        break;
                    }
                    index              = index + 1;
                    input[index]       = ' ';
                }
            
                if (feof (fptr))
                {
                    break;
                }
                ////////////////////////////////////////////////////////////////////////
                //
                // Vircon32 assembler .debug files are of the following format:
                //
                // 0x10001AE4,obj/debuggerBIOS.asm,4654
                // 0x10001AE6,obj/debuggerBIOS.asm,4656,__for_3281_continue
                //
                // For label loading, we are specifically interested in the lines
                // with a label in the last field (second example)
                //
                input_string           = strtok (input, ","); // offset
                offset                 = strtol (input_string, NULL, 16);

                input_string           = strtok (NULL, ",");  // filename
                input_string           = strtok (NULL, ",");  // line number
                line_number            = strtol (input_string, NULL, 10);

                input_string           = strtok (NULL, ",");  // label, if present
                ltmp                   = listnode (LIST_MEM, offset);
                ltmp -> number         = line_number;
                if (input_string      != NULL)
                {
                    value              = sizeof (int8_t) * strlen (input_string) + 1;
                    ltmp -> label      = (int8_t *) malloc (value);
                    strcpy (ltmp -> label, input_string);
                }
                lpoint                 = list_add (lpoint, ltmp);
                tally                  = tally + 1;
            }
            fclose (fptr);
        }
    }

    return (tally);
}    

void load_command (void)
{
    FILE     *fptr                     = NULL;
    int32_t   index                    = 0;
    uint8_t   deref_flag               = FALSE;
    uint8_t   input[64];
    //uint8_t   token_type               = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Load commands from command-file
    //
    if (commandfile                   != NULL)
    {
        fptr                           = fopen (commandfile, "r");
        if (fptr                      == NULL)
        {
            fprintf (stderr, "[ERROR] Could not open '%s' for reading!\n", commandfile);
            exit (1);
        }

        while (!feof (fptr))
        {
            index                      = 0;
            token_label                = NULL;
            while (!feof (fptr))
            {
                input[index]           = fgetc (fptr);
                if (input[index]      == '\n')
                {
                    input[index]       = '\0';
                    break;
                }
                index                  = index + 1;
            }

            if (feof (fptr))
            {
                break;
            }
            //token_type                 = tokenize_input (input, &deref_flag);
            tokenize_input (input, &deref_flag);
        }
        commandfile                    = NULL;
        fclose (fptr);
    }
    action                             = INPUT_INIT;
}
