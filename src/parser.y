%{
/*
 * parser.y: bison grammar for v32sim prompt / command-file lines.
 *
 * The grammar only *recognizes* a command and records its pieces in a cmd_t; it
 * performs no simulator side effects (see command.c: cmd_execute()). One call to
 * yyparse() consumes one line.
 *
 * Summary of the language (case-insensitive; every command word may be abbreviated,
 * see cmd_lookup() in command.c):
 *
 *   break [ADDR|LABEL]            unbreak INDEX             backtrace
 *   continue | next | step | ignore | quit | profile | inventory | version
 *   print[/F]   OPERAND           display[/F] [OPERAND [LABEL]]
 *   label [ADDR NAME]             unlabel INDEX             undisplay INDEX
 *   set [NAME = VALUE]            replace IP:0xA IR:0xB IV:0xC
 *   watch [REG OP VALUE [LABEL]]  watchlist                unwatch INDEX|LABEL
 *   load PAGE:PATH                unload PAGE
 *   gamepad [N] [COMMAND]         help [COMMAND]            screenshot [FILE]
 *   save [FILE]                   restore FILE
 *
 *   run N | wait N                until "TEXT" [timeout N] [rows N]
 *   expect [/d] LOCATION OP VALUE   expect console "TEXT" [rows N]     quit [N]
 *   x[/F] LOCATION [COUNT]
 *
 *   OPERAND  := BASE | '[' BASE ']'                   ('[ ]' dereferences)
 *             | SYMREF | '[' SYMREF ']' | '[[' SYMREF ']]' | '&' SYMREF
 *   BASE     := REGISTER | reg | IOPORT_SYMBOL | HEXNUM | HEXNUM '-' HEXNUM
 *   SYMREF   := NAME | NAME '+' NUMBER | NAME '-' NUMBER      (a label or a C global)
 *   F        := b B d D f o s u x X
 */
#include "defines.h"
#include "command.h"

#define  CMD  (cmd_current ())

static void  set_operand    (uint8_t kind, uint8_t reg, uint32_t lo, uint32_t hi);
static int   operand_number (tok_t *t, uint32_t *value);
static int   operand_iosym  (tok_t *t);
static void  set_fmt        (char ch);
static int   set_lhs_word   (tok_t *t);
static int   break_all      (tok_t *t);
static int   loc_number     (tok_t *t, wloc_t *loc);
static int   loc_iosym      (tok_t *t, wloc_t *loc);
static int   sym_resolve    (tok_t *t, int sign, tok_t *offset, symv_t *out);
static void  set_operand_sym (symv_t *sym, uint8_t deref);
static int   console_form   (tok_t *word, tok_t *text);
static int   print_form     (tok_t *word);
static int   add_option     (tok_t *word, tok_t *value);
static int   check_signed   (void);
%}

%union
{
    tok_t   t;
    char    ch;
    int     op;
    symv_t  s;
}

%token <t>   T_NUM T_REG T_IOSYM T_WORD T_PATH T_FILELINE
%token <ch>  T_FMT
%token       T_REGS
%token       T_EQEQ T_NE T_LE T_GE
%token       T_BACKTRACE T_BREAK T_CONTINUE T_DISPLAY T_GAMEPAD T_HELP T_IGNORE
%token       T_INVENTORY T_LABEL T_LOAD T_NEXT T_PRINT T_PROFILE T_QUIT T_REPLACE
%token       T_ASM T_RESTORE T_SAVE T_SCREENSHOT T_SET T_STEP T_UNBREAK T_UNDISPLAY T_UNLABEL T_UNLOAD
%token       T_UNWATCH T_VERSION T_WATCH T_WATCHLIST
%token       T_KBD T_MAILBOX T_CONSOLE
%token       T_RUN T_UNTIL T_EXPECT T_EXAMINE
%token       T_SSTEP T_SNEXT T_FINISH T_TBREAK T_LIST T_FRAME T_UP T_DOWN
%token       T_BADCMD T_BAD

%type  <t>   name
%type  <op>  relop
%type  <s>   symref

%%

line
    : /* empty line */
    | command
    ;

