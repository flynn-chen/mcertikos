#include <proc.h>
#include <stdio.h>
#include <syscall.h>

int main(int argc, char **argv)
{
    unsigned int val = 300;
    unsigned int *addr = (unsigned int *)0xe0000000;

    printf("ding started.\n");
    printf("ding: the value at address %x: %d\n", addr, *addr);
    printf("ding: writing the value %d to the address %x\n", val, addr);
    *addr = val;
    yield();
    printf("ding: the new value at address %x: %d\n", addr, *addr);

    unsigned int pid = sys_fork();
    if (pid == 0)
    {
        printf("hello from the child\n");
    }
    else
    {
        printf("hello from the parent\n");
    }
    return 0;
}
