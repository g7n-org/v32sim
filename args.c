#include "defines.h"

void       process_args (int32_t  argc, int8_t **argv)
{
    int32_t   opt                  = 0;
    int32_t   option_index         = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // getopt(3) long options and mapping to short options
    //
    struct option long_options[]   = {
       { "biosfile",       required_argument, 0, 'B' },
       { "binary",         no_argument,       0, 'b' },
       { "colors",         no_argument,       0, 'c' },
	   { "command-file",   required_argument, 0, 'C' },
       { "index-math",     no_argument,       0, 'i' },
       { "run",            no_argument,       0, 'r' },
       { "seek-to",        required_argument, 0, 's' },
       { "verbose",        no_argument,       0, 'v' },
       { "help",           no_argument,       0, 'h' },
       { 0,                0,                 0,  0  }
    };

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Process command-line arguments, via getopt(3)
    //
    opt                            = getopt_long ((int) argc, (char **) argv,
                                                  "B:bC:cirs:vh", long_options,
                                                  &option_index);
    while (opt                    != -1)
    {
        switch (opt)
        {
            case 'B':
                biosfile           = optarg;
                break;

            case 'b':
                //binaryflag         = 1;
                break;

            case 'C':
                commandfile        = optarg;
                break;

            case 'c':
                //fancyflag          = FANCY_COLORS;
                break;

            case 'i':
                indexflag          = TRUE;
                break;

            case 'r':
                runflag            = TRUE;
                break;

            case 's':
                runflag            = TRUE;
                seek_word          = strtol (optarg, NULL, 16);
                break;

            case 'v':
                if (verbose       == NULL)
                {
                    verbose        = stderr;
                }
                break;

            case 'h':
                usage ((int8_t *) argv[0]);
                break;
        }
        opt                        = getopt_long ((int) argc, (char **) argv,
                                                  "B:bC:cirs:vh", long_options,
                                                  &option_index);
    }

    if (optind                    <  argc)
    {
        fprintf (stdout, "[optind] %u\n", optind);
        fprintf (stdout, "[argc]   %u\n", argc);
        cartfile                   = *(argv+optind);
    }
}
