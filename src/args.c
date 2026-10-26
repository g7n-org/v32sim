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
       { "bios-asm-debug", required_argument, 0, 16  },
       { "cart-asm-debug", required_argument, 0, 17  },
       { "bios-c-debug",   required_argument, 0, 18  },
       { "cart-c-debug",   required_argument, 0, 19  },
       { "debug-dir",      required_argument, 0, 20  },
       { "source",         no_argument,       0, 21  },
       { "entry-point",    required_argument, 0, 'E' },
       { "errorcheck",     no_argument,       0, 'e' },
       { "langmode",       required_argument, 0, 'L' },
       { "memcfile",       required_argument, 0, 'M' },
       { "no-debug",       no_argument,       0, 'n' },
       { "profile",        no_argument,       0, 'p' },
       { "run",            no_argument,       0, 'r' },
       { "bios-start",     no_argument,       0, 'S' },
       { "watch-for",      required_argument, 0, 'w' },
       { "verbose",        no_argument,       0, 'v' },
       { "version",        no_argument,       0, 'V' },
       { "v32kbd",         optional_argument, 0, 22  },
       { "kbd-input",      required_argument, 0, 23  },
       { "fast",           no_argument,       0, 24  },
       { "frame-us",       required_argument, 0, 25  },
       { "max-frames",     required_argument, 0, 26  },
       { "console-color",  required_argument, 0, 29  },
       { "cursor-color",   required_argument, 0, 30  },
       { "tty",            no_argument,       0, 28  },
       { "console",        no_argument,       0, 27  },
       { "dump",           required_argument, 0, 31  },
       { "fixed-clock",    optional_argument, 0, 32  },
       { "seed",           required_argument, 0, 33  },
       { "profile-out",    required_argument, 0, 34  },
       { "profile-format", required_argument, 0, 35  },
       { "profile-top",    required_argument, 0, 36  },
       { "fixed-cycles",   no_argument,       0, 37  },
       { "batch",          no_argument,       0, 'q' },
       { "quiet",          no_argument,       0, 'q' },
       { "profile-by",     required_argument, 0, 38  },
       { "symbol-file",    required_argument, 0, 39  },
       { "exit-status",    no_argument,       0, 'x' },
       { "help",           no_argument,       0, 'h' },
       { 0,                0,                 0,  0  }
    };

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Process command-line arguments, via getopt(3)
    //
    opt                            = getopt_long ((int) argc, (char **) argv,
                                                  "B:b:C:cdDeE:L:M:nqprSw:vVhx", long_options,
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

            case 16: // bios-asm-debug
                biosasmdebug       = (int8_t *) ralloc (sizeof (int8_t),
                                                        strlen (optarg) + 1,
                                                        FLAG_NONE);
                strcpy (biosasmdebug, optarg);
                biosasmdebugflag   = TRUE;
                break;

            case 17: // cart-asm-debug
                cartasmdebug       = (int8_t *) ralloc (sizeof (int8_t),
                                                        strlen (optarg) + 1,
                                                        FLAG_NONE);
                strcpy (cartasmdebug, optarg);
                cartasmdebugflag   = TRUE;
                break;

            case 18: // bios-c-debug
                bioscdebug         = (int8_t *) ralloc (sizeof (int8_t),
                                                        strlen (optarg) + 1,
                                                        FLAG_NONE);
                strcpy (bioscdebug, optarg);
                bioscdebugflag     = TRUE;
                break;

            case 21: // source
                sourceflag         = TRUE;
                break;

            case 20: // debug-dir
                debugdir           = optarg;
                break;

            case 19: // cart-c-debug
                cartcdebug         = (int8_t *) ralloc (sizeof (int8_t),
                                                        strlen (optarg) + 1,
                                                        FLAG_NONE);
                strcpy (cartcdebug, optarg);
                cartcdebugflag     = TRUE;
                break;

            case 'L':
                if (0             == strcmp (optarg, "C"))
                {
                    modeflag       = FLAG_C;
                    sourceflag     = TRUE;
                }
                else if (0        == strcmp (optarg, "lua"))
                {
                    modeflag       = FLAG_LUA;
                }
                break;

            case 'M':
                memcfile           = optarg;
                break;

            case 'n':
                biosasmdebugflag   = MU;
                bioscdebugflag     = MU;
                cartasmdebugflag   = MU;
                cartcdebugflag     = MU;
                break;

            case 'p':
                profileflag        = TRUE;
                break;

            case 'r':
                runflag            = TRUE;
                break;

            case 'S':
                rom_offset         = BIOS_START_OFFSET;
                break;

            case 'w':
                runflag            = TRUE;
                watch_word         = strtol (optarg, NULL, 16);
                break;

            case 'v':
                verbose            = stderr;
                break;

            case 'V':
                fprintf (stdout, "v32sim %s\n", VERSION);
                exit (0);
                break;

            case 22: // v32kbd[=PORT]
                kbdport            = (optarg != NULL) ? atoi (optarg) : 0;
                if ((kbdport < 0) || (kbdport >= V32_NUM_GAMEPADS))
                {
                    fprintf (stderr, "[ERROR] --v32kbd: gamepad port must be 0-3\n");
                    exit (1);
                }
                break;

            case 23: // kbd-input
                kbdinput           = optarg;
                break;

            case 24: // fast
                frame_us           = 0;
                break;

            case 25: // frame-us
                frame_us           = (uint32_t) strtoul (optarg, NULL, 10);
                break;

            case 26: // max-frames
                max_frames         = (uint32_t) strtoul (optarg, NULL, 10);
                break;

            case 32: // fixed-clock[=YEAR,DAY,SECONDS]
                fixed_clock        = TRUE;
                fixed_year         = 2026;
                fixed_day          = 0;              // days count from 0 (January 1st), as on the console
                fixed_secs         = 0;
                if (optarg        != NULL)
                {
                    if (sscanf (optarg, "%u,%u,%u", &fixed_year, &fixed_day, &fixed_secs) != 3)
                    {
                        fprintf (stderr, "[ERROR] --fixed-clock: use YEAR,DAY,SECONDS (e.g. 2026,0,0)\n");
                        exit (1);
                    }
                    // the console's own limits (V32Console::SetCurrentDate / SetCurrentTime)
                    if (fixed_year    >  32767) fixed_year = 32767;
                    if (fixed_day     >  365)   fixed_day  = 365;
                    if (fixed_secs    >  86399) fixed_secs = 86399;
                }
                break;

            case 34: // profile-out
                profile_out        = optarg;
                break;

            case 35: // profile-format
                if (0             == strcmp (optarg, "bin"))
                {
                    profile_bin    = TRUE;
                }
                else if (0        != strcmp (optarg, "text"))
                {
                    fprintf (stderr, "[ERROR] --profile-format: use text or bin\n");
                    exit (1);
                }
                else
                {
                    profile_bin    = FALSE;
                }
                break;

            case 36: // profile-top
                profile_top        = (uint32_t) strtoul (optarg, NULL, 10);
                break;

            case 37: // fixed-cycles
                fixed_cycles       = TRUE;
                break;

            case 'q': // batch
                batchflag          = TRUE;
                break;

            case 38: // profile-by
                if (0             == strcmp (optarg, "function"))
                {
                    profile_by_function = TRUE;
                }
                else if (0        != strcmp (optarg, "address"))
                {
                    fprintf (stderr, "[ERROR] --profile-by: use address or function\n");
                    exit (1);
                }
                else
                {
                    profile_by_function = FALSE;
                }
                break;

            case 39: // symbol-file
                symbolfile         = optarg;
                break;

            case 33: // seed
                rng_seed_set       = TRUE;
                rng_seed           = (uint32_t) strtoul (optarg, NULL, 0);
                break;

            case 29: // console-color
                if (gpu_console_set_color (optarg) != 0)
                {
                    fprintf (stderr, "[ERROR] --console-color: use none, 16, 256, true or auto\n");
                    exit (1);
                }
                break;

            case 30: // cursor-color
                if (gpu_console_set_cursor (optarg) != 0)
                {
                    fprintf (stderr, "[ERROR] --cursor-color: a color name, #RRGGBB, or off\n");
                    exit (1);
                }
                break;

            case 28: // tty
                v32io_tty (TRUE);
                runflag            = TRUE;
                break;

            case 27: // console
                consoleflag        = TRUE;
                break;

            case 31: // dump
                dumpspec           = optarg;
                break;

            case 'x':
                exitflag           = TRUE;
                break;

            case 'h':
                usage ((int8_t *) argv[0]);
                break;

            case '?':   // getopt_long() has already named the offending option
                fprintf (stderr, "Try '%s --help' for more information.\n", argv[0]);
                exit (1);
                break;
        }
        opt                        = getopt_long ((int) argc, (char **) argv,
                                                  "B:b:C:cdDeE:L:M:nqprSw:vVhx", long_options,
                                                  &option_index);
    }

    if (optind                    <  argc)
    {
        cartfile                   = *(argv+optind);
    }
}
