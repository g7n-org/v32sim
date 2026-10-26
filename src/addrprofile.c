#include "defines.h"

////////////////////////////////////////////////////////////////////////////////////////
//
// Per-address execution profile (--profile-out): how many times the instruction at each
// address was executed. One flat counter array per memory page, indexed by the word
// offset in the page; one increment per executed instruction.
//
// The totals do not depend on timing, so the same cartridge with the same --fixed-clock,
// --seed and --fixed-cycles gives the same file.
//

int8_t   *profile_out        = NULL;     // --profile-out=FILE
uint8_t   profile_bin        = FALSE;    // --profile-format=bin
uint32_t  profile_top        = 0;        // --profile-top=N
uint8_t   fixed_cycles       = FALSE;    // --fixed-cycles
uint8_t   profile_by_function = FALSE;   // --profile-by=function

static uint32_t *counts[NUM_MEMORY_PAGES];
static uint64_t  total       = 0;

typedef struct
{
    uint32_t   addr;
    uint32_t   count;
}  prof_t;

typedef struct
{
    uint32_t   addr;
    int8_t    *label;
}  lab_t;

void  addrprof_init (void)
{
    int32_t  page = 0;

    if ((profile_out == NULL) && (profile_top == 0))
    {
        return;
    }
    for (page = 0; page < NUM_MEMORY_PAGES; page = page + 1)
    {
        if ((memory+page) -> size > 0)
        {
            counts[page] = (uint32_t *) calloc ((memory+page) -> size, sizeof (uint32_t));
            if (counts[page] == NULL)
            {
                fprintf (stderr, "[ERROR] could not allocate the address profile\n");
                exit (MEMORY_ALLOC_FAIL);
            }
        }
    }
}

uint8_t  addrprof_enabled (void)
{
    return ((profile_out != NULL) || (profile_top != 0));
}

void  addrprof_count (uint32_t  address)
{
    uint32_t  page   = (address & V32_PAGE_MASK) >> 28;    // the bus ignores bits 30-31
    uint32_t  offset = (address & 0x0FFFFFFF);

    if ((page < NUM_MEMORY_PAGES) && (counts[page] != NULL) &&
        (offset < (memory+page) -> size))
    {
        counts[page][offset] = counts[page][offset] + 1;
        total                = total + 1;
    }
}

static int  cmp_hot (const void *a, const void *b)
{
    const prof_t *x = (const prof_t *) a;
    const prof_t *y = (const prof_t *) b;

    if (x -> count != y -> count)
    {
        return ((x -> count < y -> count) ? 1 : -1);
    }
    return ((x -> addr > y -> addr) - (x -> addr < y -> addr));
}

//
// per-function totals (--profile-by=function): a function is what the debug files call
// the closest label at or before an address (for C compiled with v32cc -g and assembled
// with v32as --map, that is the C function; hand-written assembly gives one entry per
// label). Addresses before the first label of their page are one "(no label)" entry.
//
typedef struct
{
    int8_t    *name;        // NULL: no label covers the addresses
    uint32_t   page;        // the page, for the unlabeled ones
    uint32_t   addr;        // the lowest executed address
    uint64_t   count;       // instructions executed in the function
    uint32_t   naddr;       // distinct addresses executed
}  func_t;

static int  cmp_func_name (const void *a, const void *b)
{
    const func_t *x = (const func_t *) a;
    const func_t *y = (const func_t *) b;

    if ((x -> name == NULL) || (y -> name == NULL))
    {
        if (x -> name != y -> name)  return ((x -> name == NULL) ? 1 : -1);
        return ((x -> page > y -> page) - (x -> page < y -> page));
    }
    return (strcmp ((char *) x -> name, (char *) y -> name));
}

static int  cmp_func_hot (const void *a, const void *b)
{
    const func_t *x = (const func_t *) a;
    const func_t *y = (const func_t *) b;

    if (x -> count != y -> count)
    {
        return ((x -> count < y -> count) ? 1 : -1);
    }
    return ((x -> addr > y -> addr) - (x -> addr < y -> addr));
}

static int  cmp_lab (const void *a, const void *b)
{
    const lab_t *x = (const lab_t *) a;
    const lab_t *y = (const lab_t *) b;

    return ((x -> addr > y -> addr) - (x -> addr < y -> addr));
}

////////////////////////////////////////////////////////////////////////////////////////
//
// the label covering ADDRESS: the closest labeled address at or before it, in the same
// memory page (NULL when no debug file gave one)
//
static int8_t *cover (lab_t *labs, size_t nlabs, uint32_t address)
{
    size_t  lo = 0;
    size_t  hi = nlabs;

    while (lo < hi)
    {
        size_t  mid = (lo + hi) / 2;
        if (labs[mid].addr <= address)
        {
            lo = mid + 1;
        }
        else
        {
            hi = mid;
        }
    }
    if ((lo > 0) && ((labs[lo-1].addr & 0xF0000000) == (address & 0xF0000000)))
    {
        return (labs[lo-1].label);
    }
    return (NULL);
}