command
    : T_BACKTRACE                       { CMD -> id = CMD_BACKTRACE; }
    | T_CONTINUE                        { CMD -> id = CMD_CONTINUE;  }
    | T_NEXT                            { CMD -> id = CMD_NEXT;      }
    | T_STEP                            { CMD -> id = CMD_STEP;      }
    | T_SSTEP                           { CMD -> id = CMD_SSTEP;     }
    | T_SNEXT                           { CMD -> id = CMD_SNEXT;     }
    | T_FINISH                          { CMD -> id = CMD_FINISH;    }
    | T_FRAME                           { CMD -> id = CMD_FRAME;     }
    | T_FRAME T_NUM                     { CMD -> id = CMD_FRAME;     CMD -> has_num = TRUE; CMD -> num = $2.u; }
    | T_UP                              { CMD -> id = CMD_UP;        }
    | T_UP T_NUM                        { CMD -> id = CMD_UP;        CMD -> has_num = TRUE; CMD -> num = $2.u; }
    | T_DOWN                            { CMD -> id = CMD_DOWN;      }
    | T_DOWN T_NUM                      { CMD -> id = CMD_DOWN;      CMD -> has_num = TRUE; CMD -> num = $2.u; }
    | list_cmd
    | T_IGNORE                          { CMD -> id = CMD_IGNORE;    }
    | T_QUIT                            { CMD -> id = CMD_QUIT;      }
    | T_QUIT T_NUM                      { CMD -> id = CMD_QUIT;      CMD -> has_num = TRUE; CMD -> num = $2.u; }
    | T_PROFILE                         { CMD -> id = CMD_PROFILE;   }
    | T_INVENTORY                       { CMD -> id = CMD_INVENTORY; }
    | T_VERSION                         { CMD -> id = CMD_VERSION;   }
    | T_WATCHLIST                       { CMD -> id = CMD_WATCHLIST; }
    | T_SAVE                            { CMD -> id = CMD_SAVE; }
    | T_ASM T_PATH                      { CMD -> id = CMD_ASM;     CMD -> path = $2.text; }
    | T_SAVE T_PATH                     { CMD -> id = CMD_SAVE;    CMD -> path = $2.text; }
    | T_RESTORE T_PATH                  { CMD -> id = CMD_RESTORE; CMD -> path = $2.text; }
    | T_RESTORE
        {
            cmd_seterr ("restore: a file name is required");
            YYERROR;
        }
    | T_SCREENSHOT                      { CMD -> id = CMD_SCREENSHOT; }
    | T_SCREENSHOT T_PATH               { CMD -> id = CMD_SCREENSHOT; CMD -> path = $2.text; }
    | break_cmd
    | label_cmd
    | print_cmd
    | display_cmd
    | set_cmd
    | replace_cmd
    | watch_cmd
    | run_cmd
    | until_cmd
    | expect_cmd
    | examine_cmd
    | gamepad_cmd
    | T_KBD                             { CMD -> id = CMD_KBD; }
    | T_KBD T_PATH                      { CMD -> id = CMD_KBD; CMD -> path = $2.text; }
    | T_CONSOLE                         { CMD -> id = CMD_CONSOLE; }
    | T_CONSOLE T_PATH                  { CMD -> id = CMD_CONSOLE; CMD -> path = $2.text; }
    | T_MAILBOX                         { CMD -> id = CMD_MAILBOX; }
    | T_MAILBOX T_NUM                   { CMD -> id = CMD_MAILBOX; CMD -> has_num = TRUE; CMD -> num = $2.u; }
    | help_cmd
    | load_cmd
    | undo_cmd
    | T_BADCMD
        {
            cmd_seterr ("unknown command '%s' (try 'help')", CMD -> badword);
            YYERROR;
        }
    ;

/*
 * Names: anything that can legitimately be used as a label, even if it happens
 * to spell a register or a port (eg: a subroutine called 'cr')
 */
name
    : T_WORD                            { $$ = $1; }
    | T_REG                             { $$ = $1; }
    | T_IOSYM                           { $$ = $1; }
    ;

/* ------------------------------------------------------------------ break */

