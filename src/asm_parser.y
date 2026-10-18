%{
/*
 * asm_parser.y: bison grammar for the live assembler. Generated with the prefix 'asm'
 * (bison -p asm). Fills in the asm_inst_t of the instruction being assembled; encoding
 * happens in assembly.c.
 *
 *   NONE                          HLT WAIT RET MOVS SETS
 *   JMP   reg | imm               JMP CALL
 *   REG1  reg                     PUSH POP CMPS CIF CFI CIB CFB NOT BNOT ISGN IABS FSGN
 *                                 FABS FLR CEIL ROUND SIN ACOS LOG
 *   RI    reg , reg | int-imm     JT JF IEQ INE IGT IGE ILT ILE AND OR XOR SHL IADD ISUB
 *                                 IMUL IDIV IMOD IMIN IMAX
 *   RF    reg , reg | float-imm   FEQ FNE FGT FGE FLT FLE FADD FSUB FMUL FDIV FMOD FMIN
 *                                 FMAX ATAN2 POW
 *   LEA   reg , [reg] | [reg+imm]
 *   MOV   the eight addressing modes (see the Vircon32 reference)
 *   IN    reg , port              OUT  port , reg | imm
 *
 * Immediates may be numbers (decimal, 0x hex, 0b binary, 0 octal, floats) or symbols
 * (labels, GPU/SPU command constants).
 */
#include "defines.h"
#include "assembly.h"

#define  I   (asm_current ())

static int   a_int    (asm_val_t *v, uint32_t *out);
static int   a_neg    (asm_val_t *v, uint32_t *out, int wantfloat);
static int   a_symbol (const char *name, uint32_t *out);
static int   a_port   (asm_val_t *v, const char *name);
static void  a_imm    (uint32_t bits);
%}

%union
{
    uint32_t   num;
    char       id[64];
    asm_val_t  val;
}

%token <num>  A_NONE A_JMP A_REG1 A_RI A_RF A_LEA A_MOV A_IN A_OUT A_REG
%token <val>  A_NUM
%token <id>   A_IDENT
%token        A_BAD

%type  <num>  imm_int imm_any imm_flt offset

%%

stmt
    : /* empty line */
    | inst
    ;

inst
    : A_NONE                                { I -> opcode = $1; }
    | A_JMP A_REG                           { I -> opcode = $1; I -> dst = $2; }
    | A_JMP imm_int                         { I -> opcode = $1; a_imm ($2); }
    | A_REG1 A_REG                          { I -> opcode = $1; I -> dst = $2; }
    | A_RI A_REG ',' A_REG                  { I -> opcode = $1; I -> dst = $2; I -> src = $4; }
    | A_RI A_REG ',' imm_int                { I -> opcode = $1; I -> dst = $2; a_imm ($4); }
    | A_RF A_REG ',' A_REG                  { I -> opcode = $1; I -> dst = $2; I -> src = $4; }
    | A_RF A_REG ',' imm_flt                { I -> opcode = $1; I -> dst = $2; a_imm ($4); }
    | A_LEA A_REG ',' '[' A_REG ']'         { I -> opcode = $1; I -> dst = $2; I -> src = $5; }
    | A_LEA A_REG ',' '[' A_REG offset ']'  { I -> opcode = $1; I -> dst = $2; I -> src = $5; a_imm ($6); }
    | A_MOV A_REG ',' imm_any               { I -> opcode = $1; I -> mode = 0; I -> dst = $2; a_imm ($4); }
    | A_MOV A_REG ',' A_REG                 { I -> opcode = $1; I -> mode = 1; I -> dst = $2; I -> src = $4; }
    | A_MOV A_REG ',' '[' imm_int ']'       { I -> opcode = $1; I -> mode = 2; I -> dst = $2; a_imm ($5); }
    | A_MOV A_REG ',' '[' A_REG ']'         { I -> opcode = $1; I -> mode = 3; I -> dst = $2; I -> src = $5; }
    | A_MOV A_REG ',' '[' A_REG offset ']'  { I -> opcode = $1; I -> mode = 4; I -> dst = $2; I -> src = $5; a_imm ($6); }
    | A_MOV '[' imm_int ']' ',' A_REG       { I -> opcode = $1; I -> mode = 5; I -> src = $6; a_imm ($3); }
    | A_MOV '[' A_REG ']' ',' A_REG         { I -> opcode = $1; I -> mode = 6; I -> dst = $3; I -> src = $6; }
    | A_MOV '[' A_REG offset ']' ',' A_REG  { I -> opcode = $1; I -> mode = 7; I -> dst = $3; I -> src = $7; a_imm ($4); }
    | A_IN A_REG ',' port                   { I -> opcode = $1; I -> dst = $2; }
    | A_OUT port ',' A_REG                  { I -> opcode = $1; I -> src = $4; }
    | A_OUT port ',' imm_any                { I -> opcode = $1; a_imm ($4); }
    | A_BAD                                 { YYERROR; }
    ;

