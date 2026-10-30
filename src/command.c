#include "defines.h"
#include "command.h"
#include "assembly.h"
#include <stdarg.h>
#include <ctype.h>
#include <time.h>

////////////////////////////////////////////////////////////////////////////////////////
//
// command.c
//
// The "back half" of the command front end. The flex scanner (lexer.l) and bison
// grammar (parser.y) turn one line of text into a cmd_t; this file
//
//   * owns the parse context (string pool, error message, abbreviation table)
//   * executes parsed commands - every simulator side effect of a prompt command
//     or command-file line happens in cmd_execute() and its helpers
//
// Execution always leaves the global `action` set: INPUT_INIT for commands that do
// not move the CPU (the default), or INPUT_STEP / INPUT_NEXT / INPUT_CONTINUE /
// INPUT_IGNORE / INPUT_QUIT for the ones prompt() and load_command() must act on.
//
////////////////////////////////////////////////////////////////////////////////////////

static cmd_t  *g_cmd  = NULL;       // the command currently being parsed

////////////////////////////////////////////////////////////////////////////////////////
//
// command word table: full names, and the shortest accepted abbreviation of each
//
static const struct
{
    const char *name;
    int         minlen;
    int         id;
} cmdtab[] =
{
    { "asm",         2, CMD_ASM       },
    { "break",       1, CMD_BREAK     },
    { "backtrace",   2, CMD_BACKTRACE },
    { "bt",          2, CMD_BACKTRACE },
    { "console",     3, CMD_CONSOLE   },
    { "continue",    1, CMD_CONTINUE  },
    { "display",     1, CMD_DISPLAY   },
    { "gamepad",     1, CMD_GAMEPAD   },
    { "help",        1, CMD_HELP      },
    { "?",           1, CMD_HELP      },
    { "ignore",      1, CMD_IGNORE    },
    { "inventory",   2, CMD_INVENTORY },
    { "kbd",         2, CMD_KBD       },
    { "mailbox",     2, CMD_MAILBOX   },
    { "label",       1, CMD_LABEL     },
    { "load",        2, CMD_LOAD      },
    { "next",        1, CMD_NEXT      },
    { "print",       1, CMD_PRINT     },
    { "profile",     3, CMD_PROFILE   },
    { "quit",        1, CMD_QUIT      },
    { "replace",     1, CMD_REPLACE   },
    { "restore",     3, CMD_RESTORE   },
    { "save",        2, CMD_SAVE      },
    { "screenshot",  3, CMD_SCREENSHOT},
    { "step",        1, CMD_STEP      },
    { "set",         2, CMD_SET       },
    { "unbreak",     3, CMD_UNBREAK   },
    { "undisplay",   3, CMD_UNDISPLAY },
    { "unlabel",     3, CMD_UNLABEL   },
    { "unload",      4, CMD_UNLOAD    },
    { "unwatch",     3, CMD_UNWATCH   },
    { "version",     3, CMD_VERSION   },
    { "watch",       1, CMD_WATCH     },
    { "watchlist",   6, CMD_WATCHLIST },
    { "run",         2, CMD_RUN       },
    { "wait",        4, CMD_RUN       },
    { "until",       3, CMD_UNTIL     },
    { "expect",      2, CMD_EXPECT    },
    { "x",           1, CMD_EXAMINE   },
    { "sstep",       2, CMD_SSTEP     },
    { "snext",       2, CMD_SNEXT     },
    { "finish",      3, CMD_FINISH    },
    { "tbreak",      2, CMD_TBREAK    },
    { "list",        2, CMD_LIST      },
    { "frame",       2, CMD_FRAME     },
    { "up",          2, CMD_UP        },
    { "down",        3, CMD_DOWN      },
    { NULL,          0, CMD_NONE      }
};

