#include "defines.h"
#include <ctype.h>

////////////////////////////////////////////////////////////////////////////////////////
//
// symbols.c
//
// Names for addresses, wherever a command takes an address (print, display, watch,
// expect, x, --dump). Two sources feed the lookup:
//
//   * the labels of the debug files (lpoint): functions and assembly labels, as before
//   * data symbols: the global variables of a C program. The debug files do not carry
//     them, but v32cc writes them (-g) to NAME.asm.sym, one line per symbol:
//
//         kind,name,address,words,cfile,cline        V RAM variable, F function, R ROM datum
//
//     and, failing that, the `%define global_NAME ADDRESS` lines at the top of the
//     generated NAME.asm give the same addresses (without sizes).
//
// A name is looked up as given, then as global_NAME (the assembler's name for a C
// variable). An exact match wins over a match that ignores case.
//
////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    char      *name;
    uint32_t   addr;
    uint32_t   words;
} dsym_t;

static dsym_t   *dsyms     = NULL;
static size_t    ndsyms    = 0;
static size_t    capdsyms  = 0;

static void  dsym_add (const char *name, uint32_t addr, uint32_t words)
{
    dsym_t  *grown  = NULL;
    size_t   index  = 0;

    for (index = 0; index < ndsyms; index = index + 1)
    {
        if (0 == strcmp (dsyms[index].name, name))   // a later file redefines the name
        {
            dsyms[index].addr   = addr;
            dsyms[index].words  = words;
            return;
        }
    }
    if (ndsyms >= capdsyms)
    {
        capdsyms  = (capdsyms == 0) ? 64 : (capdsyms * 2);
        grown     = (dsym_t *) realloc (dsyms, capdsyms * sizeof (dsym_t));
        if (grown == NULL)
        {
            fprintf (stderr, "[ERROR] out of memory while loading symbols\n");
            exit (STRING_ALLOC_FAIL);
        }
        dsyms     = grown;
    }
    dsyms[ndsyms].name   = strdup (name);
    dsyms[ndsyms].addr   = addr;
    dsyms[ndsyms].words  = (words == 0) ? 1 : words;
    ndsyms               = ndsyms + 1;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// sym_parse_number(): decimal or 0x hexadecimal; FALSE if the text is not a number
// (a function's address in a .sym file is an assembly label, "@__function_f")
//
static int  sym_parse_number (const char *text, uint32_t *value)
{
    char           *end  = NULL;
    unsigned long   v    = 0;

    if ((text == NULL) || (!isdigit ((unsigned char) text[0])))
    {
        return (FALSE);
    }
    v = strtoul (text, &end, 0);
    if ((end == text) || (*end != '\0'))
    {
        return (FALSE);
    }
    *value = (uint32_t) v;
    return (TRUE);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// sym_load_file(): read a v32cc symbol file (NAME.asm.sym). Returns the number of
// symbols added. Only entries whose address is already a number can be used: the
// variables. (Functions and ROM data are placed by the assembler; the debug files
// name them.)
//
uint32_t  sym_load_file (const char *path)
{
    FILE      *fptr   = fopen (path, "r");
    char       line[4096];
    char      *cursor = NULL;
    char      *kind   = NULL;
    char      *name   = NULL;
    char      *addr   = NULL;
    char      *words  = NULL;
    char      *comma  = NULL;
    uint32_t   value  = 0;
    uint32_t   size   = 0;
    uint32_t   tally  = 0;
    size_t     len    = 0;

    if (fptr == NULL)
    {
        return (0);
    }
    while (fgets (line, sizeof (line), fptr) != NULL)
    {
        len = strlen (line);
        while ((len > 0) && ((line[len-1] == '\n') || (line[len-1] == '\r')))
        {
            line[len-1] = '\0';
            len         = len - 1;
        }
        if ((len == 0) || (line[0] == '#'))
        {
            continue;
        }

        cursor = line;
        kind   = cursor;
        name   = NULL;
        addr   = NULL;
        words  = NULL;
        comma  = strchr (cursor, ',');
        if (comma != NULL)  { *comma = '\0';  name = comma + 1;  comma = strchr (name, ','); }
        if (comma != NULL)  { *comma = '\0';  addr = comma + 1;  comma = strchr (addr, ','); }
        if (comma != NULL)  { *comma = '\0';  words = comma + 1; comma = strchr (words, ','); }
        if (comma != NULL)  { *comma = '\0'; }
        if ((name == NULL) || (addr == NULL) || (*name == '\0'))
        {
            continue;
        }
        if (sym_parse_number (addr, &value) == FALSE)
        {
            continue;
        }
        size = 1;
        if ((words != NULL) && (sym_parse_number (words, &size) == FALSE))
        {
            size = 1;
        }
        dsym_add (name, value, size);
        tally = tally + 1;
    }
    fclose (fptr);
    return (tally);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// sym_load_asm(): the `%define global_NAME ADDRESS` lines of a generated .asm file
//
static uint32_t  sym_load_asm (FILE *fptr)
{
    char       line[1024];
    char      *p      = NULL;
    char      *name   = NULL;
    char      *end    = NULL;
    uint32_t   value  = 0;
    uint32_t   tally  = 0;

    while (fgets (line, sizeof (line), fptr) != NULL)
    {
        for (p = line; (*p == ' ') || (*p == '\t'); p = p + 1) ;
        if (0 != strncmp (p, "%define", 7))
        {
            continue;
        }
        for (p = p + 7; (*p == ' ') || (*p == '\t'); p = p + 1) ;
        if (0 != strncmp (p, "global_", 7))
        {
            continue;
        }
        name = p + 7;
        for (end = name; (*end != '\0') && (*end != ' ') && (*end != '\t'); end = end + 1) ;
        if ((end == name) || (*end == '\0'))
        {
            continue;
        }
        *end = '\0';
        for (p = end + 1; (*p == ' ') || (*p == '\t'); p = p + 1) ;
        if (sym_parse_number (p, &value) == FALSE)
        {
            continue;
        }
        dsym_add (name, value, 1);
        tally = tally + 1;
    }
    return (tally);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// sym_load_cart(): the data symbols that go with a V32 file, found next to it (or in
// the places the debug files are looked for). NAME.asm.sym first, NAME.asm otherwise.
//
uint32_t  sym_load_cart (const char *datafile)
{
    char      found[2048];
    FILE     *fptr   = NULL;
    uint32_t  tally  = 0;

    if (datafile == NULL)
    {
        return (0);
    }
    fptr = open_search_file (datafile, ".asm.sym", found, sizeof (found));
    if (fptr != NULL)
    {
        fclose (fptr);
        tally = sym_load_file (found);
        fprintf (verbose, "LOADING SYMBOLS FROM: %s (%u)\n", found, tally);
        return (tally);
    }
    fptr = open_search_file (datafile, ".asm", found, sizeof (found));
    if (fptr != NULL)
    {
        tally = sym_load_asm (fptr);
        fclose (fptr);
        fprintf (verbose, "LOADING SYMBOLS FROM: %s (%u)\n", found, tally);
    }
    return (tally);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// lookups
//
static int  names_match (const char *a, const char *b, int nocase)
{
    return ((nocase == TRUE) ? (0 == strcasecmp (a, b)) : (0 == strcmp (a, b)));
}

static int  find_data (const char *name, int nocase, uint32_t *addr, uint32_t *words)
{
    size_t  index  = 0;

    for (index = 0; index < ndsyms; index = index + 1)
    {
        if (names_match (dsyms[index].name, name, nocase) == TRUE)
        {
            *addr   = dsyms[index].addr;
            *words  = dsyms[index].words;
            return (TRUE);
        }
    }
    return (FALSE);
}

static int  find_code (const char *name, int nocase, uint32_t *addr)
{
    linked_l  *node  = NULL;

    for (node = lpoint; node != NULL; node = node -> next)
    {
        if ((node -> label != NULL) && (names_match (node -> label, name, nocase) == TRUE))
        {
            *addr = node -> RAW;
            return (TRUE);
        }
    }
    return (FALSE);
}

static int  find_name (const char *name, int nocase, uint32_t *addr, uint32_t *words)
{
    char  alt[320];

    *words = 1;
    if (find_data (name, nocase, addr, words) == TRUE)  return (TRUE);
    if (find_code (name, nocase, addr)        == TRUE)  return (TRUE);
    if (0 == strncmp (name, "global_", 7))
    {
        if (find_data (name + 7, nocase, addr, words) == TRUE)  return (TRUE);
    }
    else
    {
        snprintf (alt, sizeof (alt), "global_%s", name);
        if (find_code (alt, nocase, addr) == TRUE)  return (TRUE);
    }
    return (FALSE);
}

//
// sym_lookup(): the address a name stands for, and the number of words it covers (1
// for labels and for variables of unknown size). TRUE if the name is known.
//
int  sym_lookup (const char *name, uint32_t *addr, uint32_t *words)
{
    uint32_t  dummy  = 0;

    if (name == NULL)
    {
        return (FALSE);
    }
    if (words == NULL)
    {
        words = &dummy;
    }
    if (find_name (name, FALSE, addr, words) == TRUE)  return (TRUE);
    return (find_name (name, TRUE, addr, words));
}

//
// sym_name_for(): "NAME" or "NAME+OFFSET" for an address inside a variable. Only data
// symbols are named this way (a watchpoint on a code label is not a common sight).
//
int  sym_name_for (uint32_t address, char *out, size_t len)
{
    size_t  index  = 0;

    for (index = 0; index < ndsyms; index = index + 1)
    {
        if ((address >= dsyms[index].addr) && (address < dsyms[index].addr + dsyms[index].words))
        {
            if (address == dsyms[index].addr)
            {
                snprintf (out, len, "%s", dsyms[index].name);
            }
            else
            {
                snprintf (out, len, "%s+%u", dsyms[index].name, address - dsyms[index].addr);
            }
            return (TRUE);
        }
    }
    return (FALSE);
}
