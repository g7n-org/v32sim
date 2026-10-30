%{
/*
 * lexer.l: flex scanner for v32sim prompt / command-file lines.
 *
 * One call to yyparse() consumes exactly one command line. The scanner has four
 * start conditions:
 *
 *   INITIAL   expecting the command word at the start of the line
 *   ARGS      scanning the arguments of an ordinary command
 *   LOADARGS  scanning 'load'/'unload' arguments:  PAGE[:PATH]
 *   PATH      the rest of the line after the ':' of a load command is a file path
 *
 * Command words are classified by cmd_lookup() (command.c), which also implements
 * the accepted abbreviations ('s' = step, 'se' = set, 'c' = continue, ...).
 *
 * Case does not matter anywhere, except that the letter in a /FORMAT suffix keeps
 * its case (/x lowercase hex vs /X uppercase hex).
 */
#include "defines.h"
#include "command.h"
#include "parser.h"

static int    lex_pos        = 0;      /* columns consumed so far              */
static int    lex_col        = 0;      /* column (1-based) of current token    */
static char   lex_text[64];            /* text of the current token (for errs) */

#define YY_USER_ACTION  lex_col = lex_pos + 1; lex_pos += yyleng;              \
                        snprintf (lex_text, sizeof (lex_text), "%s", yytext);

static int  lex_number  (int base, const char *digits, int ndigits, uint8_t kind);
static int  lex_float   (void);
static int  lex_value   (int token, uint32_t u);
static int  lex_command (void);
static int  lex_fileline (void);
%}

%option noyywrap nounput noinput 8bit case-insensitive nodefault

%x ARGS
%x LOADARGS
%x PATH

WORD     [A-Za-z_.$@][A-Za-z0-9_.$@]*

%%

<INITIAL>{
[ \t\r\n]+                  { /* leading whitespace */ }
"#".*                       { /* whole-line comment (command files) */ }
[A-Za-z?]+                  { return lex_command (); }
.                           { cmd_seterr ("a command must start with a letter"); return T_BAD; }
}

<ARGS>{
[ \t\r\n]+                  { /* separators */ }
"#".*                       { /* trailing comment */ }

"/"[A-Za-z]                 { yylval.ch = yytext[1]; return T_FMT; }

[A-Za-z0-9_$@~+/\\-]*[./\\][A-Za-z0-9_.$@~+/\\-]*":"[0-9]+  { return lex_fileline (); }

0b[01]{1,32}                { return lex_number (2,  yytext + 2, yyleng - 2, TOK_BIN); }
0x[0-9a-f]{1,8}h?           { return lex_number (16, yytext + 2, yyleng - 2 - (yytext[yyleng-1] == 'h' || yytext[yyleng-1] == 'H'), TOK_HEX); }
0[0-7]+                     { return lex_number (8,  yytext + 1, yyleng - 1, TOK_OCT); }
[0-9]+                      { return lex_number (10, yytext,     yyleng,     TOK_DEC); }
[0-9]+"."[0-9]*             { return lex_float (); }
"."[0-9]+                   { return lex_float (); }

R(1[0-5]|[0-9])             { return lex_value (T_REG, (uint32_t) atoi (yytext + 1)); }
BP                          { return lex_value (T_REG, BP); }
SP                          { return lex_value (T_REG, SP); }
CR                          { return lex_value (T_REG, CR); }
SR                          { return lex_value (T_REG, SR); }
DR                          { return lex_value (T_REG, DR); }
IP                          { return lex_value (T_REG, IP); }
IR                          { return lex_value (T_REG, IR); }
IV                          { return lex_value (T_REG, IV); }

reg|regs|register|registers { return lex_value (T_REGS, 0); }
"r*"                        { return lex_value (T_REGS, 0); }

(TIM|RNG|GPU|SPU|INP|CAR|MEM)_[A-Za-z0-9]+  { return lex_value (T_IOSYM, 0); }

{WORD}                      { return lex_value (T_WORD, 0); }
\"[^"\n]*\"                 { yylval.t.text = cmd_strdup (yytext + 1, yyleng - 2);
                              yylval.t.u = 0; return T_WORD; }