////////////////////////////////////////////////////////////////////////////////////////
//
// cmd_lookup(): map a command word to its enum cmd_id. An exact name always wins;
// otherwise any prefix of a name that is at least as long as that name's minimum
// abbreviation matches. Returns CMD_NONE for an unrecognized word.
//
int  cmd_lookup (const char *word)
{
    char     lower[32];
    size_t   len     = 0;
    size_t   index   = 0;

    if (word == NULL)
    {
        return (CMD_NONE);
    }

    len = strlen (word);
    if ((len == 0) || (len >= sizeof (lower)))
    {
        return (CMD_NONE);
    }

    for (index = 0; index <= len; index = index + 1)
    {
        lower[index] = (char) tolower ((unsigned char) word[index]);
    }

    for (index = 0; cmdtab[index].name != NULL; index = index + 1)
    {
        if (0 == strcmp (cmdtab[index].name, lower))
        {
            return (cmdtab[index].id);
        }
    }

    for (index = 0; cmdtab[index].name != NULL; index = index + 1)
    {
        if (((int) len >= cmdtab[index].minlen) &&
            (0 == strncmp (cmdtab[index].name, lower, len)))
        {
            return (cmdtab[index].id);
        }
    }

    return (CMD_NONE);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// parse context
//
cmd_t  *cmd_current (void)
{
    return (g_cmd);
}

//
// cmd_strdup(): copy 'len' bytes of text into a string owned by the current command
// (released all at once by cmd_free(), so the grammar never has to free anything)
//
char  *cmd_strdup (const char *text, size_t len)
{
    char   *copy  = NULL;
    char  **grown = NULL;

    if (g_cmd == NULL)
    {
        return (NULL);
    }

    copy = (char *) ralloc (sizeof (char), len + 1, FLAG_NONE);
    memcpy (copy, text, len);
    copy[len] = '\0';

    if (g_cmd -> npool >= g_cmd -> cappool)
    {
        g_cmd -> cappool = (g_cmd -> cappool == 0) ? 16 : (g_cmd -> cappool * 2);
        grown            = (char **) realloc (g_cmd -> pool,
                                              sizeof (char *) * g_cmd -> cappool);
        if (grown == NULL)
        {
            fprintf (stderr, "[ERROR] out of memory while parsing a command\n");
            exit    (STRING_ALLOC_FAIL);
        }
        g_cmd -> pool    = grown;
    }
    g_cmd -> pool[g_cmd -> npool] = copy;
    g_cmd -> npool                = g_cmd -> npool + 1;

    return (copy);
}

//
// cmd_seterr(): record why a command was rejected. The first error wins, so the
// precise message from the scanner or a grammar action is not overwritten by the
// generic "syntax error" that bison reports afterwards.
//
void  cmd_seterr (const char *format, ...)
{
    va_list  args;

    if ((g_cmd == NULL) || (g_cmd -> errmsg[0] != '\0'))
    {
        return;
    }

    va_start (args, format);
    vsnprintf (g_cmd -> errmsg, sizeof (g_cmd -> errmsg), format, args);
    va_end   (args);
}

//
// yyerror(): bison's syntax error callback
//
void  yyerror (const char *message)
{
    const char *text  = lex_lasttext ();

    if (text[0] == '\0')
    {
        cmd_seterr ("syntax error: incomplete command (try 'help')");
    }
    else
    {
        cmd_seterr ("syntax error near '%s' (column %d); try 'help'", text, lex_lastcol ());
    }
}

void  cmd_free (cmd_t *cmd)
{
    int  index  = 0;

    for (index = 0; index < cmd -> npool; index = index + 1)
    {
        rfree (cmd -> pool[index]);
    }
    free (cmd -> pool);
    cmd -> pool     = NULL;
    cmd -> npool    = 0;
    cmd -> cappool  = 0;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// cmd_parse(): run the scanner and parser over one line, filling in `cmd`. Returns
// 0 on success; on failure the reason has been printed and a nonzero value returned.
//
int  cmd_parse (const char *line, cmd_t *cmd)
{
    int  result  = 0;

    memset (cmd, 0, sizeof (cmd_t));
    cmd -> id   = CMD_NONE;
    cmd -> fmt  = FORMAT_DEFAULT;

    g_cmd       = cmd;
    lex_begin (line);
    result      = yyparse ();
    lex_end   ();
    g_cmd       = NULL;

    if (result != 0)
    {
        fprintf (stderr, "[ERROR] %s\n",
                         (cmd -> errmsg[0] != '\0') ? cmd -> errmsg : "malformed command");
        script_errors  = script_errors + 1;
    }

    return (result);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// execution helpers
//
////////////////////////////////////////////////////////////////////////////////////////

static int8_t *copy_string (const char *text)
{
    int8_t  *copy  = (int8_t *) ralloc (sizeof (int8_t), strlen (text) + 1, FLAG_NONE);

    strcpy (copy, text);
    return (copy);
}

//
// valid_address(): is `address` inside a mapped memory page? (guards the page index
// before anything indexes the `memory` array with it)
//
static int  valid_address (uint32_t address, uint8_t mode)
{
    if (((address & V32_PAGE_MASK) >> 28) >= NUM_MEMORY_PAGES)
    {
        return (FALSE);
    }
    return (memory_chk (address, mode, TRUE) == TRUE);
}

//
// parse_bool(): true/false/on/off/yes/no/1/0; returns -1 if not a boolean
//
static int  parse_bool (const char *text)
{
    if (0 == strcasecmp (text, "true")  || 0 == strcasecmp (text, "on") ||
        0 == strcasecmp (text, "yes")   || 0 == strcasecmp (text, "1"))
    {
        return (TRUE);
    }
    if (0 == strcasecmp (text, "false") || 0 == strcasecmp (text, "off") ||
        0 == strcasecmp (text, "no")    || 0 == strcasecmp (text, "0"))
    {
        return (FALSE);
    }
    return (-1);
}

//
// page_from_name(): bios/cart/memc (any prefix of at least 4 letters, or the long
// names memcard etc.) -> V32_PAGE_*, or -1
//
static int  page_from_name (const char *name)
{
    if (0 == strncasecmp (name, "bios", 4))  return (V32_PAGE_BIOS);
    if (0 == strncasecmp (name, "cart", 4))  return (V32_PAGE_CART);
    if (0 == strncasecmp (name, "memc", 4))  return (V32_PAGE_MEMC);
    return (-1);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// show_profile(): the `profile` report
//
static void  show_profile (void)
{
    linked_l  *tmp    = NULL;
    int32_t    value  = 0;

    if (profileflag != TRUE)
    {
        fprintf (stdout, "profiling not enabled\n");
        return;
    }

    fprintf (stdout, "profiling report\n");
    fprintf (stdout, "================\n");
    fprintf (stdout, "total number of instructions: %u\n", profcount);
    fprintf (stdout, "per-instruction breakdown:");
    for (value = 0; value < 64; value = value + 1)
    {
        if ((value % 4) == 0)
        {
            fprintf (stdout, "\n");
        }
        fprintf (stdout, "    %5s: %8u", lookup[value].name, profcountopcode[value]);
    }
    fprintf (stdout, "\n----------------------------------------------------------------------------\n");
    fprintf (stdout, "total number of subroutine calls: %u\n", profcountsub);
    fprintf (stdout, "per-subroutine breakdown:\n\n");
    fprintf (stdout, "%-37s %6s %6s  %6s %6s %8s\n", "subroutine", "active", "calls", "cycles", "frames", "time(s)");
    fprintf (stdout, "===================================== ====== ====== ======= ====== ========\n");
    tmp = ppoint;
    while (tmp != NULL)
    {
        tmp -> CYCLES     = tmp -> CYCLES + tmp -> COUNT;
        tmp -> FRAMES     = tmp -> CYCLES / V32_CYCLES_PER_FRAME;
        tmp -> time       = (float) tmp -> CYCLES / (float) (V32_FRAMES_PER_SECOND * V32_CYCLES_PER_FRAME);
        tmp -> CYCLES     = tmp -> CYCLES % V32_CYCLES_PER_FRAME;

        switch ((tmp -> RAW & V32_PAGE_MASK) >> 28)
        {
            case V32_PAGE_RAM:   fprintf (stdout, " RAM:"); break;
            case V32_PAGE_BIOS:  fprintf (stdout, "BIOS:"); break;
            case V32_PAGE_CART:  fprintf (stdout, "CART:"); break;
            case V32_PAGE_MEMC:  fprintf (stdout, "MEMC:"); break;
        }

        if (tmp -> label != NULL)
        {
            fprintf (stdout, "%-32s ",     tmp -> label);
        }
        else
        {
            fprintf (stdout, "0x%-30.8X ", tmp -> RAW);
        }

        if (tmp -> time != -1.0) // completed subroutine
        {
            fprintf (stdout, "%6s ", (tmp -> ACTIVE == TRUE) ? "*" : " ");
            fprintf (stdout, "%6u %7u %6u %8.4f\n", tmp -> SUBCALL, tmp -> COUNT, tmp -> FRAMES, tmp -> time);
        }
        else // an uncompleted subroutine (such as the one we are currently in)
        {
            fprintf (stdout, "%6u %7u %6u %8s\n", tmp -> SUBCALL, tmp -> COUNT, tmp -> FRAMES, "active");
        }

        tmp = tmp -> next;
    }
    fprintf (stdout, "----------------------------------------------------------------------------\n");
}

////////////////////////////////////////////////////////////////////////////////////////
//
// break / backtrace / label / unbreak ...
//
//
// add_break(): put a breakpoint at an address (once: a second one for the same address is not added)
//
static linked_l  *add_break (uint32_t addr, uint8_t temp)
{
    linked_l  *tmp  = NULL;

    for (tmp = bpoint; tmp != NULL; tmp = tmp -> next)
    {
        if (tmp -> RAW == addr)
        {
            if (temp == FALSE)
            {
                tmp -> space  = 7;              // a plain break turns a one-shot into a permanent one
            }
            return (tmp);
        }
    }
    tmp = listnode (LIST_MEM, addr);
    if (temp == TRUE)
    {
        tmp -> space  = BP_TEMP;
    }
    bpoint = list_add (bpoint, tmp);
    return (tmp);
}

static void  exec_break (cmd_t *c)
{
    linked_l  *tmp    = NULL;
    linked_l  *ltmp   = NULL;
    int32_t    count  = 0;
    uint8_t    temp   = (c -> id == CMD_TBREAK) ? TRUE : FALSE;
    uint32_t   addrs[64];
    uint32_t   actual = 0;
    uint32_t   faddr  = 0;
    int        n      = 0;
    int        k      = 0;
    char       where[600];

    // 'break 25' is a line when there is line information and the number is a decimal one
    // (an address is written in hex); 'break FILE:LINE' always is
    if ((c -> has_num == TRUE) && (c -> num_dec == TRUE) && (c -> num < BIOS_FIRST_ADDR) &&
        (srcdebug_have () == TRUE))
    {
        c -> line      = c -> num;
        c -> has_line  = TRUE;
        c -> has_num   = FALSE;
    }

    if (c -> has_line == TRUE)
    {
        n = srcdebug_resolve (c -> file, c -> line, c -> all, addrs, 64, &actual);
        if (n <= 0)
        {
            if (srcdebug_have () == FALSE)
            {
                fprintf (stderr, "[ERROR] break: no source line information is loaded "
                                 "(assemble with v32as -g, or give the debug file)\n");
            }
            else
            {
                fprintf (stderr, "[ERROR] break: no code at or after %s%s%u\n",
                                 (c -> file != NULL) ? c -> file : "", (c -> file != NULL) ? ":" : "line ", c -> line);
            }
            return;
        }
        if (actual != c -> line)
        {
            status_printf ("[%s] line %u has no code; using line %u\n", temp ? "tbreak" : "break", c -> line, actual);
        }
        for (k = 0; k < n; k = k + 1)
        {
            add_break (addrs[k], temp);
            srcdebug_describe (addrs[k], where, sizeof (where), TRUE);
            status_printf ("[%s] 0x%.8X (%s)\n", temp ? "tbreak" : "break", addrs[k], where);
        }
        return;
    }
    if (c -> has_num == TRUE)
    {
        fprintf (debug, "[break] adding the offset 0x%.8X\n", c -> num);
        add_break (c -> num, temp);
    }
    else if (c -> name != NULL)
    {
        if (srcdebug_function (c -> name, &faddr) == TRUE)       // a function: its first instruction
        {
            fprintf (debug, "[break] adding the function '%s' (0x%.8X)\n", c -> name, faddr);
            add_break (faddr, temp);
            return;
        }
        for (ltmp = lpoint; ltmp != NULL; ltmp = ltmp -> next)
        {
            if ((ltmp -> label != NULL) &&
                (0 == strcasecmp (ltmp -> label, c -> name)))
            {
                fprintf (debug, "[break] adding the label '%s' (0x%.8X)\n", ltmp -> label, ltmp -> I32);
                add_break (ltmp -> I32, temp);
                count  = count + 1;
            }
        }
        if (count == 0)
        {
            fprintf (stderr, "[ERROR] break: no label named '%s'\n", c -> name);
        }
    }
    else // list the breakpoints
    {
        count = 0;
        for (tmp = bpoint; tmp != NULL; tmp = tmp -> next)
        {
            srcdebug_describe (tmp -> RAW, where, sizeof (where), FALSE);
            fprintf (stdout, "[%u] 0x%.8X%s%s%s\n", count, tmp -> RAW,
                             (where[0] != '\0') ? " (" : "", where, (where[0] != '\0') ? ")" : "");
            if (tmp -> space == BP_TEMP)
            {
                fprintf (stdout, "      (one-shot)\n");
            }
            count = count + 1;
        }
    }
}

static void  exec_backtrace (void)
{
    linked_l  *tmp    = tpoint;
    int32_t    count  = 0;

    if (srcdebug_backtrace () == TRUE)              // names and lines, from the BP frames
    {
        return;
    }

    while (tmp != NULL)
    {
        if (tmp -> label != NULL)
        {
            fprintf (stdout, "#%u  %s\n",     count, tmp -> label);
        }
        else
        {
            fprintf (stdout, "#%u  0x%.8X\n", count, tmp -> RAW);
        }
        tmp   = tmp -> next;
        count = count + 1;
    }
}

static void  exec_label (cmd_t *c)
{
    linked_l  *ltmp   = NULL;
    int32_t    count  = 0;

    if (c -> has_num == TRUE)
    {
        ltmp          = listnode (LIST_MEM, c -> num);
        ltmp -> label = copy_string (c -> name);
        lpoint        = list_add (lpoint, ltmp);
        return;
    }

    for (ltmp = lpoint; ltmp != NULL; ltmp = ltmp -> next)
    {
        if ((ltmp -> label != NULL) &&
            (modeflag      == FLAG_ASM))
        {
            fprintf (stdout, "[%u] %s -> 0x%.8X\n",
                             count, ltmp -> label, ltmp -> RAW);
        }
        else if ((ltmp -> cname != NULL) &&
                 (modeflag      != FLAG_ASM))
        {
            fprintf (stdout, "[%u] ASM:%u -> C:%s:%u -> offset:0x%.8X\n",
                             count, ltmp -> NUMBER, ltmp -> cname, ltmp -> LINE, ltmp -> RAW);
        }
        count = count + 1;
    }
}

//
// unbreak / undisplay / unlabel: remove list entry number `index`
//
static void  exec_undo (cmd_t *c)
{
    linked_l **list   = NULL;
    linked_l  *tmp    = NULL;
    const char *what  = "";
    uint32_t   count  = 0;

    switch (c -> id)
    {
        case CMD_UNBREAK:    list = &bpoint;  what = "breakpoint"; break;
        case CMD_UNDISPLAY:  list = &dpoint;  what = "display";    break;
        case CMD_UNLABEL:    list = &lpoint;  what = "label";      break;
    }

    for (tmp = *list; tmp != NULL; tmp = tmp -> next)
    {
        if (count == c -> num)
        {
            break;
        }
        count = count + 1;
    }

    if (tmp == NULL)
    {
        fprintf (stderr, "[ERROR] no %s number %u\n", what, c -> num);
        return;
    }

    tmp = list_grab (list, tmp);
    if (c -> id == CMD_UNLABEL)
    {
        srcdebug_invalidate ();        // line entries may have been removed
    }
    if (tmp != NULL)
    {
        if (tmp -> label != NULL)
        {
            rfree (tmp -> label);
        }
        free (tmp);
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// print / display
//
static void  check_range (operand_t *o, int *ok)
{
    *ok = TRUE;
    if ((o -> lo > o -> hi) || ((o -> hi - o -> lo) > 16))
    {
        fprintf (stderr, "[ERROR] memory range must be ascending and cover at most 17 words\n");
        *ok = FALSE;
    }
}

static void  exec_print (cmd_t *c)
{
    operand_t  *o      = &(c -> opd);
    data_t     *dtmp   = NULL;
    uint32_t    index  = 0;
    int         ok     = TRUE;

    switch (o -> kind)
    {
        case OPD_RANGE:
            check_range (o, &ok);
            for (index = o -> lo; (ok == TRUE) && (index <= o -> hi); index = index + 1)
            {
                output_mem (index, c -> fmt, o -> deref, NULL);
            }
            break;

        case OPD_MEM:
            if ((o -> name != NULL) && (o -> deref == FALSE))
            {
                output_mem_named (o -> name, o -> lo, c -> fmt, FALSE, NULL);
            }
            else
            {
                output_mem (o -> lo, c -> fmt, o -> deref, NULL);
            }
            break;

        case OPD_VALUE:
            output_val (o -> name, o -> lo, c -> fmt);
            break;

        case OPD_REGS:
            for (index = 0; index <= 15; index = index + 1)
            {
                output_reg (index, c -> fmt, o -> deref, NULL);
            }
            break;

        case OPD_REG:
            output_reg (o -> reg, c -> fmt, o -> deref, NULL);
            break;

        case OPD_IOPORT:
            dtmp = ioports_ptr ((uint16_t) o -> lo);
            if (dtmp != NULL)
            {
                output_iop (o -> lo,
                            (c -> fmt != FORMAT_DEFAULT) ? c -> fmt : dtmp -> fmt,
                            dtmp -> name);
            }
            else
            {
                output_iop (o -> lo, FORMAT_DEFAULT, NULL);
            }
            break;
    }
}

//
// add_display(): append a displaylist node
//
static void  add_display (uint8_t type, uint32_t value, uint8_t fmt, const char *label)
{
    linked_l  *tmp  = listnode (type, value);

    if (label != NULL)
    {
        tmp -> label = copy_string (label);
    }
    tmp -> FMT = fmt;
    dpoint     = list_add (dpoint, tmp);
}

static void  exec_display (cmd_t *c)
{
    operand_t  *o      = &(c -> opd);
    data_t     *dtmp   = NULL;
    uint32_t    index  = 0;
    int         ok     = TRUE;

    switch (o -> kind)
    {
        case OPD_NONE:  // bare 'display': the list is shown at every prompt anyway (--batch: show it now)
            if (batchflag == TRUE)
            {
                displayshow (dpoint, 0);
            }
            break;

        case OPD_RANGE:
            check_range (o, &ok);
            for (index = o -> lo; (ok == TRUE) && (index <= o -> hi); index = index + 1)
            {
                add_display ((o -> deref == TRUE) ? LIST_MEM_DEREF : LIST_MEM,
                             index, c -> fmt, c -> name);
            }
            break;

        case OPD_MEM:
            add_display ((o -> deref == TRUE) ? LIST_MEM_DEREF : LIST_MEM,
                         o -> lo, c -> fmt, (c -> name != NULL) ? c -> name : o -> name);
            break;

        case OPD_VALUE:
            fprintf (stderr, "[ERROR] display: &%s is a constant, there is nothing to watch change (use print)\n", o -> name);
            break;

        case OPD_REGS:
            for (index = 0; index <= 15; index = index + 1)
            {
                add_display (LIST_REG, index, c -> fmt, NULL);
            }
            break;

        case OPD_REG:
            if (o -> reg > 15) // system registers have their own panel
            {
                sys_reg_show = TRUE;
            }
            else
            {
                add_display ((o -> deref == TRUE) ? LIST_REG_DEREF : LIST_REG,
                             o -> reg, c -> fmt, c -> name);
            }
            break;

        case OPD_IOPORT:
            dtmp = ioports_ptr ((uint16_t) o -> lo);
            if (dtmp == NULL)
            {
                fprintf (stderr, "[ERROR] display: 0x%.3X is not a valid IOPort\n", o -> lo);
                break;
            }
            add_display (LIST_IOP, o -> lo,
                         (c -> fmt != FORMAT_DEFAULT) ? c -> fmt : dtmp -> fmt,
                         (c -> name != NULL) ? c -> name : (const char *) dtmp -> name);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// gamepad
//
#define  GP_SELECT      -2
#define  GP_CONNECT     -3
#define  GP_DISCONNECT  -4
#define  GP_UNKNOWN     -1

static int  gp_prefix (const char *word, const char *name, size_t minlen)
{
    size_t  len  = strlen (word);

    return ((len >= minlen) && (len <= strlen (name)) && (0 == strncasecmp (word, name, len)));
}

static int  gp_resolve (const char *word)
{
    if (strlen (word) == 1)
    {
        switch (tolower ((unsigned char) word[0]))
        {
            case 's':  return (INP_GamepadButtonStart);
            case 'a':  return (INP_GamepadButtonA);
            case 'b':  return (INP_GamepadButtonB);
            case 'x':  return (INP_GamepadButtonX);
            case 'y':  return (INP_GamepadButtonY);
            case 'l':  return (INP_GamepadButtonL);
            case 'r':  return (INP_GamepadButtonR);
            case 'c':  return (GP_CONNECT);
            case 'u':  return (INP_GamepadUp);
            case 'd':  return (INP_GamepadDown);
        }
        return (GP_UNKNOWN);
    }

    if (gp_prefix (word, "select",     2))  return (GP_SELECT);
    if (gp_prefix (word, "start",      2))  return (INP_GamepadButtonStart);
    if (gp_prefix (word, "connect",    2))  return (GP_CONNECT);
    if (gp_prefix (word, "disconnect", 2))  return (GP_DISCONNECT);
    if (gp_prefix (word, "left",       2))  return (INP_GamepadLeft);
    if (gp_prefix (word, "right",      2))  return (INP_GamepadRight);
    if (gp_prefix (word, "up",         2))  return (INP_GamepadUp);
    if (gp_prefix (word, "down",       2))  return (INP_GamepadDown);
    return (GP_UNKNOWN);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// kbd: the v32kbd keyboard (see v32io.h)
//
static void  exec_kbd (cmd_t *c)
{
    char   line[2048];
    char  *verb  = NULL;
    char  *rest  = NULL;
    char  *p     = NULL;
    char  *mode  = NULL;
    int    n     = 0;

    if (c -> path == NULL)
    {
        v32io_show ();
        return;
    }
    snprintf (line, sizeof (line), "%s", c -> path);
    verb  = line;
    p     = line;
    while ((*p != '\0') && (*p != ' ') && (*p != '\t'))  p = p + 1;
    if (*p != '\0')
    {
        *p  = '\0';
        p   = p + 1;
        while ((*p == ' ') || (*p == '\t'))  p = p + 1;
    }
    rest  = p;

    if (0 == strcasecmp (verb, "attach"))
    {
        n  = (*rest != '\0') ? atoi (rest) : 0;
        v32io_kbd_attach (n);
    }
    else if (0 == strcasecmp (verb, "detach"))
    {
        v32io_kbd_detach ();
    }
    else if (0 == strcasecmp (verb, "type"))
    {
        v32io_kbd_type (rest);
    }
    else if (0 == strcasecmp (verb, "key"))
    {
        p = rest;
        while ((*p != '\0') && (*p != ' ') && (*p != '\t'))  p = p + 1;
        if (*p != '\0')
        {
            *p    = '\0';
            mode  = p + 1;
            while ((*mode == ' ') || (*mode == '\t'))  mode = mode + 1;
        }
        v32io_kbd_key (rest, mode);
    }
    else if (0 == strcasecmp (verb, "input"))
    {
        v32io_kbd_source ((*rest != '\0') ? rest : "off");
    }
    else if (0 == strcasecmp (verb, "clear"))
    {
        v32io_kbd_clear ();
    }
    else if (0 == strcasecmp (verb, "status"))
    {
        v32io_show ();
    }
    else
    {
        fprintf (stderr, "[ERROR] kbd: unknown subcommand '%s' (try 'help kbd')\n", verb);
    }
}

//
// console [FILE]               the text screen, on stdout or in FILE
// console --attrs[=WHAT] [FILE]  the screen followed by grids of its colors (WHAT: fg, bg or all; fg by default)
// console color|cursor|status ...
//
static void  exec_console_dump (const char *file, int attrs)
{
    FILE  *out  = stdout;
    int    rows = 0;

    if (file != NULL)
    {
        out = fopen (file, "w");
        if (out == NULL)
        {
            fprintf (stderr, "[ERROR] console: could not write '%s'\n", file);
            return;
        }
    }
    rows = gpu_console_show (out);
    if (attrs != 0)
    {
        gpu_console_attrs (out, attrs);
    }
    if (file != NULL)
    {
        fclose (out);
        status_printf ("[console] wrote %s (%d rows)\n", file, rows);
    }
}

static void  exec_console (cmd_t *c)
{
    char      line[256];
    char     *verb = NULL, *arg = NULL, *p = NULL;
    uint8_t   codes[8];
    int       n = 0;

    if (c -> path == NULL)
    {
        exec_console_dump (NULL, 0);
        return;
    }
    snprintf (line, sizeof (line), "%s", c -> path);
    verb = line;
    for (p = line; (*p != '\0') && (*p != ' ') && (*p != '\t'); p = p + 1) ;
    if (*p != '\0')
    {
        *p = '\0';
        for (p = p + 1; (*p == ' ') || (*p == '\t'); p = p + 1) ;
    }
    arg = p;

    if (0 == strcmp (verb, "color") || 0 == strcmp (verb, "colour"))
    {
        if (gpu_console_set_color (arg) != 0)
        {
            fprintf (stderr, "[ERROR] console color: use none, 16, 256, true or auto\n");
        }
    }
    else if (0 == strcmp (verb, "cursor"))
    {
        // console cursor COLOR [CODE ...]: recolor the solid block (and the listed codes)
        char *col = arg, *rest = arg;

        for (; (*rest != '\0') && (*rest != ' '); rest = rest + 1) ;
        if (*rest != '\0')  { *rest = '\0';  rest = rest + 1; }
        if (gpu_console_set_cursor (col) != 0)
        {
            fprintf (stderr, "[ERROR] console cursor: a color name, #RRGGBB, or off\n");
            return;
        }
        codes[0] = 20;  n = 1;
        while ((*rest != '\0') && (n < 8))
        {
            codes[n++] = (uint8_t) strtoul (rest, &p, 0);
            if (p == rest)  break;
            rest = p;
            while (*rest == ' ')  rest = rest + 1;
        }
        gpu_console_set_cursor_codes (codes, n);
    }
    else if (0 == strcmp (verb, "status"))
    {
        gpu_console_status (stdout);
    }
    else if (0 == strncmp (verb, "--attrs", 7))
    {
        int  what  = 1;

        if (verb[7] == '=')
        {
            if      (0 == strcmp (verb + 8, "fg"))   what = 1;
            else if (0 == strcmp (verb + 8, "bg"))   what = 2;
            else if (0 == strcmp (verb + 8, "all"))  what = 3;
            else
            {
                fprintf (stderr, "[ERROR] console --attrs: use fg, bg or all\n");
                return;
            }
        }
        else if (verb[7] != '\0')
        {
            fprintf (stderr, "[ERROR] console: unknown option '%s' (try 'help console')\n", verb);
            return;
        }
        exec_console_dump ((*arg != '\0') ? c -> path + (arg - line) : NULL, what);
    }
    else if (verb[0] == '-')
    {
        fprintf (stderr, "[ERROR] console: unknown option '%s' (try 'help console')\n", verb);
    }
    else
    {
        exec_console_dump (c -> path, 0);      // anything else is a file name
    }
}

static void  exec_mailbox (cmd_t *c)
{
    if ((c -> has_num == TRUE) && (c -> num >= DEV_CHANNELS))
    {
        fprintf (stderr, "[ERROR] mailbox: no channel %u (valid: 0-%d)\n", c -> num, DEV_CHANNELS - 1);
        return;
    }
    memc_show ((c -> has_num == TRUE) ? (int) c -> num : -1);
}

static void  exec_gamepad (cmd_t *c)
{
    int  which  = GP_UNKNOWN;

    if (c -> word != NULL)
    {
        which = gp_resolve (c -> word);
        if (which == GP_UNKNOWN)
        {
            fprintf (stderr, "[ERROR] gamepad: unknown command '%s'\n", c -> word);
            return;
        }
    }

    if (c -> has_num == TRUE)
    {
        if (c -> num > 3)
        {
            fprintf (stderr, "[ERROR] gamepad: no gamepad %u (valid: 0-3)\n", c -> num);
            return;
        }
        SYSPORTSET(INP_SelectedGamepad, c -> num);
    }
    else if (which == GP_SELECT)
    {
        fprintf (stderr, "[ERROR] gamepad: 'select' needs a gamepad number (0-3)\n");
        return;
    }

    fprintf (verbose, "[gamepad%d] %s\n", IPORTGET(INP_SelectedGamepad),
                      (c -> word != NULL) ? c -> word : "(display)");

    if (c -> word == NULL)
    {
        display_config (INPUT_GAMEPAD);
    }
    else if (which == GP_CONNECT)
    {
        SYSPORTSET(INP_GamepadConnected, 1);
    }
    else if (which == GP_DISCONNECT)
    {
        SYSPORTSET(INP_GamepadConnected, 0);
    }
    else if (which != GP_SELECT)
    {
        gamepad_io ((uint16_t) which);
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// set
//
////////////////////////////////////////////////////////////////////////////////////////
//
// source-level debugging: sstep snext finish until list frame up down
//
static void  exec_srcstep (cmd_t *c)
{
    int  armed  = FALSE;

    switch (c -> id)
    {
        case CMD_SSTEP:     armed = srcdebug_step (FALSE);                                  break;
        case CMD_SNEXT:     armed = srcdebug_step (TRUE);                                   break;
        case CMD_FINISH:    armed = srcdebug_finish ();                                     break;
        case CMD_UNTILSRC:  armed = srcdebug_until (c -> file, c -> line, c -> has_line);   break;
    }
    if (armed == TRUE)
    {
        action  = INPUT_SRCSTEP;
    }
}

static void  exec_list (cmd_t *c)
{
    srcdebug_list (c -> file, c -> line, c -> has_line, c -> name);
}

static void  exec_frame (cmd_t *c)
{
    srcdebug_frame ((c -> id == CMD_UP) ? 1 : (c -> id == CMD_DOWN) ? -1 : 0, c -> num, c -> has_num);
}

static void  exec_set_list (void)
{
    fprintf (stdout, "v32sim settings\n");
    fprintf (stdout, "===============\n");
    fprintf (stdout, "color:   %s\n", (colorflag   == TRUE)   ? "TRUE" : "FALSE");
    fprintf (stdout, "deref:   %s\n", (derefaddr   == TRUE)   ? "TRUE" : "FALSE");
    fprintf (stdout, "error:   %s\n", (errorcheck  == TRUE)   ? "TRUE" : "FALSE");
    fprintf (stdout, "debug:   %s\n", (debug       == stderr) ? "TRUE" : "FALSE");
    fprintf (stdout, "profile: %s\n", (profileflag == TRUE)   ? "TRUE" : "FALSE");
    fprintf (stdout, "verbose: %s\n", (verbose     == stderr) ? "TRUE" : "FALSE");
    fprintf (stdout, "source:  %s\n", (sourceflag  == TRUE)   ? "TRUE" : "FALSE");
    fprintf (stdout, "mode:    %s\n", (modeflag == FLAG_C) ? "C" : (modeflag == FLAG_LUA) ? "lua" :
                                      (modeflag == FLAG_SRC) ? "src" : "asm");
}

static void  exec_set_option (cmd_t *c)
{
    int  state  = -1;

    if (c -> set_opt == OPT_MODE)
    {
        if      ((c -> word != NULL) && (0 == strcasecmp (c -> word, "asm")))  modeflag = FLAG_ASM;
        else if ((c -> word != NULL) && (0 == strcasecmp (c -> word, "c")))    modeflag = FLAG_C;
        else if ((c -> word != NULL) && (0 == strcasecmp (c -> word, "lua")))  modeflag = FLAG_LUA;
        else if ((c -> word != NULL) && (0 == strcasecmp (c -> word, "src")))  modeflag = FLAG_SRC;
        else
        {
            fprintf (stderr, "[ERROR] set mode: value must be asm, src, C, or lua\n");
        }
        return;
    }

    if (c -> set_isnum == TRUE)
    {
        state = (c -> set_val.u != 0) ? TRUE : FALSE;
    }
    else if (c -> word != NULL)
    {
        state = parse_bool (c -> word);
    }

    if (state < 0)
    {
        fprintf (stderr, "[ERROR] set %s: value must be true/false (or on/off, yes/no, 1/0)\n", c -> name);
        return;
    }

    switch (c -> set_opt)
    {
        case OPT_COLOR:
            colorflag   = state;
            break;

        case OPT_DEREF:
            derefaddr   = state;
            fprintf (debug, "[set] derefaddr %s!\n", state ? "ENABLED" : "DISABLED");
            break;

        case OPT_ERRORCHK:
            errorcheck  = state;
            fprintf (verbose, "errorchk %s!\n", state ? "ENABLED" : "DISABLED");
            break;

        case OPT_DEBUG:
            if (state == TRUE)
            {
                debug   = stderr;
                fprintf (debug, "[set] debugging ENABLED!\n");
            }
            else
            {
                fprintf (debug, "[set] disabling debugging!\n");
                debug   = devnull;
            }
            break;

        case OPT_SOURCE:
            sourceflag  = state;
            break;

        case OPT_PROFILE:
            profileflag = state;
            fprintf (verbose, "instruction profiling %s!\n", state ? "ENABLED" : "DISABLED");
            break;

        case OPT_VERBOSE:
            if (state == TRUE)
            {
                verbose = stderr;
                fprintf (verbose, "verbosity ENABLED!\n");
            }
            else
            {
                fprintf (verbose, "disabling verbosity!\n");
                verbose = devnull;
            }
            break;
    }
}

static void  exec_set (cmd_t *c)
{
    word_t  value;

    if (c -> set_kind == SETK_NONE)
    {
        exec_set_list ();
        return;
    }

    if (c -> set_kind == SETK_OPTION)
    {
        exec_set_option (c);
        return;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // everything else needs a numeric right-hand side
    //
    if (c -> set_isnum != TRUE)
    {
        fprintf (stderr, "[ERROR] set: a numeric value is required (0x.., 0b.., 0.., decimal, or float)\n");
        return;
    }

    value.raw = c -> set_val.u;
    if (c -> set_neg == TRUE)
    {
        if (c -> set_val.kind == TOK_FLT)
        {
            value.f32 = -(c -> set_val.f);
        }
        else
        {
            value.raw = (uint32_t) (0 - c -> set_val.u);
        }
    }

    switch (c -> set_kind)
    {
        case SETK_REG:
            REG(c -> set_reg) = value.i32;   // raw bits: also right for floats
            fprintf (debug, "[%s] setting to 0x%.8X\n", REGNAME(c -> set_reg), REG(c -> set_reg));

            ////////////////////////////////////////////////////////////////////////////
            //
            // Setting IP is a jump: move the simulator's own program counter as well,
            // and refresh IR and (if needed) IV to match the new location
            //
            if (c -> set_reg == IP)
            {
                rom_offset  = value.raw;
                REG(IR)     = ISYSMEMGET(REG(IP));
                REG(IV)     = (0 < (REG(IR) & IMMVAL_MASK)) ? ISYSMEMGET(REG(IP) + 1) : 0;
            }
            break;

        case SETK_MEM:
            if (valid_address (c -> set_addr, FLAG_WRITE) == FALSE)
            {
                fprintf (stderr, "[ERROR] set: 0x%.8X is not a writable memory address\n", c -> set_addr);
                break;
            }
            SYSMEMSET(c -> set_addr, value.raw);
            break;

        case SETK_IOPORT:
            if (ioports_chk ((uint16_t) c -> set_addr, FLAG_WRITE, TRUE) == FALSE)
            {
                fprintf (stderr, "[ERROR] set: 0x%.3X is not a valid IOPort\n", c -> set_addr);
                break;
            }
            fprintf (debug, "[set] Setting IOPort 0x%.3X to 0x%.8X\n", c -> set_addr, value.raw);
            ioports_set ((uint16_t) c -> set_addr, value.i32, value.f32, TRUE);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// replace IP:.. IR:.. IV:..
//
static void  asm_refresh_ir (void);

static void  exec_replace (cmd_t *c)
{
    int32_t   index   = 0;
    int32_t   offset  = REG(IP);
    uint32_t  word    = 0xFFFFFFFF;

    fprintf (debug, "[replace] replacing ");
    for (index = 0; index < c -> rep_count; index = index + 1)
    {
        switch (c -> rep[index].which)
        {
            case IP:
                offset = c -> rep[index].value;
                fprintf (debug, "IP:0x%.8X ", offset);
                break;

            case IR:
                fprintf (debug, "IR:0x%.8X ", ISYSMEMGET(offset));
                word   = c -> rep[index].value;
                fprintf (debug, "with IR:0x%.8X ", word);
                SYSMEMSET(offset, word);
                break;

            case IV:
                if (0 < (word & IMMVAL_MASK))
                {
                    fprintf (debug, "IV:0x%.8X ", ISYSMEMGET(offset + 1));
                    fprintf (debug, "with IV:0x%.8X ", c -> rep[index].value);
                    SYSMEMSET(offset + 1, c -> rep[index].value);
                }
                break;
        }
    }
    fprintf (debug, "\n");

    // the instruction being stepped was fetched before the patch: fetch it again
    asm_refresh_ir ();
}

////////////////////////////////////////////////////////////////////////////////////////
//
// watch / unwatch
//
static void  exec_watch (cmd_t *c)
{
    wpoint_t  *wp  = wpoint_new (c -> watch_target, c -> watch_op, c -> watch_rhs, c -> watch_signed);
    char       desc[160];

    if (wp == NULL)
    {
        return;
    }

    if (c -> name != NULL)
    {
        wp -> label = copy_string (c -> name);
    }
    wpoint_add (&wpoint, wp);

    wpoint_describe (wp, desc, sizeof (desc));
    status_printf ("Watchpoint set: %s", desc);
    if (wp -> label != NULL)
    {
        status_printf (" (%s)", wp -> label);
    }
    status_printf ("\n");
}

static void  exec_unwatch (cmd_t *c)
{
    if (c -> has_num == TRUE)
    {
        if (wpoint_remove (&wpoint, (int) c -> num) == TRUE)
        {
            status_printf ("Watchpoint #%u removed.\n", c -> num);
        }
    }
    else if (wpoint_remove_by_label (&wpoint, c -> name) == TRUE)
    {
        status_printf ("Watchpoint '%s' removed.\n", c -> name);
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// run N / until TEXT / expect ... / x
//
static void  exec_run (cmd_t *c)
{
    if (c -> count == 0)
    {
        return;                      // zero frames: nothing to run
    }
    runctl_run (c -> count);
    action  = INPUT_CONTINUE;
}

static void  exec_until (cmd_t *c)
{
    runctl_until (c -> text, c -> timeout, c -> rows);
    action  = INPUT_CONTINUE;
}

static void  exec_expect (cmd_t *c)
{
    if (c -> expect_kind == EXPECT_CONSOLE)
    {
        runctl_expect_console (c -> text, c -> rows);
    }
    else
    {
        runctl_expect_value (c -> watch_target, c -> watch_op, c -> watch_rhs, c -> watch_signed);
    }
}

#define  EXAMINE_MAX  65536

static void  exec_examine (cmd_t *c)
{
    uint32_t  count  = (c -> has_count == TRUE) ? c -> count : c -> x_words;
    uint32_t  index  = 0;
    uint32_t  addr   = c -> opd.lo;
    word_t   *w      = NULL;

    if ((count == 0) || (count > EXAMINE_MAX))
    {
        fprintf (stderr, "[ERROR] x: the count must be 1-%d\n", EXAMINE_MAX);
        return;
    }
    for (index = 0; index < count; index = index + 1)
    {
        if ((c -> opd.name != NULL) && (index == 0) && (c -> fmt != FORMAT_DECODE))
        {
            output_mem_named (c -> opd.name, addr, c -> fmt, FALSE, NULL);
        }
        else
        {
            output_mem (addr, c -> fmt, FALSE, NULL);
        }
        // instructions are one or two words long
        w     = (c -> fmt == FORMAT_DECODE) ? memory_get (addr, TRUE) : NULL;
        addr  = addr + (((w != NULL) && (IMMVAL_MASK == (w -> raw & IMMVAL_MASK))) ? 2 : 1);
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// load / unload
//
static void  exec_load (cmd_t *c)
{
    int    page  = page_from_name (c -> name);
    FILE  *fptr  = NULL;

    switch (page)
    {
        case V32_PAGE_BIOS:
        case V32_PAGE_CART:
        case V32_PAGE_MEMC:
            break;

        default:
            fprintf (stderr, "[ERROR] load: unknown component '%s' (use bios, cart, or memc)\n", c -> name);
            return;
    }

    //
    // load_memory() terminates the simulator if it cannot open the file; check first
    // so that a typo at the prompt does not end the session
    //
    fptr = fopen (c -> path, "rb");
    if (fptr == NULL)
    {
        fprintf (stderr, "[ERROR] load: could not open '%s' for reading\n", c -> path);
        return;
    }
    fclose (fptr);

    if ((memory + page) -> data != NULL)
    {
        unload_memory (page);   // replacing a component: unload (and save a MEMC) first
    }

    fprintf (debug, "[load] %s filename: '%s'\n", c -> name, c -> path);
    if (load_memory (page, c -> path) == FALSE)
    {
        fprintf (stderr, "[ERROR] load: '%s' was not loaded\n", c -> path);
        return;
    }

    if (page == V32_PAGE_BIOS)
    {
        //
        // a new BIOS: its labels replace those of the old one, and the instruction in
        // IR/IV is fetched again in case it lies in the BIOS
        //
        biosfile  = (int8_t *) strdup (c -> path);   // the command's own copy dies with it
        if ((biosasmdebugflag != MU) && (biosasmdebug == NULL))
        {
            load_labels (biosfile, V32_PAGE_BIOS, FLAG_SEARCH | FLAG_ASM);
        }
        if ((bioscdebugflag != MU) && (bioscdebug == NULL))
        {
            load_labels (biosfile, V32_PAGE_BIOS, FLAG_SEARCH | FLAG_C);
        }
        asm_refresh_ir ();
    }
}

static void  exec_unload (cmd_t *c)
{
    int  page  = page_from_name (c -> name);

    if (page < 0)
    {
        fprintf (stderr, "[ERROR] unload: unknown component '%s' (use bios, cart, or memc)\n", c -> name);
        return;
    }

    if ((memory + page) -> data == NULL)
    {
        fprintf (stderr, "[ERROR] unload: %s is not loaded\n", c -> name);
        return;
    }

    switch (page)
    {
        case V32_PAGE_BIOS:   fprintf (verbose, "[unload] UNLOADING BIOS\n");     break;
        case V32_PAGE_CART:   fprintf (verbose, "[unload] UNLOADING CART\n");     break;
        case V32_PAGE_MEMC:   fprintf (verbose, "[unload] UNLOADING MEMCARD\n");  break;
    }
    unload_memory (page);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// screenshot: write the GPU framebuffer to a PNG file
//
static void  exec_screenshot (cmd_t *c)
{
    char       def[64];
    char      *name  = c -> path;
    time_t     now   = time (NULL);

    if (name == NULL)
    {
        strftime (def, sizeof (def), "v32sim.%Y%m%d-%H%M%S.png", localtime (&now));
        name  = def;
    }
    if (gpu_screenshot (name) == TRUE)
    {
        status_printf ("[screenshot] wrote %s\n", name);
    }
    else
    {
        fprintf (stderr, "[ERROR] screenshot: could not write '%s'\n", name);
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// save / restore: machine state files
//
static void  exec_save (cmd_t *c)
{
    char       def[64];
    char      *name  = c -> path;
    time_t     now   = time (NULL);

    if (name == NULL)
    {
        strftime (def, sizeof (def), "v32sim.%Y%m%d-%H%M%S.sav", localtime (&now));
        name  = def;
    }
    if (state_save (name) == TRUE)
    {
        status_printf ("[save] wrote %s\n", name);
    }
    else
    {
        fprintf (stderr, "[ERROR] save: could not write '%s'\n", name);
    }
}

static void  exec_restore (cmd_t *c)
{
    if (state_restore (c -> path) == TRUE)
    {
        status_printf ("[restore] state restored from %s\n", c -> path);
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// asm: assemble one instruction (see assembly.h)
//
//   asm INSTRUCTION           patch the instruction at the current IP
//   asm @ADDR INSTRUCTION     patch the instruction at ADDR
//   asm -x INSTRUCTION        execute it once, without touching memory
//
static void  asm_refresh_ir (void)
{
    REG(IR)  = IMEMGET(rom_offset);
    REG(IV)  = ((REG(IR) & IMMVAL_MASK) == IMMVAL_MASK) ? IMEMGET(rom_offset + 1) : 0;
}

static void  exec_asm (cmd_t *c)
{
    char      *text     = c -> path;
    char      *end      = NULL;
    char       err[160];
    uint32_t   words[2] = { 0, 0 };
    int        count    = 0;
    uint32_t   address  = REG(IP);
    uint32_t   old      = 0;
    uint8_t    execute  = FALSE;
    uint32_t   flags    = FLAG_NONE;
    float      fimm     = 0.0f;

    if (text == NULL)
    {
        return;
    }
    if ((text[0] == '-') && ((text[1] == 'x') || (text[1] == 'X')) &&
        ((text[2] == ' ') || (text[2] == '\t')))
    {
        execute  = TRUE;
        text     = text + 2;
    }
    else if (text[0] == '@')
    {
        address  = (uint32_t) strtoul (text + 1, &end, 0);
        if ((end == text + 1) || ((*end != ' ') && (*end != '\t')))
        {
            fprintf (stderr, "[ERROR] asm: expected '@ADDRESS INSTRUCTION'\n");
            return;
        }
        text     = end;
    }

    if (asm_assemble (text, words, &count, err, sizeof (err)) != 0)
    {
        fprintf (stderr, "[ERROR] asm: %s\n", err);
        return;
    }

    if (execute == TRUE)
    {
        uint32_t  ir  = REG(IR);
        uint32_t  iv  = REG(IV);

        flags  = (count == 2) ? FLAG_IMMEDIATE : FLAG_NONE;
        REG(IR)  = words[0];
        REG(IV)  = (count == 2) ? words[1] : 0;
        memcpy (&fimm, &REG(IV), sizeof (fimm));
        branchflag  = FALSE;
        decode (words[0], REG(IV), fimm, flags | FLAG_PROCESS);
        if (waitflag == TRUE)
        {
            update_frame ();            // a WAIT ends the frame (main loop does it for real runs)
        }
        if (branchflag == TRUE)
        {
            branchflag  = FALSE;
            asm_refresh_ir ();
        }
        else
        {
            REG(IR)  = ir;
            REG(IV)  = iv;
        }
        status_printf ("[asm] executed 0x%.8X", words[0]);
        if (count == 2)
        {
            status_printf (" 0x%.8X", words[1]);
        }
        status_printf ("\n");
        return;
    }

    if ((valid_address (address, FLAG_READ) == FALSE) ||
        (valid_address (address + count - 1, FLAG_READ) == FALSE))
    {
        fprintf (stderr, "[ERROR] asm: 0x%.8X is not a valid address\n", address);
        return;
    }

    // the instruction being replaced: the new one must fit in the space it occupies
    old  = ((ISYSMEMGET(address) & IMMVAL_MASK) == IMMVAL_MASK) ? 2 : 1;
    if ((uint32_t) count > old)
    {
        fprintf (stderr, "[ERROR] asm: the new instruction takes %d words but the one at 0x%.8X "
                         "takes %u (it would overwrite the next instruction)\n", count, address, old);
        return;
    }

    SYSMEMSET(address, words[0]);
    if (count == 2)
    {
        SYSMEMSET(address + 1, words[1]);
    }
    else if (old == 2)
    {
        // pad with MOV R0, R0 (a one-word no-op) so the next instruction is untouched
        SYSMEMSET(address + 1, (0x13u << OPCODESHIFT) | (1u << MOVADRSHIFT));
        status_printf ("[asm] padded with MOV R0, R0\n");
    }
    status_printf ("[asm] 0x%.8X <- 0x%.8X", address, words[0]);
    if (count == 2)
    {
        status_printf (" 0x%.8X", words[1]);
    }
    status_printf ("\n");
    if (address == rom_offset)
    {
        asm_refresh_ir ();
    }
}

// help
//
static void  exec_help (cmd_t *c)
{
    int  id  = CMD_HELP;

    if (c -> name != NULL)
    {
        id = cmd_lookup (c -> name);
        if (id == CMD_NONE)
        {
            fprintf (stderr, "[ERROR] help: no such command '%s'\n", c -> name);
            return;
        }
    }

    switch (id)
    {
        case CMD_ASM:         help (INPUT_ASM);        break;
        case CMD_HELP:        help (INPUT_HELP);       break;
        case CMD_BACKTRACE:   help (INPUT_BTRACE);     break;
        case CMD_BREAK:       help (INPUT_BREAK);      break;
        case CMD_CONTINUE:    help (INPUT_CONTINUE);   break;
        case CMD_DISPLAY:     help (INPUT_DISPLAY);    break;
        case CMD_GAMEPAD:     help (INPUT_GAMEPAD);    break;
        case CMD_CONSOLE:     help (INPUT_CONSOLE);    break;
        case CMD_KBD:         help (INPUT_KBD);        break;
        case CMD_MAILBOX:     help (INPUT_MAILBOX);    break;
        case CMD_IGNORE:      help (INPUT_IGNORE);     break;
        case CMD_INVENTORY:   help (INPUT_INVENTORY);  break;
        case CMD_LABEL:       help (INPUT_LABEL);      break;
        case CMD_LOAD:        help (INPUT_LOAD);       break;
        case CMD_NEXT:        help (INPUT_NEXT);       break;
        case CMD_PRINT:       help (INPUT_PRINT);      break;
        case CMD_PROFILE:     help (INPUT_PROFILE);    break;
        case CMD_QUIT:        help (INPUT_QUIT);       break;
        case CMD_REPLACE:     help (INPUT_REPLACE);    break;
        case CMD_SCREENSHOT:  help (INPUT_SCREENSHOT); break;
        case CMD_SAVE:        help (INPUT_SAVE);       break;
        case CMD_RESTORE:     help (INPUT_RESTORE);    break;
        case CMD_SET:         help (INPUT_SET);        break;
        case CMD_STEP:        help (INPUT_STEP);       break;
        case CMD_SSTEP:       help (INPUT_SSTEP);      break;
        case CMD_SNEXT:       help (INPUT_SNEXT);      break;
        case CMD_FINISH:      help (INPUT_FINISH);     break;
        case CMD_TBREAK:      help (INPUT_TBREAK);     break;
        case CMD_LIST:        help (INPUT_LIST);       break;
        case CMD_FRAME:
        case CMD_UP:
        case CMD_DOWN:        help (INPUT_FRAME);      break;
        case CMD_UNBREAK:
        case CMD_UNDISPLAY:
        case CMD_UNLABEL:     help (INPUT_UNDO);       break;
        case CMD_UNLOAD:      help (INPUT_UNLOAD);     break;
        case CMD_UNWATCH:     help (INPUT_UNWATCH);    break;
        case CMD_VERSION:     help (INPUT_VERSION);    break;
        case CMD_WATCH:       help (INPUT_WATCH);      break;
        case CMD_RUN:         help (INPUT_RUN);        break;
        case CMD_UNTIL:       help (INPUT_UNTIL);      break;
        case CMD_EXPECT:      help (INPUT_EXPECT);     break;
        case CMD_EXAMINE:     help (INPUT_EXAMINE);    break;
        case CMD_WATCHLIST:   help (INPUT_WATCHLIST);  break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// cmd_execute(): carry out a parsed command; returns (and stores in `action`) what
// the caller needs to do next
//
uint8_t  cmd_execute (cmd_t *c)
{
    action = INPUT_INIT;

    switch (c -> id)
    {
        case CMD_NONE:                                              break;
        case CMD_BACKTRACE:   exec_backtrace ();                    break;
        case CMD_BREAK:       exec_break (c);                       break;
        case CMD_CONTINUE:    action = INPUT_CONTINUE;              break;
        case CMD_DISPLAY:     exec_display (c);                     break;
        case CMD_GAMEPAD:     exec_gamepad (c);                     break;
        case CMD_CONSOLE:     exec_console (c);                     break;
        case CMD_KBD:         exec_kbd (c);                         break;
        case CMD_MAILBOX:     exec_mailbox (c);                     break;
        case CMD_ASM:         exec_asm (c);                         break;
        case CMD_HELP:        exec_help (c);                        break;
        case CMD_IGNORE:      action = INPUT_IGNORE;                break;
        case CMD_INVENTORY:   display_config (INPUT_INVENTORY);     break;
        case CMD_LABEL:       exec_label (c);                       break;
        case CMD_LOAD:        exec_load (c);                        break;
        case CMD_NEXT:        action = INPUT_NEXT;                  break;
        case CMD_PRINT:       exec_print (c);                       break;
        case CMD_PROFILE:     show_profile ();                      break;
        case CMD_QUIT:
            action = INPUT_QUIT;
            if (c -> has_num == TRUE)
            {
                quit_status_set  = TRUE;
                quit_status      = (uint8_t) (c -> num & 0xFF);
            }
            break;
        case CMD_RUN:         exec_run (c);                         break;
        case CMD_UNTIL:       exec_until (c);                       break;
        case CMD_EXPECT:      exec_expect (c);                      break;
        case CMD_EXAMINE:     exec_examine (c);                     break;
        case CMD_REPLACE:     exec_replace (c);                     break;
        case CMD_SCREENSHOT:  exec_screenshot (c);                  break;
        case CMD_SAVE:        exec_save (c);                        break;
        case CMD_RESTORE:     exec_restore (c);                     break;
        case CMD_SET:         exec_set (c);                         break;
        case CMD_STEP:        action = INPUT_STEP;                  break;
        case CMD_SSTEP:
        case CMD_SNEXT:
        case CMD_FINISH:
        case CMD_UNTILSRC:    exec_srcstep (c);                     break;
        case CMD_TBREAK:      exec_break (c);                       break;
        case CMD_LIST:        exec_list (c);                        break;
        case CMD_FRAME:
        case CMD_UP:
        case CMD_DOWN:        exec_frame (c);                       break;
        case CMD_UNBREAK:
        case CMD_UNDISPLAY:
        case CMD_UNLABEL:     exec_undo (c);                        break;
        case CMD_UNLOAD:      exec_unload (c);                      break;
        case CMD_UNWATCH:     exec_unwatch (c);                     break;
        case CMD_VERSION:     fprintf (stdout, "v32sim %s\n", VERSION); break;
        case CMD_WATCH:       exec_watch (c);                       break;
        case CMD_WATCHLIST:   wpoint_display ();                    break;
    }

    return (action);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// process_command(): parse and execute one command line (prompt or command file).
// A line that fails to parse is reported and leaves the simulator where it was.
//
uint8_t  process_command (const char *line)
{
    cmd_t  cmd;

    if (cmd_parse (line, &cmd) == 0)
    {
        cmd_execute (&cmd);
    }
    else
    {
        action = INPUT_INIT;
    }
    cmd_free (&cmd);

    return (action);
}
