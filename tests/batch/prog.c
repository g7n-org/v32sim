#include "video.h"
#include "time.h"

int g;
int counter;
int buf[4];
int neg;

int f (int x)
{
    g = x + 1;
    return g;
}

int work (int n)
{
    int i, s;
    s = 0;
    for (i = 0; i < n; i++)
        s = s + f (i);
    return s;
}

void main (void)
{
    while (1)
    {
        clear_screen (color_blue);
        counter = counter + 1;
        buf[2] = work (5);
        neg = 3 - counter;
        end_frame ();
    }
}