//
// build_funcs(): the per-function totals of the executed addresses (ascending), sorted by
// the lowest address of each function
//
static func_t  *build_funcs (prof_t *prof, size_t nprof, lab_t *labs, size_t nlabs, size_t *nfuncs)
{
    func_t   *funcs  = (func_t *) calloc ((nprof > 0) ? nprof : 1, sizeof (func_t));
    size_t    n      = 0;
    size_t    i      = 0;
    size_t    j      = 0;
    int8_t   *name   = NULL;
    uint32_t  page   = 0;

    *nfuncs = 0;
    if (funcs == NULL)
    {
        return (NULL);
    }
    for (i = 0; i < nprof; i = i + 1)       // runs of consecutive addresses covered by the same label
    {
        name  = (labs != NULL) ? cover (labs, nlabs, prof[i].addr) : NULL;
        page  = prof[i].addr >> 28;
        if ((n > 0) && (funcs[n-1].name == name) && ((name != NULL) || (funcs[n-1].page == page)))
        {
            funcs[n-1].count  = funcs[n-1].count + prof[i].count;
            funcs[n-1].naddr  = funcs[n-1].naddr + 1;
            continue;
        }
        funcs[n].name   = name;
        funcs[n].page   = page;
        funcs[n].addr   = prof[i].addr;
        funcs[n].count  = prof[i].count;
        funcs[n].naddr  = 1;
        n               = n + 1;
    }

    qsort (funcs, n, sizeof (func_t), cmp_func_name);     // the same name at several places is one function
    for (i = 0, j = 0; i < n; i = i + 1)
    {
        if ((j > 0) && (cmp_func_name (&funcs[j-1], &funcs[i]) == 0))
        {
            funcs[j-1].count  = funcs[j-1].count + funcs[i].count;
            funcs[j-1].naddr  = funcs[j-1].naddr + funcs[i].naddr;
            if (funcs[i].addr < funcs[j-1].addr)  funcs[j-1].addr = funcs[i].addr;
        }
        else
        {
            funcs[j]  = funcs[i];
            j         = j + 1;
        }
    }
    *nfuncs = j;
    return (funcs);
}

static int  cmp_func_addr (const void *a, const void *b)
{
    const func_t *x = (const func_t *) a;
    const func_t *y = (const func_t *) b;

    return ((x -> addr > y -> addr) - (x -> addr < y -> addr));
}

static void  func_label (const func_t *f, char *out, size_t len)
{
    static const char *pages[] = { "RAM", "BIOS", "CART", "MEMC" };

    if (f -> name != NULL)
    {
        snprintf (out, len, "%s", (char *) f -> name);
    }
    else
    {
        snprintf (out, len, "(no label, %s)", (f -> page < 4) ? pages[f -> page] : "?");
    }
}

