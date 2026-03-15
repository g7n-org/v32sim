#include "defines.h"

void  init_ioports  (void)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    int32_t    index                       = 0;
    int32_t    value                       = 0;
    int8_t    *nptr                        = NULL;
    data_t    *pptr                        = NULL;
    size_t     len                         = 0;
    struct tm *current_time_tm;
    time_t     current_time_raw;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // ioports is the top-level (double pointer) nexus, each category is a single-
    // pointer array hanging off of each element of ioports.
    //
    len                                    = sizeof (data_t *) * NUM_PORT_CATEGORIES;
    ioports                                = (data_t **) malloc (len);
    if (ioports                           == NULL)
    {
        fprintf (stderr, "[error] failed to allocate memory for 'ioports'\n");
        exit (IOPORTS_ALLOC_FAIL);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // TIM ports: allocate and initialize
    //
    len                                    = sizeof (data_t)   * NUM_TIM_PORTS;
    *(ioports+TIM_PORT)                    = (data_t *) malloc (len);
    pptr                                   = *(ioports+TIM_PORT);
    pptr -> qty                            = NUM_TIM_PORTS;

    current_time_raw                       = time (NULL); // obtain current time (raw)
    current_time_tm                        = localtime (&current_time_raw);

    for (index                             = 0;
         index                            <  NUM_TIM_PORTS;
         index                             = index + 1)
    {
        (pptr+index) -> value.i32          = 0x00000000;
        (pptr+index) -> flag               = FLAG_READ;
        (pptr+index) -> fmt                = FORMAT_UNSIGNED;
        (pptr+index) -> name               = (int8_t *) malloc (sizeof (int8_t) * 32);
        nptr                               = (pptr+index) -> name;

        switch (TIM_CurrentDate | index)
        {
            case TIM_CurrentDate:
                value                      = current_time_tm -> tm_year + 1900;
                value                      = value << 16;
                value                     |= current_time_tm -> tm_yday + 1;
                (pptr+index) -> value.i32  = value;
                (pptr+index) -> fmt        = FORMAT_DEFAULT;
                sprintf (nptr, "TIM_CurrentDate");
                break;

            case TIM_CurrentTime:
                value                      = current_time_tm -> tm_hour * 3600;
                value                     += current_time_tm -> tm_min  * 60;
                value                     += current_time_tm -> tm_sec  * 60;
                (pptr+index) -> value.i32  = value;
                (pptr+index) -> fmt        = FORMAT_DEFAULT;
                sprintf (nptr, "TIM_CurrentTime");
                break;

            case TIM_FrameCounter:
                sprintf (nptr, "TIM_FrameCounter");
                break;

            case TIM_CycleCounter:
                sprintf (nptr, "TIM_CycleCounter");
                break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // RNG ports: allocate and initialize
    //
    len                                    = sizeof (data_t)   * NUM_RNG_PORTS;
    *(ioports+RNG_PORT)                    = (data_t *) malloc (len);
    pptr                                   = *(ioports+RNG_PORT);
    pptr -> qty                            = NUM_RNG_PORTS;

    (pptr+0) -> value.i32                  = 0x00000000; //rand ();
    (pptr+0) -> flag                       = FLAG_READ | FLAG_WRITE;
    (pptr+index) -> fmt                    = FORMAT_SIGNED;
    (pptr+0) -> name                       = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                                   = (pptr+0) -> name;
    sprintf (nptr, "RNG_CurrentValue");

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // GPU ports: allocate and initialize
    //
    len                                    = sizeof (data_t)   * NUM_GPU_PORTS;
    *(ioports+GPU_PORT)                    = (data_t *) malloc (len);
    pptr                                   = *(ioports+GPU_PORT);
    pptr -> qty                            = NUM_GPU_PORTS;

    for (index                             = 0;
         index                            <  NUM_GPU_PORTS;
         index                             = index + 1)
    {
        (pptr+index) -> value.i32          = 0x00000000;
        (pptr+index) -> flag               = FLAG_READ | FLAG_WRITE;
        (pptr+index) -> fmt                = FORMAT_UNSIGNED;
        (pptr+index) -> name               = (int8_t *) malloc (sizeof (int8_t) * 32);
        nptr                               = (pptr+index) -> name;

        switch (GPU_Command | index)
        {
            case GPU_Command:
                (pptr+index) -> flag       = FLAG_WRITE;
                sprintf (nptr, "GPU_Command");
                break;

            case GPU_RemainingPixels:
                (pptr+index) -> flag       = FLAG_READ;
                sprintf (nptr, "GPU_RemainingPixels");
                break;

            case GPU_ClearColor:
                sprintf (nptr, "GPU_ClearColor");
                break;

            case GPU_MultiplyColor:
                sprintf (nptr, "GPU_MultiplyColor");
                break;

            case GPU_ActiveBlending:
                sprintf (nptr, "GPU_ActiveBlending");
                break;

            case GPU_SelectedTexture:
                sprintf (nptr, "GPU_SelectedTexture");
                break;

            case GPU_SelectedRegion:
                sprintf (nptr, "GPU_SelectedRegion");

            case GPU_DrawingPointX:
                sprintf (nptr, "GPU_DrawingPointX");
                break;

            case GPU_DrawingPointY:
                sprintf (nptr, "GPU_DrawingPointY");
                break;

            case GPU_DrawingScaleX:
                (pptr+index) -> fmt        = FORMAT_FLOAT;
                sprintf (nptr, "GPU_DrawingScaleX");
                break;

            case GPU_DrawingScaleY:
                (pptr+index) -> fmt        = FORMAT_FLOAT;
                sprintf (nptr, "GPU_DrawingScaleY");
                break;

            case GPU_DrawingAngle:
                (pptr+index) -> fmt        = FORMAT_FLOAT;
                sprintf (nptr, "GPU_DrawingAngle");
                break;

            case GPU_RegionMinX:
                sprintf (nptr, "GPU_RegionMinX");
                break;

            case GPU_RegionMinY:
                sprintf (nptr, "GPU_RegionMinY");
                break;

            case GPU_RegionMaxX:
                sprintf (nptr, "GPU_RegionMaxX");
                break;

            case GPU_RegionMaxY:
                sprintf (nptr, "GPU_RegionMaxY");
                break;

            case GPU_RegionHotspotX:
                sprintf (nptr, "GPU_RegionHotspotX");
                break;

            case GPU_RegionHotspotY:
                sprintf (nptr, "GPU_RegionHotspotY");
                break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // INP ports: allocate and initialize
    //
    len                                    = sizeof (data_t)   * NUM_INP_PORTS;
    *(ioports+INP_PORT)                    = (data_t *) malloc (len);
    pptr                                   = *(ioports+INP_PORT);
    pptr -> qty                            = NUM_INP_PORTS;

    for (index                        = 0;
         index                       <  NUM_INP_PORTS;
         index                        = index + 1)
    {
        (pptr+index) -> value.i32     = -1; // not pressed, negative value
        (pptr+index) -> flag          = FLAG_READ;
        (pptr+index) -> fmt           = FORMAT_SIGNED;
        (pptr+index) -> name          = (int8_t *) malloc (sizeof (int8_t) * 32);
        nptr                          = (pptr+index) -> name;

        switch (INP_SelectedGamepad | index)
        {
            case INP_SelectedGamepad:
                (pptr+index) -> flag  = FLAG_READ | FLAG_WRITE;
                sprintf (nptr, "INP_SelectedGamepad");
                break;

            case INP_GamepadConnected:
                sprintf (nptr, "INP_GamepadConnected");
                break;

            case INP_GamepadLeft:
                sprintf (nptr, "INP_GamepadLeft");
                break;

            case INP_GamepadRight:
                sprintf (nptr, "INP_GamepadRight");
                break;

            case INP_GamepadUp:
                sprintf (nptr, "INP_GamepadUp");
                break;

            case INP_GamepadDown:
                sprintf (nptr, "INP_GamepadDown");
                break;

            case INP_GamepadButtonStart:
                sprintf (nptr, "INP_GamepadButtonStart");
                break;

            case INP_GamepadButtonA:
                sprintf (nptr, "INP_GamepadButtonA");
                break;

            case INP_GamepadButtonB:
                sprintf (nptr, "INP_GamepadButtonB");
                break;

            case INP_GamepadButtonX:
                sprintf (nptr, "INP_GamepadButtonX");
                break;

            case INP_GamepadButtonY:
                sprintf (nptr, "INP_GamepadButtonY");
                break;

            case INP_GamepadButtonL:
                sprintf (nptr, "INP_GamepadButtonL");
                break;

            case INP_GamepadButtonR:
                sprintf (nptr, "INP_GamepadButtonR");
                break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // CAR ports: allocate and initialize
    //
    len                                    = sizeof (data_t)   * NUM_CAR_PORTS;
    *(ioports+CAR_PORT)                    = (data_t *) malloc (len);
    pptr                                   = *(ioports+CAR_PORT);
    pptr -> qty                            = NUM_CAR_PORTS;

    for (index                             = 0;
         index                            <  NUM_CAR_PORTS;
         index                             = index + 1)
    {
        (pptr+index) -> value.i32          = 0x00000000;
        (pptr+index) -> flag               = FLAG_READ;
        (pptr+index) -> fmt                = FORMAT_DEFAULT;
        (pptr+index) -> name               = (int8_t *) malloc (sizeof (int8_t) * 32);
        nptr                               = (pptr+index) -> name;

        switch (CAR_Connected | index)
        {
            case CAR_Connected:
                sprintf (nptr, "CAR_Connected");
                break;

            case CAR_ProgramROMSize:
                sprintf (nptr, "CAR_ProgramROMSize");
                break;

            case CAR_NumberOfTextures:
                sprintf (nptr, "CAR_NumberOfTextures");
                break;

            case CAR_NumberOfSounds:
                sprintf (nptr, "CAR_NumberOfSounds");
                break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // MEM ports: allocate and initialize
    //
    len                                    = sizeof (data_t)   * NUM_MEM_PORTS;
    *(ioports+MEM_PORT)                    = (data_t *) malloc (len);
    pptr                                   = *(ioports+MEM_PORT);
    pptr -> qty                            = NUM_MEM_PORTS;
    pptr -> value.i32                      = 0x00000000;
    pptr -> flag                           = FLAG_READ;
    pptr -> fmt                            = FORMAT_DEFAULT;
    pptr -> name                           = (int8_t *) malloc (sizeof (int8_t) * 32);
    nptr                                   = pptr -> name;
    sprintf (nptr, "MEM_Connected");
}

////////////////////////////////////////////////////////////////////////////////////////
//
// ioports_chk(): check that the address being transacted is valid
//
// returns a TRUE or FALSE
//
uint8_t  ioports_chk  (uint16_t  portaddr, uint8_t  sys_force)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize local variables
    //
    uint16_t  type          = (portaddr & 0x0700) >> 8;  // port category
    uint16_t  attr          = (portaddr & 0x00FF);       // item within category
    uint8_t   flag          = FLAG_NONE;                 // short form access
    uint8_t   result        = TRUE;
    data_t   *pptr          = *(ioports+type);           // pointer for sanity
    int32_t  *dptr          = (pptr+attr) -> value.i32;  // pointer to port data

    flag                    = (pptr+attr) -> flag;
    if ((flag & FLAG_READ) != FLAG_READ)
    {
        if (sys_force      == FALSE)
        {
            fprintf (stderr, "[ERROR] port '%s' not accessible via READ!\n",
                             (pptr+attr) -> name);
            sys_error       = ERROR_PORT_READ;
            result          = FALSE;
        }
    }

    return (result);
}

data_t  *ioports_ptr  (uint16_t  portaddr)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize local variables
    //
    uint16_t  type          = (portaddr & 0x0700) >> 8;  // port category
    uint16_t  attr          = (portaddr & 0x00FF);       // item within category
    data_t   *pptr          = *(ioports+type);           // pointer for sanity

    return ((pptr+attr));
}

int32_t  ioports_get  (uint16_t  portaddr, uint8_t  sys_force)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize local variables
    //
    int32_t   value         = 0x00000000;                // obtained value
    uint16_t  type          = (portaddr & 0x0700) >> 8;  // port category
    uint16_t  attr          = (portaddr & 0x00FF);       // item within category
    uint8_t   flag          = FLAG_NONE;                 // short form access
    data_t   *pptr          = *(ioports+type);           // pointer for sanity
    int32_t  *dptr          = (pptr+attr) -> value.i32;  // pointer to port data

    flag                    = (pptr+attr) -> flag;
    if ((flag & FLAG_READ) != FLAG_READ)
    {
        if (sys_force      == FALSE)
        {
            fprintf (stderr, "[ERROR] port '%s' not accessible via READ!\n",
                             (pptr+attr) -> name);
            sys_error       = ERROR_PORT_READ;
        }
    }

    switch (portaddr)
    {
        case RNG_CurrentValue: // obtain pseudorandom value, place in port
            *dptr           = rand ();
            value           = (pptr+attr) -> value.i32;
            break;

        default:
            value           = (pptr+attr) -> value.i32;
            break;
    }

    return (value);
}

void      ioports_set  (uint16_t  portaddr, int32_t  value, uint8_t  sys_force)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize local variables
    //
    uint16_t  type           = (portaddr & 0x0700) >> 8;    // port category
    uint16_t  attr           = (portaddr & 0x00FF);         // item within category
    uint8_t   flag           = FLAG_NONE;                   // short form access
    data_t   *pptr           = *(ioports+type);             // pointer for sanity

    flag                     = (pptr+attr) -> flag;
    if ((flag & FLAG_WRITE) != FLAG_WRITE)
    {
        if (sys_force       == FALSE)
        {
            fprintf (stderr, "[ERROR] port '%s' not accessible via WRITE!\n",
                             (pptr+attr) -> name);
            exit (IOPORTS_WRITE_ERROR);
        }
        sys_force            = FALSE;
    }

    switch (type)
    {
        case TIM_PORT:
            if (attr        >= NUM_TIM_PORTS)
            {
                fprintf (stderr, "[ERROR] invalid TIM port 0x%.3hX\n", portaddr);
                exit (IOPORTS_BAD_PORT);
            }
            break;

        case RNG_PORT:
            if (attr        >= NUM_RNG_PORTS)
            {
                fprintf (stderr, "[ERROR] invalid RNG port 0x%.3hX\n", portaddr);
                exit (IOPORTS_BAD_PORT);
            }
            break;

        case GPU_PORT:
            if (attr        >= NUM_GPU_PORTS)
            {
                fprintf (stderr, "[ERROR] invalid GPU port 0x%.3hX\n", portaddr);
                exit (IOPORTS_BAD_PORT);
            }
            break;

        case SPU_PORT:
            if (attr        >= NUM_SPU_PORTS)
            {
                fprintf (stderr, "[ERROR] invalid SPU port 0x%.3hX\n", portaddr);
                exit (IOPORTS_BAD_PORT);
            }
            break;

        case INP_PORT:
            if (attr        >= NUM_INP_PORTS)
            {
                fprintf (stderr, "[ERROR] invalid INP port 0x%.3hX\n", portaddr);
                exit (IOPORTS_BAD_PORT);
            }
            break;

        case CAR_PORT:
            if (attr        >= NUM_CAR_PORTS)
            {
                fprintf (stderr, "[ERROR] invalid CAR port 0x%.3hX\n", portaddr);
                exit (IOPORTS_BAD_PORT);
            }
            break;

        case MEM_PORT:
            if (attr        >= NUM_MEM_PORTS)
            {
                fprintf (stderr, "[ERROR] invalid MEM port 0x%.3hX\n", portaddr);
                exit (IOPORTS_BAD_PORT);
            }
            break;
    }

    switch (portaddr)
    {
        case RNG_CurrentValue:
            srand (value);
            break;

        default: // catch all- the standard transaction for external setting
            (pptr+attr) -> value.i32  = value;
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// update_ioports(): maintenance function to be run after each frame has completed
//
void  update_ioports (void)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize local variables
    //
    int32_t   type  = 0;     // type of port (category)
    int32_t   attr  = 0;     // specific port in the category
    data_t   *pptr  = NULL;  // pointer to port
    int32_t  *dptr  = NULL;  // pointer to port data

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // update INP_ ports, specifically the dpad and buttons: each frame the value
    // moves away from 0 by 1 (be it negative- no pressed, or positive- pressed)
    //
    type            = INP_PORT;
    pptr            = *(ioports+type); // pointer for sanity
    for (attr       = INP_GamepadLeft;
         attr      <= INP_GamepadButtonR;
         attr       = attr + 1)
    {
        dptr        = (pptr+attr) -> value.i32;  // pointer to port data
        if (*dptr  <  0) // if the button is NOT currently pressed
        {
            *dptr   = *dptr - 1;
        }
        else             // the button IS currently pressed
        {
            *dptr   = *dptr + 1;
        }
    }        
}
