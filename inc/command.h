#ifndef  _COMMAND_H
#define  _COMMAND_H

////////////////////////////////////////////////////////////////////////////////////////
//
// command.h
//
// Interface between the prompt/command-file front end and the flex/bison command
// parser:
//
//   lexer.l    flex scanner   - turns one command line into tokens
//   parser.y   bison grammar  - turns the tokens into a cmd_t (a parsed command)
//   command.c  executor       - carries out a cmd_t (all simulator side effects)
//
// The parser never touches simulator state: it only fills in a cmd_t. Everything
// with a side effect lives in command.c. process_command() ties the three together
// and is the only entry point most of the program needs.
//
////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
//
// command identifiers (cmd_t.id)
//
enum cmd_id
{
    CMD_NONE                     = 0,
    CMD_ASM,
    CMD_BACKTRACE,
    CMD_BREAK,
    CMD_CONSOLE,
    CMD_CONTINUE,
    CMD_DISPLAY,
    CMD_GAMEPAD,
    CMD_HELP,
    CMD_IGNORE,
    CMD_INVENTORY,
    CMD_KBD,
    CMD_LABEL,
    CMD_LOAD,
    CMD_MAILBOX,
    CMD_NEXT,
    CMD_PRINT,
    CMD_PROFILE,
    CMD_QUIT,
    CMD_REPLACE,
    CMD_RESTORE,
    CMD_SAVE,
    CMD_SCREENSHOT,
    CMD_SET,
    CMD_STEP,
    CMD_UNBREAK,
    CMD_UNDISPLAY,
    CMD_UNLABEL,
    CMD_UNLOAD,
    CMD_UNWATCH,
    CMD_VERSION,
    CMD_WATCH,
    CMD_WATCHLIST,
    CMD_RUN,
    CMD_UNTIL,
    CMD_EXPECT,
    CMD_EXAMINE
};

////////////////////////////////////////////////////////////////////////////////////////
//
// literal kinds (tok_t.kind)
//
#define  TOK_DEC                 0
#define  TOK_HEX                 1
#define  TOK_OCT                 2
#define  TOK_BIN                 3
#define  TOK_FLT                 4

////////////////////////////////////////////////////////////////////////////////////////
//
// token semantic value: every value-carrying token (number, register, IOPort symbol,
// word, path) uses this one structure. 'text' is the original spelling, owned by the
// cmd_t string pool. For registers 'u' is the register id. For numbers 'u' holds the
// 32-bit value (for TOK_FLT, the IEEE-754 bit pattern), 'digits' the number of digits
// written (needed to tell a 3-digit IOPort from an 8-digit memory address).
//
typedef struct tok
{
    char      *text;
    uint32_t   u;
    float      f;
    uint8_t    kind;
    uint8_t    digits;
} tok_t;

////////////////////////////////////////////////////////////////////////////////////////
//
// print/display operand kinds (operand_t.kind)
//
#define  OPD_NONE                0
#define  OPD_REG                 1   // one register (id)
#define  OPD_REGS                2   // all of R0-R15
#define  OPD_MEM                 3   // memory address (lo)
#define  OPD_RANGE               4   // memory range (lo - hi)
#define  OPD_IOPORT              5   // IOPort number (lo)
#define  OPD_VALUE               6   // a number that is not read from anywhere: &SYMBOL (lo)

typedef struct operand
{
    uint8_t    kind;
    uint8_t    deref;                // enclosed in [ ]
    uint8_t    reg;                  // register id (OPD_REG)
    uint32_t   lo;
    uint32_t   hi;
    char      *name;                 // the symbol the address came from, if any ("buf+2")
} operand_t;

////////////////////////////////////////////////////////////////////////////////////////
//
// a symbol resolved by the parser (symbols.c): its text as written, the address it stands
// for (offset included) and the number of words the symbol covers from there
//
typedef struct symv
{
    char      *text;
    uint32_t   addr;
    uint32_t   words;
} symv_t;

