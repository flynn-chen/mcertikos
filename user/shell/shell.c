#include <proc.h>
#include <stdio.h>
#include <syscall.h>
#include <x86.h>
#include <file.h>
#include <gcc.h>

#define exit(...) return __VA_ARGS__
#define CMD_NUM_ARGS 4
#define CMD_BUFF_SIZE 10000
#define DIRSIZ 14

char buff[CMD_BUFF_SIZE];
char command_args[CMD_NUM_ARGS][CMD_BUFF_SIZE];
char return_buff[10000];
// 1 command: rm, ls etc.
// 2 flags: -r
// 3 target 1: filename, dirname
// 4 target 2: filename, dirname

int extract_cmd()
{
    unsigned int command_idx = 0;
    char *front, *end;
    int len;

    // extract arguments
    end = buff;
    for (command_idx = 0; command_idx < CMD_NUM_ARGS && *end != '\0'; command_idx++)
    {
        front = end;
        while (*front == ' ') //find first non-space element
        {
            front++;
        }

        end = front;
        while (*end != ' ' && *end != '\0') //find the next space or hit the end
        {
            end++;
        }

        len = end - front; //calculate length of path element
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

void zero_cmd_buff()
{
    memzero(buff, CMD_BUFF_SIZE);
    memzero(return_buff, 10000);
    for (unsigned int command_idx = 0; command_idx < CMD_NUM_ARGS; command_idx++)
    {
        memzero(command_args[command_idx], CMD_BUFF_SIZE);
    }
}

int shell_cat(char *path)
{
    int fd;
    fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        return -1;
    }
    read(fd, return_buff, CMD_BUFF_SIZE - 1);
    return_buff[CMD_BUFF_SIZE - 1] = '\0';
    close(fd);
    return 0;
}

int shell_write(char *string, char *path)
{
    int fd, write_size;
    fd = open(path, O_CREATE | O_RDWR);
    if (fd < 0)
    {
        return -1;
    }
    write_size = write(fd, string, strlen(string));
    if (write_size != strlen(string))
    {
        return -2;
    }
    close(fd);
    return 0;
}

int shell_append(char *string, char *path)
{
    if (shell_cat(path) != 0)
    {
        return -1;
    }
    unsigned int length = strlen(string);
    if (length > CMD_BUFF_SIZE - strlen(return_buff))
    {
        length = CMD_BUFF_SIZE - strlen(return_buff) - 1;
    }
    strncpy(&return_buff[strlen(return_buff)], string, length);
    if (shell_write(return_buff, path) != 0)
    {
        return -2;
    }
    return 0;
}

int shell_copy(char *path1, char *path2)
{
    if (shell_cat(path1) != 0)
    {
        return -1;
    }
    if (shell_write(return_buff, path2) != 0)
    {
        return -2;
    }
    return 0;
}

int file_exist(char *path)
{
    int fd;
    fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        return 0;
    }
    close(fd);
    return 1;
}

int is_directory(char *path)
{
    int is_dir_status;
    if (file_exist(path))
    {
        is_dir_status = is_dir(path);
    }
    else
    {
        return 0;
    }
    return is_dir_status;
}

