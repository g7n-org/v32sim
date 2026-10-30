#include "defines.h"
#include <ctype.h>

////////////////////////////////////////////////////////////////////////////////////////
//
// srcdebug.c: source-level debugging (V32DBG)
//
// The debug files give one entry per instruction (assembler: ADDRESS,FILE,LINE[,LABEL])
// or per line change (C compiler side file, matched to addresses by load_labels()).
// This module turns those entries into one sorted table and builds the line-level
// commands on it:
//
//     sstep  snext  finish  until [LOCATION]    run until the source line changes ...
//     break/tbreak FILE:LINE                     resolve a line to an address
//     bt  frame  up  down                        the call chain, from the BP frames
//     list [LOCATION]                            source around a line
//
// An entry whose file is "-" or whose line is 0 says "no source from here on" (what a
// `;@src -` marker means for the runtime code); lookups treat it as a gap.
//
// The stepping engine does not touch the CPU. The main loop calls srcdebug_check() once
// per instruction, after the instruction at IP has been fetched and before it runs, and
// the engine says whether to stop. Everything is decided from IP, SP and the opcode of
// the instruction about to run, so it works on code with and without frame pointers.
//
// Frames: both v32cc and v32lua use `PUSH BP; MOV BP,SP`, so after the prologue the
// return address is at [BP+1] and the caller's BP at [BP]. The prologue and the epilogue
// (`POP BP; RET`) are recognized by the instructions themselves, so a stop at the first
// instruction of a function still unwinds correctly.
//
////////////////////////////////////////////////////////////////////////////////////////

#define  MAX_FRAMES              64
#define  MAX_LOCS                16
#define  LIST_LINES              10

extern uint64_t  insn_count;                     // main.c: instructions executed so far

typedef struct
{
    uint32_t     addr;
    const char  *file;                           // NULL: no source from here on
    uint32_t     line;
    const char  *func;                           // function name, or NULL
} sl_t;

static sl_t      *tab        = NULL;
static size_t     ntab       = 0;
static uint8_t    tab_built  = FALSE;

////////////////////////////////////////////////////////////////////////////////////////
//
// the table
//
static int  is_none (const char *file, uint32_t line)
{
    return ((file == NULL) || (line == 0) || (0 == strcmp (file, "-")));
}

void  srcdebug_invalidate (void)
{
    tab_built  = FALSE;
}

// is this node of the label list a line entry? (an entry named "-" says "no source from here",
// and needs no line)
static int  is_entry (const linked_l *n, int use_c)
{
    if (use_c == TRUE)
    {
        return ((n -> cname != NULL) && (n -> RAW != 0));
    }
    return ((n -> cname == NULL) && (n -> name != NULL) &&
            ((n -> NUMBER != 0) || (0 == strcmp (n -> name, "-"))));
}

static void  build (void)
{
    linked_l  *n      = NULL;
    size_t     count  = 0;
    size_t     i      = 0;
    size_t     out    = 0;
    int        use_c  = FALSE;
    const char *carry = NULL;

    free (tab);
    tab        = NULL;
    ntab       = 0;
    tab_built  = TRUE;

    // C-compiler entries are preferred; they exist only when the assembler entries could be
    // matched to them (RAW is the address found through the assembly line)
    for (n = lpoint; n != NULL; n = n -> next)
    {
        if ((n -> cname != NULL) && (n -> RAW != 0))
        {
            use_c  = TRUE;
            break;
        }
    }
    for (n = lpoint; n != NULL; n = n -> next)
    {
        if (is_entry (n, use_c))
        {
            count  = count + 1;
        }
    }
    if (count == 0)
    {
        return;
    }
    tab  = (sl_t *) calloc (count, sizeof (sl_t));
    if (tab == NULL)
    {
        return;
    }
    for (n = lpoint; n != NULL; n = n -> next)
    {
        if (is_entry (n, use_c) == FALSE)
        {
            continue;
        }
        if (use_c == TRUE)
        {
            tab[ntab].file  = n -> cname;
            tab[ntab].line  = n -> LINE;
            tab[ntab].func  = (const char *) n -> pointer;
        }
        else
        {
            tab[ntab].file  = n -> name;
            tab[ntab].line  = n -> NUMBER;
            tab[ntab].func  = n -> label;
        }
        tab[ntab].addr  = n -> RAW;
        if (is_none (tab[ntab].file, tab[ntab].line))
        {
            tab[ntab].file  = NULL;
            tab[ntab].line  = 0;
        }
        ntab  = ntab + 1;
    }

    // a stable sort keeps the first of two entries for one address
    for (i = 1; i < ntab; i = i + 1)             // insertion sort by address, stable
    {
        sl_t  key  = tab[i];
        size_t j   = i;

        while ((j > 0) && (tab[j - 1].addr > key.addr))
        {
            tab[j]  = tab[j - 1];
            j       = j - 1;
        }
        tab[j]  = key;
    }

    // one entry per address; a function name carries forward to the entries without one
    for (i = 0; i < ntab; i = i + 1)
    {
        if ((out > 0) && (tab[out - 1].addr == tab[i].addr))
        {
            continue;
        }
        if ((out > 0) && ((tab[out - 1].addr ^ tab[i].addr) & V32_PAGE_MASK))
        {
            carry  = NULL;
        }
        if (tab[i].file == NULL)
        {
            carry  = NULL;
        }
        else if (tab[i].func != NULL)
        {
            carry  = tab[i].func;
        }
        else
        {
            tab[i].func  = carry;
        }
        tab[out]  = tab[i];
        out       = out + 1;
    }
    ntab  = out;
}