//
// kinds of 'expect'
//
#define  EXPECT_VALUE            0   // expect LOCATION OP VALUE
#define  EXPECT_CONSOLE          1   // expect console "TEXT" [rows N]

////////////////////////////////////////////////////////////////////////////////////////
//
// 'set' left-hand side kinds (cmd_t.set_kind)
//
#define  SETK_NONE               0
#define  SETK_REG                1
#define  SETK_MEM                2
#define  SETK_IOPORT             3
#define  SETK_OPTION             4   // a simulator setting (color, debug, ...)

////////////////////////////////////////////////////////////////////////////////////////
//
// 'set' option identifiers (cmd_t.set_opt)
//
#define  OPT_NONE                0
#define  OPT_COLOR               1
#define  OPT_DEREF               2
#define  OPT_ERRORCHK            3
#define  OPT_DEBUG               4
#define  OPT_PROFILE             5
#define  OPT_VERBOSE             6
#define  OPT_SOURCE              7
#define  OPT_MODE                8

#define  CMD_MAX_REPLACE         3

typedef struct cmd
{
    int        id;                   // enum cmd_id
    uint8_t    fmt;                  // FORMAT_* (print/display)
    operand_t  opd;                  // print/display operand
    char      *name;                 // label, help topic, page name, option name
    char      *path;                 // load: file path
    uint32_t   num;                  // address / index / gamepad number
    uint8_t    has_num;
    char      *word;                 // gamepad subcommand, set's word value

    // set
    uint8_t    set_kind;             // SETK_*
    uint8_t    set_opt;              // OPT_*
    uint8_t    set_reg;
    uint32_t   set_addr;             // memory address or IOPort number
    tok_t      set_val;              // numeric value (if set_isnum)
    uint8_t    set_isnum;
    uint8_t    set_neg;

    // replace IP/IR/IV
    int        rep_count;
    struct
    {
        uint8_t   which;             // IP, IR, or IV
        uint32_t  value;
    } rep[CMD_MAX_REPLACE];

    // watch, expect
    wloc_t     watch_target;
    wloc_t     watch_rhs;
    uint8_t    watch_op;             // WATCH_OP_*
    uint8_t    watch_signed;         // compare as signed numbers (watch/d, expect/d)

    // run / until / expect console / x
    uint32_t   count;                // run: frames; x: words
    uint8_t    has_count;
    uint32_t   timeout;              // until: frames (0 = default)
    uint32_t   rows;                 // until, expect console: only the last N rows (0 = all)
    char      *text;                 // until, expect console: the text
    uint8_t    expect_kind;          // EXPECT_*
    uint32_t   x_words;              // x: words a symbol covers (the default count)

    // parsing bookkeeping
    char     **pool;                 // every string allocated while parsing
    int        npool;
    int        cappool;
    char       errmsg[192];          // set when parsing/validation failed
    char       badword[64];          // unrecognized command word
} cmd_t;

////////////////////////////////////////////////////////////////////////////////////////
//
// command.c
//
cmd_t    *cmd_current  (void);                       // command being parsed
char     *cmd_strdup   (const char *, size_t);       // string owned by that command
void      cmd_seterr   (const char *, ...);          // record a parse/validation error
int       cmd_lookup   (const char *);               // command word -> enum cmd_id
int       cmd_parse    (const char *, cmd_t *);      // line -> cmd_t (0 == success)
void      cmd_free     (cmd_t *);
uint8_t   cmd_execute  (cmd_t *);                    // carry out a parsed command
uint8_t   process_command (const char *);            // parse + execute one line
void      yyerror      (const char *);

////////////////////////////////////////////////////////////////////////////////////////
//
// lexer.l / parser.y
//
int       yyparse      (void);
int       yylex        (void);
void      lex_begin    (const char *);               // start scanning a line
void      lex_end      (void);                       // release the scan buffer
const char *lex_lasttext (void);                     // most recent token's text
int       lex_lastcol  (void);                       // ...and its column (1-based)

#endif
