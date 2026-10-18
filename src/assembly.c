#include "defines.h"
#include <stdarg.h>
#include "assembly.h"

////////////////////////////////////////////////////////////////////////////////////////
//
// assembly.c: the live assembler. The scanner (asm_lexer.l) and grammar (asm_parser.y)
// recognise one instruction and fill in an asm_inst_t; this file owns the mnemonic
// table, symbol resolution, and the final encoding into 1 or 2 machine words.
//
// Instruction word layout (see the Vircon32 reference):
//     bits 31-26 opcode | 25 immediate flag | 24-21 DSTREG | 20-17 SRCREG
//     bits 16-14 MOV addressing mode | 13-0 IOPort number
// An instruction with an immediate is followed by one 32-bit immediate word.
//

static asm_inst_t  cur;

struct mnemonic
{
    const char  *name;
    uint8_t      opcode;
    uint8_t      cls;
};

// opcodes are the positions of the names in the global 'lookup' table (main.c); the
// operand class of each is listed here
static const struct mnemonic  mtab[]  =
{
    { "HLT",   0x00, ACLS_NONE }, { "WAIT",  0x01, ACLS_NONE }, { "JMP",   0x02, ACLS_JMP  },
    { "CALL",  0x03, ACLS_JMP  }, { "RET",   0x04, ACLS_NONE }, { "JT",    0x05, ACLS_RI   },
    { "JF",    0x06, ACLS_RI   }, { "IEQ",   0x07, ACLS_RI   }, { "INE",   0x08, ACLS_RI   },
    { "IGT",   0x09, ACLS_RI   }, { "IGE",   0x0A, ACLS_RI   }, { "ILT",   0x0B, ACLS_RI   },
    { "ILE",   0x0C, ACLS_RI   }, { "FEQ",   0x0D, ACLS_RF   }, { "FNE",   0x0E, ACLS_RF   },
    { "FGT",   0x0F, ACLS_RF   }, { "FGE",   0x10, ACLS_RF   }, { "FLT",   0x11, ACLS_RF   },
    { "FLE",   0x12, ACLS_RF   }, { "MOV",   0x13, ACLS_MOV  }, { "LEA",   0x14, ACLS_LEA  },
    { "PUSH",  0x15, ACLS_REG  }, { "POP",   0x16, ACLS_REG  }, { "IN",    0x17, ACLS_IN   },
    { "OUT",   0x18, ACLS_OUT  }, { "MOVS",  0x19, ACLS_NONE }, { "SETS",  0x1A, ACLS_NONE },
    { "CMPS",  0x1B, ACLS_REG  }, { "CIF",   0x1C, ACLS_REG  }, { "CFI",   0x1D, ACLS_REG  },
    { "CIB",   0x1E, ACLS_REG  }, { "CFB",   0x1F, ACLS_REG  }, { "NOT",   0x20, ACLS_REG  },
    { "AND",   0x21, ACLS_RI   }, { "OR",    0x22, ACLS_RI   }, { "XOR",   0x23, ACLS_RI   },
    { "BNOT",  0x24, ACLS_REG  }, { "SHL",   0x25, ACLS_RI   }, { "IADD",  0x26, ACLS_RI   },
    { "ISUB",  0x27, ACLS_RI   }, { "IMUL",  0x28, ACLS_RI   }, { "IDIV",  0x29, ACLS_RI   },
    { "IMOD",  0x2A, ACLS_RI   }, { "ISGN",  0x2B, ACLS_REG  }, { "IMIN",  0x2C, ACLS_RI   },
    { "IMAX",  0x2D, ACLS_RI   }, { "IABS",  0x2E, ACLS_REG  }, { "FADD",  0x2F, ACLS_RF   },
    { "FSUB",  0x30, ACLS_RF   }, { "FMUL",  0x31, ACLS_RF   }, { "FDIV",  0x32, ACLS_RF   },
    { "FMOD",  0x33, ACLS_RF   }, { "FSGN",  0x34, ACLS_REG  }, { "FMIN",  0x35, ACLS_RF   },
    { "FMAX",  0x36, ACLS_RF   }, { "FABS",  0x37, ACLS_REG  }, { "FLR",   0x38, ACLS_REG  },
    { "CEIL",  0x39, ACLS_REG  }, { "ROUND", 0x3A, ACLS_REG  }, { "SIN",   0x3B, ACLS_REG  },
    { "ACOS",  0x3C, ACLS_REG  }, { "ATAN2", 0x3D, ACLS_RF   }, { "LOG",   0x3E, ACLS_REG  },
    { "POW",   0x3F, ACLS_RF   }
};
#define  NMNEMONICS  ((int) (sizeof (mtab) / sizeof (mtab[0])))