break_cmd
    : T_BREAK                           { CMD -> id = CMD_BREAK; }
    | T_BREAK T_NUM                     { CMD -> id = CMD_BREAK; CMD -> has_num = TRUE; CMD -> num = $2.u;
                                          CMD -> num_dec = ($2.kind == TOK_DEC); }
    | T_BREAK name                      { CMD -> id = CMD_BREAK; CMD -> name = $2.text; }
    | T_BREAK T_FILELINE                { CMD -> id = CMD_BREAK; CMD -> file = $2.text; CMD -> line = $2.u;
                                          CMD -> has_line = TRUE; }
    | T_BREAK '-' T_WORD T_FILELINE     { if (break_all (&$3)) YYERROR;
                                          CMD -> id = CMD_BREAK; CMD -> file = $4.text; CMD -> line = $4.u;
                                          CMD -> has_line = TRUE; }
    | T_BREAK '-' T_WORD T_NUM          { if (break_all (&$3)) YYERROR;
                                          CMD -> id = CMD_BREAK; CMD -> line = $4.u; CMD -> has_line = TRUE; }
    | T_TBREAK T_NUM                    { CMD -> id = CMD_TBREAK; CMD -> has_num = TRUE; CMD -> num = $2.u;
                                          CMD -> num_dec = ($2.kind == TOK_DEC); }
    | T_TBREAK name                     { CMD -> id = CMD_TBREAK; CMD -> name = $2.text; }
    | T_TBREAK T_FILELINE               { CMD -> id = CMD_TBREAK; CMD -> file = $2.text; CMD -> line = $2.u;
                                          CMD -> has_line = TRUE; }
    ;

/* ------------------------------------------------------------------- list */

list_cmd
    : T_LIST                            { CMD -> id = CMD_LIST; }
    | T_LIST T_NUM                      { CMD -> id = CMD_LIST; CMD -> line = $2.u; CMD -> has_line = TRUE; }
    | T_LIST T_FILELINE                 { CMD -> id = CMD_LIST; CMD -> file = $2.text; CMD -> line = $2.u;
                                          CMD -> has_line = TRUE; }
    | T_LIST name                       { CMD -> id = CMD_LIST; CMD -> name = $2.text; }
    ;

/* ------------------------------------------------------------------ label */

label_cmd
    : T_LABEL                           { CMD -> id = CMD_LABEL; }
    | T_LABEL T_NUM name                { CMD -> id = CMD_LABEL; CMD -> has_num = TRUE;
                                          CMD -> num = $2.u; CMD -> name = $3.text; }
    ;

/* ------------------------------------------------------- print / display */

format
    : /* none */
    | T_FMT                             { set_fmt ($1); if (CMD -> errmsg[0]) YYERROR; }
    ;

print_cmd
    : T_PRINT format operand            { CMD -> id = CMD_PRINT; }
    ;

display_cmd
    : T_DISPLAY                         { CMD -> id = CMD_DISPLAY; }
    | T_DISPLAY format operand          { CMD -> id = CMD_DISPLAY; }
    | T_DISPLAY format operand name     { CMD -> id = CMD_DISPLAY; CMD -> name = $4.text; }
    ;

operand
    : base                              { CMD -> opd.deref = FALSE; }
    | '[' base ']'                      { CMD -> opd.deref = TRUE;  }
    | symref                            { set_operand_sym (&$1, FALSE); }
    | '[' symref ']'                    { set_operand_sym (&$2, FALSE); }
    | '[' '[' symref ']' ']'            { set_operand_sym (&$3, TRUE);  }
    | '&' symref                        { set_operand (OPD_VALUE, 0, $2.addr, 0);
                                          CMD -> opd.name = $2.text; }
    ;

/*
 * A symbol (a label of the debug files or a C global, see symbols.c) stands for the
 * variable it names: 'print counter' and 'print [counter]' both show its value, and
 * '[[p]]' follows a pointer variable. '&counter' is its address. One '+' or '-' with a
 * number (decimal or hexadecimal) reaches into arrays and structures.
 */
symref
    : T_WORD                            { if (sym_resolve (&$1, 0, NULL, &$$)) YYERROR; }
    | T_WORD '+' T_NUM                  { if (sym_resolve (&$1, +1, &$3, &$$)) YYERROR; }
    | T_WORD '-' T_NUM                  { if (sym_resolve (&$1, -1, &$3, &$$)) YYERROR; }
    ;

