#include "defines.h"

uint8_t  decode (uint32_t  instruction,
                 uint32_t  immediate,
                 float     fimmediate,
                 uint8_t   flags)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    uint8_t   displayflag  = (flags & FLAG_DISPLAY)   ? TRUE : FALSE;
    uint8_t   processflag  = (flags & FLAG_PROCESS)   ? TRUE : FALSE;
    uint8_t   result       = TRUE;

    ////////////////////////////////////////////////////////////////////////////////
    //
    // If we are only concerned with displaying the assembly, call the
    // decode_display() function
    //
    if (displayflag       == TRUE)
    {
        decode_display (instruction, immediate, fimmediate, flags);
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // If we are only concerned with processing the assembly, call the
    // decode_process() function
    //
    else if (processflag  == TRUE)
    {
        decode_process (instruction, immediate, fimmediate, flags);
    }

    return (result);
}

void  decode_display (uint32_t  instruction,
                      uint32_t  immediate,
                      float     fimmediate,
                      uint8_t   flags)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    linked_l *ltmp                 = NULL;
    int8_t    sign                 = '+';
    int8_t    space                = -5;
    int8_t    spacing              = -16;
    uint8_t   displayflag          = (flags & FLAG_DISPLAY)   ? TRUE : FALSE;
    uint8_t   immflag              = (flags & FLAG_IMMEDIATE) ? TRUE : FALSE;
    uint8_t   dataflag             = (flags & FLAG_DATA)      ? TRUE : FALSE;
    uint8_t   errorflag            = (flags & FLAG_ERROR)     ? TRUE : FALSE;
    uint8_t   opcode               = (instruction & OPCODE_MASK) >> OPCODESHIFT;
    uint32_t  dst                  = (instruction & DSTREG_MASK) >> DSTREGSHIFT;
    uint32_t  src                  = (instruction & SRCREG_MASK) >> SRCREGSHIFT;
    uint8_t   addr                 = (instruction & MOVADR_MASK) >> MOVADRSHIFT;
    uint16_t  port                 = (instruction & IOPORT_MASK);
    uint32_t  value                = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // initialize our opcodes array with the available instructions
    //
    opcode_t  lookup[64]           =
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

    ////////////////////////////////////////////////////////////////////////////////
    //
    // Set display FILE pointer to appropriate destination (based on flags)
    //
    display                        = (displayflag == TRUE) ? stdout : devnull;

    ////////////////////////////////////////////////////////////////////////////////
    //
    // If colors are enabled, highlight the decoded instruction
    //
    if ((colorflag                == TRUE) &&
        (dataflag                 == FALSE))
    {
        if (errorflag             == FALSE)
        {
            fprintf (stdout, "\e[1;33m");
        }
        else
        {
            fprintf (stdout, "\e[1;31m");
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // If errorflag is tripped, display visual warning indicator
    //
    if (errorflag                 == TRUE)
    {
        fprintf (stdout, "[!]");
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // If decoding is in secondary form, eliminate pretty spacing
    //
    if (dataflag                  == TRUE)
    {
        space                        = 1;
        spacing                      = 1;
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // Determine opcode and perform needed processing, based on flags
    //
    switch (opcode)
    {
        case HLT: // special case for HLT to try and distinguish from data
            if (instruction         == 0x00000000)
            {
                fprintf (display,     "%*s",
                                      space, lookup[opcode].name);
            }
            break;

        case WAIT:
        case RET:
        case MOVS:
        case SETS:
            fprintf (display,         "%*s",
                                      space, lookup[opcode].name);
            break;

        case JMP:
        case CALL:
            if (immflag             == TRUE)
            {
                ltmp                 = find_value (lpoint, immediate);

                if (ltmp            == NULL)
                {
                    sprintf (destination, "0x%.8X", immediate);
                }
                else
                {
                    sprintf (destination, "%s",     ltmp -> label);
                }
            }
            else
            {
                sprintf (destination, "R%u",    dst);
            }
            fprintf (display,         "%*s %*s",
                                      space,   lookup[opcode].name,
                                      spacing, destination);
            break;

        case JT:
        case JF:
            sprintf (destination, "R%u,", dst);
            if (immflag             == TRUE)
            {
                ltmp                 = find_value (lpoint, immediate);

                if (ltmp            == NULL)
                {
                    sprintf (source, "0x%.8X", immediate);
                }
                else
                {
                    sprintf (source, "%s",     ltmp -> label);
                }
            }
            else
            {
                sprintf (source, "R%u",        src);
            }
            fprintf (display,    "%*s %*s %*s",
                                 space,   lookup[opcode].name,
                                 spacing, destination,
                                 spacing, source);
            break;

        case IEQ:
        case INE:
        case IGT:
        case IGE:
        case ILT:
        case ILE:
        case AND:
        case OR:
        case XOR:
        case SHL:
        case IADD:
        case ISUB:
        case IMUL:
        case IDIV:
        case IMOD:
        case IMIN:
        case IMAX:
            sprintf (destination, "R%u,", dst);
            if (immflag               == TRUE)
            {
                sprintf (source, "0x%.8X", immediate);
            }
            else
            {
                sprintf (source, "R%u",    src);
            }
            fprintf (display,    "%*s %*s %*s",
                                 space,   lookup[opcode].name,
                                 spacing, destination,
                                 spacing, source);
            break;

        case FEQ:
        case FNE:
        case FGT:
        case FGE:
        case FLT:
        case FLE:
        case FADD:
        case FSUB:
        case FMUL:
        case FDIV:
        case FMOD:
        case FMIN:
        case FMAX:
        case ATAN2:
        case POW:
            sprintf (destination, "R%u,", dst);
            if (immflag               == TRUE)
            {
                sprintf (source, "%.3f", fimmediate);
            }
            else
            {
                sprintf (source, "R%u",    src);
            }
            fprintf (display,      "%*s %*s %*s",
                                   space,   lookup[opcode].name,
                                   spacing, destination,
                                   spacing, source);
            break;

        case LEA:
            sprintf (destination,      "R%u,",          dst);
            if (immflag               == TRUE)
            {
                sign  = ((signed) immediate >= 0) ? '+' : '-';
                sprintf (source,       "[R%u%c%d]",     src, sign, abs ((signed) immediate));
                if (sign              == '+')
                {
                    value              = REG(src) + abs ((signed) immediate);
                }
                else
                {
                    value              = REG(src) - abs ((signed) immediate);
                }
                fprintf (display,      "%*s %*s %*s (deref address: 0x%.8X)",
                                       space,   lookup[opcode].name,
                                       spacing, destination,
                                       spacing, source, value);
            }
            else
            {
                sprintf (source,       "[R%u]",         src);
                fprintf (display,      "%*s %*s %*s",
                                       space,   lookup[opcode].name,
                                       spacing, destination,
                                       spacing, source);
            }
            break;

        case MOV:
            value                      = 0;
            switch (addr)
            {
                case 00: // MOV DSTREG, Immediate
                    sprintf (destination, "R%u,",          dst);
                    sprintf (source,      "0x%.8X",        immediate);
                    break;

                case 01: // MOV DSTREG, SRCREG
                    sprintf (destination, "R%u,",          dst);
                    sprintf (source,      "R%u",           src);
                    break;

                case 02: // MOV DSTREG, [Immediate]
                    sprintf (destination, "R%u,",          dst);
                    sprintf (source,      "[0x%.8X]",      immediate);
                    value              = immediate;
                    break;

                case 03: // MOV DSTREG, [SRCREG]
                    sprintf (destination, "R%u,",          dst);
                    sprintf (source,      "[R%u]",         src);
                    value              = REG(src);
                    if (derefaddr     == TRUE)
                    {
                        fprintf (debug, "REG(src): 0x%.8X\n", REG(src));
                        fprintf (debug, "[SRCREG]: 0x%.8X\n", IMEMGET(REG(src)));
                    }
                    break;

                case 04: // MOV DSTREG, [SRCREG+Immediate]
                    sprintf (destination, "R%u,",          dst);
                    sign  = ((signed) immediate >= 0) ? '+' : '-';
                    sprintf (source,      "[R%u%c%d]",  src, sign, abs ((signed) immediate));
                    if (sign          == '+')
                    {
                        value          = REG(src) + abs ((signed) immediate);
                    }
                    else
                    {
                        value          = REG(src) - abs ((signed) immediate);
                    }
                    if (derefaddr     == TRUE)
                    {
                        fprintf (debug, "value: 0x%.8X\n", value);
                        fprintf (debug, "[SRCREG+Imm]: 0x%.8X\n", IMEMGET(value));
                    }
                    break;

                case 05: // MOV [Immediate], SRCREG
                    sprintf (destination, "[0x%.8X],",     immediate);
                    sprintf (source,      "R%u",           src);
                    value              = immediate;
                    break;

                case 06: // MOV [DSTREG], SRCREG
                    sprintf (destination, "[R%u],",        dst);
                    sprintf (source,      "R%u",           src);
                    value              = REG(dst);
                    break;

                case 07: // MOV [DSTREG+Immediate], SRCREG
                    sign               = ((signed) immediate >= 0) ? '+' : '-';
                    value              = ((signed) immediate >= 0) ? immediate : -immediate;
                    sprintf (destination, "[R%u%c%d],", dst, sign, value);
                    sprintf (source,      "R%u",           src);
                    fprintf (debug,       "sign: '%c', value: %d\n", sign, value);
                    value              = REG(dst) + immediate;
                    break;
            }
            
            if ((derefaddr            == TRUE) && 
                (value                != 0))
            {
                fprintf (display,     "%*s %*s %*s ",
                                      space,   lookup[opcode].name,
                                      spacing, destination,
                                      spacing, source);
                if (colorflag                 == TRUE)
                {
                    fprintf (stdout, "\e[1;35m");
                }
                fprintf (display,     "(deref addr: 0x%.8X)", value);
                if (colorflag                 == TRUE)
                {
                    fprintf (stdout, "\e[m");
                }
            }
            else
            {
                fprintf (display,     "%*s %*s %*s",
                                      space,   lookup[opcode].name,
                                      spacing, destination,
                                      spacing, source);
            }
            break;

        case PUSH:
        case POP:
        case CIF:
        case CFI:
        case CIB:
        case CFB:
        case NOT:
        case BNOT:
        case CMPS:
        case ISGN:
        case IABS:
        case FSGN:
        case FABS:
        case FLR:
        case CEIL:
        case ROUND:
        case SIN:
        case ACOS:
        case LOG:
            sprintf (destination, "R%u",    dst);
            fprintf (display,     "%*s %*s",
                                  space,   lookup[opcode].name,
                                  spacing, destination);
            break;

        case IN:
            sprintf (destination, "R%u,",    dst);
            sprintf (source,      "0x%.3X",  port);
            fprintf (display,     "%*s %*s %*s",
                                  space,   lookup[opcode].name,
                                  spacing, destination,
                                  spacing, source);
            break;

        case OUT:
            sprintf (destination, "0x%.3X,", port);
            if (immflag                 == TRUE)
            {
                sprintf (source,  "0x%.3X",  immediate);
            }
            else
            {
                sprintf (source,  "R%u",     dst);
            }
            fprintf (display,     "%*s %*s %*s",
                                  space,   lookup[opcode].name,
                                  spacing, destination,
                                  spacing, source);
            break;

        default:
            fprintf (stderr, "[decode_display] unimplemented instruction! (0x%hhX)\n", opcode);
            break;
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // If errorflag is tripped, display visual warning indicator
    //
    if (errorflag                 == TRUE)
    {
        fprintf (stdout, "[!]");
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // If colors are enabled, complete the highlight of the decoded instruction
    //
    if (colorflag                 == TRUE)
    {
        fprintf (display, "\e[m");
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // If this decode display is part of a print or displaylist, don't do newline
    //
    if (dataflag                  == FALSE)
    {
        fprintf (display, "\n");
    }
}

void  decode_process (uint32_t  instruction,
                      uint32_t  immediate,
                      float     fimmediate,
                      uint8_t   flags)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    float     fvalue        = 0.0;
    int32_t   value         = 0;
    uint8_t   immflag       = (flags & FLAG_IMMEDIATE) ? TRUE : FALSE;
    uint8_t   opcode        = (instruction & OPCODE_MASK) >> OPCODESHIFT;
    uint32_t  dst           = (instruction & DSTREG_MASK) >> DSTREGSHIFT;
    uint32_t  src           = (instruction & SRCREG_MASK) >> SRCREGSHIFT;
    uint8_t   addr          = (instruction & MOVADR_MASK) >> MOVADRSHIFT;
    uint16_t  port          = (instruction & IOPORT_MASK);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Determine opcode and perform needed processing, based on flags
    //
    switch (opcode)
    {
        case HLT:
            haltflag        = TRUE;
            break;

        case WAIT:
                waitflag    = TRUE; // triggers TIM_CycleCounter to reset
                update_frame ();        // update Frame Counter
            break;

        case JMP:
            REG(IP)         = (immflag == TRUE)   ? immediate  : DSTREG;
            rom_offset      = REG(IP);
            branchflag      = TRUE;
            break;

        case CALL:
            REG(SP)         = REG(SP) - 1;   // PUSH REG(IP) value to stack
            value           = (immflag == TRUE)   ? 2          : 1;
            memory_set (REG(SP), (REG(IP) + value), FALSE);
            REG(IP)         = (immflag == TRUE)   ? immediate  : DSTREG;
            rom_offset      = REG(IP);
            branchflag      = TRUE;
            break;

        case RET:
            REG(IP)         = IMEMGET(REG(SP));
            rom_offset      = REG(IP);  // POP REG(IP) off stack
            REG(SP)         = REG(SP) + 1;
            branchflag      = TRUE;
            break;

        case JT:
            if (DSTREG     == TRUE)
            {
                REG(IP)     = (immflag == TRUE)   ? immediate  : SRCREG;
                rom_offset  = REG(IP);
                branchflag  = TRUE;
            }
            break;

        case JF:
            if (DSTREG     == FALSE)
            {
                REG(IP)     = (immflag == TRUE)   ? immediate  : SRCREG;
                rom_offset  = REG(IP);
                branchflag  = TRUE;
            }
            break;

        case IEQ:
            value           = (immflag == TRUE)   ? immediate  : SRCREG;
            DSTREG          = (DSTREG  == value)  ? TRUE       : FALSE;
            break;

        case INE:
            value           = (immflag == TRUE)   ? immediate  : SRCREG;
            DSTREG          = (DSTREG  != value)  ? TRUE       : FALSE;
            break;

        case IGT:
            value           = (immflag == TRUE)   ? immediate  : SRCREG;
            DSTREG          = (DSTREG  >  value)  ? TRUE       : FALSE;
            break;

        case IGE:
            value           = (immflag == TRUE)   ? immediate  : SRCREG;
            DSTREG          = (DSTREG  >= value)  ? TRUE       : FALSE;
            break;

        case ILT:
            value           = (immflag == TRUE)   ? immediate  : SRCREG;
            DSTREG          = (DSTREG  <  value)  ? TRUE       : FALSE;
            break;

        case ILE:
            fvalue          = (immflag == TRUE)   ? immediate  : SRCREG;
            DSTREG          = (DSTREG  <= value)  ? TRUE       : FALSE;
            break;

        case FEQ:
            fvalue          = (immflag == TRUE)   ? fimmediate : FSRCREG;
            FDSTREG         = (FDSTREG == fvalue) ? TRUE       : FALSE;
            break;

        case FNE:
            fvalue          = (immflag == TRUE)   ? fimmediate : FSRCREG;
            FDSTREG         = (FDSTREG != fvalue) ? TRUE       : FALSE;
            break;

        case FGT:
            fvalue          = (immflag == TRUE)   ? fimmediate : FSRCREG;
            FDSTREG         = (FDSTREG >  fvalue) ? TRUE       : FALSE;
            break;

        case FGE:
            fvalue          = (immflag == TRUE)   ? fimmediate : FSRCREG;
            FDSTREG         = (FDSTREG >= fvalue) ? TRUE       : FALSE;
            break;

        case FLT:
            fvalue          = (immflag == TRUE)   ? fimmediate : FSRCREG;
            FDSTREG         = (FDSTREG <  fvalue) ? TRUE       : FALSE;
            break;

        case FLE:
            fvalue          = (immflag == TRUE)   ? fimmediate : FSRCREG;
            FDSTREG         = (FDSTREG <= fvalue) ? TRUE       : FALSE;
            break;

        case LEA:
            if (immflag    == FALSE)
            {
                DSTREG      = SRCREG;
            }
            else
            {
                fprintf (debug, "SRCREG: 0x%.8X, immediate: 0x%.8X, S+i: 0x%.8X\n",
                        SRCREG, immediate, (SRCREG + immediate));
                DSTREG      = SRCREG + immediate;
            }
            break;

        case MOV:
            switch (addr)
            {
                case 00: // MOV DSTREG, Immediate
                    DSTREG  = immediate;
                    break;

                case 01: // MOV DSTREG, SRCREG
                    DSTREG  = SRCREG;
                    break;

                case 02: // MOV DSTREG, [Immediate]
                    fprintf (debug, "[decode_process] MOV(2): memory_get (0x%.8X)\n",
                            immediate);
                    DSTREG  = IMEMGET(immediate);
                    break;

                case 03: // MOV DSTREG, [SRCREG]
                    fprintf (debug, "[decode_process] MOV(3): memory_get (0x%.8X)\n",
                            SRCREG);
                    DSTREG  = IMEMGET(SRCREG);
                    break;

                case 04: // MOV DSTREG, [SRCREG+Immediate]
                    fprintf (debug, "[decode_process] MOV(4): memory_get (0x%.8X)\n",
                            (SRCREG + immediate));
                    DSTREG  = IMEMGET(SRCREG + immediate);
                    break;

                case 05: // MOV [Immediate], SRCREG
                    fprintf (debug, "[decode_process] MOV(5): memory_set (0x%.8X, 0x%.8X)\n",
                            immediate, SRCREG);
                    MEMSET(immediate,  SRCREG);
                    break;

                case 06: // MOV [DSTREG], SRCREG
                    fprintf (debug, "[decode_process] MOV(6): memory_set (0x%.8X, 0x%.8X)\n",
                            DSTREG,    SRCREG);
                    MEMSET(DSTREG,     SRCREG);
                    break;

                case 07: // MOV [DSTREG+Immediate], SRCREG
                    fprintf (debug, "[decode_process] MOV(7): memory_set (0x%.8X, 0x%.8X)\n",
                            (DSTREG + immediate), SRCREG);
                    MEMSET((DSTREG + immediate),  SRCREG);
                    break;
            }
            break;

        case PUSH:
            fprintf (debug, "[decode_process] PUSH: memory_set (0x%.8X, 0x%.8X)\n", REG(SP), DSTREG);
            REG(SP)         = REG(SP) - 1; // adjust top of stack
            MEMSET(REG(SP), DSTREG);      // place on top of stack
            break;

        case POP:
            fprintf (debug, "[decode_process] POP: memory_get (0x%.8X)\n", REG(SP));
            DSTREG          = IMEMGET(REG(SP));
            REG(SP)         = REG(SP) + 1;
            break;

        case IN:
            fprintf (debug, "[decode_process] IN: ioports_get (0x%.3X)\n", port);
            DSTREG          = IPORTGET(port);
            break;

        case OUT:
            value           = (immflag == TRUE)  ? immediate  : DSTREG;
            fprintf (debug, "[decode_process] OUT: ioports_set (0x%.3X, 0x%.8X)\n",
                            port, value);
            PORTSET(port, value);
            break;

        case MOVS:
            do
            {
                value       = IMEMGET(REG(SR));
                MEMSET(REG(DR), value);
                REG(DR)     = REG(DR) + 1;
                REG(SR)     = REG(SR) + 1;
                REG(CR)     = REG(CR) - 1;
            }
            while (REG(CR) >  0);
            break;

        case SETS:
            do
            {
                MEMSET(REG(DR), SR);
                REG(DR)     = REG(DR) + 1; // DR: Destination Register (R13)
                REG(CR)     = REG(CR) - 1; // CR: Count Register (R11)
            }
            while (REG(CR) >  0);
            break;

        case CMPS:
            do
            {
                DSTREG      = IMEMGET(REG(DR)) - IMEMGET(REG(SR));
                if (DSTREG != 0)
                {
                    break;
                }
                REG(DR)     = REG(DR) + 1; // DR: Destination Register (R13)
                REG(SR)     = REG(SR) + 1; // SR: Source Register (R12)
                REG(CR)     = REG(CR) - 1; // CR: Count Register (R11)
            }
            while (REG(CR) >  0);
            break;

        case CIF:
            FDSTREG         = (float) DSTREG;
            break;

        case CFI:
            DSTREG          = (int32_t) FDSTREG;
            break;

        case CIB:
            DSTREG          = (DSTREG  != FALSE) ? TRUE       : FALSE;
            break;

        case CFB:
            DSTREG          = (FDSTREG != 0.0)   ? TRUE       : FALSE;
            break;

        case NOT:
            DSTREG          = ~(DSTREG);
            break;

        case AND:
            DSTREG         &= (immflag == TRUE)  ? immediate  : SRCREG;
            break;

        case OR:
            DSTREG         |= (immflag == TRUE)  ? immediate  : SRCREG;
            break;

        case XOR:
            DSTREG         ^= (immflag == TRUE)  ? immediate  : SRCREG;
            break;

        case BNOT:
            DSTREG          = (DSTREG  == FALSE) ? TRUE       : FALSE;
            break;

        case SHL:
            value           = (immflag == TRUE)  ? immediate  : SRCREG;
            if (value      >= 0)
            {
                DSTREG      = DSTREG   << value;
            }
            else
            {
                DSTREG      = DSTREG   >> abs ((signed) value);
            }
            break;

        case IADD:
            DSTREG         += (immflag == TRUE)  ? immediate  : SRCREG;
            break;

        case ISUB:
            DSTREG         -= (immflag == TRUE)  ? immediate  : SRCREG;
            break;

        case IMUL:
            DSTREG         *= (immflag == TRUE)  ? immediate  : SRCREG;
            break;

        case IDIV:
            value           = (immflag == TRUE)  ? immediate  : SRCREG;
            if (value      != 0)
            {
                DSTREG      = DSTREG / value;
            }
            else
            {
                sys_error   = ERROR_DIVISION;
            }
            break;

        case IMOD:
            value           = (immflag == TRUE)  ? immediate  : SRCREG;
            if (value      != 0)
            {
                DSTREG      = DSTREG % value;
            }
            else
            {
                sys_error   = ERROR_DIVISION;
            }
            break;

        case ISGN:
            DSTREG          = -DSTREG;
            break;

        case IMIN:
            if (immflag    == TRUE)
            {
                if (DSTREG >  immediate)
                {
                    DSTREG  = immediate;
                }
            }
            else
            {
                if (DSTREG >  SRCREG)
                {
                    DSTREG  = SRCREG;
                }
            }
            break;

        case IMAX:
            if (immflag    == TRUE)
            {
                if (DSTREG <  immediate)
                {
                    DSTREG  = immediate;
                }
            }
            else
            {
                if (DSTREG <  SRCREG)
                {
                    DSTREG  = SRCREG;
                }
            }
            break;

        case IABS:
            DSTREG          = abs ((signed) DSTREG);
            break;

        case FADD:
            FDSTREG        += (immflag == TRUE)  ? fimmediate : FSRCREG;
            break;

        case FSUB:
            FDSTREG        -= (immflag == TRUE)  ? fimmediate : FSRCREG;
            break;

        case FMUL:
            FDSTREG        *= (immflag == TRUE)  ? fimmediate : FSRCREG;
            break;

        case FDIV:
            fvalue          = (immflag == TRUE)  ? fimmediate : FSRCREG;
            if (fvalue     != 0)
            {
                FDSTREG     = FDSTREG / fvalue;
            }
            else
            {
                sys_error   = ERROR_DIVISION;
            }
            break;

        case FMOD:
            fvalue          = (immflag == TRUE)  ? fimmediate : FSRCREG;
            if (fvalue     != 0)
            {
                FDSTREG     = fmodf (FDSTREG, fvalue);
            }
            else
            {
                sys_error   = ERROR_DIVISION;
            }
            break;

        case FSGN:
            FDSTREG         = -FDSTREG;
            break;

        case FMIN:
            fvalue          = (immflag == TRUE)  ? fimmediate : FSRCREG;
            FDSTREG         = fminf (FDSTREG, fvalue);
            break;

        case FMAX:
            fvalue          = (immflag == TRUE)  ? fimmediate : FSRCREG;
            FDSTREG         = fmaxf (FDSTREG, fvalue);
            break;

        case FABS:
            FDSTREG         = fabsf (FDSTREG);
            break;

        case FLR:
            FDSTREG         = floorf (FDSTREG);
            break;

        case CEIL:
            FDSTREG         = ceilf (FDSTREG);
            break;

        case ROUND:
            FDSTREG         = roundf (FDSTREG);
            break;

        case SIN:
            FDSTREG         = sinf (FDSTREG);
            break;

        case ACOS:
            FDSTREG         = acosf (FDSTREG);
            break;

        case ATAN2:
            FDSTREG         = atan2f (FDSTREG, FSRCREG);
            break;

        case LOG:
            FDSTREG         = logf (FDSTREG);
            break;

        case POW:
            FDSTREG         = powf (FDSTREG, FSRCREG);
            break;

        default:
            fprintf (stderr, "[decode_process] unimplemented instruction (0x%hhX)\n", opcode);
            sys_error       = ERROR_UNKNOWN;
            break;
    }
}

uint8_t  decode_check (uint32_t  instruction,
                       uint32_t  immediate,
                       float     fimmediate,
                       uint8_t   flags)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    float     fvalue        = 0.0;
    int32_t   value         = 0;
    uint8_t   immflag       = (flags & FLAG_IMMEDIATE) ? TRUE : FALSE;
    uint8_t   opcode        = (instruction & OPCODE_MASK) >> OPCODESHIFT;
    uint32_t  dst           = (instruction & DSTREG_MASK) >> DSTREGSHIFT;
    uint32_t  src           = (instruction & SRCREG_MASK) >> SRCREGSHIFT;
    uint8_t   addr          = (instruction & MOVADR_MASK) >> MOVADRSHIFT;
    uint16_t  port          = (instruction & IOPORT_MASK);
    uint32_t  data          = 0;
    uint8_t   result        = TRUE;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Determine opcode and perform needed processing, based on flags
    //
    switch (opcode)
    {
        case HLT:
        case WAIT:
        case IEQ:
        case INE:
        case IGT:
        case IGE:
        case ILT:
        case ILE:
        case FEQ:
        case FNE:
        case FGT:
        case FGE:
        case FLT:
        case FLE:
        case LEA:
        case CIF:
        case CFI:
        case CIB:
        case CFB:
        case NOT:
        case AND:
        case OR:
        case XOR:
        case BNOT:
        case SHL:
        case IADD:
        case ISUB:
        case IMUL:
        case ISGN:
        case IMIN:
        case IMAX:
        case IABS:
        case FADD:
        case FSUB:
        case FMUL:
        case FSGN:
        case FMIN:
        case FMAX:
        case FABS:
        case FLR:
        case CEIL:
        case ROUND:
        case SIN:
            break;

        case JMP:
            data            = (immflag == TRUE)   ? immediate  : DSTREG;
            result          = memory_chk  (data,               FLAG_READ,  TRUE);
            break;

        case CALL:
            data            = (immflag == TRUE)   ? immediate  : DSTREG;
            result          = memory_chk  (REG(SP)-1,          FLAG_WRITE, TRUE);
            if (result     == TRUE)
            {
                result      = memory_chk  (data,               FLAG_READ,  TRUE);
            }
            break;

        case RET:
            result          = memory_chk  (REG(SP),            FLAG_READ,  TRUE);
            break;

        case JT:
            if (DSTREG     == TRUE)
            {
                data        = (immflag == TRUE)   ? immediate  : SRCREG;
                result      = memory_chk  (data,               FLAG_READ,  TRUE);
            }
            break;

        case JF:
            if (DSTREG     == FALSE)
            {
                data        = (immflag == TRUE)   ? immediate  : SRCREG;
                result      = memory_chk  (data,               FLAG_READ,  TRUE);
            }
            break;

        case MOV:
            switch (addr)
            {
                case 00: // MOV DSTREG, Immediate
                case 01: // MOV DSTREG, SRCREG
                    break;

                case 02: // MOV DSTREG, [Immediate]
                    result  = memory_chk  (immediate,          FLAG_READ,  TRUE);
                    break;

                case 03: // MOV DSTREG, [SRCREG]
                    result  = memory_chk  (SRCREG,             FLAG_READ,  TRUE);
                    break;

                case 04: // MOV DSTREG, [SRCREG+Immediate]
                    result  = memory_chk  ((SRCREG+immediate), FLAG_READ,  TRUE);
                    break;

                case 05: // MOV [Immediate], SRCREG
                    result  = memory_chk  (immediate,          FLAG_WRITE, TRUE);
                    break;

                case 06: // MOV [DSTREG], SRCREG
                    result  = memory_chk  (DSTREG,             FLAG_WRITE, TRUE);
                    break;

                case 07: // MOV [DSTREG+Immediate], SRCREG
                    result  = memory_chk  ((DSTREG+immediate), FLAG_WRITE, TRUE);
                    break;
            }
            break;

        case PUSH:
            result          = memory_chk  ((REG(SP)-1),        FLAG_WRITE, TRUE);
            break;

        case POP:
            result          = memory_chk  (REG(SP),            FLAG_READ,  TRUE);
            break;

        case IN:
            result          = ioports_chk (port,               FLAG_READ,  TRUE);
            break;

        case OUT:
            result          = ioports_chk (port,               FLAG_WRITE, TRUE);
            break;

        case MOVS:
            for (value      = 0;
                 value     <  REG(CR);
                 value      = value + 1)
            {
                result      = memory_chk  ((REG(SR)+value),   FLAG_READ,  TRUE);
                if (result == TRUE)
                {
                    result  = memory_chk  ((REG(DR)+value),   FLAG_WRITE, TRUE);
                }
            }
            break;

        case SETS:
            for (value      = 0;
                 value     <  REG(CR);
                 value      = value + 1)
            {
                result      = memory_chk  ((REG(DR)+value),   FLAG_WRITE, TRUE);
            }
            break;

        case CMPS:
            for (value      = 0;
                 value     <  REG(CR);
                 value      = value + 1)
            {
                result      = memory_chk  ((REG(DR)+value),   FLAG_READ,  TRUE);
                if (result == TRUE)
                {
                    result  = memory_chk  ((REG(SR)+value),   FLAG_READ,  TRUE);
                }

                /* btw: I think this is implemented wrong
                DSTREG      = IMEMGET(REG(DR) - REG(SR));
                if (DSTREG != 0)
                {
                    break;
                }
                */
            }
            break;

        case IDIV:
        case IMOD:
            value           = (immflag == TRUE)  ? immediate  : SRCREG;
            if (value      == 0)
            {
                result      = FALSE;
                //sys_error   = ERROR_DIVISION;
            }
            break;

        case FDIV:
        case FMOD:
            fvalue          = (immflag == TRUE)  ? fimmediate  : FSRCREG;
            if (fvalue     == 0.0)
            {
                result      = FALSE;
                //sys_error   = ERROR_DIVISION;
            }
            break;

        case ACOS:
            // does SIN also fall under this error??
            if ((FDSTREG   <  -1.0) ||
                (FDSTREG   >  +1.0))
            {
                result      = FALSE;
                //sys_error  == ERROR_ARC_COSINE;
            }
            break;

        case ATAN2:
            // need to find how to instigate ATAN2 error
            //FDSTREG         = atan2f (FDSTREG, FSRCREG);
            break;

        case LOG:
            if (FDSTREG    == 0)
            {
                result      = FALSE;
                //sys_error  == ERROR_DIVISION;
            }
            
            if ((result    == TRUE) &&
                (FDSTREG   <  0.0))
            {
                result      = FALSE;
                //sys_error  == ERROR_LOGARITHM;
            }
            break;

        case POW:
            if ((FDSTREG   == 0.0) &&
                (FSRCREG   <  0.0))
            {
                result      = FALSE;
                //sys_error  == ERROR_DIVISION;
            }
            // there may be another error state
            break;

        default:
            fprintf (stderr, "[decode_process] unimplemented instruction (0x%hhX)\n", opcode);
            sys_error       = ERROR_UNKNOWN;
            break;
    }

    return (result);
}

