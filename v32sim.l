%{
/* Vircon32 Simulator - Debugger Command Lexer */
/* Flex lexer specification for v32sim debugger commands */

#include "defines.h"
#include "parser.tab.h"

#define YYDEBUG 1

void yyerror(const char *);
int yywrap(void);
%}

%option noyywrap
%option reentrant
%option bison-bridge
%option prefix="v32"

DIGIT    [0-9]
HEXDIGIT [0-9A-Fa-f]
LETTER   [A-Za-z]
ALNUM    [A-Za-z0-9]
WHITESPACE [ \t]

%%

[ \t]+     { /* ignore */ }
\n         { return NEWLINE; }

"step"        { return T_STEP; }
"s"          { return T_STEP; }
"continue"   { return T_CONTINUE; }
"c"          { return T_CONTINUE; }
"quit"       { return T_QUIT; }
"q"          { return T_QUIT; }
"help"       { return T_HELP; }
"?"          { return T_HELP; }
"break"      { return T_BREAK; }
"b"          { return T_BREAK; }
"display"    { return T_DISPLAY; }
"d"          { return T_DISPLAY; }
"set"        { return T_SET; }
"replace"    { return T_REPLACE; }
"r"          { return T_REPLACE; }
"watch"      { return T_WATCH; }
"w"          { return T_WATCH; }
"unwatch"    { return T_UNWATCH; }
"watchlist"  { return T_WATCHLIST; }
"profile"    { return T_PROFILE; }
"p"          { return T_PROFILE; }
"backtrace"  { return T_BACKTRACE; }
"bt"         { return T_BACKTRACE; }
"next"       { return T_NEXT; }
"n"          { return T_NEXT; }
"ignore"     { return T_IGNORE; }
"i"          { return T_IGNORE; }
"undo"       { return T_UNDO; }
"u"          { return T_UNDO; }
"label"      { return T_LABEL_CMD; }
"l"          { return T_LABEL_CMD; }
"gamepad"    { return T_GAMEPAD; }
"g"          { return T_GAMEPAD; }

R[0-9]       { yylval->reg = parse_reg_str(yytext); return T_REGISTER; }
R1[0-5]      { yylval->reg = parse_reg_str(yytext); return T_REGISTER; }
[BCDS][PR]   { yylval->reg = parse_reg_str(yytext); return T_REGISTER; }
I[PRV]       { yylval->reg = parse_reg_str(yytext); return T_REGISTER; }

"reg"       { return T_REGISTERS; }
"regs"      { return T_REGISTERS; }
"register"  { return T_REGISTERS; }
"registers" { return T_REGISTERS; }
"r"         { return T_REGISTERS; }
"r*"        { return T_REGISTERS; }

0x{HEXDIGIT}{8} {
    yylval->hexval = strtoul(yytext, NULL, 16);
    return T_HEX_VALUE;
}

0x{HEXDIGIT}{1,7} {
    yylval->hexval = strtoul(yytext, NULL, 16);
    return T_HEX_VALUE;
}

{DIGIT}+ {
    yylval->intval = atoi(yytext);
    return T_DECIMAL;
}

[CGIMRST][AEINP][GMPRU]_{LETTER}+ {
    yylval->string = strdup(yytext);
    return T_IOPORT_SYMBOL;
}

0x[0-7][01]{HEXDIGIT} {
    yylval->hexval = strtoul(yytext, NULL, 16);
    return T_IOPORT;
}

"="     { return T_EQ; }
"!="    { return T_NE; }
"<"     { return T_LT; }
">"     { return T_GT; }
"<="    { return T_LE; }
">="    { return T_GE; }
"<<"    { return T_SHL; }
">>"    { return T_SHR; }

":"      { return T_COLON; }
","      { return T_COMMA; }
"["      { return T_LBRACKET; }
"]"      { return T_RBRACKET; }
"-"      { return T_DASH; }
"_"      { return T_UNDERSCORE; }
"/"      { return T_SLASH; }

{LETTER}{ALNUM}* {
    yylval->string = strdup(yytext);
    return T_LABEL;
}

. { return T_UNKNOWN; }

%%

uint8_t parse_reg_str(const char *text)
{
    if (strcasecmp (text, "R0")  == 0) return (R0);
    if (strcasecmp (text, "R1")  == 0) return (R1);
    if (strcasecmp (text, "R2")  == 0) return (R2);
    if (strcasecmp (text, "R3")  == 0) return (R3);
    if (strcasecmp (text, "R4")  == 0) return (R4);
    if (strcasecmp (text, "R5")  == 0) return (R5);
    if (strcasecmp (text, "R6")  == 0) return (R6);
    if (strcasecmp (text, "R7")  == 0) return (R7);
    if (strcasecmp (text, "R8")  == 0) return (R8);
    if (strcasecmp (text, "R9")  == 0) return (R9);
    if (strcasecmp (text, "R10") == 0) return (R10);
    if (strcasecmp (text, "R11") == 0) return (R11);
    if (strcasecmp (text, "CR")  == 0) return (R11);
    if (strcasecmp (text, "SR")  == 0) return (R12);
    if (strcasecmp (text, "R13") == 0) return (R13);
    if (strcasecmp (text, "DR")  == 0) return (R13);
    if (strcasecmp (text, "R12") == 0) return (R12);
    if (strcasecmp (text, "R14") == 0) return (R14);
    if (strcasecmp (text, "BP")  == 0) return (R14);
    if (strcasecmp (text, "R15") == 0) return (R15);
    if (strcasecmp (text, "SP")  == 0) return (R15);
    if (strcasecmp (text, "IP")  == 0) return (IP);
    if (strcasecmp (text, "IR")  == 0) return (IR);
    if (strcasecmp (text, "IV")  == 0) return (IV);
    return (0);
}

int yywrap(void) {
    return 1;
}