base
    : T_REG                             { set_operand (OPD_REG, (uint8_t) $1.u, 0, 0); }
    | T_REGS                            { set_operand (OPD_REGS, 0, 0, 0); }
    | T_IOSYM                           { if (operand_iosym (&$1)) YYERROR; }
    | T_NUM                             { uint32_t v = 0;
                                          if (operand_number (&$1, &v)) YYERROR;
                                          if ($1.digits <= 3)
                                              set_operand (OPD_IOPORT, 0, v, 0);
                                          else
                                              set_operand (OPD_MEM,    0, v, 0); }
    | T_NUM '-' T_NUM                   { uint32_t lo = 0, hi = 0;
                                          if (operand_number (&$1, &lo)) YYERROR;
                                          if (operand_number (&$3, &hi)) YYERROR;
                                          set_operand (OPD_RANGE, 0, lo, hi); }
    ;

/* -------------------------------------------------------------------- set */

set_cmd
    : T_SET                             { CMD -> id = CMD_SET; }
    | T_SET set_lhs '=' set_rhs         { CMD -> id = CMD_SET; }
    ;

set_lhs
    : T_REG                             { CMD -> set_kind = SETK_REG; CMD -> set_reg = (uint8_t) $1.u; }
    | T_IOSYM                           { int16_t p = ioports_num ((uint8_t *) $1.text);
                                          if (p < 0) { cmd_seterr ("unknown IOPort '%s'", $1.text); YYERROR; }
                                          CMD -> set_kind = SETK_IOPORT; CMD -> set_addr = (uint32_t) p; }
    | T_NUM                             { uint32_t v = 0;
                                          if (operand_number (&$1, &v)) YYERROR;
                                          CMD -> set_kind = ($1.digits <= 3) ? SETK_IOPORT : SETK_MEM;
                                          CMD -> set_addr = v; }
    | T_WORD                            { if (set_lhs_word (&$1)) YYERROR; }
    ;

set_rhs
    : T_NUM                             { CMD -> set_isnum = TRUE; CMD -> set_val = $1; }
    | '-' T_NUM                         { CMD -> set_isnum = TRUE; CMD -> set_val = $2; CMD -> set_neg = TRUE; }
    | '+' T_NUM                         { CMD -> set_isnum = TRUE; CMD -> set_val = $2; }
    | T_WORD                            { CMD -> word = $1.text; }
    ;

/* ---------------------------------------------------------------- replace */

replace_cmd
    : T_REPLACE repl_list               { CMD -> id = CMD_REPLACE; }
    ;

repl_list
    : repl_item
    | repl_list repl_item
    ;

repl_item
    : T_REG ':' T_NUM
        {
            if (($1.u != IP) && ($1.u != IR) && ($1.u != IV))
            {
                cmd_seterr ("replace: only IP, IR and IV can be replaced (got '%s')", $1.text);
                YYERROR;
            }
            if (CMD -> rep_count >= CMD_MAX_REPLACE)
            {
                cmd_seterr ("replace: at most %d items (IP, IR, IV)", CMD_MAX_REPLACE);
                YYERROR;
            }
            CMD -> rep[CMD -> rep_count].which = (uint8_t) $1.u;
            CMD -> rep[CMD -> rep_count].value = $3.u;
            CMD -> rep_count = CMD -> rep_count + 1;
        }
    ;

/* ------------------------------------------------------ watch / unwatch */

watch_cmd
    : T_WATCH                           { CMD -> id = CMD_WATCHLIST; }
    | T_WATCH format watch_target relop watch_rhs
        {
            CMD -> id = CMD_WATCH; CMD -> watch_op = (uint8_t) $4;
            if (check_signed ()) YYERROR;
        }
    | T_WATCH format watch_target relop watch_rhs name
        {
            CMD -> id = CMD_WATCH; CMD -> watch_op = (uint8_t) $4;
            CMD -> name = $6.text;
            if (check_signed ()) YYERROR;
        }
    ;

/* what is watched: a register, an IOPort (symbol or 0x...), or a memory word (0x.... or a symbol) */
watch_target
    : T_REG                             { CMD -> watch_target.kind = WLOC_REG;    CMD -> watch_target.id = $1.u; }
    | T_IOSYM                           { if (loc_iosym (&$1, &CMD -> watch_target)) YYERROR; }
    | T_NUM                             { if (loc_number (&$1, &CMD -> watch_target)) YYERROR; }
    | symref                            { CMD -> watch_target.kind = WLOC_MEM;    CMD -> watch_target.id = $1.addr; }
    | '[' symref ']'                    { CMD -> watch_target.kind = WLOC_MEM;    CMD -> watch_target.id = $2.addr; }
    ;

/* what it is compared with: a number, a register, an IOPort symbol, [0x...] or [SYMBOL] contents,
 * or the address of a symbol (SYMBOL, &SYMBOL) */
