#include "defines.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <regex.h>

uint64_t   parse_imm (uint8_t *token)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    uint64_t    result       = 0xFFFFFFFFFFFFFFFF;
    int32_t     check        = 0;
    int32_t     index        = 0;
    int32_t     value        = 0;
    uint8_t     byte         = 0;
    regex_t     regex;
    regmatch_t  match[2];
    uint8_t   **pattern      = NULL;
    uint8_t    *form0        = "^ *(0x[0-9a-fA-F]{1,8}) *$";             // hex
    uint8_t    *form1        = "^ *s[[] *(0x[0-9a-fA-F]{1,8}) *[]] *$";  // hex ptr
    uint8_t    *form2        = "^ *([0-9a-fA-F]{1,8}[hH]) *$";           // hex
    uint8_t    *form3        = "^ *[[] *([0-9a-fA-F]{1,8}[hH]) *[]] *$"; // hex ptr
    uint8_t    *form4        = "^ *(0b[01]{1,32}) *$";                   // bin
    uint8_t    *form5        = "^ *[[] *(0b[01]{1,32}) *[]] *$";         // bin ptr
    uint8_t    *form6        = "^ *(0[0-7]{1,10}) *$";                   // oct
    uint8_t    *form7        = "^ *[[] *(0[0-7]{1,10}) *[]] *$";         // oct ptr
    uint8_t    *form8        = "^ *([0-9]|[1-9][0-9]*) *$";              // dec
    uint8_t    *form9        = "^ *[[] *([0-9]|[1-9][0-9]*) *[]] *$";    // dec ptr

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
        check                = regcomp (&regex, *(pattern+index), REG_EXTENDED);
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
            fprintf (stderr, "ERROR: invalid immediate value\n");
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
            }
            else if ((index >= 2) && // hex
                     (index <= 3))
            {
                //trim the suffixing H/h
                result       = strtol (token, NULL, 16);
            }
            else if ((index >= 4) &&
                     (index <= 5))   // bin
            {
                //trim the prefixing 0b
                result       = strtol (token, NULL, 2);
            }
            else if ((index >= 6) &&
                     (index <= 7))   // oct
            {
                result       = strtol (token, NULL, 8);
            }
            else
            {
                result       = strtol (token, NULL, 10);
            }

            ////////////////////////////////////////////////////////////////////////////
            //
            // for odd values of index, we have a dereference case; set the MSB
            // of result (a 64-bit unsigned) high.
            //
            if ((index % 1) == 1)
            {
                result       = result | 0x8000000000000000;
            }

            regfree (&regex);
            break;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx execution error: bail out (shouldn't happen)
        //
        else
        {
            fprintf (stderr, "[ERROR] RegEx execution failed\n");
            exit    (REGEX_EXECUTE_ERROR);
        }

        regfree (&regex);
    }
    free    (pattern);

    fprintf (verbose, "[parse_imm] result: %llu (%.16llX)\n", result, result);

    return  (result);
}