int main(int argc, char *argv[])
{
    while (1)
    {
        zero_cmd_buff();
        printf("> ");
        readline(buff);
        int cmd_extract_status = extract_cmd();
        int cmd_ret_val = 0;
        switch (cmd_extract_status)
        {
        case 0:
            printf("no arguments specified\n");
        case -1:
            printf("argument element too long\n");
            continue;
        case -2:
            printf("too many arguments\n");
            continue;
        }

        // for (unsigned int command_idx = 0; command_idx < CMD_NUM_ARGS; command_idx++)
        // {
        //     printf("argument #%d length: %d: %s \n", command_idx, strlen(command_args[command_idx]), command_args[command_idx]);
        // }

        if (!strcmp(command_args[0], "touch"))
        {
            if (cmd_extract_status != 2)
            {
                printf("wrong arguments for touch with %d argument\n", cmd_extract_status);
                continue;
            }
            cmd_ret_val = open(command_args[1], O_CREATE | O_RDWR);
            if (cmd_ret_val < 0)
            {
                printf("%s %s failed\n", command_args[0], command_args[1]);
            }
            close(cmd_ret_val);
            continue;
        }

        if (!strcmp(command_args[0], "mkdir"))
        {
            if (cmd_extract_status != 2)
            {
                printf("wrong arguments for mkdir with %d argument\n", cmd_extract_status);
                continue;
            }
            printf("about to call mkdir with %s\n", command_args[1]);
            cmd_ret_val = mkdir(command_args[1]);
            if (cmd_ret_val < 0)
            {
                printf("%s %s failed\n", command_args[0], command_args[1]);
            }
            continue;
        }

        if (!strcmp(command_args[0], "cd"))
        {
            if (cmd_extract_status != 2)
            {
                printf("cd: wrong arguments\n");
                continue;
            }
            cmd_ret_val = chdir(command_args[1]);
            if (cmd_ret_val < 0)
            {
                printf("cd: cd %s failed\n", command_args[1]);
            }
            continue;
        }

        if (!strcmp(command_args[0], "ls"))
        {
            if (cmd_extract_status == 1)
            {
                // move pwd into some buffer
                command_args[1][0] = '.';
                command_args[1][1] = '\0';
                cmd_ret_val = ls(command_args[1]);
                printf("%s\n", command_args[1]);
            }

            if (cmd_extract_status == 2)
            {
                if (is_directory(command_args[1]) == 1)
                { // if it is a directory
                    // printf("%s is a directory\n", command_args[1]);
                    cmd_ret_val = ls(command_args[1]);
                    printf("%s\n", command_args[1]);
                }
                else
                { // if it is a file
                    // printf("%s is a file\n", command_args[1]);
                    if (file_exist(command_args[1]))
                    {
                        printf("%s\n", command_args[1]);
                    }
                }
            }
        }

        if (!strcmp(command_args[0], "pwd"))
        {
            if (cmd_extract_status != 1)
            {
                printf("pwd: wrong number of arguments");
            }
            pwd(return_buff);
            printf("%s\n", return_buff);
        }

        if (!strcmp(command_args[0], "cat"))
        {
            if (cmd_extract_status != 2)
            {
                printf("cat: wrong number of arguments");
            }
            cmd_ret_val = shell_cat(command_args[1]);
            if (cmd_ret_val != 0)
            {
                printf("cat: cannot open file %s\n", command_args[1]);
            }
            else
            {
                printf("%s\n", return_buff);
            }
        }

        if (!strcmp(command_args[0], "write") && cmd_extract_status == 3)
        {
            if (cmd_extract_status != 3)
            {
                printf("write: wrong number of arguments");
            }
            cmd_ret_val = shell_write(command_args[1], command_args[2]);
            if (cmd_ret_val == -1)
            {
                printf("write: cannot open file %s\n", command_args[2]);
            }
            if (cmd_ret_val == -2)
            {
                printf("write: did not write enough bytes to %s\n", command_args[2]);
            }
        }

        if (!strcmp(command_args[0], "append") && cmd_extract_status == 3)
        {
            if (cmd_extract_status != 3)
            {
                printf("append: wrong number of arguments");
            }
            cmd_ret_val = shell_append(command_args[1], command_args[2]);
            if (cmd_ret_val == -1)
            {
                printf("append: failed to read from %s\n", command_args[2]);
            }
            if (cmd_ret_val == -2)
            {
                printf("append: failed to write to %s\n", command_args[2]);
            }
        }

        if (!strcmp(command_args[0], "cp") && cmd_extract_status == 3)
        {
            if (cmd_extract_status != 3)
            {
                printf("cp: wrong number of arguments");
            }
            cmd_ret_val = shell_copy(command_args[1], command_args[2]);
            if (cmd_ret_val == -1)
            {
                printf("cp: failed to read from %s\n", command_args[2]);
            }
            if (cmd_ret_val == -2)
            {
                printf("cp: failed to write to %s\n", command_args[2]);
            }
        }

        if (!strcmp(command_args[0], "rm") && cmd_extract_status == 2)
        {
            if (is_directory(command_args[1]) == 1)
            { // if it is a directory
                printf("rm: %s is a directory\n", command_args[1]);
            }
            else
            {
                unlink(command_args[1]);
            }
        }
    }
    return 0;
}

/*
    How to wire up new system calls:

        0. Define a macro somewhere to route "name" to "sys_name" (ex: define it in /user/include/file.h)
        1. Write new inline assembly in /user/include/syscall.h
        2. Add new SYS_name enum to /kern/lib/syscall.h
        3. Add new switch case for SYS_name in /kern/trap/TDispatch/TDispatch.c
        4. Write new sys_name function (ex: write it in /kern/trap/TSyscall/TSyscall.c or /kern/fs/sysfile.c)
            Make sure to add it to the header file!!

*/