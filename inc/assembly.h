#ifndef  _ASSEMBLY_H
#define  _ASSEMBLY_H

////////////////////////////////////////////////////////////////////////////////////////
//
// assembly.h: the live assembler (one Vircon32 instruction, text -> machine words)
//
//   asm_lexer.l    flex scanner   (generated with prefix 'asm')
//   asm_parser.y   bison grammar  (generated with prefix 'asm')
//   assembly.c     mnemonic table, symbol resolution, instruction encoding
//
// The syntax is that of the Vircon32 assembler, one instruction per call:
//
//     mov R1, 0x10            mov R2, [R3+4]          mov [0x2000], R5
//     iadd R0, -1             fadd R4, 1.5            jmp main (a label)
//     out GPU_Command, GPUCommand_DrawRegion          in R0, TIM_FrameCounter
//
////////////////////////////////////////////////////////////////////////////////////////

// operand classes of the mnemonics (token classes of the grammar)
#define  ACLS_NONE               0   // HLT WAIT RET MOVS SETS
#define  ACLS_JMP                1   // JMP CALL           reg | imm
#define  ACLS_REG                2   // PUSH POP CMPS CIF ... LOG       reg
#define  ACLS_RI                 3   // JT JF IEQ ... IMAX             reg, reg|int-imm
#define  ACLS_RF                 4   // FEQ ... POW                    reg, reg|float-imm
#define  ACLS_LEA                5
#define  ACLS_MOV                6
#define  ACLS_IN                 7
#define  ACLS_OUT                8

// a numeric literal as written: decimal integers convert to float where a float is
// expected, other forms (hex, binary, octal) are raw 32-bit patterns
#define  AV_DEC                  0
#define  AV_RAW                  1
#define  AV_FLT                  2

typedef struct asm_val
{
    uint32_t  bits;
    float     fvalue;
    uint8_t   kind;                  // AV_*
} asm_val_t;

typedef struct asm_inst
{
    uint8_t   opcode;
    uint8_t   dst;
    uint8_t   src;
    uint8_t   mode;                  // MOV addressing mode
    uint16_t  port;                  // IN/OUT port number
    uint8_t   has_imm;
    uint32_t  imm;
    char      err[160];              // set (non-empty) when assembly failed
} asm_inst_t;

// assembly.c
int         asm_mnemonic   (const char *, uint8_t *opcode);   // -> ACLS_*, or -1
int         asm_assemble   (const char *, uint32_t [2], int *, char *, size_t);  // 0 == ok
int         asm_symbol     (const char *, uint32_t *);        // label / port / constant -> value
int         asm_port       (const char *, uint16_t *);        // IOPort symbol -> number
asm_inst_t *asm_current    (void);
void        asm_seterr     (const char *, ...);

// generated: asm_parser.y / asm_lexer.l (prefix 'asm')
int         asmparse       (void);
int         asmlex         (void);
void        asmerror       (const char *);
void        asm_lex_begin  (const char *);
void        asm_lex_end    (void);
const char *asm_lex_lasttext (void);
int         asm_lex_lastcol  (void);

#endif