uint8_t   parse_reg (uint8_t *string)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    int32_t     check           = 0;
    int32_t     value           = 0;
    uint8_t     byte            = 0;
    uint8_t     result          = 0xFF;
    regex_t     regex;
    regmatch_t  match[2];
    uint8_t    *pattern         = "^ *([rR][0-9]|[rR]1[0-5]|[sS][pP]|[bB][pP]|[cC][rR]|[sS][rR]|[dD][rR]|[iI][pP]|[iI][rR]|[iI][vV]) *$";

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // RegEx compilation: compile pattern into our regex for processing
    //
    check                       = regcomp (&regex, pattern, REG_EXTENDED);
    if (check                  != 0)
    {
        fprintf (stderr, "[ERROR] RegEx compilation failed\n");
        exit    (REGEX_COMPILE_ERROR);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // RegEx execution: make sure input conforms to provided pattern
    //
    check                       = regexec (&regex, string, 2, match, 0);
    if (check                  == REG_NOMATCH)
    {
        fprintf (stderr, "ERROR: invalid register\n");
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // RegEx execution success! Display the results
    //
    else if (check             == 0)
    {
        byte                    = (string + match[1].rm_so);
        switch (byte)
        {
            case 'r':
            case 'R':
                value           = atoi ((string + match[1].rm_so + 1));
                result          = value & 0x000000FF;
                fprintf (stdout, "[parse_reg] result: %u\n", result);
                break;

            case 'b':
            case 'B':
                result          = BP; // BP (R14)
                break;

            case 'c':
            case 'C':
                result          = CR; // CR (R11)
                break;

            case 'd':
            case 'D':
                result          = DR; // DR (R13)
                break;

            case 's':
            case 'S':
                byte            = *(string + match[1].rm_so + 2);
                switch (byte)
                {
                    case 'p':
                    case 'P':
                        result  = SP; // SP (R15)
                        break;

                    case 'r':
                    case 'R':
                        result  = SR; // SR (R12)
                        break;
                }
                break;

            case 'i':
            case 'I':
                byte            = *(string + match[1].rm_so + 2);
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
                }
                break;
        }
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

    fprintf (verbose, "[parse_reg] result: %hhu (%.2hhX)\n", result, result);

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
    uint8_t     opcode           = 0;
    uint8_t     immflag          = 0;
    uint8_t     dstreg           = 0;
    uint8_t     srcreg           = 0;
    uint8_t     addr             = 0;
    uint16_t    port             = 0;
    regex_t     regex;
    regmatch_t  match[4];
    uint8_t    *pattern          = "^ *([a-zA-Z][a-zA-Z2]{1,4}) +([rR][0-9]|[rR]1[0-5]|[sSbBcCdDsS][pPrR]|0x[0-9a-fA-F]{1,8}|[0-9]|[1-9][0-9]+) *, +([rR][0-9]|[rR]1[0-5]|[sSbBcCdDsS][pPrR]|0x[0-9a-fA-F]{1,8}|[0-9]|[1-9][0-9]+) *$";

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
    check                        = regcomp (&regex, pattern, REG_EXTENDED);
    if (check                   != 0)
    {
        fprintf (stderr, "[ERROR] RegEx compilation failed\n");
        exit    (REGEX_COMPILE_ERROR);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // RegEx execution: make sure input conforms to provided pattern
    //
    check                        = regexec (&regex, string, 4, match, 0);
    if (check                   == REG_NOMATCH)
    {
        fprintf (stderr, "ERROR: malformed input on ASM\n");
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
            fprintf (stdout, "match[%d]: %.*s (%lld - %lld)\n",
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
        for (index               = 0;
             index              <  64;
             index               = index + 1)
        {
            check                = strcmp ((string+match[2].rm_so), lookup[index].name);
			fprintf (stdout, "[check: %u] token: '%s', opcode: '%s'\n", check, (string+match[2].rm_so), lookup[index].name);
            if (check           == 0)
            {
                opcode           = index;
                break;
            }
        }
		fprintf (stdout, "opcode is: 0x%.2hhX\n", opcode);

        ////////////////////////////////////////////////////////////////////////////////
        //
        // parse the instruction tokens
        //
        switch (opcode)
        {
            case HLT:
            case WAIT:
            case RET:
                instruction      = opcode << 26;
                break;

            case CALL:
            case JMP:
                dstreg           = parse_reg ((string + match[2].rm_so));
                if (dstreg      == 0xFF) // was not a valid register, try immediate
                {
                    result       = parse_imm ((string + match[2].rm_so));
                    immflag      = 1;
                }
                break;

			case MOV:
				fprintf (stdout, "[MOV]\n");
                dstreg           = parse_reg ((string + match[2].rm_so));
                if (dstreg      == 0xFF) // was not a valid register, try immediate
                {
                    result       = parse_imm ((string + match[2].rm_so));
                    immflag      = 1;
                }
                break;
        }
        instruction              = (opcode  << 26);
        instruction             |= (immflag << 25);
        instruction             |= (dstreg  << 21);
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

uint8_t  tokenize_input (uint8_t *string)
{
    ////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    int32_t     index      = 0;
    int32_t     result     = 0;
    regex_t     regex;
    regmatch_t  match[4];
//uint8_t    *pattern    = "^ *([a-zA-Z][a-zA-Z2]{1,4}) +([rR][0-9]|[rR]1[0-5]|[sSbBcCdDsS][pPrR]|0x[0-9a-fA-F]{1,8}|[0-9]|[1-9][0-9]+) *, +([rR][0-9]|[rR]1[0-5]|[sSbBcCdDsS][pPrR]|0x[0-9a-fA-F]{1,8}|[0-9]|[1-9][0-9]+) *$";
    uint8_t    *pattern    = "^ *([cC][oO][nN][tT][iI][nN][uU][eE]|[cC]|[sS][tT][eE][pP]|[sS]|[rR][eE][gG][iI][sS][tT][eE][rR]|[rR]|[dD][iI][sS][pP][lL][aA][yY]|[dD]) *$";

    ////////////////////////////////////////////////////////////////////////////////
    //
    // RegEx compilation: compile pattern into our regex for processing
    //
    result                 = regcomp (&regex, pattern, REG_EXTENDED);
    if (result            != 0)
    {
        fprintf (stderr, "[ERROR] RegEx compilation failed\n");
        exit    (REGEX_COMPILE_ERROR);
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // RegEx execution: make sure input conforms to provided pattern
    //
    result                 = regexec (&regex, string, 4, match, 0);
    if (result            == REG_NOMATCH)
    {
        fprintf (stderr, "ERROR: malformed input\n");
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // RegEx execution success! Display the results
    //
    else if (result       == 0)
    {
        for (index         = 0;
             index        <  4;
             index         = index + 1)
        {
            fprintf (stdout, "match[%d]: %.*s (%lld - %lld)\n",
                             index,
                             (int) (match[index].rm_eo - match[index].rm_so),
                             (string + match[index].rm_so),
                             (long long int) match[index].rm_so,
                             (long long int) match[index].rm_eo);
        }
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

    return (result);
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
    // obtain input (via readline)
    //
    input_string           = readline (prompt);

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
    fprintf (verbose, "[get_input] input_string: \"%s\" (input_length: %lu)\n",
                      input_string, input_length);

    return (input_string);
}
