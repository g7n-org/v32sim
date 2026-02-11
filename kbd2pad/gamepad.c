#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define  BUTTONS 0
#define  PAD     1
#define  XAXIS   2
#define  YAXIS   3

// A B X Y L R START unused
int32_t  main (int32_t  argc, uint8_t **argv)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    FILE     *gamepad  = fopen ("/dev/hidg0", "w");
    int32_t   index    = 0;
    uint8_t  *packet   = (uint8_t *) malloc (sizeof (uint8_t) * 4);
    uint8_t  *inputs   = (packet+BUTTONS);
    uint8_t   pos      = 7;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Open the gadget device
    //
    if (gamepad       == NULL)
    {
        fprintf (stderr, "[ERROR] COULD NOT OPEN '/dev/hidg0' FOR WRITING!\n");
        gamepad        = fopen ("/dev/null", "w");
        //exit    (1);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Initialize the gadget packet
    //
    *(packet+BUTTONS)  = 0x00;
    *(packet+PAD)      = 0x00;
    *(packet+XAXIS)    = 0x00;
    *(packet+YAXIS)    = 0x00;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Verify that enough arguments were provided
    //
    if (argc          <  8)
    {
        fprintf (stderr, "[ERROR] NOT ENOUGH GAMEPAD INPUTS SPECIFIED!\n");
        exit    (2);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Convert arguments into individual button states, and pack the BUTTONS
    // packet byte
    //
    for (index         = 0;
         index        <  8;
         index         = index + 1)
    {
        *inputs       |= (atoi (*(argv+index)) << pos);
        pos            = pos   - 1;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Write the packet out to the gadget device
    //
    fwrite (packet, sizeof (uint8_t), 4, gamepad);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Display the states of each button
    //
    fprintf (stdout, "[inputs] A: %u, B: %u, X: %u, Y: %u, L: %u, R: %u, START: %u, unused: %u\n",
                     (((*inputs) & 0x80) >> 7),
                     (((*inputs) & 0x40) >> 6),
                     (((*inputs) & 0x20) >> 5),
                     (((*inputs) & 0x10) >> 4),
                     (((*inputs) & 0x08) >> 3),
                     (((*inputs) & 0x04) >> 2),
                     (((*inputs) & 0x02) >> 1),
					 (((*inputs) & 0x01)));

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Delay
    //
    sleep (4);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Close and de-allocate resources
    //
    fclose (gamepad);
    free   (packet);
    
    return (0);
}
