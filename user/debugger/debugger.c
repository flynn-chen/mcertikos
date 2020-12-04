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
    int ret;
    char *ptr;
    // printf("spawned debuggee as process %d\n", user_pid);

    // prompt for input
    // 0x40000260 -> printf should get a breakpoint fault because the contents are copied to kernel memory
    // 0x40000f18 -> call printf from do_stuff
    // 0x40000f10 -> address of do_stuff
    // 0x40000000
    // 0x4000000e
    printf("Enter: \n\t1. hex or integer to set breakpoint addresses \n\t2. 'start' or press enter to start debugee\n");
    while (1)
    {
        readline(line, LINE_BUF);
        if (strcmp(line, "start") == 0)
        {
            break;
        }
        if (strlen(line) != 0 && line[0] == '0' && line[1] == 'x')
        {
            address = strtol(&line[2], &ptr, 16);
            ret = debug_invalidate(user_pid, address);
            if (ret == 0)
            {
                printf("invalidated 0x%08x successfully\n", address);
            }
            else
            {
                printf("failed to invalidate 0x%08xs\n", address);
            }
        }
        else if (strlen(line) != 0)
        {
            address = strtol(line, &ptr, 10);
            ret = debug_invalidate(user_pid, address);
            if (ret == 0)
            {
                printf("invalidated 0x%08x successfully\n", address);
            }
            else
            {
                printf("failed to invalidate 0x%08x\n", address);
            }
        }
    }

    /*
    
    0
    1   
    2   
    3   * <---   invalid, trap to kernel.
    4               re-validate so that the user can keep going, but also
    5               invalidate so if you ever return, it's still a breakpoint
        ideally, we somehow ignore the page fault caused by our invalidation

    */

    printf("starting debuggee\n");
    debug_start(user_pid);
    while (1)
    {
        // trap handler just called thread_yield_to after finding a PTE_BRK
        printf("debugger started back up\n");
        readline(line, LINE_BUF);
        debug_start(user_pid);
    }
    // printf("woken up\n");

    // debug_end(user_pid);

    return 0;
}