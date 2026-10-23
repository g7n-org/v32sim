#include <stdio.h>
#include <v32libc.c>
int main()
{
    int i;
    printf("Hello from v32libc, an 80x24 terminal\n");
    for (i = 0; i < 3; i++) printf("line %d: gjpqy,Q_ %d\n", i, i * 7);
    v32term_frame();
    return 0;
}
