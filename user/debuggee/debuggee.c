#include <stdio.h>

void do_stuff()
{
    printf("Hello, ");
}

int x;
int main()
{
    for (int i = 0; i < 4; i++)
        do_stuff();
    printf("world!\n");

    x = 5;
    printf("x = %d\n", x);
    x = 10;
    printf("x = %d\n", x);
    return 0;
}