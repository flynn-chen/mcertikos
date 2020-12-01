#include <proc.h>
#include <stdio.h>
#include <syscall.h>
#include <x86.h>
#include <file.h>
#include <gcc.h>

#define LINE_BUF 100

int main(int argc, char *argv[])
{
    // printf("spawning debuggee\n");
    int user_pid = debug_spawn(7, 1000);
    char line[LINE_BUF];
    line[0] = '\0';
    long int address = 0;
    char *ptr;
    // printf("spawned debuggee as process %d\n", user_pid);

    // prompt for input
    printf("Enter: \n\t1. integer to set breakpoint addresses \n\t2. 'start' or press enter to start debugee\n");
    while (strcmp(line, "start") != 0)
    {
        readline(line, LINE_BUF);
        if (strlen(line) != 0 && line[0] == '0' && line[1] == 'x')
        {
            address = strtol(line[2], &ptr, 16);
        }
        else if (strlen(line) != 0)
        {
            address = strtol(line, &ptr, 10);
        }
        else
        {
            break;
        }
    }

    // printf("starting debuggee\n");
    debug_start(user_pid);
    // printf("woken up\n");

    return 0;
}