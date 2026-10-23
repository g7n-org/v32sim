#include "defines.h"
#include <readline/readline.h>
#include <readline/history.h>
#include "command.h"

uint8_t *get_input (FILE *fptr, const uint8_t *prompt)
{
    ////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    size_t   input_length  = 0;
    uint8_t *input_string  = NULL;

    ////////////////////////////////////////////////////////////////////////////////
    //
    // If colors are enabled, highlight the decoded instruction
    //
    if (colorflag         == TRUE)
    {
        fprintf (stdout, "\e[1;32m");
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // obtain input (via readline)
    //
    input_string           = readline (prompt);

    if (input_string      == NULL)
    {
        action             = INPUT_QUIT;
        return (NULL);
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // If colors are enabled, complete the highlight the decoded instruction
    //
    if (colorflag         == TRUE)
    {
        fprintf (stdout, "\e[m");
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // if the line is not empty, add it to the readline history
    //
    input_length           = strlen (input_string);
    if (input_length      >  0)
    {
        add_history (input_string);
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // display what was input
    //
    fprintf (debug, "[get_input] input_string: \"%s\" (input_length: %lu)\n",
                    input_string, input_length);

    if (input_length      == 0)
    {
        action             = INPUT_NONE;
    }

    return (input_string);
}

uint8_t  prompt (uint32_t  word)
{
    int32_t    value                    = 0;
    static uint8_t   *input             = NULL;
    static uint8_t    lastaction        = INPUT_INIT;
    uint8_t    processflag              = FALSE;

    v32io_term_suspend ();      // the prompt takes the host terminal back (if v32kbd had it)

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Display the prompt and obtain input
    //
    if (input                          != NULL)
    {
        free (input);
    }

    input                               = get_input (stdin, "v32sim> ");

    if (input                          == NULL)
    {
        action                          = INPUT_QUIT;
        lastaction                      = INPUT_QUIT;
        return (2);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // an empty line repeats the previous action (eg: hit Enter to keep stepping);
    // anything else is parsed (flex/bison) and executed by process_command(), which
    // leaves `action` set to what must happen next: INPUT_INIT for the commands that
    // do not move the CPU, otherwise step/next/continue/ignore/quit
    //
    if (*input                         == '\0')
    {
        action                          = lastaction;
    }
    else
    {
        process_command (input);
    }

    switch (action)
    {
        case INPUT_IGNORE:
            ignoreflag                  = TRUE;   // skip the current instruction
            // fall through: then run freely
        case INPUT_CONTINUE:
            processflag                 = TRUE;
            runflag                     = TRUE;
            nextflag                    = FALSE;
            break;

        case INPUT_QUIT:
            processflag                = 2;
            break;

        case INPUT_STEP:
            processflag                = TRUE;
            break;

        case INPUT_NEXT:
            ////////////////////////////////////////////////////////////////////////////
            //
            // Save the current backtrace depth
            //
            next_tpoint                = tpoint;  // Save the current backtrace list

            ////////////////////////////////////////////////////////////////////////////
            //
            // if the instruction is a CALL, set seek_word and then
            // set runflag to TRUE; otherwise, should behave like step
            //
            value                      = (word & OPCODE_MASK) >> OPCODESHIFT;
            if (value                 == CALL)  // nexting over a CALL instruction
            {
                runflag                = TRUE;
                seek_word              = REG(IP) + 1;
                value                  = (word & IMMVAL_MASK);
                if (value             >  0)
                {
                    seek_word          = seek_word + 1;  // Adjust for immediate value
                }
            }
            processflag                = TRUE;
            nextflag                   = TRUE;
            break;

        default:                // nothing else moves the CPU
            processflag                = FALSE;
            break;
    }
    lastaction                         = action;

    return (processflag);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// open_debug_file(): find and open the debug file that goes with a V32 file.
//
// With FLAG_SEARCH, 'datafile' is the V32 file (e.g. dir/game.v32) and the debug file
// is named game.vbin.debug (assembler) or game.asm.debug (C compiler). These
// directories are tried in order:
//
//     --debug-dir=DIR (if given)      next to the V32 file       DIR/obj
//     obj/ below the V32 file's dir   ../obj/ from the V32 file  obj/ and . (cwd)
//
// Without FLAG_SEARCH, 'datafile' is the debug file itself.
//
static FILE  *open_debug_file (const char *datafile, uint8_t flag, char *found, size_t foundlen)
{
    char        dir[1024];
    char        base[512];
    char        cand[2048];
    const char *suffix  = ((flag & FLAG_ASM) == FLAG_ASM) ? ".vbin.debug" : ".asm.debug";
    const char *slash   = NULL;
    const char *dot     = NULL;
    FILE       *fptr    = NULL;
    int         i       = 0;
    char        sub[6][1100];
    int         nsub    = 0;

    found[0]  = '\0';
    if ((flag & FLAG_SEARCH) != FLAG_SEARCH)
    {
        snprintf (found, foundlen, "%s", datafile);
        return (fopen (found, "r"));
    }

    slash  = strrchr (datafile, '/');
    if (slash != NULL)
    {
        snprintf (dir, sizeof (dir), "%.*s", (int) (slash - datafile), datafile);
        slash  = slash + 1;
    }
    else
    {
        snprintf (dir, sizeof (dir), ".");
        slash  = datafile;
    }
    snprintf (base, sizeof (base), "%s", slash);
    dot  = strrchr (base, '.');
    if (dot != NULL)
    {
        base[dot - base]  = '\0';      // strip the final extension only
    }

    if (debugdir != NULL)
    {
        snprintf (sub[nsub++], sizeof (sub[0]), "%s", debugdir);
    }
    snprintf (sub[nsub++], sizeof (sub[0]), "%s",        dir);
    snprintf (sub[nsub++], sizeof (sub[0]), "%s/obj",    dir);
    snprintf (sub[nsub++], sizeof (sub[0]), "%s/../obj", dir);
    snprintf (sub[nsub++], sizeof (sub[0]), "obj");
    snprintf (sub[nsub++], sizeof (sub[0]), ".");

    for (i = 0; i < nsub; i = i + 1)
    {
        snprintf (cand, sizeof (cand), "%s/%s%s", sub[i], base, suffix);
        fptr  = fopen (cand, "r");
        if (fptr != NULL)
        {
            snprintf (found, foundlen, "%s", cand);
            return (fptr);
        }
        fprintf (debug, "[load_labels] No debug file '%s' found.\n", cand);
    }
    return (NULL);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// next_field(): destructively split a comma separated line; returns the next field
// (possibly empty) or NULL at the end of the line
//
static char  *next_field (char **cursor)
{
    char  *start  = *cursor;
    char  *comma  = NULL;

    if (start == NULL)
    {
        return (NULL);
    }
    comma  = strchr (start, ',');
    if (comma != NULL)
    {
        *comma   = '\0';
        *cursor  = comma + 1;
    }
    else
    {
        *cursor  = NULL;
    }
    return (start);
}

static char  *copy_string (const char *text)
{
    size_t  len   = strlen (text) + 1;
    char   *copy  = (char *) ralloc (sizeof (char), len, FLAG_NONE);

    memcpy (copy, text, len);
    return (copy);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// load_labels(): read the label data of a debug file into the label list (lpoint).
//
// Vircon32 assembler .debug files (FLAG_ASM) look like:
//
//     0x10001AE4,obj/debuggerBIOS.asm,4654
//     0x10001AE6,obj/debuggerBIOS.asm,4656,__for_3281_continue
//
// and C compiler .debug files (FLAG_C) like:
//
//     obj/debuggerBIOS.asm,24,/PATH/TO/audio.h,40
//     obj/debuggerBIOS.asm,31,/PATH/TO/audio.h,49,get_selected_sound
//
// Returns the number of entries added.
//
uint32_t  load_labels (uint8_t *datafile, uint8_t  page, uint8_t  flag)
{
    FILE     *fptr                          = NULL;
    linked_l *ltmp                          = NULL;
    linked_l *ltmp2                         = NULL;
    uint32_t  tally                         = 0;
    char      found[2048];
    char      line[4096];
    char     *cursor                        = NULL;
    char     *f0                            = NULL;
    char     *f1                            = NULL;
    char     *f2                            = NULL;
    char     *f3                            = NULL;
    char     *f4                            = NULL;
    size_t    len                           = 0;

    if (datafile                           == NULL)
    {
        return (0);
    }

    fptr  = open_debug_file ((const char *) datafile, flag, found, sizeof (found));
    if (fptr                               == NULL)
    {
        fprintf (debug, "[load_labels] no debug file for '%s'\n", datafile);
        return (0);
    }
    fprintf (debug, "[load_labels] debugfile:    '%s'\n", found);
    {
        // debug-file names are relative to the build directory: remember where the
        // debug file and the V32 file live (and their parents) to resolve them
        char   root[2048];
        char  *slash  = NULL;
        int    k      = 0;
        const char *from[2]  = { found, (const char *) datafile };

        for (k = 0; k < 2; k = k + 1)
        {
            snprintf (root, sizeof (root), "%s", from[k]);
            slash  = strrchr (root, '/');
            if (slash == NULL)
            {
                continue;
            }
            *slash  = '\0';
            source_add_root (root);
            strncat (root, "/..", sizeof (root) - strlen (root) - 1);
            source_add_root (root);
        }
    }

    if (page                               == V32_PAGE_BIOS)
    {
        fprintf (verbose, "[BIOS] ");
    }
    else if (page                          == V32_PAGE_CART)
    {
        fprintf (verbose, "[CART] ");
    }
    fprintf (verbose, "LOADING DEBUGGING DATA FROM: %s\n", found);

    while (fgets (line, sizeof (line), fptr) != NULL)
    {
        len  = strlen (line);
        while ((len > 0) && ((line[len-1] == '\n') || (line[len-1] == '\r')))
        {
            line[len-1]  = '\0';
            len          = len - 1;
        }
        if (len == 0)
        {
            continue;
        }

        cursor  = line;
        f0      = next_field (&cursor);
        f1      = next_field (&cursor);
        f2      = next_field (&cursor);
        f3      = next_field (&cursor);
        f4      = next_field (&cursor);

        if ((flag & FLAG_ASM) == FLAG_ASM)
        {
            // f0 offset, f1 asm file, f2 asm line, f3 label (optional)
            if ((f0 == NULL) || (f1 == NULL) || (f2 == NULL))
            {
                fprintf (debug, "[load_labels] skipping malformed line '%s'\n", line);
                continue;
            }
            ltmp            = listnode (page, (uint32_t) strtoul (f0, NULL, 16));
            ltmp -> name    = copy_string (f1);
            ltmp -> NUMBER  = (uint32_t) strtoul (f2, NULL, 10);
            if ((f3 != NULL) && (*f3 != '\0'))
            {
                ltmp -> label  = copy_string (f3);
            }
            lpoint  = list_add (lpoint, ltmp);
            tally   = tally + 1;
        }
        else if ((flag & FLAG_C) == FLAG_C)
        {
            // f0 asm file, f1 asm line, f2 C file, f3 C line, f4 function (optional)
            if ((f0 == NULL) || (f1 == NULL) || (f2 == NULL) || (f3 == NULL))
            {
                fprintf (debug, "[load_labels] skipping malformed line '%s'\n", line);
                continue;
            }
            ltmp            = listnode (page, 0);
            ltmp -> name    = copy_string (f0);
            ltmp -> NUMBER  = (uint32_t) strtoul (f1, NULL, 10);
            ltmp -> cname   = copy_string (f2);
            ltmp -> LINE    = (uint32_t) strtoul (f3, NULL, 10);

            // find the matching assembler entry (same page, same asm line) for the offset
            for (ltmp2  = lpoint; ltmp2 != NULL; ltmp2 = ltmp2 -> next)
            {
                if ((ltmp2 -> NUMBER == ltmp -> NUMBER) &&
                    (ltmp2 -> type   == page)           &&
                    (ltmp2 -> cname  == NULL))
                {
                    ltmp -> RAW  = ltmp2 -> RAW;
                    break;
                }
            }
            lpoint  = list_add (lpoint, ltmp);
            tally   = tally + 1;
        }
    }
    fclose (fptr);

    return (tally);
}

void load_command(void)
{
    FILE     *fptr = NULL;
    uint8_t   input[1024];

    if (commandfile != NULL)
    {
        fptr = fopen(commandfile, "r");
        if (fptr == NULL)
        {
            fprintf(stderr, "[ERROR] Could not open '%s' for reading!\n", commandfile);
            exit(1);
        }

        while (fgets((char *)input, sizeof(input), fptr) != NULL)
        {
            // Remove trailing newline
            size_t len = strlen((char *)input);
            if (len > 0 && input[len-1] == '\n')
                input[len-1] = '\0';

            if (input[0] == '\0')
                continue;  // Skip empty lines

            // Parse (flex/bison) and execute the command
            process_command((char *)input);

            // Execute the action set by tokenize_input
            switch (action)
            {
                case INPUT_IGNORE:
                    ignoreflag = TRUE;   // skip the current instruction
                    // fall through
                case INPUT_CONTINUE:
                    runflag = TRUE;
                    nextflag = FALSE;
                    break;

                case INPUT_STEP:
                    // Will single-step once, then stop
                    break;

                case INPUT_NEXT:
                    next_tpoint = tpoint;
                    nextflag = TRUE;
                    break;

                // Display, break, watch, etc. are handled by process_command itself
                default:
                    break;
            }
        }
        commandfile = NULL;
        fclose(fptr);
    }
}
