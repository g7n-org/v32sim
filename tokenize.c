#include "defines.h"
#include <regex.h>

uint8_t  tokenize_input (uint8_t *input, uint8_t *flag)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    data_t     *dtmp               = NULL;
    data_t      dval;
    linked_l  **list               = NULL;
    linked_l   *ltmp               = NULL;
    linked_l   *tmp                = NULL;
    int32_t     check              = 0;
    int32_t     count              = 0;
    int32_t     value              = 0;
    int32_t     index              = 0;
    int32_t     len                = 0;
    int32_t     offset             = REG(IP);
    int32_t     word               = 0xFFFFFFFF;
    int32_t     immv               = 0xFFFFFFFF;
    regex_t     regex;
    regmatch_t  match[5];
    uint8_t     byte               = 0;
    uint8_t     entry[24];
    uint8_t     lval[24];
    uint8_t     fmt                = FORMAT_DEFAULT;
    int8_t     *pos                = NULL;
    int8_t     *string             = NULL;
    int8_t     *token              = NULL;
    uint8_t   **form               = NULL;
    uint8_t    *form0              = "^ *([a-z?]+) *$";
    uint8_t    *form1              = "^ *([a-z]+) *(IP:0x[0-9A-F]{8})? *(IR:0x[0-9A-F]{8})? *(IV:0x[0-9A-F]{8})? *$";
    uint8_t    *form2              = "^ *([a-z]+) *([^ ]+) *= *([^ ]+) *$";
    uint8_t    *form3              = "^ *([a-z/]+) *([^ ]+) *([A-Z_][A-Z0-9_+-]*)? *$";

    uint8_t   **pattern            = NULL;
    uint8_t    *pattern0           = "^ *(R[0-9]|R1[0-5]|[BCDS][PR]|I[PRV]) *$";  // register
    uint8_t    *pattern1           = "^ *(reg|regs|register|registers|r[*]) *$";  // registers
    uint8_t    *pattern2           = "^ *(0x[0-9A-F]{8}) *$";                     // memory
    uint8_t    *pattern3           = "^ *(0x[0-9A-F]{8}) *- *(0x[0-9A-F]{8}) *$"; // memrange
    uint8_t    *pattern4           = "^ *(0x[0-7][01][0-9A-F]) *$";               // ioport
    uint8_t    *pattern5           = "^ *([CGIMRST][AEINP][GMPRU]_[A-Z]+) *$";    // ioport symbols
    uint8_t     result             = 0;

    fprintf (debug, "[tokenize_input] passed string: '%s'\n", input);

    string                         = parse_deref (input, flag);

    fprintf (debug, "[tokenize_input] processed string: '%s'\n", string);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // allocate and populate form array
    //
    form                           = (uint8_t **) ralloc (sizeof (uint8_t *), 4, FLAG_NONE);
    *(form+0)                      = form0;
    *(form+1)                      = form1;
    *(form+2)                      = form2;
    *(form+3)                      = form3;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // allocate and populate pattern array
    //
    pattern                        = (uint8_t **) ralloc (sizeof (uint8_t *), 6, FLAG_NONE);
    *(pattern+0)                   = pattern0;
    *(pattern+1)                   = pattern1;
    *(pattern+2)                   = pattern2;
    *(pattern+3)                   = pattern3;
    *(pattern+4)                   = pattern4;
    *(pattern+5)                   = pattern5;

    for (index                             = 0;
         index                            <  4;
         index                             = index + 1)
    {
        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx compilation: compile form pattern into our regex for processing
        //
        check                              = regcomp (&regex,
                                                     *(form+index),
                                                     REG_EXTENDED | REG_ICASE);
        if (check                         != 0)
        {
            fprintf (stderr, "[ERROR] Invalid pattern: RegEx compilation failed\n");
            exit    (REGEX_COMPILE_ERROR);
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx execution: make sure input conforms to provided pattern
        //
        check                              = regexec (&regex, string, 5, match, 0);
        if (check                         == REG_NOMATCH)
        {
            continue;
            fprintf (stderr, "ERROR: malformed input\n");
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx execution success! Display the results
        //
        else if (check                    == 0)
        {
            fprintf (debug, "[tokenize_input] MATCH on form %u\n", index);
            if (index                     == 0) // single-word command
            {
                byte                       = *(string + match[1].rm_so);
                if (byte                  == 'b')   // break
                {
                    tmp                    = bpoint;
                    count                  = 0;
                    while (tmp            != NULL)
                    {
                        fprintf (stdout, "[%u] 0x%.8X\n", count, tmp -> data.raw);
                        tmp                = tmp -> next;
                        count              = count + 1;
                    }
                    action                 = INPUT_BREAK;
                }
                else if (byte             == 'c')   // continue
                {
                    action                 = INPUT_CONTINUE;
                }
                else if (byte             == 'd')   // display
                {
                    action                 = INPUT_DISPLAY;
                }
                else if (byte             == 'g')   // gamepad
                {
                    action                 = INPUT_GAMEPAD;
                    display_config (action);
                }
                else if (byte             == 'i')   // ignore
                {
                    if (0                 == strncasecmp ((string+match[1].rm_so), "in", 2))
                    {
                        action             = INPUT_INVENTORY;
                        display_config (action);
                    }
                    else
                    {
                        action             = INPUT_IGNORE;
                    }
                }
                else if (byte             == 'l')   // labels
                {
                    ltmp                   = lpoint;
                    count                  = 0;
                    while (ltmp           != NULL)
                    {
                        if ((ltmp -> label != NULL) &&
                            (modeflag      == FLAG_ASM))
                        {
                            fprintf (stdout, "[%u] %s -> 0x%.8X\n",
                                    count, ltmp -> label, ltmp -> data.raw);
                        }
                        else if ((ltmp -> cname != NULL) &&
                                 (modeflag == FLAG_C))
                        {
                            fprintf (stdout, "[%u] ASM:%u -> C:%s:%u -> offset:0x%.8X\n",
                                    count, ltmp -> number, ltmp -> cname, ltmp -> line, ltmp -> data.raw);
                        }
                        ltmp               = ltmp -> next;
                        count              = count + 1;
                    }
                    action                 = INPUT_LABEL;
                }
                else if (byte             == 'n')   // next
                {
                    action                 = INPUT_NEXT;
                }
                else if (byte             == 's')   // step
                {
                    token                  = (string + match[1].rm_so);
                    if (0                 == strncasecmp (token, "se", 2))
                    {
                        action             = INPUT_SET;
                    }
                    else if (0            == strncasecmp (token, "s",  1))
                    {
                        action             = INPUT_STEP;
                    }
                }
                else if (byte             == 'h')   // help
                {
                    action                 = INPUT_HELP;
                }
                else if (byte             == '?') // help
                {
                    action                 = INPUT_HELP;
                }
                else if (byte             == 'q') // quit
                {
                    action                 = INPUT_QUIT;
                }
            }
            else if (index                == 1) // replace-style command
            {
                byte                       = *(string + match[1].rm_so);
                if (byte                  == 'r') // replace
                {
                    action                 = INPUT_REPLACE;
                    fprintf (debug, "[tokenize_input] replacing ");
                    for (count             = 2;
                         count            <  5;
                         count             = count + 1)
                    {
                        if (-1            == match[count].rm_so)
                        {
                            break;
                        }

                        len                = (match[count].rm_eo-match[count].rm_so);
                        pos                = (string + match[count].rm_so + 1);
                        snprintf (entry, len, "%.*s", len, pos);
                        byte               = *(pos);
                        token              = strtok (entry,  ":");
                        token              = strtok (NULL,   ":");
                        if (byte          == 'P')
                        {
                            offset         = strtol (token, NULL, 16);
                            fprintf (debug, "IP:0x%.8X ", REG(IP));
                        }
                        else if (byte     == 'R')
                        {
                            fprintf (debug, "IR:0x%.8X ", IMEMGET(offset));
                            word           = strtol (token, NULL, 16);
                            fprintf (debug, "with IR:0x%.8X ", word);
                            SYSMEMSET(offset, word);
                        }
                        else if (byte     == 'V')
                        {
                            if (0         <  (word & IMMVAL_MASK))
                            {
                                fprintf (debug, "IV:0x%.8X ", IMEMGET(offset+1));
                                immv       = strtol (token, NULL, 16);
                                fprintf (debug, "with IV:0x%.8X ", immv);
                                SYSMEMSET(offset+1, immv);
                            }
                        }
                    }
                    fprintf (debug, "\n");
                }
            }
            else if (index                == 2) // assign-style command
            {
                //action                     = INPUT_SET;
                len                        = (match[2].rm_eo - match[2].rm_so);
                pos                        = (string + match[2].rm_so);
                snprintf (lval, (len+1), "%.*s", (len+1), pos);

                len                        = (match[3].rm_eo - match[3].rm_so);
                pos                        = (string + match[3].rm_so);
                snprintf (entry, (len+1), "%.*s", (len+1), pos);
                fprintf (debug, "[lval]  '%s'\n", lval);
                fprintf (debug, "[entry] '%s'\n", entry);
                fprintf (debug, "[token] '%s'\n", token);

                parse_imm (entry, &dval); // maybe check if it is legit immediate data?

                ////////////////////////////////////////////////////////////////////////
                //
                // Check if we are setting a register of any sort
                //
                result                     = parse_token (lval,
                                                          *(pattern+0),
                                                          PARSE_REGISTER);
                if (result                == PARSE_REGISTER)
                {
                    result                 = parse_reg (lval);
                    if (dval.fmt          == FORMAT_FLOAT)
                    {
                        FREG(result)       = dval.value.f32;
                        fprintf (debug, "[%s] setting to %.2f\n",
                                        REGNAME(result), FREG(result));
                    }
                    else
                    {
                        REG(result)        = dval.value.i32;
                        fprintf (debug, "[%s] setting to 0x%.8X\n",
                                        REGNAME(result), REG(result));
                    }

                    ////////////////////////////////////////////////////////////////////
                    //
                    // If setting IP, adjust IR and (if needed) IV as well.
                    //
                    if (result            == IP)
                    {
                        REG(IR)            = IMEMGET(REG(IP));
                        if (0             <  (REG(IR) & IMMVAL_MASK))
                        {
                            REG(IV)        = IMEMGET(REG(IP)+1);
                        }
                        else
                        {
                            REG(IV)        = 0;
                        }
                    }
                }

                ////////////////////////////////////////////////////////////////////////
                //
                // Check if we are setting a memory address
                //
                result                     = parse_token (lval,
                                                          *(pattern+2),
                                                          PARSE_MEMORY);
                if (result                == PARSE_MEMORY)
                {
                    count                  = strtol (lval,  NULL, 16);
                    SYSMEMSET(count, dval.value.raw);
                }

                ////////////////////////////////////////////////////////////////////////
                //
                // Check if we are setting an IOPort
                //
                result                     = parse_token (lval,
                                                          *(pattern+4),
                                                          PARSE_IOPORT);
                if (result                == PARSE_IOPORT)
                {
                    count                  = strtol (lval,  NULL, 16);
                    SYSPORTSET(count, dval.value.raw);
                }

                ////////////////////////////////////////////////////////////////////////
                //
                // Check if we are setting an IOPort via symbolic name
                //
                result                     = parse_token (lval,
                                                          *(pattern+5),
                                                          PARSE_IOPORT);
                if (result                == PARSE_IOPORT)
                {
                    count                  = ioports_num (lval);
                    fprintf (debug, "[set] Setting %s (0x%.3X) to 0x%.8X\n",
                                    lval, count, dval.value.raw);
                    SYSPORTSET(count, dval.value.raw);
                }

                else if ((lval[0]         == 'C') ||
                         (lval[0]         == 'c'))
                {
                    check                  = strncasecmp (entry, "true", 4);
                    fprintf (debug, "[color]: %s\n", entry);
                    if (check             == 0)
                    {    
                        colorflag          = TRUE;
                    }
                    else
                    {
                        colorflag          = FALSE;
                    }
                }
                else if ((lval[0]         == 'E') ||
                         (lval[0]         == 'e'))
                {
                    check                  = strncasecmp (entry, "true", 4);
                    if (check             == 0)
                    {    
                        errorcheck         = TRUE;
                        fprintf (verbose, "errorchk ENABLED!\n");
                    }
                    else
                    {
                        errorcheck         = FALSE;
                        fprintf (verbose, "errorchk DISABLED!\n");
                    }
                }
                else if ((lval[0]         == 'D') ||
                         (lval[0]         == 'd'))
                {
                    check                  = strncasecmp (lval,  "debug", 5);
                    if (check             == 0)
                    {
                        check              = strncasecmp (entry, "true", 4);
                        if (check         == 0)
                        {    
                            debug          = stderr;
                            fprintf (debug, "[set] debugging ENABLED!\n");
                        }
                        else
                        {
                            fprintf (debug, "[set] disabling debugging!\n");
                            debug          = devnull;
                        }
                    }

                    check                  = strncasecmp (lval,  "deref", 5);
                    if (check             == 0)
                    {
                        check              = strncasecmp (entry, "true", 4);
                        if (check         == 0)
                        {    
                            derefaddr      = TRUE;
                            fprintf (debug, "[set] derefaddr ENABLED!\n");
                        }
                        else
                        {
                            derefaddr      = FALSE;
                            fprintf (debug, "[set] derefaddr DISABLED!\n");
                        }
                    }
                }
                else if ((lval[0]         == 'V') ||
                         (lval[0]         == 'v'))
                {
                    check                  = strncasecmp (lval,  "verbo", 5);
                    if (check             == 0)
                    {
                        check              = strncasecmp (entry, "true", 4);
                        if (check         == 0)
                        {    
                            verbose        = stderr;
                            fprintf (verbose, "verbosity ENABLED!\n");
                        }
                        else
                        {
                            fprintf (verbose, "disabling verbosity!\n");
                            debug          = devnull;
                        }
                    }
                }
            }
            else if (index                == 3) // parametered command
            {
                byte                       = *(string + match[1].rm_so);
                fprintf (debug, "[debug] data is: '%s', byte is: '%c'\n", (string + match[1].rm_so), byte);
                if (byte                  == 'b') // break
                {
                    fprintf (debug, "BREAK encountered\n");
                    action                 = INPUT_BREAK;
                    token                  = strtok ((string + match[2].rm_so), " ");
                    result                 = parse_token (token, *(pattern+2), PARSE_MEMORY);
                    if (result            != PARSE_NONE) // memory address
                    {
                        fprintf (debug, "BREAK adding the offset '%s'\n", token);
                        value              = strtol (token, NULL, 16);
                        tmp                = listnode (LIST_MEM, value);
                        bpoint             = list_add (bpoint, tmp);
                    }
                    else // not a memory address (assuming label)
                    {
                        ltmp               = lpoint;
                        while (ltmp       != NULL)
                        {
                            if (ltmp -> label != NULL)
                            {
                                check          = strncasecmp (ltmp -> label, token, strlen (ltmp -> label));
                                if (check     == 0) // existing label found in list
                                {
                                    fprintf (debug, "BREAK adding the label '%s'\n", ltmp -> label);
                                    fprintf (debug, "adding 0x%.8X to the list\n", ltmp -> data.i32);
                                    tmp        = listnode (LIST_MEM, ltmp -> data.i32);
                                    bpoint     = list_add (bpoint, tmp);
                                }
                            }
                            ltmp       = ltmp -> next;
                        }
                    }
                }
                else if (byte         == 'd') // display
                {
                    action             = INPUT_DISPLAY;
                    fprintf (debug, "(string+match[1].rm_so): %s\n", (string+match[1].rm_so));
                    strcpy (entry, (string+match[1].rm_so));
                    fprintf (debug, "[before] entry: %s\n", entry);
                    token              = strtok (entry, " ");
                    fprintf (debug, "[after]  entry: %s\n", entry);
                    token              = strtok (entry,  "/");
                    fprintf (debug, "[after]  token: %s\n", token);
                    pos                = strtok (NULL,   "/");
                    fprintf (debug, "pos:            %s\n", pos);
                    if ((pos          != NULL) &&
                        (*(pos+0)     != '\0'))
                    {
                        if (*(pos+0)  == 'b')
                        {
                            fmt        = FORMAT_BINARY;
                        }
                        else if (*pos == 'B')
                        {
                            fmt        = FORMAT_BOOLEAN;
                        }
                        else if (*pos == 'd')
                        {
                            fmt        = FORMAT_SIGNED;
                        }
                        else if (*pos == 'D')
                        {
                            fmt        = FORMAT_DECODE;
                        }
                        else if (*pos == 'f')
                        {
                            fmt        = FORMAT_FLOAT;
                        }
                        else if (*pos == 'o')
                        {
                            fmt        = FORMAT_OCTAL;
                        }
                        else if (*pos == 's')
                        {
                            fmt        = FORMAT_STRING;
                        }
                        else if (*pos == 'u')
                        {
                            fmt        = FORMAT_UNSIGNED;
                        }
                        else if (*pos == 'x')
                        {
                            fmt        = FORMAT_LOWERHEX;
                        }
                        else if (*pos == 'X')
                        {
                            fmt        = FORMAT_HEX;
                        }
                    }
                    token              = strtok ((string + match[2].rm_so), " ");
                    for (count         = (PARSE_REGISTER - 0x7A);
                         count        <= (PARSE_IOPORT   - 0x7A) + 1;
                         count         = count + 1)
                    {
                        fprintf (debug,  "[tokenize_input] count: 0x%.2X, pattern: 0x%.2X\n",
                                count, (count-0x7A));

                        ////////////////////////////////////////////////////////////////
                        //
                        // PARSE_REGISTER          0x7A
                        // PARSE_REGISTERS         0x7B
                        // PARSE_MEMORY            0x7C
                        // PARSE_MEMRANGE          0x7D
                        // PARSE_IOPORT            0x7E
                        // PARSE_NONE              0x7F
                        //
                        value          = 0;
                        if (count     == ((PARSE_IOPORT - 0x7A) + 1))
                        {
                            value      = 1;
                        }
                        result         = parse_token (token,
                                                      *(pattern+count),
                                                      ((count-value) + 0x7A));
                        if (result    != PARSE_NONE)
                        {
                            break;
                        }
                    }
                    token_label                = strtok (NULL, " ");

                    switch (result)
                    {
                        case PARSE_NONE:
                            fprintf (stderr, "[ERROR] malformed display value\n");
                            break;

                        case PARSE_MEMRANGE:
                            ////////////////////////////////////////////////////////////
                            //
                            // obtain the two range extents
                            //
                            pos                    = strtok (token, "-");
                            value                  = strtol (pos,   NULL, 16);
                            pos                    = strtok (NULL,  "-");
                            check                  = strtol (pos,   NULL, 16);

                            ////////////////////////////////////////////////////////////
                            //
                            // check that we have a valid range, within reason
                            //
                            if ((value            >  check) ||
                                ((check - value)  >  16))
                            {
                                break;
                            }

                            ////////////////////////////////////////////////////////////
                            //
                            // add each address in the range to the displaylist
                            //
                            for (index             = value;
                                 index            <= check;
                                 index             = index + 1)
                            {
                                if (*flag         == TRUE)
                                {
                                    tmp            = listnode (LIST_MEM_DEREF, index);
                                }
                                else
                                {
                                    tmp            = listnode (LIST_MEM,       index);
                                }

                                if (token_label   != NULL)
                                {
                                    tmp -> label   = (int8_t *) ralloc (sizeof  (int8_t),
                                                                        strlen (token_label) + 1,
                                                                        FLAG_NONE);
                                    strcpy (tmp -> label, token_label);
                                }
                                tmp -> fmt         = fmt;
                                dpoint             = list_add (dpoint, tmp);
                            }
                            break;

                        case PARSE_MEMORY:
                            value              = strtol (token, NULL, 16);
                            if (*flag         == TRUE)
                            {
                                tmp            = listnode (LIST_MEM_DEREF, value);
                            }
                            else
                            {
                                tmp            = listnode (LIST_MEM,       value);
                            }

                            if (token_label   != NULL)
                            {
                                tmp -> label   = (int8_t *) ralloc (sizeof (int8_t),
                                                                    strlen (token_label) + 1,
                                                                    FLAG_NONE);
                                strcpy (tmp -> label, token_label);
                            }
                            tmp -> fmt         = fmt;
                            dpoint             = list_add (dpoint, tmp);
                            break;

                        case PARSE_REGISTERS:
                            for (index         = 0;
                                 index        <= 15;
                                 index         = index + 1)
                            {
                                tmp            = listnode (LIST_REG, index);
                                tmp -> fmt     = fmt;
                                dpoint         = list_add (dpoint, tmp);
                            }
                            break;

                        case PARSE_REGISTER: // specific, general register
                            result             = parse_reg (token);
                            if (result        >  15)
                            {
                                sys_reg_show   = TRUE;
                                break;
                            }

                            if (*flag         == TRUE)
                            {
                                tmp            = listnode (LIST_REG_DEREF, result);
                            }
                            else
                            {
                                tmp            = listnode (LIST_REG,       result);
                            }

                            if (token_label   != NULL)
                            {
                                tmp -> label   = (int8_t *) ralloc (sizeof (int8_t),
                                                                    strlen (token_label) + 1,
                                                                    FLAG_NONE);
                                strcpy (tmp -> label, token_label);
                            }
                            tmp -> fmt         = fmt;
                            dpoint             = list_add (dpoint, tmp);
                            break;

                        case PARSE_IOPORT:
                            ////////////////////////////////////////////////////////////
                            //
                            // determine if the hex value or symbolic name was given
                            //
                            value              = ioports_num (token);
                            if (value         == -1)
                            {
                                value          = strtol (token, NULL, 16);
                            }
                            fprintf (debug, "[display/iop] 0x%.3hX was specified\n", value);
                            tmp                = listnode (LIST_IOP, value);

                            if (token_label   == NULL)
                            {
                                dtmp           = ioports_ptr (value);
                                token_label    = dtmp -> name;
                                if (fmt       != FORMAT_DEFAULT)
                                {
                                    tmp -> fmt = fmt;
                                }
                                else
                                {
                                    tmp -> fmt = dtmp -> fmt;
                                }
                            }

                            tmp -> label       = (int8_t *) ralloc (sizeof (int8_t),
                                                                    strlen (token_label) + 1,
                                                                     FLAG_NONE);
                            strcpy (tmp -> label, token_label);
                            dpoint             = list_add (dpoint, tmp);
                            break;
                    }
                }
                else if (byte         == 'g') // gamepad
                {
                    ////////////////////////////////////////////////////////////////////
                    //
                    // determine which gamepad was specified (0-3)
                    //
                    // example: gamepad0
                    //
                    len                = 2;
                    token              = (string + match[len].rm_so);
                    fprintf (debug, "[gamepad] token: %s\n", token);
                    if ((token[0]     >= '0') &&
                        (token[0]     <= '3'))
                    {
                        value          = atoi (token);
                        len            = len + 1;
                    }
                    else
                    {
                        value          = IPORTGET(INP_SelectedGamepad);
                    }

                    token              = (string + match[len].rm_so);
                    
                    /*
    int32_t     check              = 0;
    int32_t     count              = 0;
    int32_t     value              = 0;
    int32_t     index              = 0;
    int32_t     len                = 0;
    */

                    ////////////////////////////////////////////////////////////////////
                    //
                    // determine which gamepad command is issued
                    //
                    //token              = strtok ((string + match[2].rm_so), " ");
                    //token              = (string + match[3].rm_so);
                    fprintf (verbose, "[gamepad%d] %s\n", value, token);
                    if (0             == strncasecmp (token, "se", 2)) // select
                    {
                        SYSPORTSET(INP_SelectedGamepad, value); 
                    }
                    else if (0        == strncasecmp (token, "c",  1)) // connect
                    {
                        SYSPORTSET(INP_GamepadConnected, 1); 
                    }
                    else if (0        == strncasecmp (token, "di", 2)) // disconnect
                    {
                        SYSPORTSET(INP_GamepadConnected, 0); 
                    }
                    else if (0        == strncasecmp (token, "le", 2)) // left
                    {
                        gamepad_io (INP_GamepadLeft);
                    }
                    else if (0        == strncasecmp (token, "ri", 2)) // right
                    {
                        gamepad_io (INP_GamepadRight);
                    }
                    else if (0        == strncasecmp (token, "u",  1)) // up
                    {
                        gamepad_io (INP_GamepadUp);
                    }
                    else if (0        == strncasecmp (token, "d",  1)) // down
                    {
                        gamepad_io (INP_GamepadDown);
                    }
                    else if (0        == strncasecmp (token, "S",  1)) // START
                    {
                        gamepad_io (INP_GamepadButtonStart);
                    }
                    else if (0        == strncasecmp (token, "A",  1)) // A
                    {
                        gamepad_io (INP_GamepadButtonA);
                    }
                    else if (0        == strncasecmp (token, "B",  1)) // B
                    {
                        gamepad_io (INP_GamepadButtonB);
                    }
                    else if (0        == strncasecmp (token, "X",  1)) // X
                    {
                        gamepad_io (INP_GamepadButtonX);
                    }
                    else if (0        == strncasecmp (token, "Y",  1)) // Y
                    {
                        gamepad_io (INP_GamepadButtonY);
                    }
                    else if (0        == strncasecmp (token, "L",  1)) // L
                    {
                        gamepad_io (INP_GamepadButtonL);
                    }
                    else if (0        == strncasecmp (token, "R",  1)) // R
                    {
                        gamepad_io (INP_GamepadButtonR);
                    }
                }
                else if (byte         == 'h') // help
                {
                    token              = strtok ((string + match[2].rm_so), " ");
                    if (0             == strncasecmp (token, "b", 1))
                    {
                        help (INPUT_BREAK);
                        action         = INPUT_INIT;
                    }
                    else if (0        == strncasecmp (token, "c", 1))
                    {
                        help (INPUT_CONTINUE);
                        action         = INPUT_INIT;
                    }
                    else if (0        == strncasecmp (token, "d", 1))
                    {
                        help (INPUT_DISPLAY);
                        action         = INPUT_INIT;
                    }
                    else if (0        == strncasecmp (token, "g", 1))
                    {
                        help (INPUT_GAMEPAD);
                        action         = INPUT_INIT;
                    }
                    else if (0        == strncasecmp (token, "ig", 2))
                    {
                        help (INPUT_IGNORE);
                        action         = INPUT_INIT;
                    }
                    else if (0        == strncasecmp (token, "in", 2))
                    {
                        help (INPUT_INVENTORY);
                        action         = INPUT_INIT;
                    }
                    else if (0        == strncasecmp (token, "la", 2))
                    {
                        help (INPUT_LABEL);
                        action         = INPUT_INIT;
                    }
                    else if (0        == strncasecmp (token, "lo", 2))
                    {
                        help (INPUT_LOAD);
                        action         = INPUT_INIT;
                    }
                    else if (0        == strncasecmp (token, "n", 1))
                    {
                        help (INPUT_NEXT);
                        action         = INPUT_INIT;
                    }
                    else if (0        == strncasecmp (token, "p", 1))
                    {
                        help (INPUT_PRINT);
                        action         = INPUT_INIT;
                    }
                    else if (0        == strncasecmp (token, "r", 1))
                    {
                        help (INPUT_REPLACE);
                        action         = INPUT_INIT;
                    }
                    else if (0        == strncasecmp (token, "se", 2))
                    {
                        help (INPUT_SET);
                        action         = INPUT_INIT;
                    }
                    else if (0        == strncasecmp (token, "st", 2))
                    {
                        help (INPUT_STEP);
                        action         = INPUT_INIT;
                    }
                    else if (0        == strncasecmp (token, "unlo", 4))
                    {
                        help (INPUT_UNLOAD);
                        action         = INPUT_INIT;
                    }
                    else if (0        == strncasecmp (token, "un", 2))
                    {
                        help (INPUT_UNDO);
                        action         = INPUT_INIT;
                    }
                }
                else if (byte         == 'l') // label
                {
                    if (0             == strncasecmp ((string+match[1].rm_so), "la", 2))
                    {
                        action         = INPUT_LABEL;
                        token          = strtok ((string + match[2].rm_so), " ");
                        result         = parse_token (token, *(pattern+2), PARSE_MEMORY);
                        value          = strtol (token, NULL, 16);
                        ltmp           = listnode (LIST_MEM, value);
                        token_label    = strtok ((string + match[3].rm_so), " ");
                        ltmp -> label  = (int8_t *) ralloc (sizeof (int8_t),
                                                              strlen (token_label) + 1,
                                                            FLAG_NONE);
                        strcpy (ltmp -> label, token_label);
                        lpoint         = list_add (lpoint, ltmp);
                    }
                    else if (0        == strncasecmp ((string+match[1].rm_so), "lo", 2))
                    {
                        action         = INPUT_LOAD;
                        token          = strtok ((string + match[2].rm_so), ":");
                        fprintf (debug, "[load] page token: '%s'\n", token);
                        if (0         == strncasecmp (token, "bios", 4))
                        {
                            token      = strtok (NULL, ":");
                            fprintf (debug, "[load] BIOS filename: '%s'\n", token);
                        }
                        else if (0    == strncasecmp (token, "cart", 4))
                        {
                            token      = strtok (NULL, ":");
                            fprintf (debug, "[load] CART filename: '%s'\n", token);
                            load_memory (V32_PAGE_CART, token);
                        }
                        else if (0    == strncasecmp (token, "memc", 4))
                        {
                            token      = strtok (NULL, ":");
                            fprintf (debug, "[load] MEMC filename: '%s'\n", token);
                            load_memory (V32_PAGE_MEMC, token);
                        }
                        else
                        {
                            fprintf (debug, "[load] OTHER: '%s'\n", token);
                        }
                    }
                }
                else if (byte         == 'p') // print
                {
                    action             = INPUT_PRINT;
                    fprintf (debug, "(string+match[1].rm_so): %s\n", (string+match[1].rm_so));
                    strcpy (entry, (string+match[1].rm_so));
                    fprintf (debug, "[before] entry: %s\n", entry);
                    token              = strtok (entry, " ");
                    fprintf (debug, "[after]  entry: %s\n", entry);
                    token              = strtok (entry,  "/");
                    fprintf (debug, "[after]  token: %s\n", token);
                    pos                = strtok (NULL,   "/");
                    fprintf (debug, "pos:            %s\n", pos);
                    if ((pos          != NULL) &&
                        (*(pos+0)          != '\0'))
                    {
                        if (*(pos+0)      == 'b')
                        {
                            fmt            = FORMAT_BINARY;
                        }
                        else if (*pos     == 'B')
                        {
                            fmt            = FORMAT_BOOLEAN;
                        }
                        else if (*pos     == 'd')
                        {
                            fmt            = FORMAT_SIGNED;
                        }
                        else if (*pos     == 'D')
                        {
                            fmt            = FORMAT_DECODE;
                        }
                        else if (*pos     == 'f')
                        {
                            fmt            = FORMAT_FLOAT;
                        }
                        else if (*pos     == 'o')
                        {
                            fmt            = FORMAT_OCTAL;
                        }
                        else if (*pos     == 's')
                        {
                            fmt            = FORMAT_STRING;
                        }
                        else if (*pos     == 'u')
                        {
                            fmt            = FORMAT_UNSIGNED;
                        }
                        else if (*pos     == 'x')
                        {
                            fmt            = FORMAT_LOWERHEX;
                        }
                        else if (*pos     == 'X')
                        {
                            fmt            = FORMAT_HEX;
                        }
                    }
                    token              = strtok ((string + match[2].rm_so), " "); // move to next token

                    fprintf (debug, "[print] parsing: token: %s, fmt: %u\n", token, fmt);
                    for (count      = (PARSE_REGISTER - 0x7A);
                         count     <= (PARSE_IOPORT   - 0x7A) + 1;
                         count      = count + 1)
                    {
                        ////////////////////////////////////////////////////////////////
                        //
                        // PARSE_REGISTER          0x7A
                        // PARSE_REGISTERS         0x7B
                        // PARSE_MEMORY            0x7C
                        // PARSE_MEMRANGE          0x7D
                        // PARSE_IOPORT            0x7E
                        // PARSE_NONE              0x7F
                        //
                        value       = 0;
                        if (count  == ((PARSE_IOPORT - 0x7A) + 1))
                        {
                            value   = 1;
                        }
                        result      = parse_token (token,
                                                   *(pattern+count),
                                                   ((count-value) + 0x7A));
                        if (result != PARSE_NONE)
                        {
                            break;
                        }
                    }

                    switch (result)
                    {
                        case PARSE_NONE:
                            fprintf (stderr, "[ERROR] malformed display value\n");
                            break;

                        case PARSE_MEMRANGE:
                            pos                 = strtok (token, "-");
                            value               = strtol (pos,   NULL, 16);
                            pos                 = strtok (NULL,  "-");
                            check               = strtol (pos,   NULL, 16);

                            ////////////////////////////////////////////////////////////
                            //
                            // check that we have a valid range, within reason
                            //
                            if ((value            >  check) ||
                                ((check - value)  >  16))
                            {
                                break;
                            }

                            for (index          = value;
                                 index         <= check;
                                 index          = index + 1)
                            {
                                output_mem (index, fmt, *flag, NULL);
                            }
                            break;

                        case PARSE_MEMORY:
                            value               = strtol (token, NULL, 16);
                            fprintf (debug, "[input/print] token: '%s', value: 0x%.8X\n", token, value);
                            output_mem (value, fmt, *flag, NULL);
                            break;

                        case PARSE_REGISTERS:
                            for (index          = 0;
                                 index         <= 15;
                                 index          = index + 1)
                            {
                                output_reg (index, fmt, *flag, NULL);
                            }
                            break;

                        case PARSE_REGISTER: // specific, general register
                            result              = parse_reg (token);
                            if (result         <  NUM_REGISTERS)
                            {
                                output_reg (result, fmt, *flag, NULL);
                            }
                            break;

                        case PARSE_IOPORT:
                            ////////////////////////////////////////////////////////////
                            //
                            // determine if the hex value or symbolic name was given
                            //
                            value              = ioports_num (token);
                            if (value         == -1)
                            {
                                value          = strtol (token, NULL, 16);
                            }
                            fprintf (debug, "[print/iop] 0x%.3hX was specified\n", value);
                            dtmp                = ioports_ptr (value);
                            output_iop (value, dtmp -> fmt, dtmp -> name);
                            break;
                    }
                }
                else if (byte         == 'u') // un-something
                {
                    if (0             == strncasecmp ((string+match[1].rm_so), "unlo", 4))
                    {
                        action         = INPUT_UNLOAD;
                        token          = (string + match[2].rm_so);
                        fprintf (debug, "[unload] token: '%s'\n", token);
                        if (0         == strncasecmp (token, "bios", 4))
                        {
                            fprintf (verbose, "[unload] UNLOADING BIOS\n");
                            unload_memory (V32_PAGE_BIOS);
                        }
                        else if (0    == strncasecmp (token, "cart", 4))
                        {
                            fprintf (verbose, "[unload] UNLOADING CART\n");
                            unload_memory (V32_PAGE_CART);
                        }
                        else if (0    == strncasecmp (token, "memc", 4))
                        {
                            fprintf (verbose, "[unload] UNLOADING MEMCARD\n");
                            unload_memory (V32_PAGE_MEMC);
                        }
                        else
                        {
                            fprintf (debug, "[unload] OTHER: '%s'\n", token);
                        }
                    }
                    else
                    {
                        action         = INPUT_UNDO;
                        token          = strtok ((string + match[2].rm_so), " ");

                        value          = strtol (token, NULL, 10);
                        byte           = *(string + match[1].rm_so + 2);
                        if (byte      == 'b')
                        {
                            list       = &bpoint;
                        }
                        else if (byte == 'd')
                        {
                            list       = &dpoint;
                        }
                        else if (byte == 'l')
                        {
                            list       = &lpoint;
                        }

                        count          = 0;
                        tmp            = *list;
                        while (tmp    != NULL)
                        {
                            if (count == value)
                            {
                                break;
                            }
                            count      = count + 1;
                            tmp        = tmp -> next;
                        }

                        tmp            = list_grab (list, tmp);
                        if (tmp       != NULL)
                        {
                            free (tmp);
                            tmp        = NULL;
                        }
                    }
                }
            }

            for (count          = 0;
                 count         <  5;
                 count          = count + 1)
            {
                fprintf (debug, "match[%d]: %.*s (%lld - %lld)\n",
                                count,
                                (int) (match[count].rm_eo - match[count].rm_so),
                                (string + match[count].rm_so),
                                (long long int) match[count].rm_so,
                                (long long int) match[count].rm_eo);
            }
            regfree (&regex);
            break;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // RegEx execution error: bail out
        //
        else
        {
            fprintf (stderr, "[ERROR] RegEx execution failed\n");
            exit    (REGEX_EXECUTE_ERROR);
        }

        regfree (&regex);
    }

    rfree   (form);
    rfree   (pattern);

    return  (result);
}
