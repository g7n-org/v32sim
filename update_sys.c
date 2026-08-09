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
#define FRAME_TIME_NS    (1000000000ULL / V32_FRAMES_PER_SECOND)  // ~16.67ms

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
            if (value >= 365) { // TODO: compensate for leap years
                value = 0;                  // new year, reset the day to 0
                upper = upper + 0x00010000; // increment the year
                value = upper;              // recombine YEAR and DAY
            }
            SYSPORTSET(TIM_CurrentDate, value);

            value = 0; // TIM_CurrentTime resets to 0
        }

        SYSPORTSET(TIM_CurrentTime, value);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // update all the ports that require per-frame changes
    //
    update_ioports();

    // === NEW: Frame-based throttling with busy-wait ===
    uint64_t now_ns = get_ns();
    uint64_t elapsed_ns = now_ns - frame_start_ns;

    if (elapsed_ns < FRAME_TIME_NS) {
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
