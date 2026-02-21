#include "defines.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <regex.h>

uint8_t  tokenize_input (uint8_t *string)
{
    ////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    int32_t     index      = 0;
    int32_t     result     = 0;
    regex_t     regex;
    regmatch_t  match[4];
    uint8_t    *pattern    = "^ *([a-zA-Z][a-zA-Z2]{1,4}) +([rR][0-9]|[rR]1[0-5]|[sSbBcCdDsS][pPrR]|0x[0-9a-fA-F]{1,8}|[0-9]|[1-9][0-9]+) *, +([rR][0-9]|[rR]1[0-5]|[sSbBcCdDsS][pPrR]|0x[0-9a-fA-F]{1,8}|[0-9]|[1-9][0-9]+) *$";

    ////////////////////////////////////////////////////////////////////////////////
    //
    // RegEx compilation: compile pattern into our regex for processing
    //
    result                 = regcomp (&regex, pattern, REG_EXTENDED);
    if (result            != 0)
    {
        fprintf (stderr, "[ERROR] RegEx compilation failed\n");
        exit    (REGEX_COMPILE_ERROR);
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // RegEx execution: make sure input conforms to provided pattern
    //
    result                 = regexec (&regex, string, 4, match, 0);
    if (result            == REG_NOMATCH)
    {
        fprintf (stderr, "ERROR: malformed input\n");
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // RegEx execution success! Display the results
    //
    else if (result       == 0)
    {
        for (index         = 0;
             index        <  4;
             index         = index + 1)
        {
            fprintf (stdout, "match[%d]: %.*s (%lld - %lld)\n",
                             index,
                             (int) (match[index].rm_eo - match[index].rm_so),
                             (string + match[index].rm_so),
                             (long long int) match[index].rm_so,
                             (long long int) match[index].rm_eo);
        }
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

    return (result);
}

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
    // obtain input (via readline)
    //
    input_string           = readline (prompt);

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
    fprintf (verbose, "[get_input] input_string: \"%s\" (input_length: %lu)\n",
                      input_string, input_length);

    return (input_string);
}