"=="                        { return T_EQEQ; }
"!="                        { return T_NE;   }
"<="                        { return T_LE;   }
">="                        { return T_GE;   }
"<"|">"|"="|"["|"]"|"-"|"+"|":"|"&"  { return yytext[0]; }

.                           { cmd_seterr ("unexpected character '%s'", yytext); return T_BAD; }
}

<LOADARGS>{
[ \t\r\n]+                  { /* separators */ }
":"                         { BEGIN (PATH); return ':'; }
[A-Za-z]+                   { return lex_value (T_WORD, 0); }
.                           { cmd_seterr ("unexpected character '%s'", yytext); return T_BAD; }
}

<PATH>{
[ \t\r\n]+                  { /* skip blanks before the path */ }
\"[^"\n]*\"                 { yylval.t.text = cmd_strdup (yytext + 1, yyleng - 2);
                              yylval.t.u = 0; return T_PATH; }
[^ \t\r\n].*                { int n = yyleng;                  /* trim trailing blanks */
                              while ((n > 0) && ((yytext[n-1] == ' ')  ||
                                                 (yytext[n-1] == '\t') ||
                                                 (yytext[n-1] == '\r')))
                              {
                                  n = n - 1;
                              }
                              yylval.t.text = cmd_strdup (yytext, n);
                              yylval.t.u = 0; return T_PATH; }
}

<*><<EOF>>                  { lex_text[0] = '\0'; yyterminate (); }

%%

/*
 * lex_value(): build the semantic value for a text-bearing token
 */
static int  lex_value (int token, uint32_t u)
{
    yylval.t.text   = cmd_strdup (yytext, yyleng);
    yylval.t.u      = u;
    yylval.t.kind   = 0;
    yylval.t.digits = 0;
    yylval.t.f      = 0.0;
    return (token);
}

/*
 * lex_number(): integer literal. Values wider than 32 bits are rejected here
 * (a negative number is a '-' token followed by a number, so a decimal 2147483648
 * is legal as the magnitude of -2147483648).
 */
static int  lex_number (int base, const char *digits, int ndigits, uint8_t kind)
{
    unsigned long long  value  = 0;
    char                buf[48];

    if (ndigits > (int) sizeof (buf) - 1)
    {
        ndigits = (int) sizeof (buf) - 1;
    }
    snprintf (buf, sizeof (buf), "%.*s", ndigits, digits);
    value = strtoull (buf, NULL, base);
    if ((value > 0xFFFFFFFFULL) || (strlen (buf) > 32))
    {
        cmd_seterr ("number '%s' does not fit in 32 bits", yytext);
        return (T_BAD);
    }

    yylval.t.text   = cmd_strdup (yytext, yyleng);
    yylval.t.u      = (uint32_t) value;
    yylval.t.f      = 0.0;
    yylval.t.kind   = kind;
    yylval.t.digits = (uint8_t) ((ndigits > 255) ? 255 : ndigits);
    return (T_NUM);
}

/*
 * lex_fileline(): FILE:LINE (the file has a '.' or a '/', so IP:0x10 and the like never match).
 * 'text' is the file name and 'u' the line.
 */
static int  lex_fileline (void)
{
    char  *colon  = strrchr (yytext, ':');

    yylval.t.text   = cmd_strdup (yytext, (size_t) (colon - yytext));
    yylval.t.u      = (uint32_t) strtoul (colon + 1, NULL, 10);
    yylval.t.kind   = TOK_DEC;
    yylval.t.digits = 0;
    yylval.t.f      = 0.0;
    return (T_FILELINE);
}

/*
 * lex_float(): floating point literal; 'u' carries the IEEE-754 bit pattern
 */
static int  lex_float (void)
{
    float  f  = strtof (yytext, NULL);

    yylval.t.text   = cmd_strdup (yytext, yyleng);
    yylval.t.f      = f;
    memcpy (&yylval.t.u, &f, sizeof (f));
    yylval.t.kind   = TOK_FLT;
    yylval.t.digits = 0;
    return (T_NUM);
}

