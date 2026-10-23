#include <stdio.h>
#include <v32libc.c>
int main()
{
    char *s = "RED GREEN";
    int i;
    v32term_init();
    for (i = 0; s[i]; i++)
        v32term_set(0, i, s[i] | V32TERM_INK(i < 3 ? V32INK_RED : (i > 3 ? V32INK_GREEN : 0)));
    v32term_set(1, 0, 'R' | V32TERM_REVERSE | V32TERM_INK(V32INK_BLUE));
    v32term_set(1, 1, 'V' | V32TERM_REVERSE);
    v32term_set(1, 3, V32TERM_SHADE_SOLID | V32TERM_INK(V32INK_YELLOW));
    v32term_frame();
    return 0;
}
