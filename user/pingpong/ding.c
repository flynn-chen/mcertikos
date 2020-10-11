#include <proc.h>
#include <stdio.h>
#include <syscall.h>
#include <types.h>

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

    pid_t pid = sys_fork();
    if (pid == 0)
    {
        printf("hello from child\n");
        printf("ding child: the shallow copy value at address %x: %d\n", addr, *addr);
        *addr = val + 1;
        printf("ding child: the deep copy value at address %x: %d\n", addr, *addr);
    }
    else
    {
        printf("hello from parent\n");
        printf("ding parent: the new value at address %x: %d\n", addr, *addr);
    }
    return 0;
}
