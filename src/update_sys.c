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
uint32_t  fixed_year    = 2026, fixed_day = 1, fixed_secs = 0;
uint8_t   rng_seed_set  = 0;       // --seed
uint32_t  rng_seed      = 0;
uint32_t  frames_run    = 0;       // frames completed since start
uint8_t   frame_limit   = FALSE;   // set when max_frames was reached

void update_cycle(void) {
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    uint32_t cycles = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Obtain the current number of cycles from the system's cycle counter port
    //
    cycles = IPORTGET(TIM_CycleCounter);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // If we are not WAITing, increment TIM_CycleCounter
    //
    if (waitflag == FALSE) {
        cycles = cycles + 1;
    } else {
        // Force frame completion on WAIT
        cycles = CYCLES_PER_FRAME;
        waitflag = FALSE;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Check for frame roll-over
    //
    if (cycles >= CYCLES_PER_FRAME) {
        update_frame();
    } else {
        SYSPORTSET(TIM_CycleCounter, cycles);
    }
}

void update_frame(void) {
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    uint32_t value = 0;
    uint32_t upper = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // profiler stats update
    //
    if ((profileflag == TRUE) && (csub != NULL)) {
        csub->CYCLES = csub->CYCLES + (CYCLES_PER_FRAME - csub->COUNT);
        csub->FRAMES = csub->CYCLES / CYCLES_PER_FRAME;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // increment frame counter
    //
    value = IPORTGET(TIM_FrameCounter);
    value = value + 1;
    SYSPORTSET(TIM_FrameCounter, value);
    waitflag = FALSE;  // reset waitflag

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // reset TIM_CycleCounter to 0 with the new frame
    //
    SYSPORTSET(TIM_CycleCounter, 0);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // adjust TIM_CurrentTime, if enough frames have elapsed
    //
    if ((value % 60) == 0) {
        value = IPORTGET(TIM_CurrentTime);
        value = value + 1;

        ////////////////////////////////////////////////////////////////////////////////
        //
        // adjust TIM_CurrentDate, if enough seconds have elapsed in TIM_CurrentTime
        //
        if (value >= 86400) {
            value = IPORTGET(TIM_CurrentDate);
            upper = value & 0xFFFF0000; // isolate year from TIM_CurrentDate
            value = value & 0x0000FFFF; // isolate day from TIM_CurrentDate
            value = value + 1;          // increment the day

            ////////////////////////////////////////////////////////////////////////////
            //
            // larger adjustment of TIM_CurrentDate if year needs incrementing
            //
            {
                uint32_t  year       = upper >> 16;
                uint32_t  daysinyear = ((((year % 4) == 0) && ((year % 100) != 0)) ||
                                        ((year % 400) == 0)) ? 366 : 365;

                if (value >= daysinyear)
                {
                    value  = 0;                  // new year, reset the day to 0
                    upper  = upper + 0x00010000; // increment the year
                }
                value = upper | value;           // recombine YEAR and DAY
            }
            SYSPORTSET(TIM_CurrentDate, value);

            value = 0; // TIM_CurrentTime resets to 0
        }

        SYSPORTSET(TIM_CurrentTime, value);
    }

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