// named constants of the Vircon32 assembler (not IOPorts)
static const struct { const char *name; uint32_t value; }  ctab[]  =
{
    { "GPUCommand_ClearScreen",           GPUCommand_ClearScreen          },
    { "GPUCommand_DrawRegion",            GPUCommand_DrawRegion           },
    { "GPUCommand_DrawRegionZoomed",      GPUCommand_DrawRegionZoomed     },
    { "GPUCommand_DrawRegionRotated",     GPUCommand_DrawRegionRotated    },
    { "GPUCommand_DrawRegionRotozoomed",  GPUCommand_DrawRegionRotozoomed },
    { "GPUBlendingMode_Alpha",            GPUBlendingMode_Alpha           },
    { "GPUBlendingMode_Add",              GPUBlendingMode_Add             },
    { "GPUBlendingMode_Subtract",         GPUBlendingMode_Subtract        },
    { "SPUCommand_PlaySelectedChannel",   SPUCommand_PlaySelectedChannel  },
    { "SPUCommand_PauseSelectedChannel",  SPUCommand_PauseSelectedChannel },
    { "SPUCommand_StopSelectedChannel",   SPUCommand_StopSelectedChannel  },
    { "SPUCommand_PauseAllChannels",      SPUCommand_PauseAllChannels     },
    { "SPUCommand_ResumeAllChannels",     SPUCommand_ResumeAllChannels    },
    { "SPUCommand_StopAllChannels",       SPUCommand_StopAllChannels      },
    { "SPUChannelState_Stopped",          SPUChannelState_Stopped         },
    { "SPUChannelState_Paused",           SPUChannelState_Paused          },
    { "SPUChannelState_Playing",          SPUChannelState_Playing         }
};
#define  NCONSTANTS  ((int) (sizeof (ctab) / sizeof (ctab[0])))

asm_inst_t *asm_current (void)
{
    return (&cur);
}

// first error wins: a failing action usually triggers a generic parser error after it
void  asm_seterr (const char *fmt, ...)
{
    va_list  ap;

    if (cur.err[0] != '\0')
    {
        return;
    }
    va_start (ap, fmt);
    vsnprintf (cur.err, sizeof (cur.err), fmt, ap);
    va_end (ap);
}

int  asm_mnemonic (const char *name, uint8_t *opcode)
{
    int  index  = 0;

    for (index = 0; index < NMNEMONICS; index = index + 1)
    {
        if (strcasecmp (name, mtab[index].name) == 0)
        {
            *opcode  = mtab[index].opcode;
            return (mtab[index].cls);
        }
    }
    return (-1);
}

int  asm_port (const char *name, uint16_t *port)
{
    int16_t  num  = ioports_num ((uint8_t *) name);

    if (num < 0)
    {
        return (1);
    }
    *port  = (uint16_t) num;
    return (0);
}

// label (from the loaded debug files) -> constant -> IOPort number
int  asm_symbol (const char *name, uint32_t *value)
{
    linked_l  *label  = find_label (lpoint, (int8_t *) name);
    uint16_t   port   = 0;
    int        index  = 0;

    if (label != NULL)
    {
        *value  = label -> RAW;
        return (0);
    }
    for (index = 0; index < NCONSTANTS; index = index + 1)
    {
        if (strcasecmp (name, ctab[index].name) == 0)
        {
            *value  = ctab[index].value;
            return (0);
        }
    }
    if (asm_port (name, &port) == 0)
    {
        *value  = port;
        return (0);
    }
    return (1);
}

//
// asm_assemble(): text of one instruction -> words[0] (+ words[1] when it has an
// immediate), *count words. Returns 0 on success; otherwise a message goes in 'err'.
//
int  asm_assemble (const char *text, uint32_t words[2], int *count, char *err, size_t errsize)
{
    uint32_t  word    = 0;
    int       result  = 0;

    memset (&cur, 0, sizeof (cur));
    cur.opcode  = 0xFF;
    asm_lex_begin (text);
    result      = asmparse ();
    asm_lex_end ();

    if ((result == 0) && (cur.err[0] == '\0') && (cur.opcode == 0xFF))
    {
        asm_seterr ("nothing to assemble");
    }
    if ((result == 0) && (cur.err[0] == '\0') &&
        ((cur.opcode == 0x3D) || (cur.opcode == 0x3F)) && (cur.has_imm))
    {
        asm_seterr ("%s takes a register as its second operand",
                    (cur.opcode == 0x3D) ? "ATAN2" : "POW");
    }
    if ((result != 0) || (cur.err[0] != '\0'))
    {
        snprintf (err, errsize, "%s", (cur.err[0] != '\0') ? cur.err : "cannot assemble");
        return (1);
    }

    word  = ((uint32_t) cur.opcode << OPCODESHIFT)
          | ((uint32_t) cur.has_imm << IMMED_SHIFT)
          | ((uint32_t) cur.dst  << DSTREGSHIFT)
          | ((uint32_t) cur.src  << SRCREGSHIFT)
          | ((uint32_t) cur.mode << MOVADRSHIFT)
          | ((uint32_t) cur.port & IOPORT_MASK);

    words[0]  = word;
    words[1]  = cur.imm;
    *count    = cur.has_imm ? 2 : 1;
    return (0);
}
