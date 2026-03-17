#include "defines.h"
#include <regex.h>

uint32_t  tokenize_asm (uint8_t *input)
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
    //uint8_t    *pattern          = "^ *([a-z][a-z2]{1,4}) +(R[0-9]|R1[0-5]|[BCDS][PR]|0x[0-9A-F]{1,8}|[0-9]|[1-9][0-9]+) *, +(R[0-9]|R1[0-5]|[BCDS][PR]|0x[0-9A-F]{1,8}|[0-9]|[1-9][0-9]+) *$";
	uint8_t    *string           = NULL;
    uint8_t   **pattern          = NULL;
    uint8_t    *form0            = "^ *(HLT|WAIT|RET|MOVS|SETS) *$";   // 0 operand
    uint8_t    *form1            = "^ *(JMP|CALL) *([^ ]+) *$"; // 1 operand (register or immediate)
    uint8_t    *form2            = "^ *(PUSH|POP|CMPS|CI[BF]|CF[BI]|B?NOT|[IF]SGN|[IF]ABS|FLR|CEIL|ROUND|SIN|ACOS|LOG) *([^ ]+) *$";   // 1 operand (only register)
    uint8_t    *form3            = "^ *([rR][0-9]|[rR]1[0-5]) *$";   // 2 operand
    uint8_t    *form4            = "^ *(J[FT]|IEQ|INE|I[GL][ET]|AND|X?OR|SHL|IADD|ISUB|IMUL|IDIV|IMOD|IMIN|IMAX) *([^ ]+) *, *([^ ]+) *$"; // 2 operand, reg and immediate
    uint8_t    *form5            = "^ *(FEQ|FNE|F[GL][ET]|FADD|FSUB|FMUL|FDIV|FMOD|FMIN|FMAX) *([^ ]+) *, *([^ ]+) *$"; // 2 operand, reg and immediate (float)
	uint8_t    *form6            = "^ *(ATAN2|POW) *([^ ]+) *, *([^ ]+) *$";
	uint8_t    *form7            = "^ *(LEA) *([^ ]+) *, *([^ ]+) *$";
	uint8_t    *form8            = "^ *(MOV) *([^ ]+) *, *([^ ]+) *$";

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
    // allocate and populate pattern array
    //
    pattern                      = (uint8_t **) malloc (sizeof (uint8_t *) * 9);
    *(pattern+0)                 = form0;
    *(pattern+1)                 = form1;
    *(pattern+2)                 = form2;
    *(pattern+3)                 = form3;
    *(pattern+4)                 = form4;
    *(pattern+5)                 = form5;
    *(pattern+6)                 = form6;
    *(pattern+7)                 = form7;
    *(pattern+8)                 = form8;

    for (index                   = 0;
         index                  <  9;
         index                   = index + 1)
    {
        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx compilation: compile current pattern into our regex for processing
        //
        check                    = regcomp (&regex,
                                            *(pattern+index),
                                            REG_EXTENDED | REG_ICASE);
        if (check               != 0)
        {
            fprintf (stderr, "[tokenize_asm] RegEx compilation failed\n");
            exit    (REGEX_COMPILE_ERROR);
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx execution: make sure input conforms to provided pattern
        //
        check                    = regexec (&regex, input, 4, match, 0);
        if (check               == REG_NOMATCH)
        {
            fprintf (verbose, "[tokenize_asm] malformed input\n");
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx execution success: obtain the value.
        //
        else if (check          == 0)
        {
            fprintf (verbose, "[tokenize_asm] match on input (form %u)\n", index);

            ////////////////////////////////////////////////////////////////////////////
            //
            // allocate memory for reconstructed string
            //
            string               = (uint8_t *) malloc (sizeof (uint8_t) * strlen (input));
            snprintf (string, strlen (input) - 1, "%.*s %.*s %.*s",
                      (int) (match[1].rm_eo - match[1].rm_so),
                      (input + match[1].rm_so),
                      (int) (match[2].rm_eo - match[2].rm_so),
                      (input + match[2].rm_so),
                      (int) (match[3].rm_eo - match[3].rm_so),
                      (input + match[3].rm_so));
			regfree (&regex);
            break;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx execution error: bail out (shouldn't happen)
        //
        else
        {
            fprintf (stderr, "[tokenize_asm] ERROR: RegEx execution failed\n");
            exit    (REGEX_EXECUTE_ERROR);
        }

		regfree (&regex);
	}

	free (pattern);

	/*
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
            check                = strncasecmp (source, lookup[index].name, 5);
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
                    
                    //if (sysflag == FALSE)
                    //{
                    //   sysflag  = ((srcreg >  0x0F) >  0) ? TRUE : FALSE;
                    //}
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

    fprintf (verbose, "[tokenize_asm] assembling '%s' to: 0x%.8X\n",
                      input, instruction);

	*/
    return  (instruction);
}