port
    : A_NUM                                 { if (a_port (&$1, NULL)) YYERROR; }
    | A_IDENT                               { if (a_port (NULL, $1))  YYERROR; }
    ;

/* [reg+imm] / [reg-imm]: the offset, as a two's complement word */
offset
    : '+' imm_int                           { $$ = $2; }
    | '-' A_NUM                             { if (a_neg (&$2, &$$, 0)) YYERROR; }
    ;

/* integer immediate: number or symbol, optionally signed; floats are rejected */
imm_int
    : A_NUM                                 { if (a_int (&$1, &$$)) YYERROR; }
    | '+' A_NUM                             { if (a_int (&$2, &$$)) YYERROR; }
    | '-' A_NUM                             { if (a_neg (&$2, &$$, 0)) YYERROR; }
    | A_IDENT                               { if (a_symbol ($1, &$$)) YYERROR; }
    ;

/* any immediate (MOV, OUT): a float literal is its bit pattern */
imm_any
    : A_NUM                                 { $$ = $1.bits; }
    | '+' A_NUM                             { $$ = $2.bits; }
    | '-' A_NUM                             { if (a_neg (&$2, &$$, 1)) YYERROR; }
    | A_IDENT                               { if (a_symbol ($1, &$$)) YYERROR; }
    ;

/* float immediate: a decimal integer becomes its float value; hex/binary/octal are raw bits */
imm_flt
    : A_NUM                                 { if ($1.kind == AV_DEC) { float f = (float) $1.bits; memcpy (&$$, &f, 4); }
                                              else                   { $$ = $1.bits; } }
    | '+' A_NUM                             { if ($2.kind == AV_DEC) { float f = (float) $2.bits; memcpy (&$$, &f, 4); }
                                              else                   { $$ = $2.bits; } }
    | '-' A_NUM                             { if ($2.kind == AV_DEC) { float f = -(float) $2.bits; memcpy (&$$, &f, 4); }
                                              else if ($2.kind == AV_FLT) { float f = -$2.fvalue; memcpy (&$$, &f, 4); }
                                              else { asm_seterr ("cannot negate the raw value '%s'", asm_lex_lasttext ()); YYERROR; } }
    | A_IDENT                               { if (a_symbol ($1, &$$)) YYERROR; }
    ;

%%

static int  a_int (asm_val_t *v, uint32_t *out)
{
    if (v -> kind == AV_FLT)
    {
        asm_seterr ("an integer is required here, not a floating point value");
        return (1);
    }
    *out  = v -> bits;
    return (0);
}

/* -NUM: integers negate as 32-bit words; floats flip the sign (only where allowed) */
static int  a_neg (asm_val_t *v, uint32_t *out, int wantfloat)
{
    if (v -> kind == AV_FLT)
    {
        float  f  = -v -> fvalue;

        if (wantfloat == 0)
        {
            asm_seterr ("an integer is required here, not a floating point value");
            return (1);
        }
        memcpy (out, &f, sizeof (f));
        return (0);
    }
    *out  = (uint32_t) (0u - v -> bits);
    return (0);
}

static int  a_symbol (const char *name, uint32_t *out)
{
    if (asm_symbol (name, out) != 0)
    {
        asm_seterr ("unknown symbol '%s' (not a label, IOPort, or constant)", name);
        return (1);
    }
    return (0);
}

static int  a_port (asm_val_t *v, const char *name)
{
    uint16_t  port  = 0;

    if (name != NULL)
    {
        if (asm_port (name, &port) != 0)
        {
            asm_seterr ("unknown IOPort '%s'", name);
            return (1);
        }
    }
    else
    {
        if ((v -> kind == AV_FLT) || (v -> bits > 0x3FFF))
        {
            asm_seterr ("an IOPort number must be an integer up to 0x3FFF");
            return (1);
        }
        port  = (uint16_t) v -> bits;
    }
    I -> port  = port;
    return (0);
}

static void  a_imm (uint32_t bits)
{
    I -> has_imm  = TRUE;
    I -> imm      = bits;
}

void  asmerror (const char *msg)
{
    const char  *text  = asm_lex_lasttext ();

    if (text[0] == '\0')
    {
        asm_seterr ("incomplete instruction");
    }
    else
    {
        asm_seterr ("syntax error near '%s' (column %d)", text, asm_lex_lastcol ());
    }
    (void) msg;
}
