%{
/* Vircon32 Simulator - Debugger Command Parser */
#include "defines.h"
#include <stdlib.h>
#include <string.h>

/* Token type union */
%}

%union {
    uint8_t reg;
    uint32_t hexval;
    uint32_t intval;
    char *string;
    struct {
        uint32_t start;
        uint32_t end;
    } range;
}

%token <reg> T_REGISTER
%token T_REGISTERS
%token <hexval> T_HEX_VALUE
%token <intval> T_DECIMAL
%token <string> T_LABEL
%token <string> T_IOPORT_SYMBOL
%token <hexval> T_IOPORT
%token <range> T_MEMORY_RANGE

%token T_STEP T_CONTINUE T_QUIT T_HELP
%token T_BREAK T_DISPLAY T_SET T_REPLACE
%token T_WATCH T_UNWATCH T_WATCHLIST T_PROFILE
%token T_BACKTRACE T_NEXT T_IGNORE T_UNDO T_LABEL_CMD T_GAMEPAD

%token T_EQ T_NE T_LT T_GT T_LE T_GE
%token T_COLON T_COMMA T_LBRACKET T_RBRACKET
%token T_DASH T_UNDERSCORE T_SLASH
%token T_UNKNOWN
%token NEWLINE

%type <hexval> address hex_value
%type <reg> register
%type <intval> value decimal_value
%type <string> label
%type <range> memory_range

%left T_EQ T_NE T_LT T_GT T_LE T_GE
%left T_DASH

%%

input: /* empty */
    | input line
    ;

line: command NEWLINE
    | NEWLINE
    ;

command: simple_cmd
       | break_cmd
       | display_cmd
       | set_cmd
       | replace_cmd
       | watch_cmd
       | unwatch_cmd
       | watchlist_cmd
       | profile_cmd
       | backtrace_cmd
       | next_cmd
       | ignore_cmd
       | undo_cmd
       | label_cmd
       | gamepad_cmd
       ;

simple_cmd: T_STEP      { action = INPUT_STEP; }
           | T_CONTINUE  { action = INPUT_CONTINUE; }
           | T_QUIT      { action = INPUT_QUIT; }
           | T_HELP      { action = INPUT_HELP; }
           ;

break_cmd: T_BREAK      { action = INPUT_BREAK; }
         | T_BREAK address {
               action = INPUT_BREAK;
               cmd_data.breakpoint.addr = $2;
           }
         | T_BREAK label {
               action = INPUT_BREAK;
               cmd_data.breakpoint.label = $2;
           }
         ;

display_cmd: T_DISPLAY register { action = INPUT_DISPLAY; cmd_data.display.reg = $2; }
            | T_DISPLAY address { action = INPUT_DISPLAY; cmd_data.display.addr = $2; }
            | T_DISPLAY T_REGISTERS { action = INPUT_DISPLAY; cmd_data.display.all_regs = 1; }
            ;

set_cmd: T_SET label T_EQ value {
        action = INPUT_SET;
        cmd_data.set.option = $2;
        cmd_data.set.value = $4;
    }
    ;

replace_cmd: T_REPLACE replace_args {
        action = INPUT_REPLACE;
    }
    ;

replace_args: replace_arg
            | replace_args replace_arg
            ;

replace_arg: T_IP T_COLON hex_value { cmd_data.replace.ip = $3; }
    | T_IR T_COLON hex_value { cmd_data.replace.ir = $3; }
    | T_IV T_COLON hex_value { cmd_data.replace.iv = $3; }
    ;

watch_cmd: T_WATCH register comparison_op value {
        action = INPUT_WATCH;
        cmd_data.watch.reg = $2;
        cmd_data.watch.op = $3;
        cmd_data.watch.value = $4;
        cmd_data.watch.label = NULL;
    }
    | T_WATCH register comparison_op value label {
        action = INPUT_WATCH;
        cmd_data.watch.reg = $2;
        cmd_data.watch.op = $3;
        cmd_data.watch.value = $4;
        cmd_data.watch.label = $5;
    }
    ;

comparison_op: T_EQ  { $$ = WATCH_EQ; }
             | T_NE  { $$ = WATCH_NE; }
             | T_LT  { $$ = WATCH_LT; }
             | T_GT  { $$ = WATCH_GT; }
             | T_LE  { $$ = WATCH_LE; }
             | T_GE  { $$ = WATCH_GE; }
             ;

unwatch_cmd: T_UNWATCH decimal_value {
        action = INPUT_UNWATCH;
        cmd_data.unwatch.index = $2;
    }
    | T_UNWATCH label {
        action = INPUT_UNWATCH;
        cmd_data.unwatch.label = $2;
    }
    ;

watchlist_cmd: T_WATCHLIST { action = INPUT_WATCHLIST; };

profile_cmd: T_PROFILE { action = INPUT_PROFILE; };

backtrace_cmd: T_BACKTRACE { action = INPUT_BACKTRACE; };

next_cmd: T_NEXT { action = INPUT_NEXT; };

ignore_cmd: T_IGNORE { action = INPUT_IGNORE; };

undo_cmd: T_UNDO decimal_value { action = INPUT_UNDO; cmd_data.undo.index = $2; }
        | T_UNDO label { action = INPUT_UNDO; cmd_data.undo.label = $2; }
        ;

label_cmd: T_LABEL_CMD label { action = INPUT_LABEL; cmd_data.label.name = $2; };

gamepad_cmd: T_GAMEPAD { action = INPUT_GAMEPAD; };

address: hex_value
       | label
       ;

hex_value: T_HEX_VALUE { $$ = $1; };

register: T_REGISTER { $$ = $1; };

value: hex_value
     | decimal_value
     ;

decimal_value: T_DECIMAL { $$ = $1; };

label: T_LABEL ;

memory_range: T_MEMORY_RANGE
            | address T_DASH address
            ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Parse error: %s\n", s);
}

int yywrap(void) {
    return 1;
}
