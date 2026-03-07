#include "defines.h"

void      update_cycle (void)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    uint32_t  value    = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // If we are not WAITing, increment TIM_CycleCounter
    //
    if (waitflag      == FALSE)
    {
        value          = ioports_get (TIM_CycleCounter, FALSE);
        value          = value + 1;
    }
    else
    {
        waitflag       = FALSE;  // reset waitflag
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Check for frame roll-over
    //
    if (value         >= 250000)
    {
        update_frame ();
        value          = 0;
    }

    ioports_set (TIM_CycleCounter, value, TRUE);
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
    value              = ioports_get (TIM_FrameCounter, FALSE);
    value              = value + 1;
    ioports_set (TIM_FrameCounter, value, TRUE);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // adjust TIM_CurrentTime, if enough frames have elapsed
    //
    if ((value % 60)  == 0)
    {
        value          = ioports_get (TIM_CurrentTime, FALSE);
        value          = value + 1;

        ////////////////////////////////////////////////////////////////////////////////
        //
        // adjust TIM_CurrentDate, if enough seconds have elapsed in TIM_CurrentTime
        //
        if (value     >= 86400)
        {
            value      = ioports_get (TIM_CurrentDate, FALSE);
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
            ioports_set (TIM_CurrentDate, value, TRUE);

            value      = 0;                  // TIM_CurrentTime resets to 0
        }

        ioports_set (TIM_CurrentTime, value, TRUE);
    }
}