watch_rhs
    : T_NUM                             { CMD -> watch_rhs.kind = WLOC_IMM; CMD -> watch_rhs.id = $1.u; }
    | '-' T_NUM                         { CMD -> watch_rhs.kind = WLOC_IMM; CMD -> watch_rhs.id = (uint32_t) (0 - $2.u); }
    | '+' T_NUM                         { CMD -> watch_rhs.kind = WLOC_IMM; CMD -> watch_rhs.id = $2.u; }
    | T_REG                             { CMD -> watch_rhs.kind = WLOC_REG; CMD -> watch_rhs.id = $1.u; }
    | T_IOSYM                           { if (loc_iosym (&$1, &CMD -> watch_rhs)) YYERROR; }
    | '[' T_NUM ']'                     { if (loc_number (&$2, &CMD -> watch_rhs)) YYERROR; }
    | '[' T_IOSYM ']'                   { if (loc_iosym (&$2, &CMD -> watch_rhs)) YYERROR; }
    | '[' T_REG ']'                     { CMD -> watch_rhs.kind = WLOC_REG; CMD -> watch_rhs.id = $2.u; }
    | '[' symref ']'                    { CMD -> watch_rhs.kind = WLOC_MEM; CMD -> watch_rhs.id = $2.addr; }
    | symref                            { CMD -> watch_rhs.kind = WLOC_IMM; CMD -> watch_rhs.id = $1.addr; }
    | '&' symref                        { CMD -> watch_rhs.kind = WLOC_IMM; CMD -> watch_rhs.id = $2.addr; }
    ;

relop
    : '='                               { $$ = WATCH_OP_EQUAL;         }
    | T_EQEQ                            { $$ = WATCH_OP_EQUAL;         }
    | T_NE                              { $$ = WATCH_OP_NOT_EQUAL;     }
    | '<'                               { $$ = WATCH_OP_LESS;          }
    | '>'                               { $$ = WATCH_OP_GREATER;       }
    | T_LE                              { $$ = WATCH_OP_LESS_EQUAL;    }
    | T_GE                              { $$ = WATCH_OP_GREATER_EQUAL; }
    ;

/* ------------------------------------------------------------------- run */

run_cmd
    : T_RUN T_NUM                       { CMD -> id = CMD_RUN; CMD -> has_count = TRUE; CMD -> count = $2.u; }
    | T_RUN
        {
            cmd_seterr ("run: the number of frames is required (run N)");
            YYERROR;
        }
    ;

/* ----------------------------------------------------------------- until */

until_cmd
    : T_UNTIL                           { CMD -> id = CMD_UNTILSRC; }
    | T_UNTIL T_NUM                     { CMD -> id = CMD_UNTILSRC; CMD -> line = $2.u; CMD -> has_line = TRUE; }
    | T_UNTIL T_FILELINE                { CMD -> id = CMD_UNTILSRC; CMD -> file = $2.text; CMD -> line = $2.u;
                                          CMD -> has_line = TRUE; }
    | T_UNTIL name until_opts           { CMD -> id = CMD_UNTIL; CMD -> text = $2.text;
                                          if ($2.text[0] == '\0') { cmd_seterr ("until: the text is empty"); YYERROR; } }
    ;

until_opts
    : /* none */
    | until_opts T_WORD T_NUM           { if (add_option (&$2, &$3)) YYERROR; }
    ;

/* ---------------------------------------------------------------- expect */

expect_cmd
    : T_EXPECT T_WORD T_WORD expect_opts
        {
            if (console_form (&$2, &$3)) YYERROR;
        }
    | T_EXPECT T_WORD watch_target relop watch_rhs
        {
            if (print_form (&$2)) YYERROR;
            CMD -> id = CMD_EXPECT; CMD -> expect_kind = EXPECT_VALUE; CMD -> watch_op = (uint8_t) $4;
        }
    | T_EXPECT T_WORD T_FMT watch_target relop watch_rhs
        {
            if (print_form (&$2)) YYERROR;
            set_fmt ($3); if (CMD -> errmsg[0]) YYERROR;
            CMD -> id = CMD_EXPECT; CMD -> expect_kind = EXPECT_VALUE; CMD -> watch_op = (uint8_t) $5;
            if (check_signed ()) YYERROR;
        }
    | T_EXPECT watch_target relop watch_rhs
        {
            CMD -> id = CMD_EXPECT; CMD -> expect_kind = EXPECT_VALUE; CMD -> watch_op = (uint8_t) $3;
        }
    | T_EXPECT T_FMT watch_target relop watch_rhs
        {
            set_fmt ($2); if (CMD -> errmsg[0]) YYERROR;
            CMD -> id = CMD_EXPECT; CMD -> expect_kind = EXPECT_VALUE; CMD -> watch_op = (uint8_t) $4;
            if (check_signed ()) YYERROR;
        }
    ;

