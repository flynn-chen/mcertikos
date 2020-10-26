#include <proc.h>
#include <stdio.h>
#include <syscall.h>

int main(int argc, char **argv)
{
    unsigned int i;
    printf("ping started.\n");

    // fast producing
    for (i = 0; i < 20; i++)
        produce();

    // slow producing
    for (i = 0; i < 80; i++)
    {
        if (i % 4 == 0)
            produce();
    }
    printf("ping finished\n");

    return 0;
}
