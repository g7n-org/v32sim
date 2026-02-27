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
                fprintf (verbose, "[parse_imm] imm type: binary\n",       result);
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

            ////////////////////////////////////////////////////////////////////////////
            //
            // for odd values of index, we have a dereference case; set the MSB
            // of result (a 64-bit unsigned) high.
            //
            if ((index % 1) == 1)
            {
                result       = result | 0x8000000000000000;
                fprintf (verbose, "[parse_imm] form: indirect\n",         result);
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
            fprintf (stderr, "[parse_imm] ERROR: RegEx execution failed\n");
            exit    (REGEX_EXECUTE_ERROR);
        }

        regfree (&regex);
    }
    free    (pattern);

    fprintf (verbose, "[parse_imm] result: %llu (%.16llX)\n", result, result);

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

    fprintf (verbose, "[parse_reg] passed token: '%s'\n", token);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // allocate and populate pattern array
    //
    pattern                     = (uint8_t **) malloc (sizeof (uint8_t *) * 8);
    *(pattern+0)                = form0;
    *(pattern+1)                = form1;
    *(pattern+2)                = form2;
    *(pattern+3)                = form3;
    *(pattern+4)                = form4;
    *(pattern+5)                = form5;
    *(pattern+6)                = form6;
    *(pattern+7)                = form7;

    for (index                  = 0;
         index                 <  8;
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
            fprintf (stderr, "ERROR: invalid register\n");
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
            else
            {
                fprintf (verbose, "[parse_reg] impossible situation\n");
            }

            ////////////////////////////////////////////////////////////////////////////
            //
            // for odd values of index, we have a dereference case; set the MSB
            // of result high.
            //
            if ((index % 1) == 1)
            {
                result       = result | 0x80;
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
            fprintf (verbose, "[tokenize_asm] match[%d]: %.*s (%lld - %lld)\n",
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
            fprintf (verbose, "[tokenize_asm] check: %d, token: '%s', opcode: '%s'\n",
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

uint8_t  tokenize_input (uint8_t *string)
{
    ////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    int32_t     check           = 0;
    int32_t     count           = 0;
    int32_t     index           = 0;
    int32_t     result          = 0;
    regex_t     regex;
    regmatch_t  match[4];
//uint8_t    *pattern    = "^ *([a-zA-Z][a-zA-Z2]{1,4}) +([rR][0-9]|[rR]1[0-5]|[sSbBcCdDsS][pPrR]|0x[0-9a-fA-F]{1,8}|[0-9]|[1-9][0-9]+) *, +([rR][0-9]|[rR]1[0-5]|[sSbBcCdDsS][pPrR]|0x[0-9a-fA-F]{1,8}|[0-9]|[1-9][0-9]+) *$";
    //uint8_t    *pattern    = "^ *(continue|c|step|s|register|r|display|d|memory|m|help|h) *$";
    uint8_t     byte            = 0;
    uint8_t   **pattern         = NULL;
    uint8_t    *form0           = "^ *(continue|c) *$";
    uint8_t    *form1           = "^ *(step|s) *$";
    uint8_t    *form2           = "^ *(next|n) *$";
    uint8_t    *form3           = "^ *(print|p) (r[0-9]|r1[0-9]|[bs]p|[csd]r|i[prv]|reg|regs|registers) *$";
    uint8_t    *form4           = "^ *(print|p) (0x[0-9A-F]{8}) *$";
    uint8_t    *form5           = "^ *(print|p) (0x[0-9A-F]{8})-(0x[0-9A-F]{8}) *$";
    uint8_t    *form6           = "^ *(print|p) (0x[0-7][01][A-F]) *$";
    uint8_t    *form7           = "^ *(display|d) (r[0-9]|r1[0-9]|[bs]p|[csd]r|i[prv]|reg|regs|registers) *$";
    uint8_t    *form8           = "^ *(display|d) (0x[0-9A-F]{8}) *$";
    uint8_t    *form9           = "^ *(display|d) (0x[0-9A-F]{8})-(0x[0-9A-F]{8}) *$";
    uint8_t    *form10          = "^ *(display|d) (0x[0-7][01][A-F]) *$";
    uint8_t    *form11          = "^ *(break|b) (0x[0-7][01][A-F]|[A-Z0-9_]+) *$";
    uint8_t    *form12          = "^ *(label|l) *([0-9]+) *([A-Z0-9_]+) *$";
    uint8_t    *form13          = "^ *(help|h|\?) *$";
    uint8_t    *form14          = "^ *(quit|exit|q) *$";

    fprintf (verbose, "[tokenize_input] passed string: '%s'\n", string);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // allocate and populate pattern array
    //
    pattern                  = (uint8_t **) malloc (sizeof (uint8_t *) * 15);
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
    *(pattern+10)            = form10;
    *(pattern+11)            = form11;
    *(pattern+12)            = form12;
    *(pattern+13)            = form13;
    *(pattern+14)            = form14;

    for (index                  = 0;
         index                 <  15;
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
        check                  = regexec (&regex, string, 4, match, 0);
        if (check             == REG_NOMATCH)
        {
            continue;
            fprintf (stderr, "ERROR: malformed input\n");
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx execution success! Display the results
        //
        else if (check        == 0)
        {
            byte               = *(string + match[count].rm_so);
            fprintf (stdout, "byte: '%c'\n", byte);
            switch (byte)
            {
                case 'b': // break
                    action     = INPUT_BREAK;
                    break;

                case 'c': // continue
                    action     = INPUT_CONTINUE;
                    break;

                case 'd': // display
                    action     = INPUT_DISPLAY;
                    break;

                case 'l': // label
                    action     = INPUT_LABEL;
                    break;

                case 'n': // next
                    action     = INPUT_NEXT;
                    break;

                case 'p': // print
                    action     = INPUT_PRINT;
                    break;

                case 's': // step
                    action     = INPUT_STEP;
                    fprintf (stdout, "action: step\n");
                    break;

                case 'h': // help
                case '?': // help
                    action     = INPUT_HELP;
                    break;

                case 'e': // exit
                case 'q': // quit
                    action     = INPUT_QUIT;
                    break;
            }
            for (count         = 0;
                 count        <  4;
                 count         = count + 1)
            {
                fprintf (stdout, "match[%d]: %.*s (%lld - %lld)\n",
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

    free    (pattern);

    fprintf (verbose, "[tokenize_input] result: %llu (%.16llX)\n", result, result);

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

    if (input_length      == 0)
    {
        action             = INPUT_NONE;
    }

    return (input_string);
}
