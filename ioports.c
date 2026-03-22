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
    // BIOS texture/regions is a 1D array of region_t's for managing the
    // regions present in the single BIOS texture.
    //
    bios_regions                           = (region_t *) calloc (sizeof (region_t),
                                                          V32_REGIONS_PER_TEXTURE); 

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
        (pptr+index) -> fmt                = FORMAT_SIGNED;
        (pptr+index) -> name               = (int8_t *) malloc (sizeof (int8_t) * 32);
        nptr                               = (pptr+index) -> name;

        switch (GPU_Command | index)
        {
            case GPU_Command:
                (pptr+index) -> flag       = FLAG_WRITE;
                (pptr+index) -> fmt        = FORMAT_HEX;
                sprintf (nptr, "GPU_Command");
                break;

            case GPU_RemainingPixels:
                (pptr+index) -> flag       = FLAG_READ;
                sprintf (nptr, "GPU_RemainingPixels");
                break;

            case GPU_ClearColor:
                sprintf (nptr, "GPU_ClearColor");
                (pptr+index) -> fmt        = FORMAT_HEX;
                break;

            case GPU_MultiplyColor:
                sprintf (nptr, "GPU_MultiplyColor");
                break;

            case GPU_ActiveBlending:
                sprintf (nptr, "GPU_ActiveBlending");
                break;

            case GPU_SelectedTexture:
                (pptr+index) -> value.i32  = -2;
                sprintf (nptr, "GPU_SelectedTexture");
                break;

            case GPU_SelectedRegion:
                (pptr+index) -> value.i32  = -2;
                sprintf (nptr, "GPU_SelectedRegion");
                break;

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
        (pptr+index) -> fmt                = FORMAT_UNSIGNED;
        (pptr+index) -> name               = (int8_t *) malloc (sizeof (int8_t) * 32);
        nptr                               = (pptr+index) -> name;

        switch (CAR_Connected | index)
        {
            case CAR_Connected:
                (pptr+index) -> fmt        = FORMAT_BOOLEAN;
                sprintf (nptr, "CAR_Connected");
                break;

            case CAR_ProgramROMSize:
                sprintf (nptr, "CAR_ProgramROMSize");
                break;

            case CAR_NumberOfTextures:
                (pptr+index) -> fmt        = FORMAT_SIGNED;
                sprintf (nptr, "CAR_NumberOfTextures");
                break;

            case CAR_NumberOfSounds:
                (pptr+index) -> fmt        = FORMAT_SIGNED;
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
    pptr -> fmt                            = FORMAT_BOOLEAN;
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
uint8_t  ioports_chk  (uint16_t  portaddr, uint8_t  mode, uint8_t  sys_force)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize local variables
    //
    uint16_t  type          = (portaddr & 0x0700) >> 8;  // port category
    uint16_t  attr          = (portaddr & 0x00FF);       // item within category
    uint8_t   flag          = FLAG_NONE;                 // short form access
    uint8_t   result        = TRUE;
    data_t   *pptr          = NULL;                      // pointer for sanity
    int8_t    error         = ERROR_NONE;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // ensure this function was called correctly (specify specific mode of
    // ioport transaction)
    //
    if ((mode              != FLAG_READ)  &&
        (mode              != FLAG_WRITE))
    {
        fprintf (stderr, "[ERROR] invalid IOPorts access mode\n");
        exit (IOPORTS_ACCESS_ERROR);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // based on the mode, set the likely error to occur (if an error occurs)
    //
    if (mode               == FLAG_READ)
    {
        error               = ERROR_PORT_READ;
    }
    else
    {
        error               = ERROR_PORT_WRITE;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // make sure the port being requested is a valid port within the category
    //
    if (type               <  7)
    {
        pptr                = *(ioports+type);
        if (attr           >= (pptr+0) -> qty)
        {
            fprintf (verbose, "[ERROR] invalid %.3s port 0x%.3hX\n",
                              (pptr+0) -> name, portaddr);
            sys_error       = error;
            result          = FALSE;
        }
    }
    else
    {
        fprintf (verbose, "[ERROR] invalid port 0x%.3hX\n", portaddr);
        sys_error           = error;
        result              = FALSE;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // if we get here: valid port within category, valid access method, now check
    // to make sure it is valid for the read/write transaction requested
    //
    if (result             != FALSE)
    {
        pptr                = *(ioports+type);
        flag                = (pptr+attr) -> flag;
        if ((flag & mode)  != mode)
        {
            if (sys_force  == FALSE)
            {
                if (mode   == FLAG_READ)
                {
                    fprintf (verbose, "[ERROR] port '%s' not accessible via READ!\n",
                                     (pptr+attr) -> name);
                }
                else
                {
                    fprintf (verbose, "[ERROR] port '%s' not accessible via WRITE!\n",
                                     (pptr+attr) -> name);
                }
                sys_error   = error;
                result      = FALSE;
            }
        }
    }

    return (result);
}

int16_t  ioports_num (uint8_t *symbol)
{
    data_t   *dtmp         = NULL;
    int32_t   count        = 0;
    int16_t   portnum      = -1;
    uint32_t  value        = 0;

    switch (symbol[0])
    {
        case 'T': // TIM_
        case 't': // TIM_
            value          = TIM_PORT;
            break;

        case 'R': // RNG_
        case 'r': // RNG_
            value          = RNG_PORT;
            break;

        case 'G': // GPU_
        case 'g': // GPU_
            value          = GPU_PORT;
            break;

        case 'S': // SPU_
        case 's': // SPU_
            value          = SPU_PORT;
            break;

        case 'I': // INP_
        case 'i': // INP_
            value          = INP_PORT;
            break;

        case 'C': // CAR_
        case 'c': // CAR_
            value          = CAR_PORT;
            break;

        case 'M': // MEM_
        case 'm': // MEM_
            value          = MEM_PORT;
            break;

        default:
            value          = -1;
            break;
    }

    if (value             != -1)
    {
        dtmp               = *(ioports+value);
        for (count         = 0;
             count        <  dtmp -> qty;
             count         = count + 1)
        {
            if (0         == (strcasecmp ((dtmp+count) -> name, symbol)))
            {
                portnum    = (value << 8) | count;
                break;
            }
        }
    }

    return (portnum);
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
    data_t   *result        = NULL;

    result                  = ioports_chk (portaddr, FLAG_READ, TRUE);
    if (result             != NULL)
    {
        result              = (pptr+attr);
    }

    return (result);
}

word_t *ioports_get  (uint16_t  portaddr, uint8_t  sys_force)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize local variables
    //
    data_t   *pptr                        = NULL;   // pointer for sanity
    uint16_t  type                        = 0;      // port category
    uint16_t  attr                        = 0;      // item within category
    uint8_t   check                       = FALSE;  // valid port status
    word_t   *result                      = NULL;   // return value

    check                                 = ioports_chk (portaddr, FLAG_READ, TRUE);
    if (check                            == TRUE)
    {
        type                              = (portaddr & 0x0700) >> 8;
        attr                              = (portaddr & 0x00FF);
        pptr                              = *(ioports+type);
        result                            = (word_t *) malloc (sizeof (word_t) * 1);
        switch (portaddr)
        {
            ////////////////////////////////////////////////////////////////////////////
            //
            // all the ports that transact in floating point values
            //
            case GPU_DrawingScaleX:
            case GPU_DrawingScaleY:
            case GPU_DrawingAngle:
            case SPU_GlobalVolume:
            case SPU_ChannelVolume:
            case SPU_ChannelSpeed:
                result -> f32             = (pptr+attr) -> value.f32;
                break;

            case RNG_CurrentValue: // obtain pseudorandom value, place in port
                (pptr+attr) -> value.i32  = rand ();
            default:
                result -> i32             = (pptr+attr) -> value.i32;
                break;
        }
    }

    return (result);
}

uint8_t  ioports_set (uint16_t  portaddr, int32_t  i32, float  f32, uint8_t  sys_force)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize local variables
    //
    uint16_t  type                = (portaddr & 0x0700) >> 8;  // port category
    uint16_t  attr                = (portaddr & 0x00FF);       // item within category
    int16_t   vtex                = 0;
    int16_t   id                  = 0;
    uint8_t   check               = FALSE;
    data_t   *pptr                = *(ioports+type);           // pointer for sanity
    region_t *rptr                = NULL;
    int32_t  *iptr                = NULL;
    int32_t  *fptr                = NULL;

    check                         = ioports_chk (portaddr, FLAG_WRITE, sys_force);
    if (check                    == TRUE)
    {
        iptr                      = &((pptr+attr) -> value.i32);
        fptr                      = &((pptr+attr) -> value.f32);
        switch (portaddr)
        {
            case RNG_CurrentValue:
                srand (i32);
                break;
                    
            case GPU_SelectedTexture:
                ////////////////////////////////////////////////////////////////////////
                //
                // Back up current selected region data in current texture
                //
                vtex              = IPORTGET(GPU_SelectedTexture);
                id                = IPORTGET(GPU_SelectedRegion);
                if (vtex         == -1)
                {
                    rptr          = (bios_regions+id);
                    rptr -> minX  = IPORTGET(GPU_RegionMinX);
                    rptr -> minY  = IPORTGET(GPU_RegionMinY);
                    rptr -> maxX  = IPORTGET(GPU_RegionMaxX);
                    rptr -> maxY  = IPORTGET(GPU_RegionMaxY);
                    rptr -> hotX  = IPORTGET(GPU_RegionHotspotX);
                    rptr -> hotY  = IPORTGET(GPU_RegionHotspotY);
                }
                else if (vtex    != -2)
                {
                    rptr          = (*(cart_regions+vtex)+id);
                    rptr -> minX  = IPORTGET(GPU_RegionMinX);
                    rptr -> minY  = IPORTGET(GPU_RegionMinY);
                    rptr -> maxX  = IPORTGET(GPU_RegionMaxX);
                    rptr -> maxY  = IPORTGET(GPU_RegionMaxY);
                    rptr -> hotX  = IPORTGET(GPU_RegionHotspotX);
                    rptr -> hotY  = IPORTGET(GPU_RegionHotspotY);
                }

                ////////////////////////////////////////////////////////////////////////
                //
                // Set the (texture) port
                //
                *iptr             = i32;
                vtex              = IPORTGET(GPU_SelectedTexture);

                ////////////////////////////////////////////////////////////////////////
                //
                // Restore region data based on selected texture
                //
                if (vtex         == -1)
                {
                    rptr          = (bios_regions+id);
                    PORTSET(GPU_RegionMinX,     rptr -> minX);
                    PORTSET(GPU_RegionMinY,     rptr -> minY);
                    PORTSET(GPU_RegionMaxX,     rptr -> maxX);
                    PORTSET(GPU_RegionMaxY,     rptr -> maxY);
                    PORTSET(GPU_RegionHotspotX, rptr -> hotX);
                    PORTSET(GPU_RegionHotspotY, rptr -> hotY);
                }
                else if (vtex    != -2)
                {
                    rptr          = (*(cart_regions+vtex)+id);
                    PORTSET(GPU_RegionMinX,     rptr -> minX);
                    PORTSET(GPU_RegionMinY,     rptr -> minY);
                    PORTSET(GPU_RegionMaxX,     rptr -> maxX);
                    PORTSET(GPU_RegionMaxY,     rptr -> maxY);
                    PORTSET(GPU_RegionHotspotX, rptr -> hotX);
                    PORTSET(GPU_RegionHotspotY, rptr -> hotY);
                }
                break;

            case GPU_SelectedRegion:
                ////////////////////////////////////////////////////////////////////////
                //
                // Back up current selected region data in current texture
                //
                vtex              = IPORTGET(GPU_SelectedTexture);
                id                = IPORTGET(GPU_SelectedRegion);
                if (vtex         == -1)
                {
                    rptr          = (bios_regions+id);
                    rptr -> minX  = IPORTGET(GPU_RegionMinX);
                    rptr -> minY  = IPORTGET(GPU_RegionMinY);
                    rptr -> maxX  = IPORTGET(GPU_RegionMaxX);
                    rptr -> maxY  = IPORTGET(GPU_RegionMaxY);
                    rptr -> hotX  = IPORTGET(GPU_RegionHotspotX);
                    rptr -> hotY  = IPORTGET(GPU_RegionHotspotY);
                }
                else if (vtex    != -2)
                {
                    rptr          = (*(cart_regions+vtex)+id);
                    rptr -> minX  = IPORTGET(GPU_RegionMinX);
                    rptr -> minY  = IPORTGET(GPU_RegionMinY);
                    rptr -> maxX  = IPORTGET(GPU_RegionMaxX);
                    rptr -> maxY  = IPORTGET(GPU_RegionMaxY);
                    rptr -> hotX  = IPORTGET(GPU_RegionHotspotX);
                    rptr -> hotY  = IPORTGET(GPU_RegionHotspotY);
                }

                ////////////////////////////////////////////////////////////////////////
                //
                // Set the (region) port
                //
                *iptr             = i32;
                id                = IPORTGET(GPU_SelectedRegion);

                ////////////////////////////////////////////////////////////////////////
                //
                // Restore region data based on selected region
                //
                if (vtex         == -1)
                {
                    rptr          = (bios_regions+id);
                    PORTSET(GPU_RegionMinX,     rptr -> minX);
                    PORTSET(GPU_RegionMinY,     rptr -> minY);
                    PORTSET(GPU_RegionMaxX,     rptr -> maxX);
                    PORTSET(GPU_RegionMaxY,     rptr -> maxY);
                    PORTSET(GPU_RegionHotspotX, rptr -> hotX);
                    PORTSET(GPU_RegionHotspotY, rptr -> hotY);
                }
                else if (vtex    != -2)
                {
                    rptr          = (*(cart_regions+vtex)+id);
                    PORTSET(GPU_RegionMinX,     rptr -> minX);
                    PORTSET(GPU_RegionMinY,     rptr -> minY);
                    PORTSET(GPU_RegionMaxX,     rptr -> maxX);
                    PORTSET(GPU_RegionMaxY,     rptr -> maxY);
                    PORTSET(GPU_RegionHotspotX, rptr -> hotX);
                    PORTSET(GPU_RegionHotspotY, rptr -> hotY);
                }
                break;

            ////////////////////////////////////////////////////////////////////////////
            //
            // all the ports that transact in floating point values
            //
            case GPU_DrawingScaleX:
            case GPU_DrawingScaleY:
            case GPU_DrawingAngle:
            case SPU_GlobalVolume:
            case SPU_ChannelVolume:
            case SPU_ChannelSpeed:
                *fptr             = f32;
                break;

            default: // catch all- the standard transaction for external setting
                *iptr             = i32;
                break;
        }
    }

    return (check);
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
    int32_t   dptr  = 0;     // port data

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
        if (dptr   <  0) // if the button is NOT currently pressed
        {
            dptr    = dptr - 1;
        }
        else             // the button IS currently pressed
        {
            dptr    = dptr + 1;
        }
    }        
}
