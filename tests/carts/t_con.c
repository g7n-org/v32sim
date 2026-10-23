#include "video.h"
#include "time.h"
void main (void)
{
    int frames = 0;
    clear_screen (color_black);
    print_at (0, 0, "Hello, v32 console\nline two");
    print_at (20, 40, "x=\x7F\xe9");
    while (frames < 3) { end_frame (); frames = frames + 1; }
}