static void  ensure (void)
{
    if (tab_built == FALSE)
    {
        build ();
    }
}

int  srcdebug_have (void)
{
    ensure ();
    return (ntab > 0);
}

static ssize_t  index_le (uint32_t addr)         // last entry with address <= addr, or -1
{
    size_t  lo  = 0;
    size_t  hi  = ntab;

    while (lo < hi)
    {
        size_t  mid  = (lo + hi) / 2;

        if (tab[mid].addr <= addr)  lo  = mid + 1;
        else                        hi  = mid;
    }
    return ((ssize_t) lo - 1);
}

static const sl_t  *find_exact (uint32_t addr)
{
    ssize_t  i  = index_le (addr);

    if ((i >= 0) && (tab[i].addr == addr) && (tab[i].file != NULL))
    {
        return (&tab[i]);
    }
    return (NULL);
}

static ssize_t  find_contain_index (uint32_t addr)
{
    ssize_t  i  = index_le (addr);

    if ((i < 0) || (tab[i].file == NULL) || ((tab[i].addr ^ addr) & V32_PAGE_MASK))
    {
        return (-1);
    }
    return (i);
}

static const sl_t  *find_contain (uint32_t addr)
{
    ssize_t  i  = find_contain_index (addr);

    return ((i < 0) ? NULL : &tab[i]);
}

static int  same_str (const char *a, const char *b)
{
    if ((a == NULL) || (b == NULL))
    {
        return (a == b);
    }
    return (0 == strcmp (a, b));
}

// first address of the run of entries with the same function name that holds entry i
static uint32_t  func_start (ssize_t i)
{
    while ((i > 0) && (tab[i - 1].file != NULL) && (tab[i].func != NULL) &&
           same_str (tab[i - 1].func, tab[i].func) &&
           !((tab[i - 1].addr ^ tab[i].addr) & V32_PAGE_MASK))
    {
        i  = i - 1;
    }
    return (tab[i].addr);
}

static const char  *base_of (const char *path)
{
    const char  *s  = strrchr (path, '/');

    return ((s == NULL) ? path : s + 1);
}

// does the file name of a debug entry stand for the name the user typed?
static int  file_matches (const char *entry, const char *want)
{
    size_t  le  = strlen (entry);
    size_t  lw  = strlen (want);

    if (0 == strcmp (entry, want))
    {
        return (TRUE);
    }
    if ((le > lw) && (0 == strcmp (entry + le - lw, want)) && (entry[le - lw - 1] == '/'))
    {
        return (TRUE);
    }
    if ((lw > le) && (0 == strcmp (want + lw - le, entry)) && (want[lw - le - 1] == '/'))
    {
        return (TRUE);
    }
    return ((strchr (want, '/') == NULL) && (0 == strcmp (base_of (entry), want)));
}

