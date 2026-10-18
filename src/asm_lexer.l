%{
/*
 * asm_lexer.l: flex scanner for the live assembler (one instruction per line).
 * Generated with the prefix 'asm' (flex -P asm) so that it can live beside the
 * command scanner in lexer.l.
 *
 * Start conditions: INITIAL scans the mnemonic, OPS scans the operands.
 * Case does not matter. Comments run from ';' or '#' to the end of the line.
 */
#include "defines.h"
#include "assembly.h"
#include "asm_parser.h"

static int   alex_pos        = 0;
static int   alex_col        = 0;
static char  alex_text[64];

#define YY_USER_ACTION  alex_col = alex_pos + 1; alex_pos += yyleng;              \
                        snprintf (alex_text, sizeof (alex_text), "%s", yytext);

static int  alex_mnemonic (void);
static int  alex_int      (int base, const char *digits, int ndigits);
static int  alex_float    (void);
%}

%option noyywrap nounput noinput 8bit case-insensitive nodefault

%x OPS

IDENT    [A-Za-z_.$@][A-Za-z0-9_.$@]*

%%

<INITIAL>{
[ \t\r\n]+                  { }
[;#].*                      { }
[A-Za-z][A-Za-z0-9]*        { return alex_mnemonic (); }
.                           { asm_seterr ("an instruction must start with a mnemonic"); return A_BAD; }
}

<OPS>{
[ \t\r\n]+                  { }
[;#].*                      { }

0b[01]{1,32}                { return alex_int (2,  yytext + 2, yyleng - 2); }
0x[0-9a-f]{1,8}             { return alex_int (16, yytext + 2, yyleng - 2); }
0[0-7]+                     { return alex_int (8,  yytext + 1, yyleng - 1); }
[0-9]+                      { return alex_int (10, yytext,     yyleng);     }
[0-9]+"."[0-9]*([eE][+-]?[0-9]+)?f?   { return alex_float (); }
"."[0-9]+([eE][+-]?[0-9]+)?f?         { return alex_float (); }

R(1[0-5]|[0-9])             { asmlval.num = (uint32_t) atoi (yytext + 1); return A_REG; }
CR                          { asmlval.num = 11; return A_REG; }
SR                          { asmlval.num = 12; return A_REG; }
DR                          { asmlval.num = 13; return A_REG; }
BP                          { asmlval.num = 14; return A_REG; }
SP                          { asmlval.num = 15; return A_REG; }

{IDENT}                     { snprintf (asmlval.id, sizeof (asmlval.id), "%s", yytext); return A_IDENT; }

","|"["|"]"|"+"|"-"         { return yytext[0]; }

.                           { asm_seterr ("unexpected character '%s'", yytext); return A_BAD; }
}

<*><<EOF>>                  { alex_text[0] = '\0'; yyterminate (); }

%%

static int  alex_mnemonic (void)
{
    uint8_t  opcode  = 0;
    int      cls     = asm_mnemonic (yytext, &opcode);

    BEGIN (OPS);
    if (cls < 0)
    {
        asm_seterr ("unknown instruction '%s'", yytext);
        return (A_BAD);
    }
    asmlval.num  = opcode;
    switch (cls)
    {
        case ACLS_NONE:  return (A_NONE);
        case ACLS_JMP:   return (A_JMP);
        case ACLS_REG:   return (A_REG1);
        case ACLS_RI:    return (A_RI);
        case ACLS_RF:    return (A_RF);
        case ACLS_LEA:   return (A_LEA);
        case ACLS_MOV:   return (A_MOV);
        case ACLS_IN:    return (A_IN);
        default:         return (A_OUT);
    }
}

static int  alex_int (int base, const char *digits, int ndigits)
{
    char                buf[48];
    unsigned long long  value  = 0;

    if (ndigits > (int) sizeof (buf) - 1)
    {
        ndigits  = (int) sizeof (buf) - 1;
    }
    snprintf (buf, sizeof (buf), "%.*s", ndigits, digits);
    value  = strtoull (buf, NULL, base);
    if ((value > 0xFFFFFFFFULL) || (strlen (buf) > 32))
    {
        asm_seterr ("number '%s' does not fit in 32 bits", yytext);
        return (A_BAD);
    }
    asmlval.val.bits    = (uint32_t) value;
    asmlval.val.kind    = (base == 10) ? AV_DEC : AV_RAW;
    asmlval.val.fvalue  = 0.0f;
    return (A_NUM);
}

static int  alex_float (void)
{
    float  f  = strtof (yytext, NULL);

    asmlval.val.fvalue  = f;
    memcpy (&asmlval.val.bits, &f, sizeof (f));
    asmlval.val.kind    = AV_FLT;
    return (A_NUM);
}

static YY_BUFFER_STATE  alex_buffer  = NULL;

void  asm_lex_begin (const char *line)
{
    alex_pos       = 0;
    alex_col       = 0;
    alex_text[0]   = '\0';
    BEGIN (INITIAL);
    alex_buffer    = asm_scan_string (line);
}

void  asm_lex_end (void)
{
    if (alex_buffer != NULL)
    {
        asm_delete_buffer (alex_buffer);
        alex_buffer  = NULL;
    }
}

const char *asm_lex_lasttext (void)
{
    return (alex_text);
}

int  asm_lex_lastcol (void)
{
    return (alex_col);
}