void  addrprof_write (void)
{
    uint32_t   page   = 0;
    uint32_t   idx    = 0;
    size_t     nprof  = 0;
    size_t     nlabs  = 0;
    size_t     i      = 0;
    prof_t    *prof   = NULL;
    lab_t     *labs   = NULL;
    linked_l  *node   = NULL;
    FILE      *fptr   = NULL;
    int8_t    *name   = NULL;
    func_t    *funcs  = NULL;
    size_t     nfuncs = 0;
    char       fname[400];

    if (addrprof_enabled () == FALSE)
    {
        return;
    }

    // every executed address, in address order (pages ascend, so do their addresses)
    for (page = 0; page < NUM_MEMORY_PAGES; page = page + 1)
    {
        if (counts[page] == NULL)
        {
            continue;
        }
        for (idx = 0; idx < (memory+page) -> size; idx = idx + 1)
        {
            if (counts[page][idx] != 0)
            {
                nprof = nprof + 1;
            }
        }
    }
    prof = (prof_t *) malloc ((nprof + 1) * sizeof (prof_t));
    if (prof == NULL)
    {
        fprintf (stderr, "[ERROR] could not allocate the address profile\n");
        return;
    }
    i = 0;
    for (page = 0; page < NUM_MEMORY_PAGES; page = page + 1)
    {
        if (counts[page] == NULL)
        {
            continue;
        }
        for (idx = 0; idx < (memory+page) -> size; idx = idx + 1)
        {
            if (counts[page][idx] != 0)
            {
                prof[i].addr  = (page << 28) | idx;
                prof[i].count = counts[page][idx];
                i             = i + 1;
            }
        }
    }

    // the labels of the debug files, if any were loaded
    for (node = lpoint; node != NULL; node = node -> next)
    {
        if (node -> label != NULL)
        {
            nlabs = nlabs + 1;
        }
    }
    if (nlabs > 0)
    {
        labs = (lab_t *) malloc (nlabs * sizeof (lab_t));
    }
    if (labs != NULL)
    {
        nlabs = 0;
        for (node = lpoint; node != NULL; node = node -> next)
        {
            if (node -> label != NULL)
            {
                labs[nlabs].addr  = node -> RAW;
                labs[nlabs].label = node -> label;
                nlabs             = nlabs + 1;
            }
        }
        qsort (labs, nlabs, sizeof (lab_t), cmp_lab);   // qsort is not stable: ties are rare and
    }                                                   // only matter to the label shown

    if (profile_by_function == TRUE)
    {
        funcs = build_funcs (prof, nprof, labs, nlabs, &nfuncs);
        if ((labs == NULL) && (nprof > 0))
        {
            fprintf (stderr, "[WARNING] --profile-by=function: no labels are loaded (a debug file made with "
                             "v32as --map is needed; -n turns them off)\n");
        }
    }

    if (profile_out != NULL)
    {
        fptr = fopen ((char *) profile_out, "wb");
        if (fptr == NULL)
        {
            fprintf (stderr, "[ERROR] could not write the profile '%s'\n", profile_out);
        }
        else if ((profile_by_function == TRUE) && (profile_bin == FALSE) && (funcs != NULL))
        {
            qsort (funcs, nfuncs, sizeof (func_t), cmp_func_addr);
            fprintf (fptr, "# v32sim function profile v1\n");
            fprintf (fptr, "# cycles %llu frames %u\n", (unsigned long long) total, frames_run);
            for (i = 0; i < nfuncs; i = i + 1)
            {
                func_label (&funcs[i], fname, sizeof (fname));
                fprintf (fptr, "0x%.8X %llu %u %s\n", funcs[i].addr, (unsigned long long) funcs[i].count,
                                                      funcs[i].naddr, fname);
            }
        }
        else if (profile_bin == TRUE)
        {
            fputs ("V32PROF1\n", fptr);
            for (i = 0; i < nprof; i = i + 1)
            {
                uint8_t  rec[8];
                for (idx = 0; idx < 4; idx = idx + 1)   // little-endian, whatever the host
                {
                    rec[idx]     = (prof[i].addr  >> (8 * idx)) & 0xFF;
                    rec[idx + 4] = (prof[i].count >> (8 * idx)) & 0xFF;
                }
                fwrite (rec, 1, sizeof (rec), fptr);
            }
        }
        else
        {
            fprintf (fptr, "# v32sim address profile v1\n");
            fprintf (fptr, "# cycles %llu frames %u\n", (unsigned long long) total, frames_run);
            for (i = 0; i < nprof; i = i + 1)
            {
                fprintf (fptr, "0x%.8X %u", prof[i].addr, prof[i].count);
                name = (labs != NULL) ? cover (labs, nlabs, prof[i].addr) : NULL;
                if (name != NULL)
                {
                    fprintf (fptr, " %s", name);
                }
                fprintf (fptr, "\n");
            }
        }
        if (fptr != NULL)
        {
            fclose (fptr);
        }
    }

    if ((profile_top > 0) && (funcs != NULL))
    {
        qsort (funcs, nfuncs, sizeof (func_t), cmp_func_hot);
        fprintf (stderr, "# %llu instructions executed; hottest functions:\n", (unsigned long long) total);
        for (i = 0; (i < nfuncs) && (i < profile_top); i = i + 1)
        {
            func_label (&funcs[i], fname, sizeof (fname));
            fprintf (stderr, "%12llu %5.1f%% %6u addresses  %s  (from 0x%.8X)\n",
                             (unsigned long long) funcs[i].count,
                             (total > 0) ? (100.0 * funcs[i].count / (double) total) : 0.0,
                             funcs[i].naddr, fname, funcs[i].addr);
        }
    }
    else if (profile_top > 0)
    {
        qsort (prof, nprof, sizeof (prof_t), cmp_hot);
        fprintf (stderr, "# %llu instructions executed; hottest addresses:\n", (unsigned long long) total);
        for (i = 0; (i < nprof) && (i < profile_top); i = i + 1)
        {
            name = (labs != NULL) ? cover (labs, nlabs, prof[i].addr) : NULL;
            fprintf (stderr, "0x%.8X %10u %5.1f%%%s%s\n", prof[i].addr, prof[i].count,
                             (total > 0) ? (100.0 * prof[i].count / (double) total) : 0.0,
                             (name != NULL) ? "  " : "", (name != NULL) ? (char *) name : "");
        }
    }

    free (funcs);
    free (prof);
    free (labs);
}
