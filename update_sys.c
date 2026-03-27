#include "defines.h"

void      update_cycle (void)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    TimeSpec  delay;
    uint32_t  cycles   = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Obtain the current number of cycles from the system's cycle counter port
    //
    cycles             = IPORTGET(TIM_CycleCounter);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // If we are not WAITing, increment TIM_CycleCounter
    //
    if (waitflag      == FALSE)
    {
        cycles         = cycles + 1;
    }
    else // wait out the current frame
    {
        ////////////////////////////////////////////////////////////////////////////////
        //
        // delay: for a 15MHz machine @ 60 frames per second and 250000 cycles per
        // frame, that's 15,000,000 total cycles per second.
        //
        // If 1 instruction == 1 cycle, that means 1 instruction will take:
        //
        // 1 / 15000000 = .0000000667s (~66 nanoseconds)
        //
        // So, if we are WAITing out the frame, we figure out how many cycles are
        // remaining for the frame, and wait the requisite amount of time.
        //
        delay.tv_sec   = 0;                      // 0 seconds
        delay.tv_nsec  = 66 * (250000 - cycles); // 66 nanoseconds per instruction

        fprintf (debug, "[update_cycle] WAIT: delaying for %ld ns\n", delay.tv_nsec);
        nanosleep (&delay, NULL);

        cycles         = 250000;                 // max out our cycle count
        waitflag       = FALSE;                  // reset waitflag
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Check for frame roll-over
    //
    if (cycles        >= 250000)
    {
        update_frame ();
    }
    else
    {
        SYSPORTSET(TIM_CycleCounter, cycles);
    }
}

void      update_frame (void)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    uint32_t  value    = 0;
    uint32_t  upper    = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // increment frame counter
    //
    value              = IPORTGET(TIM_FrameCounter);
    value              = value + 1;
    SYSPORTSET(TIM_FrameCounter, value);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // reset TIM_CycleCounter to 0 with the new frame
    //
    SYSPORTSET(TIM_CycleCounter, 0);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // adjust TIM_CurrentTime, if enough frames have elapsed
    //
    if ((value % 60)  == 0)
    {
        value          = IPORTGET(TIM_CurrentTime);
        value          = value + 1;

        ////////////////////////////////////////////////////////////////////////////////
        //
        // adjust TIM_CurrentDate, if enough seconds have elapsed in TIM_CurrentTime
        //
        if (value     >= 86400)
        {
            value      = IPORTGET(TIM_CurrentDate);
            upper      = value & 0xFFFF0000; // isolate year from TIM_CurrentDate
            value      = value & 0x0000FFFF; // isolate day  from TIM_CurrentDate
            value      = value + 1;          // increment the day

            ////////////////////////////////////////////////////////////////////////////
            //
            // larger adjustment of TIM_CurrentDate if year needs incrementing
            //
            if (value >= 365)                // TODO: compensate for leap years
            {
                value  = 0;                  // new year, reset the day to 0
                upper  = upper + 0x00010000; // increment the year
                value  = upper;              // recombine YEAR and DAY
            }
            SYSPORTSET(TIM_CurrentDate, value);

            value      = 0;                  // TIM_CurrentTime resets to 0
        }

        SYSPORTSET(TIM_CurrentTime, value);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // update all the ports that require per-frame changes
    //
    update_ioports ();
}