////////////////////////////////////////////////////////////////////////////////////////
//
// describing an address
//
// "file:line", with "in function" when asked, for the entry that holds the address
//
int  srcdebug_describe (uint32_t addr, char *out, size_t len, int with_func)
{
    const sl_t  *e  = NULL;

    out[0]  = '\0';
    ensure ();
    e  = find_contain (addr);
    if (e == NULL)
    {
        return (FALSE);
    }
    if ((with_func == TRUE) && (e -> func != NULL))
    {
        snprintf (out, len, "%s:%u in %s", base_of (e -> file), e -> line, e -> func);
    }
    else
    {
        snprintf (out, len, "%s:%u", base_of (e -> file), e -> line);
    }
    return (TRUE);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// memory and instruction helpers (no side effects: a bad address is just "no")
//
static int  rd (uint32_t addr, uint32_t *value)
{
    word_t  *w  = NULL;

    if (memory_chk (addr, FLAG_READ, TRUE) == FALSE)
    {
        return (FALSE);
    }
    w  = memory_get (addr, TRUE);
    if (w == NULL)
    {
        return (FALSE);
    }
    *value  = (uint32_t) w -> i32;
    return (TRUE);
}

#define  OP_OF(w)      (((w) & OPCODE_MASK) >> OPCODESHIFT)
#define  DST_OF(w)     (((w) & DSTREG_MASK) >> DSTREGSHIFT)
#define  SRC_OF(w)     (((w) & SRCREG_MASK) >> SRCREGSHIFT)
#define  LEN_OF(w)     ((((w) & IMMVAL_MASK) != 0) ? 2 : 1)

static int  insn_at (uint32_t addr, uint32_t *w)
{
    return (rd (addr, w));
}

static int  is_push_bp (uint32_t w)
{
    return ((OP_OF (w) == PUSH) && (DST_OF (w) == BP) && ((w & IMMVAL_MASK) == 0));
}

static int  is_mov_bp_sp (uint32_t w)
{
    return ((OP_OF (w) == MOV) && (DST_OF (w) == BP) && (SRC_OF (w) == SP) && ((w & IMMVAL_MASK) == 0));
}

static int  is_pop_bp (uint32_t w)
{
    return ((OP_OF (w) == POP) && (DST_OF (w) == BP));
}

////////////////////////////////////////////////////////////////////////////////////////
//
// frames
//
typedef struct
{
    uint32_t  pc;
    uint32_t  sp;
    uint32_t  bp;
    uint32_t  ret;                               // return address of this frame
    uint32_t  slot;                              // where it is stored
    uint32_t  cbp;                               // the caller's BP
    uint8_t   known;                             // ret/slot/cbp are valid
} frame_t;

static frame_t  frm[MAX_FRAMES];
static int      nfrm       = 0;
static int      sel_frame  = 0;

// is this the address right after a CALL instruction (with or without an immediate)?
static int  returns_after_call (uint32_t r)
{
    uint32_t  w  = 0;

    if (((r >> 28) != V32_PAGE_BIOS) && ((r >> 28) != V32_PAGE_CART))
    {
        return (FALSE);
    }
    if (rd (r - 2, &w) && (OP_OF (w) == CALL) && ((w & IMMVAL_MASK) != 0))  return (TRUE);
    if (rd (r - 1, &w) && (OP_OF (w) == CALL) && ((w & IMMVAL_MASK) == 0))  return (TRUE);
    return (FALSE);
}

// where is the return address of the frame running at pc, with these SP and BP?
static int  unwind (frame_t *f, int innermost)
{
    uint32_t  w   = 0;
    uint32_t  w2  = 0;
    uint32_t  v   = 0;
    ssize_t   i   = 0;

    f -> known  = FALSE;
    if (innermost == TRUE)
    {
        // the frame of the function being run may not be complete yet (or any more)
        if (insn_at (f -> pc, &w))
        {
            if (OP_OF (w) == RET)                                    // after POP BP
            {
                f -> slot  = f -> sp;
                f -> cbp   = f -> bp;
                goto done;
            }
            if (is_pop_bp (w) && insn_at (f -> pc + LEN_OF (w), &w2) && (OP_OF (w2) == RET))
            {
                if (!rd (f -> sp, &v))  return (FALSE);
                f -> slot  = f -> sp + 1;
                f -> cbp   = v;
                goto done;
            }
        }
        i  = find_contain_index (f -> pc);
        if (i < 0)
        {
            // code with no source (a runtime helper): usually a leaf with no frame of its own,
            // entered by a CALL, so the return address is on top of the stack
            if (rd (f -> sp, &v) && returns_after_call (v))
            {
                f -> slot  = f -> sp;
                f -> cbp   = f -> bp;
                goto done;
            }
        }
        else
        {
            uint32_t  fs  = func_start (i);

            if ((f -> pc == fs) && insn_at (fs, &w))
            {
                f -> slot  = f -> sp;                                // nothing pushed yet
                f -> cbp   = f -> bp;
                goto done;
            }
            if (insn_at (fs, &w) && is_push_bp (w) && (f -> pc == fs + 1) &&
                insn_at (f -> pc, &w2) && is_mov_bp_sp (w2))
            {
                if (!rd (f -> sp, &v))  return (FALSE);              // BP pushed, not yet set
                f -> slot  = f -> sp + 1;
                f -> cbp   = v;
                goto done;
            }
        }
    }
    if (!rd (f -> bp, &v))  return (FALSE);
    f -> cbp   = v;
    f -> slot  = f -> bp + 1;

done:
    if (!rd (f -> slot, &v))  return (FALSE);
    f -> ret    = v;
    f -> known  = TRUE;
    return (TRUE);
}

static void  compute_frames (void)
{
    frame_t  *f  = NULL;

    nfrm  = 0;
    frm[0].pc  = (uint32_t) REG(IP);
    frm[0].sp  = (uint32_t) REG(SP);
    frm[0].bp  = (uint32_t) REG(BP);
    while (nfrm < MAX_FRAMES)
    {
        f  = &frm[nfrm];
        nfrm  = nfrm + 1;
        if (unwind (f, (nfrm == 1) ? TRUE : FALSE) == FALSE)
        {
            break;
        }
        // stop at the end of the chain: not code, no caller frame, or no progress
        if ((((f -> ret >> 28) != V32_PAGE_BIOS) && ((f -> ret >> 28) != V32_PAGE_CART)) ||
            (nfrm >= MAX_FRAMES))
        {
            break;
        }
        if ((nfrm > 1) && ((f -> cbp == 0) || (f -> cbp <= f -> bp)))
        {
            break;
        }
        frm[nfrm].pc  = f -> ret;
        frm[nfrm].sp  = f -> slot + 1;
        frm[nfrm].bp  = f -> cbp;
        if (frm[nfrm].bp > RAM_FINAL_ADDR)
        {
            break;
        }
    }
}

// the entry that holds the frame's position (a caller's pc is the return address, whose
// instruction is the one before it)
static const sl_t  *frame_entry (int n)
{
    return (find_contain ((n == 0) ? frm[n].pc : frm[n].pc - 1));
}

static void  print_frame_line (int n, int mark)
{
    const sl_t  *e  = frame_entry (n);

    fprintf (stdout, "#%-2d ", n);
    (void) mark;
    if (e != NULL)
    {
        fprintf (stdout, "%s (%s:%u)", (e -> func != NULL) ? e -> func : "??", base_of (e -> file), e -> line);
        if (n > 0)
        {
            fprintf (stdout, "  [0x%.8X]", frm[n].pc);
        }
    }
    else
    {
        linked_l  *l  = find_label_at (lpoint, frm[n].pc);

        if ((l != NULL) && (l -> label != NULL))
        {
            fprintf (stdout, "%s [0x%.8X]", l -> label, frm[n].pc);
        }
        else
        {
            fprintf (stdout, "?? [0x%.8X]", frm[n].pc);
        }
    }
    fprintf (stdout, "\n");
}

// is the debug data about a high-level source (Lua, C), not just the assembly? Only then does
// 'backtrace' walk the BP frames: assembly programs need not use that convention, and for them
// the call list (CALL targets, see decode.c) is what the user has always had
static int  source_level (void)
{
    size_t  i  = 0;

    for (i = 0; i < ntab; i = i + 1)
    {
        if ((tab[i].file != NULL) && (0 != strcmp (source_lang (tab[i].file), "ASM")))
        {
            return (TRUE);
        }
    }
    return (FALSE);
}

int  srcdebug_backtrace (void)
{
    int  n  = 0;

    ensure ();
    if ((ntab == 0) || (source_level () == FALSE))
    {
        return (FALSE);
    }
    compute_frames ();
    for (n = 0; n < nfrm; n = n + 1)
    {
        print_frame_line (n, FALSE);
    }
    return (TRUE);
}

static void  show_source_line (const sl_t *e)
{
    const char  *text  = source_line (e -> file, e -> line);

    fprintf (stdout, "%u\t%s\n", e -> line, (text != NULL) ? text : "(source not found)");
}

// frame [N] / up [N] / down [N]
void  srcdebug_frame (int how, uint32_t n, int has_n)
{
    int          target  = sel_frame;
    const sl_t  *e       = NULL;

    ensure ();
    if (ntab == 0)
    {
        fprintf (stderr, "[ERROR] no source line information is loaded (assemble with v32as -g)\n");
        return;
    }
    compute_frames ();
    if (how == 0)       target  = (has_n == TRUE) ? (int) n : sel_frame;       // frame
    else if (how > 0)   target  = sel_frame + ((has_n == TRUE) ? (int) n : 1); // up
    else                target  = sel_frame - ((has_n == TRUE) ? (int) n : 1); // down
    if (target >= nfrm)
    {
        if (how > 0)
        {
            fprintf (stderr, "[ERROR] initial frame selected; you cannot go up\n");
            return;
        }
        fprintf (stderr, "[ERROR] no frame number %d (there are %d)\n", target, nfrm);
        return;
    }
    if (target < 0)
    {
        fprintf (stderr, "[ERROR] bottom (innermost) frame selected; you cannot go down\n");
        return;
    }
    sel_frame  = target;
    print_frame_line (sel_frame, FALSE);
    e  = frame_entry (sel_frame);
    if (e != NULL)
    {
        show_source_line (e);
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// the stepping engine
//
enum { SS_NONE = 0, SS_STEP, SS_NEXT, SS_FINISH, SS_UNTIL, SS_LOC };

static struct
{
    int        mode;
    const char *tag;
    uint64_t   insn0;                            // insn_count when armed
    char       file[512];
    char       func[256];
    uint32_t   line;
    uint8_t    have_start;
    uint8_t    over;                             // running over a call
    uint32_t   over_target;
    uint32_t   over_sp;
    uint32_t   prev_ip;
    uint32_t   prev_len;
    uint8_t    prev_ret;                         // the instruction before was a RET
    uint8_t    have_prev;
    uint8_t    have_ret;                         // the frame's return point is known
    uint32_t   ret_addr;
    uint32_t   ret_sp;
    uint32_t   loc[MAX_LOCS];
    int        nloc;
} ss;

static int     last_kind   = SS_NONE;            // for a blank line at the prompt
static char    last_file[512];
static uint32_t last_line  = 0;
static uint8_t last_has_loc = FALSE;

void  srcdebug_cancel (void)
{
    ss.mode  = SS_NONE;
}

// the user is back at the prompt, or the machine moved: frames and the list position are stale
static void  moved (void)
{
    sel_frame  = 0;
}

static uint32_t  list_last      = 0;
static char      list_file[512] = "";
static uint8_t   list_valid     = FALSE;

static void  begin (int mode, const char *tag)
{
    const sl_t  *e  = NULL;
    uint32_t     w  = 0;

    memset (&ss, 0, sizeof (ss));
    ss.mode   = mode;
    ss.tag    = tag;
    ss.insn0  = insn_count;
    e         = find_contain ((uint32_t) REG(IP));
    if (e != NULL)
    {
        snprintf (ss.file, sizeof (ss.file), "%s", e -> file);
        snprintf (ss.func, sizeof (ss.func), "%s", (e -> func != NULL) ? e -> func : "");
        ss.line        = e -> line;
        ss.have_start  = TRUE;
    }
    ss.prev_ip    = (uint32_t) REG(IP);
    ss.prev_len   = LEN_OF ((uint32_t) REG(IR));
    ss.prev_ret   = (OP_OF ((uint32_t) REG(IR)) == RET) ? TRUE : FALSE;
    ss.have_prev  = TRUE;
    list_valid    = FALSE;
    moved ();
    (void) w;
}

// the instruction about to run is a CALL: run over it, to its return
static void  arm_over (uint32_t ip, uint32_t ir)
{
    ss.over         = TRUE;
    ss.over_target  = ip + LEN_OF (ir);
    ss.over_sp      = (uint32_t) REG(SP);
}

static int  no_table (void)
{
    ensure ();
    if (ntab == 0)
    {
        fprintf (stderr, "[ERROR] no source line information is loaded (assemble with v32as -g, "
                         "or give the debug file)\n");
        return (TRUE);
    }
    return (FALSE);
}

int  srcdebug_step (int over)
{
    if (no_table ())  return (FALSE);
    begin (over ? SS_NEXT : SS_STEP, over ? "next" : "step");
    if ((over == TRUE) && (OP_OF ((uint32_t) REG(IR)) == CALL))
    {
        arm_over ((uint32_t) REG(IP), (uint32_t) REG(IR));
    }
    last_kind  = over ? SS_NEXT : SS_STEP;
    return (TRUE);
}

int  srcdebug_finish (void)
{
    char  where[600];

    if (no_table ())  return (FALSE);
    compute_frames ();
    if ((sel_frame >= nfrm) || (frm[sel_frame].known == FALSE) ||
        (((frm[sel_frame].ret >> 28) != V32_PAGE_BIOS) && ((frm[sel_frame].ret >> 28) != V32_PAGE_CART)))
    {
        fprintf (stderr, "[ERROR] finish: the selected frame is the outermost one (no return address)\n");
        return (FALSE);
    }
    {
        int      keep  = sel_frame;
        uint32_t ret   = frm[keep].ret;
        uint32_t sp    = frm[keep].slot + 1;
        const sl_t *e  = frame_entry (keep);

        if (e != NULL)
        {
            snprintf (where, sizeof (where), "%s (%s:%u)", (e -> func != NULL) ? e -> func : "??",
                                                          base_of (e -> file), e -> line);
        }
        else
        {
            snprintf (where, sizeof (where), "the code at 0x%.8X", frm[keep].pc);
        }
        begin (SS_FINISH, "finish");
        ss.have_ret  = TRUE;
        ss.ret_addr  = ret;
        ss.ret_sp    = sp;
        status_printf ("[finish] running until %s returns\n", where);
    }
    last_kind  = SS_FINISH;
    return (TRUE);
}

// until [LOCATION]: without a location, run to a later line of this function (a loop is
// left); with one, run to it or until the current function returns
int  srcdebug_until (const char *file, uint32_t line, int has_line)
{
    uint32_t  addrs[MAX_LOCS];
    int       n      = 0;
    uint32_t  actual = 0;

    if (no_table ())  return (FALSE);
    if (has_line == FALSE)
    {
        begin (SS_UNTIL, "until");
        if (OP_OF ((uint32_t) REG(IR)) == CALL)
        {
            arm_over ((uint32_t) REG(IP), (uint32_t) REG(IR));
        }
        last_kind  = SS_UNTIL;
        return (TRUE);
    }
    n  = srcdebug_resolve (file, line, FALSE, addrs, MAX_LOCS, &actual);
    if (n <= 0)
    {
        fprintf (stderr, "[ERROR] until: no code for %s:%u\n", (file != NULL) ? file : "(this file)", line);
        return (FALSE);
    }
    compute_frames ();
    begin (SS_LOC, "until");
    memcpy (ss.loc, addrs, (size_t) n * sizeof (uint32_t));
    ss.nloc  = n;
    if ((nfrm > 0) && (frm[0].known == TRUE) &&
        (((frm[0].ret >> 28) == V32_PAGE_BIOS) || ((frm[0].ret >> 28) == V32_PAGE_CART)))
    {
        ss.have_ret  = TRUE;
        ss.ret_addr  = frm[0].ret;
        ss.ret_sp    = frm[0].slot + 1;
    }
    last_kind     = SS_LOC;
    last_has_loc  = TRUE;
    last_line     = line;
    snprintf (last_file, sizeof (last_file), "%s", (file != NULL) ? file : "");
    return (TRUE);
}

// a blank line at the prompt repeats the previous source-level step
int  srcdebug_rearm (void)
{
    switch (last_kind)
    {
        case SS_STEP:    return (srcdebug_step (FALSE));
        case SS_NEXT:    return (srcdebug_step (TRUE));
        case SS_FINISH:  return (srcdebug_finish ());
        case SS_UNTIL:   return (srcdebug_until (NULL, 0, FALSE));
        case SS_LOC:     return (srcdebug_until ((last_file[0] != '\0') ? last_file : NULL, last_line, TRUE));
    }
    return (FALSE);
}

static void  stop_here (void)
{
    const sl_t  *e     = find_contain ((uint32_t) REG(IP));
    const char  *text  = NULL;
    const char  *tag   = ss.tag;

    ss.mode  = SS_NONE;
    runflag  = FALSE;
    moved ();
    if (e == NULL)
    {
        status_printf ("[%s] stopped at 0x%.8X (no source line)\n", tag, (uint32_t) REG(IP));
        return;
    }
    text  = source_line (e -> file, e -> line);
    status_printf ("[%s] %s:%u%s%s: %s\n", tag, base_of (e -> file), e -> line,
                   (e -> func != NULL) ? " in " : "", (e -> func != NULL) ? e -> func : "",
                   (text != NULL) ? text : "(source not found)");
    source_suppress (REG(IP));
}

int  srcdebug_check (void)
{
    uint32_t     ip     = (uint32_t) REG(IP);
    uint32_t     ir     = (uint32_t) REG(IR);
    uint32_t     sp     = (uint32_t) REG(SP);
    const sl_t  *e      = NULL;
    int          seq    = FALSE;
    int          stop   = FALSE;
    int          i      = 0;

    if (ss.mode == SS_NONE)
    {
        return (FALSE);
    }
    // reached by falling through, or by a RET (a return to the middle of a line, or to the start
    // of the next one, is not a new pass over the line); any other jump to an entry is
    seq           = ((ss.have_prev == TRUE) && ((ip == ss.prev_ip + ss.prev_len) || (ss.prev_ret == TRUE)));
    ss.prev_ip    = ip;
    ss.prev_len   = LEN_OF (ir);
    ss.prev_ret   = (OP_OF (ir) == RET) ? TRUE : FALSE;
    ss.have_prev  = TRUE;
    if (insn_count == ss.insn0)
    {
        return (FALSE);                          // nothing has run since the command
    }

    if (ss.mode == SS_FINISH)
    {
        if ((ip == ss.ret_addr) && (sp >= ss.ret_sp))
        {
            stop_here ();
            return (TRUE);
        }
        return (FALSE);
    }

    if (ss.mode == SS_LOC)
    {
        for (i = 0; i < ss.nloc; i = i + 1)
        {
            if (ip == ss.loc[i])  stop = TRUE;
        }
        if ((ss.have_ret == TRUE) && (ip == ss.ret_addr) && (sp >= ss.ret_sp))
        {
            stop = TRUE;
        }
        if (stop == TRUE)
        {
            stop_here ();
            return (TRUE);
        }
        return (FALSE);
    }

    if (ss.over == TRUE)
    {
        if ((ip == ss.over_target) && (sp >= ss.over_sp))
        {
            ss.over  = FALSE;
        }
        else
        {
            return (FALSE);
        }
    }

    e  = find_exact (ip);
    if (e != NULL)
    {
        int  same_line  = (ss.have_start == TRUE) && (e -> line == ss.line) &&
                          same_str (e -> file, ss.file) &&
                          same_str ((e -> func != NULL) ? e -> func : "", ss.func);

        if (ss.mode == SS_UNTIL)
        {
            stop  = (ss.have_start == FALSE) ||
                    (!same_str ((e -> func != NULL) ? e -> func : "", ss.func)) ||
                    (e -> line > ss.line);
        }
        else
        {
            stop  = (same_line == FALSE) || (seq == FALSE);
        }
    }
    if (stop == TRUE)
    {
        stop_here ();
        return (TRUE);
    }
    if (((ss.mode == SS_NEXT) || (ss.mode == SS_UNTIL)) && (OP_OF (ir) == CALL))
    {
        arm_over (ip, ir);
    }
    return (FALSE);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// lines to addresses
//
// the file to use when none is given: that of the current position, else the first one
//
static const char  *default_file (void)
{
    const sl_t  *e  = NULL;

    if (nfrm == 0)
    {
        compute_frames ();
    }
    e  = frame_entry (sel_frame < nfrm ? sel_frame : 0);
    if (e != NULL)
    {
        return (e -> file);
    }
    for (size_t i = 0; i < ntab; i = i + 1)
    {
        if (tab[i].file != NULL)
        {
            return (tab[i].file);
        }
    }
    return (NULL);
}

//
// srcdebug_resolve(): the addresses for FILE:LINE (FILE may be NULL). The lowest address
// of the line, or all of them. A line without code moves to the next one that has some:
// *actual tells which. Returns the number of addresses (0: nothing found)
//
int  srcdebug_resolve (const char *file, uint32_t line, int all, uint32_t *out, int max, uint32_t *actual)
{
    size_t       i       = 0;
    uint32_t     best    = 0xFFFFFFFF;
    int          n       = 0;
    const char  *want    = file;

    ensure ();
    if (ntab == 0)
    {
        return (0);
    }
    if (want == NULL)
    {
        compute_frames ();
        want  = default_file ();
    }
    if (want == NULL)
    {
        return (0);
    }
    for (i = 0; i < ntab; i = i + 1)
    {
        if ((tab[i].file == NULL) || (!file_matches (tab[i].file, want)))
        {
            continue;
        }
        if ((tab[i].line >= line) && (tab[i].line < best))
        {
            best  = tab[i].line;
        }
    }
    if (best == 0xFFFFFFFF)
    {
        return (0);
    }
    for (i = 0; (i < ntab) && (n < max); i = i + 1)
    {
        if ((tab[i].file != NULL) && (tab[i].line == best) && file_matches (tab[i].file, want))
        {
            out[n]  = tab[i].addr;
            n       = n + 1;
            if (all == FALSE)
            {
                break;                           // the table is sorted: the first is the lowest
            }
        }
    }
    *actual  = best;
    return (n);
}

// function name -> address of its first instruction
int  srcdebug_function (const char *name, uint32_t *addr)
{
    size_t  i  = 0;

    ensure ();
    for (i = 0; i < ntab; i = i + 1)
    {
        if ((tab[i].file != NULL) && (tab[i].func != NULL) && (0 == strcasecmp (tab[i].func, name)))
        {
            *addr  = func_start ((ssize_t) i);
            return (TRUE);
        }
    }
    return (FALSE);
}

// the line of a function's first instruction (list main)
static int  function_line (const char *name, const char **file, uint32_t *line)
{
    uint32_t     addr  = 0;
    const sl_t  *e     = NULL;

    if (srcdebug_function (name, &addr) == FALSE)
    {
        return (FALSE);
    }
    e  = find_contain (addr);
    if (e == NULL)
    {
        return (FALSE);
    }
    *file  = e -> file;
    *line  = e -> line;
    return (TRUE);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// list
//
void  srcdebug_list (const char *file, uint32_t line, int has_line, const char *func)
{
    const sl_t  *cur     = NULL;
    const char  *name    = file;
    uint32_t     first   = 0;
    uint32_t     last    = 0;
    uint32_t     l       = 0;
    const char  *text    = NULL;
    uint8_t      center  = FALSE;

    ensure ();
    compute_frames ();
    cur  = frame_entry (sel_frame < nfrm ? sel_frame : 0);

    if (func != NULL)
    {
        if (function_line (func, &name, &line) == FALSE)
        {
            fprintf (stderr, "[ERROR] list: no function named '%s'\n", func);
            return;
        }
        has_line  = TRUE;
    }
    if ((has_line == FALSE) && (name == NULL))
    {
        if ((list_valid == TRUE) && (list_file[0] != '\0'))
        {
            name   = list_file;
            first  = list_last + 1;
        }
        else if (cur != NULL)
        {
            name    = cur -> file;
            line    = cur -> line;
            center  = TRUE;
        }
        else
        {
            fprintf (stderr, "[ERROR] list: there is no current source line (try: list FILE:LINE)\n");
            return;
        }
    }
    else
    {
        if (name == NULL)
        {
            name  = (cur != NULL) ? cur -> file : default_file ();
        }
        if (name == NULL)
        {
            fprintf (stderr, "[ERROR] list: which file? (try: list FILE:LINE)\n");
            return;
        }
        line    = (has_line == TRUE) ? line : 1;
        center  = TRUE;
    }
    if (center == TRUE)
    {
        first  = (line > 4) ? line - 4 : 1;
    }
    last  = first + LIST_LINES - 1;

    if (source_line (name, 1) == NULL && source_line (name, first) == NULL)
    {
        fprintf (stderr, "[ERROR] list: cannot read the source file '%s'\n", name);
        return;
    }
    for (l = first; l <= last; l = l + 1)
    {
        text  = source_line (name, l);
        if (text == NULL)
        {
            if (l == first)
            {
                fprintf (stderr, "[ERROR] list: line %u is past the end of '%s'\n", first, base_of (name));
            }
            break;
        }
        fprintf (stdout, "%s%5u\t%s\n",
                 ((cur != NULL) && (cur -> line == l) && file_matches (cur -> file, name)) ? "=>" : "  ",
                 l, text);
        list_last  = l;
    }
    snprintf (list_file, sizeof (list_file), "%s", name);
    list_valid  = TRUE;
}
