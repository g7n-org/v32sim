#include "defines.h"

void  process_args (int32_t  argc, int8_t **argv)
{
    int32_t    opt                 = 0;
    int32_t    option_index        = 0;
    linked_l  *tmp                 = NULL;
    uint32_t   value               = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // getopt(3) long options and mapping to short options
    //
    struct option long_options[]   = {
       { "biosfile",       required_argument, 0, 'B' },
       { "break",          required_argument, 0, 'b' },
       { "colors",         no_argument,       0, 'c' },
       { "command-file",   required_argument, 0, 'C' },
       { "deref-addr",     no_argument,       0, 'd' },
       { "debug",          no_argument,       0, 'D' },
       { "entry-point",    required_argument, 0, 'E' },
       { "errorcheck",     no_argument,       0, 'e' },
       { "memcfile",       required_argument, 0, 'M' },
       { "no-debug",       no_argument,       0, 'n' },
       { "run",            no_argument,       0, 'r' },
       { "watch-for",      required_argument, 0, 'w' },
       { "verbose",        no_argument,       0, 'v' },
       { "help",           no_argument,       0, 'h' },
       { 0,                0,                 0,  0  }
    };

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Process command-line arguments, via getopt(3)
    //
    opt                            = getopt_long ((int) argc, (char **) argv,
                                                  "B:b:C:cdDeE:M:nrw:vh", long_options,
                                                  &option_index);
    while (opt                    != -1)
    {
        switch (opt)
        {
            case 'B':
                biosfile           = optarg;
                break;

            case 'b':
                value              = strtol (optarg, NULL, 16);
                tmp                = listnode (LIST_MEM, value);
                if (value         != 0) // offset provided
                {
                    fprintf (debug, "[arg] BREAK adding the offset '%s'\n", optarg);
                }
                else // assuming label
                {
                    fprintf (debug, "[arg] BREAK adding the label: '%s'\n", optarg);
                    tmp -> label   = (int8_t *) ralloc (sizeof (int8_t), strlen (optarg) + 1, FLAG_NONE);
                    strcpy (tmp -> label, optarg);
                }
                tpoint             = list_add (tpoint, tmp);
                break;

            case 'C':
                commandfile        = optarg;
                break;

            case 'c':
                colorflag          = TRUE;
                break;

            case 'D':
                debug              = stderr;
                break;

            case 'd':
                derefaddr          = TRUE;
                break;

            case 'e':
                errorcheck         = TRUE;
                break;

            case 'E':
                rom_offset         = strtol (optarg, NULL, 16);
                break;

            case 'M':
                memcfile           = optarg;
                break;

            case 'n':
                debugflag          = FALSE;
                break;

            case 'r':
                runflag            = TRUE;
                break;

            case 'w':
                runflag            = TRUE;
                watch_word         = strtol (optarg, NULL, 16);
                break;

            case 'v':
                verbose            = stderr;
                break;

            case 'h':
                usage ((int8_t *) argv[0]);
                break;
        }
        opt                        = getopt_long ((int) argc, (char **) argv,
                                                  "B:b:C:cdDeE:M:nrw:vh", long_options,
                                                  &option_index);
    }

    if (optind                    <  argc)
    {
        cartfile                   = *(argv+optind);
    }
}