/*
 * lex_command(): classify the first word of the line and pick the start
 * condition for its arguments
 */
static int  lex_command (void)
{
    cmd_t  *cmd  = cmd_current ();
    int     id   = cmd_lookup (yytext);

    BEGIN (ARGS);
    switch (id)
    {
        case CMD_BACKTRACE:   return (T_BACKTRACE);
        case CMD_BREAK:       return (T_BREAK);
        case CMD_CONSOLE:     BEGIN (PATH); return (T_CONSOLE);
        case CMD_CONTINUE:    return (T_CONTINUE);
        case CMD_DISPLAY:     return (T_DISPLAY);
        case CMD_GAMEPAD:     return (T_GAMEPAD);
        case CMD_ASM:         BEGIN (PATH); return (T_ASM);
        case CMD_HELP:        return (T_HELP);
        case CMD_IGNORE:      return (T_IGNORE);
        case CMD_INVENTORY:   return (T_INVENTORY);
        case CMD_KBD:         BEGIN (PATH); return (T_KBD);
        case CMD_LABEL:       return (T_LABEL);
        case CMD_MAILBOX:     return (T_MAILBOX);
        case CMD_LOAD:        BEGIN (LOADARGS); return (T_LOAD);
        case CMD_NEXT:        return (T_NEXT);
        case CMD_PRINT:       return (T_PRINT);
        case CMD_PROFILE:     return (T_PROFILE);
        case CMD_QUIT:        return (T_QUIT);
        case CMD_REPLACE:     return (T_REPLACE);
        case CMD_RESTORE:     BEGIN (PATH); return (T_RESTORE);
        case CMD_SAVE:        BEGIN (PATH); return (T_SAVE);
        case CMD_SCREENSHOT:  BEGIN (PATH); return (T_SCREENSHOT);
        case CMD_SET:         return (T_SET);
        case CMD_STEP:        return (T_STEP);
        case CMD_UNBREAK:     return (T_UNBREAK);
        case CMD_UNDISPLAY:   return (T_UNDISPLAY);
        case CMD_UNLABEL:     return (T_UNLABEL);
        case CMD_UNLOAD:      BEGIN (LOADARGS); return (T_UNLOAD);
        case CMD_UNWATCH:     return (T_UNWATCH);
        case CMD_VERSION:     return (T_VERSION);
        case CMD_WATCH:       return (T_WATCH);
        case CMD_WATCHLIST:   return (T_WATCHLIST);
        case CMD_RUN:         return (T_RUN);
        case CMD_UNTIL:       return (T_UNTIL);
        case CMD_EXPECT:      return (T_EXPECT);
        case CMD_EXAMINE:     return (T_EXAMINE);
        case CMD_SSTEP:       return (T_SSTEP);
        case CMD_SNEXT:       return (T_SNEXT);
        case CMD_FINISH:      return (T_FINISH);
        case CMD_TBREAK:      return (T_TBREAK);
        case CMD_LIST:        return (T_LIST);
        case CMD_FRAME:       return (T_FRAME);
        case CMD_UP:          return (T_UP);
        case CMD_DOWN:        return (T_DOWN);
        default:
            snprintf (cmd -> badword, sizeof (cmd -> badword), "%s", yytext);
            return (T_BADCMD);
    }
}

/*
 * lex_begin()/lex_end(): bracket the scan of one line
 */
static YY_BUFFER_STATE  lex_buffer  = NULL;

void  lex_begin (const char *line)
{
    lex_pos        = 0;
    lex_col        = 0;
    lex_text[0]    = '\0';
    BEGIN (INITIAL);
    lex_buffer     = yy_scan_string (line);
}

void  lex_end (void)
{
    if (lex_buffer != NULL)
    {
        yy_delete_buffer (lex_buffer);
        lex_buffer  = NULL;
    }
}

const char *lex_lasttext (void)
{
    return (lex_text);
}

int  lex_lastcol (void)
{
    return (lex_col);
}
