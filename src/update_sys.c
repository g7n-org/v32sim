#include "defines.h"

#include "defines.h"

// === NEW: Turbo mode flag (declare extern if defined in main.c) ===
#ifndef TURBOFLAG_DECLARED
extern uint8_t turboflag;  // Defined in main.c via --turbo
#endif

// === NEW: High-resolution timer helper ===
static uint64_t get_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// === NEW: Frame-based throttling state ===
static uint64_t frame_start_ns = 0;
#define CYCLES_PER_FRAME (turboflag ? 500000 : 250000)  // 500k (30MHz) or 250k (15MHz)
#define FRAME_TIME_NS    (frame_us * 1000ULL)                    // ~16.67ms by default

//
// Pacing and limits (see --frame-us, --fast and --max-frames): a frame lasts at least
// frame_us microseconds of real time (0 = run as fast as possible); with a frame limit
// the simulator halts when that many frames have been completed
//
uint32_t  frame_us      = 1000000 / V32_FRAMES_PER_SECOND;
uint32_t  max_frames    = 0;       // 0 = no limit
uint8_t   fixed_clock   = 0;       // --fixed-clock
uint32_t  fixed_year    = 2026, fixed_day = 0, fixed_secs = 0;   // days count from 0, as on the console
uint8_t   rng_seed_set  = 0;       // --seed
uint32_t  rng_seed      = 0;
uint32_t  frames_run    = 0;       // frames completed since start
uint8_t   frame_limit   = FALSE;   // set when max_frames was reached

////////////////////////////////////////////////////////////////////////////////////////
//
// cycle_begin(): the console counts an instruction's cycle BEFORE it runs
// (V32Console::RunNextFrame calls Timer.RunNextCycle() and then CPU.RunNextCycle()), so
// the first instruction of a frame reads TIM_CycleCounter = 1 and an instruction that
// raises a hardware error has already been counted.
//
void cycle_begin(void) {
    SYSPORTSET(TIM_CycleCounter, IPORTGET(TIM_CycleCounter) + 1);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// update_cycle(): run after each executed instruction. The frame ends when the cycle
// budget is used up, or when the instruction was a WAIT (it is still counted as a cycle,
// the console just stops running the frame)
//
void update_cycle(void) {
    uint32_t cycles = IPORTGET(TIM_CycleCounter);

    if ((waitflag == TRUE) || (cycles >= CYCLES_PER_FRAME)) {
        update_frame();
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// timer_change_frame(): V32Timer::ChangeFrame. The console does this at the START of a
// frame, before the program runs: the cycle counter returns to 0, the frame counter goes
// up (so the first frame runs with TIM_FrameCounter = 1), the clock advances once a second
// and the date once a day. Days are numbered from 0 (tm_yday), as on the console.
//
void timer_change_frame(void) {
    uint32_t value = 0;

    SYSPORTSET(TIM_CycleCounter, 0);

    value = IPORTGET(TIM_FrameCounter) + 1;
    SYSPORTSET(TIM_FrameCounter, value);

    if ((value % 60) == 0) {
        SYSPORTSET(TIM_CurrentTime, IPORTGET(TIM_CurrentTime) + 1);
    }

    if (IPORTGET(TIM_CurrentTime) >= 86400) {
        uint32_t date, year, days, daysinyear;

        SYSPORTSET(TIM_CurrentTime, 0);

        date  = IPORTGET(TIM_CurrentDate) + 1;
        year  = date >> 16;
        days  = date & 0xFFFF;
        daysinyear = ((((year % 4) == 0) && ((year % 100) != 0)) ||
                      ((year % 400) == 0)) ? 366 : 365;
        if (days >= daysinyear) {
            date = (year + 1) << 16;         // new year, back to day 0
        }
        SYSPORTSET(TIM_CurrentDate, date);
    }
}

void update_frame(void) {
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // profiler stats update
    //
    if ((profileflag == TRUE) && (csub != NULL)) {
        csub->CYCLES = csub->CYCLES + (CYCLES_PER_FRAME - csub->COUNT);
        csub->FRAMES = csub->CYCLES / CYCLES_PER_FRAME;
    }

    waitflag = FALSE;  // a WAIT has had its effect: the frame ends here

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // the next frame starts: the console's timer changes frame before the CPU runs again
    //
    timer_change_frame();

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // the memory card: what the program wrote during the frame goes to its file, and
    // what a v32dev host wrote to the mailbox comes in, ready for the next frame
    //
    memc_frame ();

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // update all the ports that require per-frame changes
    //
    gpu_new_frame ();
    spu_new_frame ();
    update_ioports();

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // frame limit (used by scripted runs)
    //
    frames_run = frames_run + 1;
    if ((max_frames != 0) && (frames_run >= max_frames))
    {
        frame_limit  = TRUE;
        haltflag     = TRUE;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // 'run N' and 'until TEXT' (runctl.c) look at the end of every frame
    //
    runctl_frame ();

    // === NEW: Frame-based throttling with busy-wait ===
    uint64_t now_ns = get_ns();
    uint64_t elapsed_ns = now_ns - frame_start_ns;

    if ((frame_us != 0) && (elapsed_ns < FRAME_TIME_NS)) {
        uint64_t sleep_ns = FRAME_TIME_NS - elapsed_ns;

        if (sleep_ns > 1000) {  // >1μs: use nanosleep
            struct timespec delay = { .tv_sec = 0, .tv_nsec = sleep_ns };
            nanosleep(&delay, NULL);
        } else if (sleep_ns > 0) {  // ≤1μs: busy-wait (faster than syscall)
            uint64_t busy_start = get_ns();
            while (get_ns() - busy_start < sleep_ns) {
                // Optional: x86 pause hint (reduce power)
                #ifdef __x86_64__
                __builtin_ia32_pause();
                #endif
            }
        }
    }
    frame_start_ns = get_ns();
}
