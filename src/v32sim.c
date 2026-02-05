#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int32_t  main (int8_t  argc,  uint8_t **argv)
{
	FILE     *program   = NULL;
	int32_t   index     = 0;
	uint8_t  *data      = NULL;
	uint8_t   wordsize  = 4;
	uint32_t  offset    = 0x00000000;
	uint32_t  word      = 0x00000000;

	data                = (uint8_t *) malloc (sizeof (uint8_t) * 4);

	program             = fopen (argv[1], "r");

	fread (data, sizeof (uint8_t), wordsize, program);
	while (!feof (program))
	{
		fprintf (stdout, "[%.8X] ", offset);
		for (index      = 0;
			 index     <  wordsize;
			 index      = index + 1)
		{
			fprintf (stdout, "%.2hhX ", *(data+index));
		}
		fprintf (stdout, "\n");
		offset          = offset + 1;
		fread (data, sizeof (uint8_t), wordsize, program);
	}
	
	return (0);
}