expect_opts
    : /* none */
    | expect_opts T_WORD T_NUM          { if (add_option (&$2, &$3)) YYERROR; }
    ;

/* ------------------------------------------------------------------- x */

examine_cmd
    : T_EXAMINE format examine_loc      { CMD -> id = CMD_EXAMINE; }
    | T_EXAMINE format examine_loc T_NUM
        {
            CMD -> id = CMD_EXAMINE; CMD -> has_count = TRUE; CMD -> count = $4.u;
        }
    ;

examine_loc
    : symref                            { set_operand (OPD_MEM, 0, $1.addr, 0); CMD -> opd.name = $1.text; CMD -> x_words = $1.words; }
    | '[' symref ']'                    { set_operand (OPD_MEM, 0, $2.addr, 0); CMD -> opd.name = $2.text; CMD -> x_words = $2.words; }
    | T_NUM                             { uint32_t v = 0;
                                          if (operand_number (&$1, &v)) YYERROR;
                                          set_operand (OPD_MEM, 0, v, 0); CMD -> x_words = 1; }
    ;

/* --------------------------------------------------------------- gamepad */

gamepad_cmd
    : T_GAMEPAD                         { CMD -> id = CMD_GAMEPAD; }
    | T_GAMEPAD T_NUM                   { CMD -> id = CMD_GAMEPAD; CMD -> has_num = TRUE; CMD -> num = $2.u; }
    | T_GAMEPAD gp_word                 { CMD -> id = CMD_GAMEPAD; }
    | T_GAMEPAD T_NUM gp_word           { CMD -> id = CMD_GAMEPAD; CMD -> has_num = TRUE; CMD -> num = $2.u; }
    ;

gp_word
    : name                              { CMD -> word = $1.text; }
    ;

/* ------------------------------------------------------------------ help */

help_cmd
    : T_HELP                            { CMD -> id = CMD_HELP; }
    | T_HELP name                       { CMD -> id = CMD_HELP; CMD -> name = $2.text; }
    ;

/* ------------------------------------------------------------ load/unload */

load_cmd
    : T_LOAD T_WORD ':' T_PATH          { CMD -> id = CMD_LOAD;   CMD -> name = $2.text; CMD -> path = $4.text; }
    | T_UNLOAD T_WORD                   { CMD -> id = CMD_UNLOAD; CMD -> name = $2.text; }
    ;

/* ----------------------------------------------------- unbreak and friends */

undo_cmd
    : T_UNBREAK T_NUM                   { CMD -> id = CMD_UNBREAK;   CMD -> has_num = TRUE; CMD -> num = $2.u; }
    | T_UNDISPLAY T_NUM                 { CMD -> id = CMD_UNDISPLAY; CMD -> has_num = TRUE; CMD -> num = $2.u; }
    | T_UNLABEL T_NUM                   { CMD -> id = CMD_UNLABEL;   CMD -> has_num = TRUE; CMD -> num = $2.u; }
    | T_UNWATCH T_NUM                   { CMD -> id = CMD_UNWATCH;   CMD -> has_num = TRUE; CMD -> num = $2.u; }
    | T_UNWATCH name                    { CMD -> id = CMD_UNWATCH;   CMD -> name = $2.text; }
    ;

%%

/* ------------------------------------------------------------- helpers */

static void  set_operand (uint8_t kind, uint8_t reg, uint32_t lo, uint32_t hi)
{
    CMD -> opd.kind  = kind;
    CMD -> opd.reg   = reg;
    CMD -> opd.lo    = lo;
    CMD -> opd.hi    = hi;
}

/*
 * operand_number(): print/display/set operands are written in hex (0x...)
 */
