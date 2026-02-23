#include "defines.h"

void  decode (uint32_t  instruction,
              uint32_t  immediate,
              float     fimmediate,
              uint8_t   flags)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    uint8_t   displayflag          = (flags & FLAG_DISPLAY)   ? TRUE : FALSE;
    uint8_t   processflag          = (flags & FLAG_PROCESS)   ? TRUE : FALSE;

    ////////////////////////////////////////////////////////////////////////////////
    //
    // If we are only concerned with displaying the assembly, call the
    // decode_display() function
    //
    if (displayflag               == TRUE)
    {
        decode_display (instruction, immediate, fimmediate, flags);
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // If we are only concerned with processing the assembly, call the
    // decode_process() function
    //
    else if (processflag          == TRUE)
    {
        decode_process (instruction, immediate, fimmediate, flags);
    }
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
    int8_t    sign                 = '+';
    uint8_t   displayflag          = (flags & FLAG_DISPLAY)   ? TRUE : FALSE;
    uint8_t   immflag              = (flags & FLAG_IMMEDIATE) ? TRUE : FALSE;
    uint8_t   opcode               = (instruction & OPCODE_MASK) >> OPCODESHIFT;
    uint32_t  dst                  = (instruction & DSTREG_MASK) >> DSTREGSHIFT;
    uint32_t  src                  = (instruction & SRCREG_MASK) >> SRCREGSHIFT;
    uint8_t   addr                 = (instruction & MOVADR_MASK) >> MOVADRSHIFT;
    uint16_t  port                 = (instruction & IOPORT_MASK);

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
    // Determine opcode and perform needed processing, based on flags
    //
    switch (opcode)
    {
        case HLT: // special case for HLT to try and distinguish from data
            if (instruction         == 0x00000000)
            {
                fprintf (display,     "%-5s ",
                                      lookup[opcode].name);
            }
            break;

        case WAIT:
        case RET:
            fprintf (display,         "%-5s\n",
                                      lookup[opcode].name);
            break;

        case JMP:
        case CALL:
            if (immflag             == TRUE)
            {
                sprintf (destination, "0x%.8X", immediate);
            }
            else
            {
                sprintf (destination, "R%u",    dst);
            }
            fprintf (display,         "%-5s %-16s\n",
                                      lookup[opcode].name, destination);
            break;

        case JT:
        case JF:
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
            sprintf (destination, "R%u,", dst);
            if (immflag               == TRUE)
            {
                sprintf (source, "0x%.8X", immediate);
            }
            else
            {
                sprintf (source, "R%u",    src);
            }
            fprintf (display,    "%-5s %-16s %-16s\n",
                                 lookup[opcode].name, destination, source);
            break;

        case FEQ:
        case FNE:
        case FGT:
        case FGE:
        case FLT:
        case FLE:
            sprintf (destination, "R%u,", dst);
            if (immflag               == TRUE)
            {
                sprintf (source, "%.2f", fimmediate);
            }
            else
            {
                sprintf (source, "R%u",    src);
            }
            fprintf (display,      "%-5s %-16s %-16s\n",
                                   lookup[opcode].name, destination, source);
            break;

        case MOV:
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
                    break;

                case 03: // MOV DSTREG, [SRCREG]
                    sprintf (destination, "R%u,",          dst);
                    sprintf (source,      "[R%u]",         src);
                    break;

                case 04: // MOV DSTREG, [SRCREG+Immediate]
                    sprintf (destination, "R%u,",          dst);
                   // sprintf (source,      "[R%u+0x%.8X]",  src, immediate);
                    sign  = (immediate >= 0) ? '+' : '-';
                    sprintf (source,      "[R%u%c%d]",  src, sign, abs (immediate));
                    break;

                case 05: // MOV [Immediate], SRCREG
                    sprintf (destination, "[0x%.8X],",     immediate);
                    sprintf (source,      "R%u",           src);
                    break;

                case 06: // MOV [DSTREG], SRCREG
                    sprintf (destination, "[R%u],",        dst);
                    sprintf (source,      "R%u",           src);
                    break;

                case 07: // MOV [DSTREG+Immediate], SRCREG
                    //sprintf (destination, "[R%u+0x%.8X],", dst, immediate);
                    sprintf (destination, "[R%u%c%d],", dst, sign, abs (immediate));
                    sprintf (source,      "R%u",           src);
                    break;
            }
            fprintf (display,     "%-5s %-16s %-16s\n",
                                  lookup[opcode].name, destination, source);
            break;

        case PUSH:
        case POP:
        case CIF:
        case CFI:
        case CIB:
        case CFB:
        case NOT:
        case BNOT:
            sprintf (destination, "R%u",    dst);
            fprintf (display,     "%-5s %-16s\n",
                                  lookup[opcode].name, destination);
            break;

        case IN:
            sprintf (destination, "R%u,",    dst);
            sprintf (source,      "0x%.3X",  port);
            fprintf (display,     "%-5s %-16s %-16s\n",
                                  lookup[opcode].name, destination, source);
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
            fprintf (display,     "%-5s %-16s %-16s\n",
                                  lookup[opcode].name, destination, source);
            break;

        case FADD:
        case FSUB:
        case FMUL:
        case FDIV:
        case FMOD:
            sprintf (destination, "R%u,",    dst);
            if (immflag           == TRUE)
            {
                sprintf (source,  "%.3f",  fimmediate);
            }
            else
            {
                sprintf (source,  "R%u",     src);
            }
            fprintf (display,     "%-5s %-16s %-16s\n",
                                  lookup[opcode].name, destination, source);
            break;
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
            waitflag        = TRUE; // triggers TIM_CycleCounter to reset
            update_frame ();           // update Frame Counter
            break;

        case JMP:
            IP_REG          = (immflag == TRUE)   ? immediate  : DSTREG;
            rom_offset      = IP_REG;
            branchflag      = TRUE;
            break;

        case CALL:
            SP_REG          = SP_REG - 1;   // PUSH IP_REG value to stack
            memory_set (SP_REG, (IP_REG + 1));
            IP_REG          = (immflag == TRUE)   ? immediate  : DSTREG;
            rom_offset      = IP_REG;
            branchflag      = TRUE;
            break;

        case RET:
            IP_REG          = word2int (memory_get (SP_REG));
            rom_offset      = IP_REG;  // POP IP_REG off stack
            SP_REG          = SP_REG + 1;
            break;

        case JT:
            if (DSTREG     == TRUE)
            {
                IP_REG      = (immflag == TRUE)   ? immediate  : SRCREG;
                rom_offset  = IP_REG;
                branchflag  = TRUE;
            }
            break;

        case JF:
            if (DSTREG     == FALSE)
            {
                IP_REG      = (immflag == TRUE)   ? immediate  : SRCREG;
                rom_offset  = IP_REG;
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
                    DSTREG  = word2int (memory_get (immediate));
                    break;

                case 03: // MOV DSTREG, [SRCREG]
                    DSTREG  = word2int (memory_get (SRCREG));
                    break;

                case 04: // MOV DSTREG, [SRCREG+Immediate]
                    DSTREG  = word2int (memory_get (SRCREG + immediate));
                    break;

                case 05: // MOV [Immediate], SRCREG
                    memory_set (immediate,            SRCREG);
                    break;

                case 06: // MOV [DSTREG], SRCREG
                    memory_set (DSTREG,               SRCREG);
                    break;

                case 07: // MOV [DSTREG+Immediate], SRCREG
                    memory_set ((DSTREG + immediate), SRCREG);
                    break;
            }
            break;

        case PUSH:
            SP_REG          = SP_REG - 1; // adjust top of stack
            memory_set (SP_REG, DSTREG);  // place on top of stack
            break;

        case POP:
            DSTREG          = word2int (memory_get (SP_REG));
            SP_REG          = SP_REG + 1;
            break;

        case IN:
            DSTREG          = ioports_get (port);
            break;

        case OUT:
            value           = (immflag == TRUE)  ? immediate  : DSTREG;
            ioports_set (port, value);
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
                DSTREG      = DSTREG   >> value;
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
            DSTREG         %= (immflag == TRUE)  ? immediate  : SRCREG;
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
            FDSTREG        /= (immflag == TRUE)  ? fimmediate : FSRCREG;
            break;

        case FMOD:
            //FDSTREG        %= (immflag == TRUE)  ? fimmediate : FSRCREG;
            break;

        default:
            sys_error       = ERROR_UNKNOWN;
            break;
    }
}
