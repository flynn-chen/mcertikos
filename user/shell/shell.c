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
#define PATH_SIZE 256

char buff[PATH_SIZE];
char dest_buff[PATH_SIZE];
char src_buff[PATH_SIZE];
char command_args[CMD_NUM_ARGS][PATH_SIZE];
char return_buff[10000];
char pwd_buff[PATH_SIZE];
char abs_dst[PATH_SIZE];
// 1 command: rm, ls etc.
// 2 flags: -r
// 3 target 1: filename, dirname
// 4 target 2: filename, dirname

void append_with_slash(char *orig, char *extra)
{
    int orig_len = strlen(orig);
    int extra_len = strlen(extra);
    orig[orig_len++] = '/';

    for (int i = 0; i < extra_len; i++)
    {
        orig[i + orig_len] = extra[i];
    }
    orig[orig_len + extra_len] = '\0';
}

void truncate(char *orig, int new_len)
{
    int orig_len = strlen(orig);
    while (orig_len > new_len)
    {
        orig[orig_len - 1] = '\0';
        orig_len--;
    }
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
    memzero(buff, PATH_SIZE);
    memzero(return_buff, 10000);
    memzero(pwd_buff, PATH_SIZE);
    memzero(abs_dst, PATH_SIZE);
    for (unsigned int command_idx = 0; command_idx < CMD_NUM_ARGS; command_idx++)
    {
        memzero(command_args[command_idx], PATH_SIZE);
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

int shell_copy_file(char *path1, char *path2)
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

int shell_recursive_rm(char *path)
{
    printf("called shell_recursive_rm with %s\n", path);
    int return_val, i;
    char subpath_buff[PATH_SIZE];
    char *current_file;
    int subpath_len;
    int is_dir_ret = is_directory(path);
    if (is_dir_ret == 0)
    {
        printf("rming %s\n", path);
        return_val = unlink(path);
        return return_val;
    }
    else if (is_dir_ret == 1)
    {
        printf("cding %s\n", path);
        if (chdir(path))
        {
            printf("rm: cd failed\n");
        }

        //get all files in current dir
        return_buff[0] = '.';
        return_buff[1] = '\0';
        return_val = ls(return_buff); //gotta use heap return buff cuz user stack reaches VM_USR_HI
        printf("return ls files: %s\n", return_buff);
        strncpy(subpath_buff, return_buff, PATH_SIZE);
        subpath_len = strlen(subpath_buff);
        for (i = 0; i < subpath_len; i++)
        {
            if (subpath_buff[i] == ' ')
            {
                subpath_buff[i] = '\0';
            }
        }

        //for every file in current dir
        for (i = 0; i < subpath_len; i += (strlen(current_file) + 1))
        {
            current_file = &subpath_buff[i];
            if (strcmp(current_file, ".") && strcmp(current_file, ".."))
            {
                printf("current file: %s\n", current_file);
                return_val = shell_recursive_rm(current_file);
                if (return_val)
                {
                    printf("rm: failed to remove %s\n", current_file);
                    return -1;
                }
            }
        }

        if (chdir(".."))
        {
            printf("rm: cd .. failed");
        }

        if (unlink(path))
        {
            printf("rm: unlink failed for %s\n", path);
            return -1;
        }
        return 0;
    }
    else
    {
        printf("is_dir failed. WHY!\n");
        return -1;
    }
}

// path = a/b/c
int shell_rm(char *path)
{
    if (!file_exist(path))
    {
        printf("rm: invalid path: %s\n", path);
        return -1;
    }
    int path_len = strlen(path);
    char *target = path;
    int should_cd = 1;
    for (int i = path_len - 1; i >= 0; i--)
    {
        if (i == path_len - 1 && path[i] == '/')
        {
            continue;
        }
        else if (path[i] == '/')
        {
            target = &path[i + 1];
            path[i] = '\0';
            break;
        }
        else if (i == 0) //rm -r a
        {
            target = path;
            should_cd = 0;
            break;
        }
    }
    printf("rm -r target: %s\n", target);
    pwd(pwd_buff, PATH_SIZE);
    if (should_cd)
    {
        chdir(path);
    }
    shell_recursive_rm(target);
    chdir(pwd_buff);
    return 0;
}

/*
    dest = absolute value of the FOLDER we fill in with source's contents

    /a/b
    /a/d/f
    /c/

    cp -r a c

    pwd = /a/
    dest = /c
    src = ""

    ls our pwd => find b, d
    for b
        do dest + src + b = /c/b
            shell_copy_file /c/b
    for d
        chdir(d)
        if file_exist: 
            if dir:
                ok, do nothing
            if file:
                fail
        else
            mkdir dest + src + d
        calls r(dest, src + d)
            pwd = /a/d
            dest = /c
            src = d

            ls our pwd => find f
            for f
                do dest + src + f => /c/d/f



    goal
    /a/b
    /c/b

*/
int shell_recursive_cp(char *dest, char *src)
{
    // printf("calling shell_recursive_cp with %d and %d\n", dest, src);
    char subpath_buff[PATH_SIZE];
    int orig_dest_len = strlen(dest);
    int orig_src_len = strlen(src);
    char *current_file;

    return_buff[0] = '.';
    return_buff[1] = '\0';
    int return_val = ls(return_buff); //gotta use heap return buff cuz user stack reaches VM_USR_HI
    if (return_val == -1)
    {
        printf("cp fail through ls\n");
        return -1;
    }

    //printf("return ls files: %s\n", return_buff);
    strncpy(subpath_buff, return_buff, PATH_SIZE);
    int subpath_len = strlen(subpath_buff);
    int i;

    for (i = 0; i < subpath_len; i++)
    {
        if (subpath_buff[i] == ' ')
        {
            subpath_buff[i] = '\0';
        }
    }

    //for every file in current dir
    for (i = 0; i < subpath_len; i += (strlen(current_file) + 1))
    {
        current_file = &subpath_buff[i];
        if (strcmp(current_file, ".") && strcmp(current_file, ".."))
        {
            printf("current file: %s\n", current_file);
            if (!is_directory(current_file))
            {
                // shell copy to: dest + src + b
                // printf("current state: %s and %s and %s\n", dest, src, current_file);
                append_with_slash(dest, src);
                append_with_slash(dest, current_file);
                // copy current_file to dest
                // printf("its not a dir, so we're copying %s to %s\n", current_file, dest);
                shell_copy_file(current_file, dest);
                truncate(dest, orig_dest_len);
            }
            else
            {
                chdir(current_file);
                append_with_slash(dest, src);
                append_with_slash(dest, current_file);
                if (file_exist(dest))
                {
                    if (!is_directory(dest))
                    {
                        printf("cp -r fail\n");
                        return -1;
                    }
                    // else: do nothing, directory is already there
                }
                else
                {
                    // create a new directory
                    mkdir(dest);
                }
                truncate(dest, orig_dest_len);
                append_with_slash(src, current_file);
                shell_recursive_cp(dest, src);
                truncate(dest, orig_dest_len);
                truncate(src, orig_src_len);
            }
        }
    }
    return 0;
}

// path = a/b/c
int shell_cp(char *dst, char *src)
{
    if (!file_exist(src))
    {
        printf("cp -r: invalid path: %s\n", src);
        return -1;
    }
    int src_len = strlen(src);
    char *src_target = src;
    int cp_ret_val;
    for (int i = src_len - 1; i >= 0; i--)
    {
        if (i == src_len - 1 && src[i] == '/')
        {
            continue;
        }
        else if (src[i] == '/')
        {
            src_target = &src[i + 1];
            src[i] = '\0';
            break;
        }
        else if (i == 0) //rm -r a
        {
            src_target = src;
            break;
        }
    }
    printf("cp -r src: %s to dst:%s\n", src_target, dst);
    pwd(pwd_buff, PATH_SIZE);
    printf("got pwd: %s\n", pwd_buff);

    /*
        /
        /a/b
        /c       <- file
        > cp -r a c
    */
    if (file_exist(dst) && !is_directory(dst))
    {
        printf("cp: %s: Not a directory\n", dst);
        return -1;
    }
    /*
        /
        /a/b
        /c/       <- directory
        > cp -r a c
        goal:
        /
        /a/b
        /c/a/b
    */
    if (is_directory(dst))
    {
        // make a new dir called src, and copy src's contents into it
        append_with_slash(dst, src_target);
        if (file_exist(dst))
        {
            if (!is_directory(dst))
            {
                printf("cp: %s: Not a directory\n", dst);
                return -1;
            }
            else
            {
                // it exists and is a dir, this is fine
            }
        }
        else
        {
            cp_ret_val = mkdir(dst);
            if (cp_ret_val != 0)
            {
                printf("cp -r: %s not a directory\n", dst);
                return -1;
            }
        }
    }
    /*
        /
        /a/b
        > cp -r a c
        goal:
        /
        /a/b
        /c/b
    */
    else
    {
        printf("HI! we couldn't find anything called %s\n", dst);
        // make a new dir called dst, and copy src'c contents into it
        cp_ret_val = mkdir(dst);
        if (cp_ret_val != 0)
        {
            printf("cp -r: %s not a directory\n", dst);
            return -1;
        }
    }

    // the contents of dst must equal the contents of src_target
    /*

    /a/b
    /d/a
    
    > cp -r a d
    ERROR

    */

    chdir(dst);
    pwd(abs_dst, PATH_SIZE);
    // printf("3 abs_dst: %s\n", abs_dst);
    chdir(pwd_buff);
    printf("cd'ing into initial src %d\n", src);
    chdir(src);
    memzero(src_target, strlen(src_target));
    shell_recursive_cp(abs_dst, src_target);
    printf("going back to %s\n", pwd_buff);
    chdir(pwd_buff);
    return 0;
    // if dst exists, we make a new dir inside of it that's called src
    // if dst doesn't exist, make a new dir called dst and fill it with src

    /*
        /
        /a/b

        > cp -r a c

        /
        /a/b
        /c/b

        > cp -r a c

        /
        /a/b
        /c/b
        /c/a/b
    
    */
    // make the target directory?

    /*
    cp -r /a/b/c   usr/d
    dst = usr/d
    src = /a/b/c

    want usr/d/c == /a/b/c

    store pwd 
        
    cd to destination
        usr/d/c
    store pwd into abs_dst <= gets us an absolute path to the destination
        abs_dst: /lib/usr/d/c

    cd to pwd
    cd to source
        in /a/b/c
    inspect and copy to abs_dst

    */
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
            // printf("about to call mkdir with %s\n", command_args[1]);
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
            pwd(return_buff, PATH_SIZE);
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
            cmd_ret_val = shell_copy_file(command_args[1], command_args[2]);
            if (cmd_ret_val == -1)
            {
                printf("cp: failed to read from %s\n", command_args[1]);
            }
            if (cmd_ret_val == -2)
            {
                printf("cp: failed to write to %s\n", command_args[2]);
            }
        }

        if (!strcmp(command_args[0], "cp") && !strcmp(command_args[1], "-r") && cmd_extract_status == 4)
        {
            strcpy(src_buff, command_args[2]);
            strcpy(dest_buff, command_args[3]);
            cmd_ret_val = shell_cp(dest_buff, src_buff);
            if (cmd_ret_val != 0)
            {
                printf("cp: failed to copy %s to %s\n", command_args[2], command_args[3]);
                // shell_rm(command_args[3]);
            }
        }

        if (!strcmp(command_args[0], "rm") && cmd_extract_status == 2)
        {
            if (!file_exist(command_args[1]))
            {
                printf("rm: invalid path: %s\n", command_args[1]);
            }
            else if (is_directory(command_args[1]) == 1)
            { // if it is a directory
                printf("rm: %s is a directory\n", command_args[1]);
            }
            else
            {
                unlink(command_args[1]);
            }
        }

        if (!strcmp(command_args[0], "rm") && !strcmp(command_args[1], "-r") && cmd_extract_status == 3)
        {
            shell_rm(command_args[2]);
        }

        if (!strcmp(command_args[0], "mv") && cmd_extract_status == 3)
        {
            strcpy(src_buff, command_args[1]);
            strcpy(dest_buff, command_args[2]);
            cmd_ret_val = shell_cp(dest_buff, src_buff);
            if (cmd_ret_val != 0)
            {
                printf("mv: failed to move %s to %s\n", command_args[1], command_args[2]);
                // shell_rm(command_args[2]);
            }
            shell_rm(command_args[1]);
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