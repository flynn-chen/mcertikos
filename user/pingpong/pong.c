#include <proc.h>
#include <stdio.h>
#include <syscall.h>

int main(int argc, char **argv)
{
    unsigned int i;
    printf("pong started.\n");

    for (i = 0; i < 160; i++)
    {
        if (i % 2 == 0)
            consume();
    }
    printf("pong finished\n");

    return 0;
}
