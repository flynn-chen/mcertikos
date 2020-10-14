#include <proc.h>
#include <stdio.h>
#include <syscall.h>
#include <x86.h>

uint32_t global_test = 0x12345678;

int main(int argc, char **argv)
{
    pid_t pid;

    printf("starting fork test with global at %p\n", &global_test);

    pid = sys_fork();

    if (pid == 0)
    {
        printf("Child pre fork, global (0x123456789) = %p\n", global_test);
        global_test = 0x1111;
        printf("Child pre fork, change global = %p\n", global_test);

        pid = sys_fork();

        if (pid == 0)
        {
            printf("Grandchild 1, global (0x1111) = %p\n", global_test);
            global_test = 0x2222;
            printf("Grandchild 1, change global = %p\n", global_test);
            return 0;
        }
        else
        {
            printf("Child forked new process %d\n", pid);
            printf("Child post fork 1, global (0x1111) = %p\n", global_test);
            global_test = 0x3333;
            printf("Child post fork 1, change global = %p\n", global_test);
        }

        pid = sys_fork();
        if (pid == 0)
        {
            printf("Grandchild 2, global (0x3333) = %p\n", global_test);
            global_test = 0x4444;
            printf("Grandchild 2, change global = %p\n", global_test);
            return 0;
        }
        else
        {
            printf("Child forked new process %d\n", pid);
            printf("Child post fork 2, global (0x3333) = %p\n", global_test);
            global_test = 0x5555;
            printf("Child post fork 2, change global = %p\n", global_test);
        }
    }
    else
    {
        printf("Parent forked new process %d\n", pid);
        printf("Parent post fork, global (0x12345678) = %p\n", global_test);
        global_test = 0x6666;
        printf("Parent post fork, change global = %p\n", global_test);
    }

    return 0;
}

// uint32_t global_test = 0x12345678;

// int main(int argc, char **argv)
// {

// pid_t pid;

// unsigned int *global_test = (unsigned int *)0xe0000000;
// *global_test = 0x12345678;

// printf("starting fork test\n");

// pid = sys_fork();

// if (pid == 0)
// {
//     pid = sys_fork();

//     if (pid == 0)
//     {
//         printf("This is grandchild, global = %p\n", *global_test);
//     }
//     else
//     {
//         printf("Child forks %d, global = %p\n", pid, *global_test);
//     }
// }
// else
// {
//     printf("parent forks %d, global = %p\n", pid, *global_test);
//     *global_test = 0x5678;
//     printf("hi\n");
//     printf("parent global_test1 = %p\n", *global_test);
// }

// return 0;
// }
