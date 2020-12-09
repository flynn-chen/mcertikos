#include <proc.h>
#include <stdio.h>
#include <syscall.h>
#include <x86.h>
#include <file.h>
#include <gcc.h>

#define LINE_BUF 100
#define CMD_NUM_ARGS 4
#define CMD_BUFF_SIZE 100
char command_args[CMD_NUM_ARGS][CMD_BUFF_SIZE];


int extract_cmd(char* line)
{
    unsigned int command_idx = 0;
    char *front, *end;
    int len, extra;

    // extract arguments
    end = line;
    for (command_idx = 0; command_idx < CMD_NUM_ARGS && *end != '\0'; command_idx++)
    {
        extra = 0;
        front = end;
        while (*front == ' ') //find first non-space element
        {
            front++;
        }

        if (*front == '"')
        {
            front++;
            end = front;
            while (*end != '"' && *end != '\0') //find the next space or hit the end
            {
                end++;
            }
            end++;
            extra = 1;
        }
        else
        {
            end = front;
            while (*end != ' ' && *end != '\0') //find the next space or hit the end
            {
                end++;
            }
        }

        len = end - front - extra; //calculate length of path element
        if (len + 1 > CMD_BUFF_SIZE)
        {
            return -1;
        }

        memmove(command_args[command_idx], front, len);
        command_args[command_idx][len] = '\0';
    }
    while (*end != '\0') //find the next space or hit the end
    {
        end++;
        if (*end != ' ')
            return -2;
    }
    for (command_idx = 0; command_idx < CMD_NUM_ARGS && command_args[command_idx][0] != '\0'; command_idx++)
        ;
    return command_idx;
}

int main(int argc, char *argv[])
{
    // printf("spawning debuggee\n");
    int user_pid = debug_spawn(7, 1000);
    char line[LINE_BUF];//, read_addr_buff[100];
    
    line[0] = '\0';
    long int address = 0;
    int ret;
    char *ptr;
    unsigned int len, cmd_extract_status;
    // printf("spawned debuggee as process %d\n", user_pid);

    // prompt for breakpoint locations
    printf("Enter: \n\t1. hex or integer to set breakpoint addresses \n\t2. 'start' or press enter to start debugee\n");
    while (1)
    {
        readline(line, LINE_BUF);
        if (strcmp(line, "start") == 0)
        {
            debug_start(user_pid);
            break;
        }
        if (strlen(line) != 0 && line[0] == '0' && line[1] == 'x')
        {
            address = strtol(&line[2], &ptr, 16);
            ret = add_breakpoint(user_pid, address);
            if (ret == 0)
            {
                printf("added breakpoint at 0x%08x\n", address);
            }
            else
            {
                printf("failed to add breakpoint at 0x%08xs\n", address);
            }
        }
        else if (strlen(line) != 0)
        {
            address = strtol(line, &ptr, 10);
            ret = add_breakpoint(user_pid, address);
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
    while (1)
    {
        // trap handler just called thread_yield_to after finding a PTE_BRK
        // printf("debugger started back up\n");
        readline(line, LINE_BUF);
        cmd_extract_status = extract_cmd(line);
        if (cmd_extract_status == 0) {
            continue;
        }

        if (strcmp(command_args[0], "continue") == 0 || strcmp(command_args[0], "c") == 0)
        {
            debug_start(user_pid);
        }
        else if (strcmp(command_args[0], "dump") == 0 || strcmp(command_args[0], "d") == 0) {
            // dump contents
            address = strtol(command_args[1], &ptr, 16);
            len = strtol(command_args[2], &ptr, 10);
            char read_addr_buff[len];
            memzero(read_addr_buff, len);
            printf("attempting to read %d bytes from %s (%d)\n", len, command_args[1], address);
            int ret = read_address(user_pid, (unsigned int) read_addr_buff, (unsigned int) address, len);
            
            if(ret == -1) {
                printf("failed to read contents at %s\n", command_args[1]);
            }
            else{
                printf("read %d bytes at %s: %s\n", len, command_args[1], read_addr_buff);
            }
        }

        for (unsigned int command_idx = 0; command_idx < CMD_NUM_ARGS; command_idx++)
        {
            memzero(command_args[command_idx], CMD_BUFF_SIZE);
        }
        // memzero(read_addr_buff, CMD_BUFF_SIZE);
    }
    // printf("woken up\n");
    // debug_end(user_pid);

    return 0;
}