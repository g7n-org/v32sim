#include "defines.h"
#include <stdarg.h>

////////////////////////////////////////////////////////////////////////////////////////
//
// runctl.c
//
// What a script needs in order to drive the simulator unattended (--batch):
//
//   * batch mode itself: the stream that acknowledgements and "why did it stop" lines
//     go to (stderr, so that stdout holds exactly what the script asked to see)
//   * run N / wait N      continue for N frames
//   * until TEXT          continue until the text console shows TEXT (or a timeout)
//   * expect ...          check a value or the text console; a failure is remembered
//                         and, in batch mode, ends the run
//   * quit N              the exit status
//
// The frame-based commands hook into update_frame() (update_sys.c), which calls
// runctl_frame() at the end of every frame. Pending requests are dropped whenever the
// prompt is entered again (runctl_reset()), so a `run 600` that stopped early at a
// breakpoint does not stop a later `continue`.
//
////////////////////////////////////////////////////////////////////////////////////////

uint8_t   batchflag         = FALSE;     // --batch
FILE     *status_out        = NULL;      // acknowledgements and stop reasons: stdout, or stderr in batch mode
uint32_t  script_errors     = 0;         // command lines that could not be parsed (batch mode)
uint32_t  check_failures    = 0;         // expect failures and until timeouts
uint8_t   quit_status_set   = FALSE;     // 'quit N' was given
uint8_t   quit_status       = 0;

static uint8_t   run_active     = FALSE;
static uint32_t  run_target     = 0;

static uint8_t   until_active   = FALSE;
static uint8_t   until_changed  = FALSE;
static uint32_t  until_start    = 0;
static uint32_t  until_timeout  = 0;
static uint32_t  until_rows     = 0;
static char     *until_text     = NULL;
static char     *until_snap     = NULL;

#define  UNTIL_DEFAULT_TIMEOUT   3600    // frames: a minute of guest time

void  runctl_init (void)
{
    status_out  = (batchflag == TRUE) ? stderr : stdout;
    if (batchflag == TRUE)
    {
        setvbuf (stdout, NULL, _IOLBF, 0);    // keeps stdout and stderr in order when they share a pipe
    }
}

//
// status_printf(): an acknowledgement or a stop reason (see status_out)
//
void  status_printf (const char *format, ...)
{
    va_list  args;

    va_start (args, format);
    vfprintf ((status_out != NULL) ? status_out : stdout, format, args);
    va_end   (args);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// pending requests
//
static void  until_clear (void)
{
    until_active  = FALSE;
    free (until_text);   until_text  = NULL;
    free (until_snap);   until_snap  = NULL;
}

void  runctl_reset (void)
{
    run_active  = FALSE;
    until_clear ();
}

void  runctl_run (uint32_t frames)
{
    run_active  = TRUE;
    run_target  = frames_run + frames;
}

void  runctl_until (const char *text, uint32_t timeout, uint32_t rows)
{
    until_clear ();
    until_active   = TRUE;
    until_changed  = FALSE;
    until_start    = frames_run;
    until_timeout  = (timeout == 0) ? UNTIL_DEFAULT_TIMEOUT : timeout;
    until_rows     = rows;
    until_text     = strdup (text);
    until_snap     = gpu_console_snapshot ();      // the screen as it is now: it takes a change to end the wait
}

////////////////////////////////////////////////////////////////////////////////////////
//
// check_failed(): an expect or until did not hold. In batch mode that is the end of the
// run (the exit status says so).
//
static void  check_failed (void)
{
    check_failures  = check_failures + 1;
    if (batchflag == TRUE)
    {
        haltflag    = TRUE;
        action      = INPUT_QUIT;
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// runctl_frame(): called at the end of every frame
//
void  runctl_frame (void)
{
    char  *now  = NULL;

    if ((run_active == TRUE) && (frames_run >= run_target))
    {
        run_active  = FALSE;
        runflag     = FALSE;
    }

    if (until_active == TRUE)
    {
        now = gpu_console_snapshot ();
        if ((now != NULL) && (until_snap != NULL) && (0 != strcmp (now, until_snap)))
        {
            until_changed  = TRUE;
        }
        free (now);

        if ((until_changed == TRUE) && (gpu_console_match (until_text, until_rows) == TRUE))
        {
            status_printf ("[until] \"%s\" appeared after %u frames\n", until_text, frames_run - until_start);
            until_clear ();
            runflag  = FALSE;
        }
        else if ((frames_run - until_start) >= until_timeout)
        {
            fprintf (stderr, "[until] FAILED: \"%s\" did not appear within %u frames%s\n", until_text,
                             until_timeout, (until_changed == TRUE) ? "" : " (the screen never changed)");
            until_clear ();
            runflag  = FALSE;
            check_failed ();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// expect
//
void  runctl_expect_value (wloc_t target, uint8_t op, wloc_t rhs, uint8_t is_signed)
{
    wpoint_t  wp;
    uint32_t  l      = 0;
    uint32_t  r      = 0;
    char      desc[160];

    memset (&wp, 0, sizeof (wp));
    wp.target     = target;
    wp.rhs        = rhs;
    wp.operator   = op;
    wp.is_signed  = is_signed;
    wpoint_describe (&wp, desc, sizeof (desc));

    if (wcond_eval (target, op, rhs, is_signed, &l, &r) == TRUE)
    {
        if (batchflag == FALSE)
        {
            status_printf ("[expect] ok: %s\n", desc);
        }
        return;
    }
    fprintf (stderr, "[expect] FAILED: %s (the two sides are 0x%.8X and 0x%.8X)\n", desc, l, r);
    check_failed ();
}

void  runctl_expect_console (const char *text, uint32_t rows)
{
    if (gpu_console_match (text, rows) == TRUE)
    {
        if (batchflag == FALSE)
        {
            status_printf ("[expect] ok: console shows \"%s\"\n", text);
        }
        return;
    }
    fprintf (stderr, "[expect] FAILED: console does not show \"%s\"", text);
    if (rows > 0)
    {
        fprintf (stderr, " in its last %u rows", rows);
    }
    fprintf (stderr, "; the screen is:\n");
    gpu_console_show (stderr);
    check_failed ();
}