static int  operand_number (tok_t *t, uint32_t *value)
{
    if (t -> kind != TOK_HEX)
    {
        cmd_seterr ("'%s': addresses and IOPort numbers must be hexadecimal (0x...)", t -> text);
        return (1);
    }
    *value = t -> u;
    return (0);
}

static int  operand_iosym (tok_t *t)
{
    int16_t   port   = ioports_num ((uint8_t *) t -> text);
    uint32_t  addr   = 0;
    uint32_t  words  = 0;

    if (port < 0)
    {
        if (sym_lookup (t -> text, &addr, &words))      // a C global that happens to be named like a port group
        {
            set_operand (OPD_MEM, 0, addr, 0);
            CMD -> opd.name  = t -> text;
            return (0);
        }
        cmd_seterr ("unknown IOPort '%s'", t -> text);
        return (1);
    }
    set_operand (OPD_IOPORT, 0, (uint32_t) port, 0);
    return (0);
}

/*
 * loc_number()/loc_iosym(): watchpoint locations. As with print, a hexadecimal number
 * of up to 3 digits is an IOPort and a longer one a memory address.
 */
static int  loc_number (tok_t *t, wloc_t *loc)
{
    if (t -> kind != TOK_HEX)
    {
        cmd_seterr ("'%s': addresses and IOPort numbers must be hexadecimal (0x...)", t -> text);
        return (1);
    }
    loc -> kind  = (t -> digits <= 3) ? WLOC_IOPORT : WLOC_MEM;
    loc -> id    = t -> u;
    return (0);
}

static int  loc_iosym (tok_t *t, wloc_t *loc)
{
    int16_t   port   = ioports_num ((uint8_t *) t -> text);
    uint32_t  addr   = 0;
    uint32_t  words  = 0;

    if (port < 0)
    {
        if (sym_lookup (t -> text, &addr, &words))      // a C global that happens to be named like a port group
        {
            loc -> kind  = WLOC_MEM;
            loc -> id    = addr;
            return (0);
        }
        cmd_seterr ("unknown IOPort '%s'", t -> text);
        return (1);
    }
    loc -> kind  = WLOC_IOPORT;
    loc -> id    = (uint32_t) port;
    return (0);
}

/*
 * set_fmt(): /b /B /d /D /f /o /s /u /x /X
 */
static void  set_fmt (char ch)
{
    switch (ch)
    {
        case 'b':  CMD -> fmt = FORMAT_BINARY;    break;
        case 'B':  CMD -> fmt = FORMAT_BOOLEAN;   break;
        case 'd':  CMD -> fmt = FORMAT_SIGNED;    break;
        case 'D':  CMD -> fmt = FORMAT_DECODE;    break;
        case 'f':
        case 'F':  CMD -> fmt = FORMAT_FLOAT;     break;
        case 'o':
        case 'O':  CMD -> fmt = FORMAT_OCTAL;     break;
        case 's':
        case 'S':  CMD -> fmt = FORMAT_STRING;    break;
        case 'u':
        case 'U':  CMD -> fmt = FORMAT_UNSIGNED;  break;
        case 'x':  CMD -> fmt = FORMAT_LOWERHEX;  break;
        case 'X':  CMD -> fmt = FORMAT_HEX;       break;
        default:
            cmd_seterr ("unknown format '/%c' (valid: b B d D f o s u x X)", ch);
            break;
    }
}

/*
 * set_lhs_word(): the left side of 'set NAME = VALUE' is a simulator setting
 */
static int  break_all (tok_t *t)
{
    if (0 != strcasecmp (t -> text, "a"))
    {
        cmd_seterr ("break: unknown option '-%s' (only -a: every address of the line)", t -> text);
        return (1);
    }
    CMD -> all  = TRUE;
    return (0);
}

static int  set_lhs_word (tok_t *t)
{
    static const struct { const char *name; uint8_t opt; } options[] =
    {
        { "color",    OPT_COLOR    }, { "colors",   OPT_COLOR    },
        { "deref",    OPT_DEREF    }, { "derefaddr", OPT_DEREF   },
        { "errorchk", OPT_ERRORCHK }, { "errorcheck", OPT_ERRORCHK },
        { "error",    OPT_ERRORCHK },
        { "debug",    OPT_DEBUG    },
        { "profile",  OPT_PROFILE  },
        { "verbose",  OPT_VERBOSE  },
        { "source",   OPT_SOURCE   },
        { "mode",     OPT_MODE     },
        { NULL,       OPT_NONE     }
    };
    int  index  = 0;

    for (index = 0; options[index].name != NULL; index = index + 1)
    {
        if (0 == strcasecmp (options[index].name, t -> text))
        {
            CMD -> set_kind = SETK_OPTION;
            CMD -> set_opt  = options[index].opt;
            CMD -> name     = t -> text;
            return (0);
        }
    }

    cmd_seterr ("set: unknown setting '%s' (color deref errorchk debug profile verbose)", t -> text);
    return (1);
}

