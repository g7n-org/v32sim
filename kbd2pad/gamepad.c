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
    FILE     *gamepad  = fopen ("/dev/hidg0", "w");
    int32_t   index    = 0;
	uint8_t   packet[4];
    uint8_t  *inputs   = (packet+BUTTONS);
    uint8_t   pos      = 7;

	*(packet+BUTTONS)  = 0x00;
	*(packet+PAD)      = 0x00;
	*(packet+XAXIS)    = 0x00;
	*(packet+YAXIS)    = 0x00;

    if (argc          <  8)
    {
        fprintf (stderr, "[ERROR] NOT ENOUGH GAMEPAD INPUTS SPECIFIED!\n");
        exit    (1);
    }

    for (index         = 0;
         index        <  8;
         index         = index + 1)
    {
        *inputs       |= (atoi (argv[index]) << pos);
        pos            = pos   - 1;
    }

	fwrite (packet, sizeof (uint8_t), 4, gamepad);
	fprintf (stdout, "[inputs] A: %u, B: %u, X: %u, Y: %u, L: %u, R: %u, START: %u\n",
					 (((*inputs) & 0x80) >> 7),
					 (((*inputs) & 0x40) >> 6),
					 (((*inputs) & 0x20) >> 5),
					 (((*inputs) & 0x10) >> 4),
					 (((*inputs) & 0x08) >> 3),
					 (((*inputs) & 0x04) >> 2),
					 (((*inputs) & 0x02) >> 1));

    sleep (4);

	fclose (gamepad);
    
    return (0);
}
