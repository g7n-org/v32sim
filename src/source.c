////////////////////////////////////////////////////////////////////////////////////////
//
// source.c: source-line overlay
//
// Using the offsets, line numbers, and file names from the debug files (loaded into the
// label list by load_labels()), show the line of assembly -- or, in C mode, the line of
// C -- that produced the instruction at the current IP.
//
// The file names stored in debug files are relative to the directory the program was
// built in (e.g. obj/game.asm), so they are tried as given, and relative to every
// directory registered with source_add_root() (the places debug and V32 files were
// found in, and their parents).
//
////////////////////////////////////////////////////////////////////////////////////////
#include "defines.h"
#include "globals.h"

#define  SRC_MAX_ROOTS           16
#define  SRC_CACHE_SIZE          8

typedef struct
{
    char      name[512];          // file name as given in the debug file
    char     *text;               // whole file (NULL if it could not be read)
    char    **line;               // start of each line (NUL terminated in place)
    uint32_t  nlines;
    uint8_t   used;
} srcfile_t;

static char       roots[SRC_MAX_ROOTS][1024];
static int        nroots        = 0;
static srcfile_t  cache[SRC_CACHE_SIZE];
static int        cachenext     = 0;

static void  cache_clear (void)
{
    int  i  = 0;

    for (i = 0; i < SRC_CACHE_SIZE; i = i + 1)
    {
        free (cache[i].text);
        free (cache[i].line);
        memset (&cache[i], 0, sizeof (cache[i]));
    }
    cachenext  = 0;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// source_add_root(): remember a directory to resolve debug-file names against
//
void  source_add_root (const char *dir)
{
    int  i  = 0;

    if ((dir == NULL) || (nroots >= SRC_MAX_ROOTS))
    {
        return;
    }
    for (i = 0; i < nroots; i = i + 1)
    {
        if (strcmp (roots[i], dir) == 0)
        {
            return;
        }
    }
    snprintf (roots[nroots], sizeof (roots[0]), "%s", dir);
    nroots  = nroots + 1;
    cache_clear ();                 // earlier lookups may have failed for lack of this root
}

static FILE  *open_source (const char *name)
{
    char   path[2048];
    FILE  *fptr  = fopen (name, "r");
    int    i     = 0;

    for (i = 0; (fptr == NULL) && (i < nroots); i = i + 1)
    {
        snprintf (path, sizeof (path), "%s/%s", roots[i], name);
        fptr  = fopen (path, "r");
    }
    return (fptr);
}

static srcfile_t  *get_file (const char *name)
{
    int         i      = 0;
    srcfile_t  *c      = NULL;
    FILE       *fptr   = NULL;
    long        size   = 0;
    char       *p      = NULL;
    uint32_t    n      = 0;
    uint32_t    k      = 0;

    for (i = 0; i < SRC_CACHE_SIZE; i = i + 1)
    {
        if ((cache[i].used) && (strcmp (cache[i].name, name) == 0))
        {
            return (&cache[i]);
        }
    }

    c  = &cache[cachenext];
    cachenext  = (cachenext + 1) % SRC_CACHE_SIZE;
    free (c -> text);
    free (c -> line);
    memset (c, 0, sizeof (*c));
    c -> used  = TRUE;
    snprintf (c -> name, sizeof (c -> name), "%s", name);

    fptr  = open_source (name);
    if (fptr == NULL)
    {
        return (c);                 // cached failure
    }
    fseek (fptr, 0, SEEK_END);
    size  = ftell (fptr);
    fseek (fptr, 0, SEEK_SET);
    if (size < 0)
    {
        fclose (fptr);
        return (c);
    }
    c -> text  = (char *) calloc ((size_t) size + 2, 1);
    if ((c -> text != NULL) && (fread (c -> text, 1, (size_t) size, fptr) == (size_t) size))
    {
        for (p = c -> text; *p != '\0'; p = p + 1)
        {
            if (*p == '\n')
            {
                n  = n + 1;
            }
        }
        n  = n + 1;
        c -> line  = (char **) calloc (n + 1, sizeof (char *));
        if (c -> line != NULL)
        {
            c -> line[0]  = c -> text;
            k             = 1;
            for (p = c -> text; *p != '\0'; p = p + 1)
            {
                if (*p == '\n')
                {
                    *p  = '\0';
                    if (p > c -> text && *(p - 1) == '\r')
                    {
                        *(p - 1)  = '\0';
                    }
                    c -> line[k]  = p + 1;
                    k             = k + 1;
                }
            }
            if ((k > 1) && (c -> line[k - 1][0] == '\0') && (c -> line[k - 1] == p))
            {
                k  = k - 1;                 // the text ends with a newline: no line after it
            }
            c -> nlines  = k;
        }
    }
    fclose (fptr);
    return (c);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// source_line(): text of 1-based line 'line' of 'name', or NULL if unavailable
//
const char  *source_line (const char *name, uint32_t line)
{
    srcfile_t  *c  = NULL;

    if ((name == NULL) || (line == 0))
    {
        return (NULL);
    }
    c  = get_file (name);
    if ((c -> line == NULL) || (line > c -> nlines))
    {
        return (NULL);
    }
    return (c -> line[line - 1]);
}

static const char  *base_name (const char *path)
{
    const char  *slash  = strrchr (path, '/');

    return ((slash == NULL) ? path : slash + 1);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// the language of a source file, from its extension (what V32DBG's ;@lang says)
//
const char  *source_lang (const char *path)
{
    const char  *dot  = strrchr (path, '.');

    if (dot != NULL)
    {
        if (0 == strcasecmp (dot, ".lua"))                                  return ("LUA");
        if ((0 == strcasecmp (dot, ".c"))   || (0 == strcasecmp (dot, ".h"))   ||
            (0 == strcasecmp (dot, ".cc"))  || (0 == strcasecmp (dot, ".cpp")) ||
            (0 == strcasecmp (dot, ".cxx")) || (0 == strcasecmp (dot, ".hpp")))  return ("C");
    }
    return ("ASM");
}

static uint32_t  suppress_ip  = 0xFFFFFFFF;

//
// source_suppress(): a source-level step has just printed the line for this IP; the
// overlay at the prompt must not print it again
//
void  source_suppress (uint32_t ip)
{
    suppress_ip  = ip;
}

static void  print_line (const char *lang, const char *file, uint32_t line)
{
    const char  *text  = source_line (file, line);

    if (colorflag == TRUE) fprintf (stdout, "\e[1;36m");
    fprintf (stdout, "  %s %s:%u: %s\n", lang, base_name (file), line,
                     (text != NULL) ? text : "(source not found)");
    if (colorflag == TRUE) fprintf (stdout, "\e[m");
}

////////////////////////////////////////////////////////////////////////////////////////
//
// source_overlay(): print the source line for the instruction at 'ip' (if the overlay
// is enabled and the debug data has an entry for it)
//
//   mode asm            the assembly line of the assembler's entries
//   mode src            the source line, whatever the language: the compiler's entry, or
//                       else the assembler's (which names the source file when the
//                       program was assembled from ;@src markers)
//   mode C | lua        the same as src (kept for old command lines); C still skips
//                       entries that are assembly
//
void  source_overlay (uint32_t ip)
{
    linked_l    *asmnode  = NULL;
    linked_l    *cnode    = NULL;
    linked_l    *tmp      = NULL;

    if (suppress_ip == ip)
    {
        suppress_ip  = 0xFFFFFFFF;
        return;
    }
    suppress_ip  = 0xFFFFFFFF;
    if (sourceflag != TRUE)
    {
        return;
    }
    for (tmp = lpoint; tmp != NULL; tmp = tmp -> next)
    {
        if (tmp -> RAW != ip)
        {
            continue;
        }
        if ((tmp -> cname != NULL) && (cnode == NULL))
        {
            cnode  = tmp;
        }
        else if ((tmp -> cname == NULL) && (tmp -> name != NULL) && (asmnode == NULL))
        {
            asmnode  = tmp;
        }
    }
    if ((asmnode != NULL) && (0 == strcmp (asmnode -> name, "-")))
    {
        asmnode  = NULL;                // an entry that says "no source here"
    }

    if (modeflag == FLAG_ASM)
    {
        if (asmnode != NULL)
        {
            print_line (source_lang (asmnode -> name), asmnode -> name, asmnode -> NUMBER);
        }
        return;
    }
    if (cnode != NULL)
    {
        print_line (source_lang (cnode -> cname), cnode -> cname, cnode -> LINE);
    }
    else if ((asmnode != NULL) &&
             ((modeflag != FLAG_C) || (0 != strcmp (source_lang (asmnode -> name), "ASM"))))
    {
        print_line (source_lang (asmnode -> name), asmnode -> name, asmnode -> NUMBER);
    }
}