/*
 * sym_resolve(): a name (with an optional '+'/'-' offset) -> the address it stands for.
 * The offset is a number of words, written in decimal or hexadecimal.
 */
static int  sym_resolve (tok_t *t, int sign, tok_t *offset, symv_t *out)
{
    uint32_t  addr   = 0;
    uint32_t  words  = 0;
    char      text[160];

    if (!sym_lookup (t -> text, &addr, &words))
    {
        cmd_seterr ("unknown symbol '%s' (a label of the debug files, or a C global from NAME.asm.sym; "
                    "'label 0xADDR NAME' defines one by hand)", t -> text);
        return (1);
    }
    out -> text   = t -> text;
    out -> words  = words;
    if (offset != NULL)
    {
        if (offset -> kind == TOK_FLT)
        {
            cmd_seterr ("'%s': an offset must be a whole number", offset -> text);
            return (1);
        }
        addr        = (sign > 0) ? (addr + offset -> u) : (addr - offset -> u);
        words       = (words > offset -> u) ? (words - offset -> u) : 1;
        out -> words  = (sign > 0) ? words : 1;
        snprintf (text, sizeof (text), "%s%c%s", t -> text, (sign > 0) ? '+' : '-', offset -> text);
        out -> text = cmd_strdup (text, strlen (text));
    }
    out -> addr = addr;
    return (0);
}

static void  set_operand_sym (symv_t *sym, uint8_t deref)
{
    set_operand (OPD_MEM, 0, sym -> addr, 0);
    CMD -> opd.deref  = deref;
    CMD -> opd.name   = sym -> text;
}

/*
 * expect console "TEXT" -- the first word must say so
 */
static int  console_form (tok_t *word, tok_t *text)
{
    if (0 != strcasecmp (word -> text, "console"))
    {
        cmd_seterr ("expect: use 'expect LOCATION OP VALUE' or 'expect console \"TEXT\"' (got '%s')", word -> text);
        return (1);
    }
    if (text -> text[0] == '\0')
    {
        cmd_seterr ("expect console: the text is empty");
        return (1);
    }
    CMD -> id           = CMD_EXPECT;
    CMD -> expect_kind  = EXPECT_CONSOLE;
    CMD -> text         = text -> text;
    return (0);
}

/*
 * expect print LOCATION OP VALUE -- 'print' is optional noise, any other word is a mistake
 */
static int  print_form (tok_t *word)
{
    if (0 != strcasecmp (word -> text, "print"))
    {
        cmd_seterr ("expect: use 'expect LOCATION OP VALUE' or 'expect console \"TEXT\"' (got '%s')", word -> text);
        return (1);
    }
    return (0);
}

/*
 * until/expect console options: timeout N (frames), rows N (only the last N rows count)
 */
static int  add_option (tok_t *word, tok_t *value)
{
    if (value -> kind == TOK_FLT)
    {
        cmd_seterr ("'%s': a whole number is required", value -> text);
        return (1);
    }
    if (0 == strcasecmp (word -> text, "rows"))
    {
        CMD -> rows     = value -> u;
        return (0);
    }
    if (0 == strcasecmp (word -> text, "timeout"))
    {
        CMD -> timeout  = value -> u;
        return (0);
    }
    cmd_seterr ("unknown option '%s' (timeout N, rows N)", word -> text);
    return (1);
}

/*
 * watch/d and expect/d compare as signed numbers; /u is the default; other formats make no sense
 */
static int  check_signed (void)
{
    switch (CMD -> fmt)
    {
        case FORMAT_DEFAULT:
        case FORMAT_UNSIGNED:
            CMD -> watch_signed = FALSE;
            return (0);

        case FORMAT_SIGNED:
            CMD -> watch_signed = TRUE;
            return (0);
    }
    cmd_seterr ("only /d (signed) and /u (unsigned) can be used when comparing");
    return (1);
}
