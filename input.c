#include "defines.h"
#include <readline/readline.h>
#include <readline/history.h>

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
    uint8_t    deref_flag               = FALSE;
    uint8_t    processflag              = FALSE;
    //uint8_t    token_type               = PARSE_NONE;

    //if ((action                        != INPUT_NONE) &&
    //    (action                        != INPUT_INIT))
    //{
    //    displayshow  (dpoint, 0);
    //}

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Display the prompt and obtain input
    //
    if (input                          != NULL)
    {
        free (input);
    }

    input                               = get_input (stdin, "v32sim> ");
    
    if (*input                         == '\0')
    {
        action                          = lastaction;
    }

    //tokenize_asm   (input);

    if (action                         != INPUT_NONE)
    {
        //token_type                      = tokenize_input (input, &deref_flag);
        tokenize_input (input, &deref_flag);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // newcommand will be the character recently input; if newline,
    // repeat lastcommand
    //
    else
    {
        action                          = lastaction;
    }

    switch (action)
    {
        case INPUT_BREAK:
        case INPUT_LABEL:
        case INPUT_REPLACE:
        case INPUT_UNDO:
            processflag                 = FALSE;
            action                      = INPUT_INIT;
            break;

        case INPUT_SET:
            fprintf (stdout, "v32sim settings\n");
            fprintf (stdout, "===============\n");
            fprintf (stdout, "color:   %s\n", (colorflag  == TRUE)   ? "TRUE" : "FALSE");
            fprintf (stdout, "deref:   %s\n", (derefaddr  == TRUE)   ? "TRUE" : "FALSE");
            fprintf (stdout, "error:   %s\n", (errorcheck == TRUE)   ? "TRUE" : "FALSE");
            fprintf (stdout, "debug:   %s\n", (debug      == stderr) ? "TRUE" : "FALSE");
            fprintf (stdout, "verbose: %s\n", (verbose    == stderr) ? "TRUE" : "FALSE");
            processflag                 = FALSE;
            action                      = INPUT_INIT;
            break;

        case INPUT_CONTINUE:
        case INPUT_IGNORE:
            processflag                 = TRUE;
            runflag                     = TRUE;
            break;

        case INPUT_PRINT:
            lastaction                  = INPUT_NONE;
            break;

        case INPUT_DISPLAY:
            lastaction                 = INPUT_NONE;
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
            // if the instruction is a CALL, set seek_word and then
            // set runflag to TRUE; otherwise, should behave like step
            //
            value                      = (word & OPCODE_MASK) >> OPCODESHIFT;
            if (value                 == 0x03)
            {
                runflag                = TRUE;
                seek_word              = REG(IP) + 1;
                value                  = (word & IMMVAL_MASK);
                if (value             >  0)
                {
                    seek_word          = seek_word  + 1; // if immediate value
                }
            }
            processflag                = TRUE;
            break;

        case INPUT_HELP:
            help (INPUT_HELP);
            action                     = INPUT_INIT;
            break;
    }
    lastaction                         = action;

    return (processflag);
}

uint32_t  load_labels (uint8_t *filename)
{
    linked_l *ltmp                     = NULL;
    FILE     *fptr                     = NULL;
    int32_t   index                    = 0;
    int32_t   value                    = 0;
    uint32_t  offset                   = 0;
    uint32_t  line_number              = 0;
    uint32_t  tally                    = 0;
    uint8_t   input[128];
    uint8_t  *input_string             = NULL;
    uint8_t   token[128];

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Generate debug filename
    //
    strncpy (token, filename, strlen (filename));
    fprintf (debug, "[load_labels] filename:     %s\n", filename);
    fprintf (debug, "[load_labels] token:        %s\n", token);

    input_string                       = strtok (token, ".");
    fprintf (debug, "[load_labels] token:        %s\n", token);
    fprintf (debug, "[load_labels] input_string: %s\n", input_string);
    sprintf (input, "%s.vbin.debug", input_string);
    fprintf (debug, "[load_labels] input:        %s\n", input);
    
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Load labels from debug file (if it exists)
    //
    if (filename                      != NULL)
    {
        fptr                           = fopen (input, "r");
        if (fptr                      == NULL)
        {
            fprintf (verbose, "No debug file for '%s' found. Skipping...\n", input);
        }
        else
        {
            fprintf (verbose, "LOADING DEBUGGING DATA FOR: %s\n", filename);
            while (!feof (fptr))
            {
                index                  = 0;
                input[index]           = ' ';
                while (input[index]   != '\0')
                {
                    input[index]       = fgetc (fptr);
                    if (input[index]  == '\n')
                    {
                        input[index]   = '\0';
                        break;
                    }

                    if (feof (fptr))
                    {
                        break;
                    }
                    index              = index + 1;
                    input[index]       = ' ';
                }
            
                if (feof (fptr))
                {
                    break;
                }
                ////////////////////////////////////////////////////////////////////////
                //
                // Vircon32 assembler .debug files are of the following format:
                //
                // 0x10001AE4,obj/debuggerBIOS.asm,4654
                // 0x10001AE6,obj/debuggerBIOS.asm,4656,__for_3281_continue
                //
                // For label loading, we are specifically interested in the lines
                // with a label in the last field (second example)
                //
                input_string           = strtok (input, ","); // offset
                offset                 = strtol (input_string, NULL, 16);

                input_string           = strtok (NULL, ",");  // filename
                input_string           = strtok (NULL, ",");  // line number
                line_number            = strtol (input_string, NULL, 10);

                input_string           = strtok (NULL, ",");  // label, if present
                ltmp                   = listnode (LIST_MEM, offset);
                ltmp -> number         = line_number;
                if (input_string      != NULL)
                {
                    value              = sizeof (int8_t) * strlen (input_string) + 1;
                    ltmp -> label      = (int8_t *) malloc (value);
                    strcpy (ltmp -> label, input_string);
                }
                lpoint                 = list_add (lpoint, ltmp);
                tally                  = tally + 1;
            }
            fclose (fptr);
        }
    }

    return (tally);
}    

void load_command (void)
{
    FILE     *fptr                     = NULL;
    int32_t   index                    = 0;
    uint8_t   deref_flag               = FALSE;
    uint8_t   input[64];
    //uint8_t   token_type               = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Load commands from command-file
    //
    if (commandfile                   != NULL)
    {
        fptr                           = fopen (commandfile, "r");
        if (fptr                      == NULL)
        {
            fprintf (stderr, "[ERROR] Could not open '%s' for reading!\n", commandfile);
            exit (1);
        }

        while (!feof (fptr))
        {
            index                      = 0;
            token_label                = NULL;
            while (!feof (fptr))
            {
                input[index]           = fgetc (fptr);
                if (input[index]      == '\n')
                {
                    input[index]       = '\0';
                    break;
                }
                index                  = index + 1;
            }

            if (feof (fptr))
            {
                break;
            }
            //token_type                 = tokenize_input (input, &deref_flag);
            tokenize_input (input, &deref_flag);
        }
        commandfile                    = NULL;
        fclose (fptr);
    }
    action                             = INPUT_INIT;
}
