// File-system system calls.

#include <kern/lib/types.h>
#include <kern/lib/debug.h>
#include <kern/lib/pmap.h>
#include <kern/lib/string.h>
#include <kern/lib/trap.h>
#include <kern/lib/syscall.h>
#include <kern/thread/PTCBIntro/export.h>
#include <kern/thread/PCurID/export.h>
#include <kern/trap/TSyscallArg/export.h>

#include "dir.h"
#include "path.h"
#include "file.h"
#include "fcntl.h"
#include "log.h"

/**
 * This function is not a system call handler, but an auxiliary function
 * used by sys_open.
 * Allocate a file descriptor for the given file.
 * You should scan the list of open files for the current thread
 * and find the first file descriptor that is available.
 * Return the found descriptor or -1 if none of them is free.
 */
static int fdalloc(struct file *f)
{
    unsigned int pid = get_curid();
    struct file **openfiles = tcb_get_openfiles(pid);
    for (int i = 0; i < NOFILE; i++)
    {
        if (openfiles[i] == 0)
        {
            tcb_set_openfiles(pid, i, f);
            return i;
        }
    }
    return -1;
}

/**
 * From the file indexed by the given file descriptor, read n bytes and save them
 * into the buffer in the user. As explained in the assignment specification,
 * you should first write to a kernel buffer then copy the data into user buffer
 * with pt_copyout.
 * Return Value: Upon successful completion, read() shall return a non-negative
 * integer indicating the number of bytes actually read. Otherwise, the
 * functions shall return -1 and set errno E_BADF to indicate the error.
 */

char buff[10000];
void sys_read(tf_t *tf)
{
    int fd = syscall_get_arg2(tf);
    if (fd < 0 || fd >= NOFILE)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        return;
    }

    size_t n = syscall_get_arg4(tf);
    if (n < 0 || n > 10000)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_INVAL_ADDR);
        return;
    }
    // char buff[n];

    struct file *fp = tcb_get_openfiles(get_curid())[fd];
    if (fp == 0 || fp->type == FD_NONE || fp->ip == 0)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        return;
    }

    unsigned int uva = syscall_get_arg3(tf); //check user address doesn't falls in kernel mem
    if (uva < VM_USERLO || uva + n > VM_USERHI)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_INVAL_ADDR);
        return;
    }

    // KERN_DEBUG("reading the file\n");
    int num_read = file_read(fp, buff, n);
    // KERN_DEBUG("character read %d the file\n", num_read);

    // size_t pt_copyout(void *kva, uint32_t pmap_id, uintptr_t uva, size_t len)
    pt_copyout(buff, get_curid(), uva, num_read);
    syscall_set_retval1(tf, num_read);
    syscall_set_errno(tf, E_SUCC);
}

/**
 * Write n bytes of data in the user's buffer into the file indexed by the file descriptor.
 * You should first copy the data info an in-kernel buffer with pt_copyin and then
 * pass this buffer to appropriate file manipulation function.
 * Upon successful completion, write() shall return the number of bytes actually
 * written to the file associated with f. This number shall never be greater
 * than nbyte. Otherwise, -1 shall be returned and errno E_BADF set to indicate the
 * error.
 */
void sys_write(tf_t *tf)
{
    int fd = syscall_get_arg2(tf);
    if (fd < 0 || fd >= NOFILE)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        return;
    }

    size_t n = syscall_get_arg4(tf);
    if (n < 0 || n > 10000)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_INVAL_ADDR);
        return;
    }
    // char buff[n];

    struct file *fp = tcb_get_openfiles(get_curid())[fd];
    if (fp == 0 || fp->type == FD_NONE || fp->ip == 0)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        return;
    }

    unsigned int uva = syscall_get_arg3(tf); //check user address doesn't falls in kernel mem
    if (uva < VM_USERLO || uva + n > VM_USERHI)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_INVAL_ADDR);
        return;
    }

    pt_copyin(get_curid(), uva, buff, n);
    int num_write = file_write(fp, buff, n);
    syscall_set_retval1(tf, num_write);
    syscall_set_errno(tf, E_SUCC);
}

/**
 * Return Value: Upon successful completion, 0 shall be returned; otherwise, -1
 * shall be returned and errno E_BADF set to indicate the error.
 */
void sys_close(tf_t *tf)
{
    int fd = syscall_get_arg2(tf);
    if (fd < 0 || fd >= NOFILE)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        return;
    }

    struct file *fp = tcb_get_openfiles(get_curid())[fd];
    if (fp == 0 || fp->type == FD_NONE || fp->ip == 0)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        return;
    }

    file_close(fp);
    tcb_set_openfiles(get_curid(), fd, 0);
    syscall_set_retval1(tf, 0);
    syscall_set_errno(tf, E_SUCC);
}

/**
 * Return Value: Upon successful completion, 0 shall be returned. Otherwise, -1
 * shall be returned and errno E_BADF set to indicate the error.
 */
void sys_fstat(tf_t *tf)
{
    int fd = syscall_get_arg2(tf);
    if (fd < 0 || fd >= NOFILE)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        return;
    }

    struct file_stat *user_fsp = (struct file_stat *)syscall_get_arg3(tf);
    if ((unsigned int)user_fsp < VM_USERLO || (unsigned int)user_fsp + sizeof(struct file_stat) > VM_USERHI)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_INVAL_ADDR);
        return;
    }

    struct file *fp = tcb_get_openfiles(get_curid())[fd];
    if (fp == 0 || fp->type == FD_NONE)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        return;
    }

    struct file_stat kern_fs;
    int state = file_stat(fp, &kern_fs);
    if (state != 0)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
    }

    pt_copyout(&kern_fs, get_curid(), (unsigned int)user_fsp, sizeof(struct file_stat));
    syscall_set_retval1(tf, 0);
    syscall_set_errno(tf, E_SUCC);
    return;
}

/**
 * Create the path new as a link to the same inode as old.
 */
void sys_link(tf_t *tf)
{
    int length_old = syscall_get_arg4(tf);
    int length_new = syscall_get_arg5(tf);
    char name[DIRSIZ], new[length_new + 1], old[length_old + 1];
    struct inode *dp, *ip;

    pt_copyin(get_curid(), syscall_get_arg2(tf), old, length_old);
    pt_copyin(get_curid(), syscall_get_arg3(tf), new, length_new);
    old[length_old] = '\0';
    new[length_new] = '\0';

    if ((ip = namei(old)) == 0)
    {
        syscall_set_errno(tf, E_NEXIST);
        return;
    }

    begin_trans();

    inode_lock(ip);
    if (ip->type == T_DIR)
    {
        inode_unlockput(ip);
        commit_trans();
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }

    ip->nlink++;
    inode_update(ip);
    inode_unlock(ip);

    if ((dp = nameiparent(new, name)) == 0)
        goto bad;
    inode_lock(dp);
    if (dp->dev != ip->dev || dir_link(dp, name, ip->inum) < 0)
    {
        inode_unlockput(dp);
        goto bad;
    }
    inode_unlockput(dp);
    inode_put(ip);

    commit_trans();

    syscall_set_errno(tf, E_SUCC);
    return;

bad:
    inode_lock(ip);
    ip->nlink--;
    inode_update(ip);
    inode_unlockput(ip);
    commit_trans();
    syscall_set_errno(tf, E_DISK_OP);
    return;
}

/**
 * Is the directory dp empty except for "." and ".." ?
 */
static int isdirempty(struct inode *dp)
{
    int off;
    struct dirent de;

    for (off = 2 * sizeof(de); off < dp->size; off += sizeof(de))
    {
        if (inode_read(dp, (char *)&de, off, sizeof(de)) != sizeof(de))
            KERN_PANIC("isdirempty: readi");
        if (de.inum != 0)
            return 0;
    }
    return 1;
}

void sys_unlink(tf_t *tf)
{
    struct inode *ip, *dp;
    struct dirent de;
    int length = syscall_get_arg3(tf);
    char name[DIRSIZ], path[length + 1];
    uint32_t off;

    pt_copyin(get_curid(), syscall_get_arg2(tf), path, length);
    path[length] = '\0';
<<<<<<<
    // KERN_DEBUG("unlinking: %s\n", path);
=======
>>>>>>> 87ecf013... 11112020

    if ((dp = nameiparent(path, name)) == 0)
    {
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }

    // KERN_DEBUG("got parent %d with ref %d to unlink: %s\n", dp->inum, dp->ref, name);

    begin_trans();

    inode_lock(dp);

    // KERN_DEBUG("locked parent to unlink: %s\n", path);

    // Cannot unlink "." or "..".
    if (dir_namecmp(name, ".") == 0 || dir_namecmp(name, "..") == 0)
        goto bad;

    if ((ip = dir_lookup(dp, name, &off)) == 0)
        goto bad;
    inode_lock(ip);

    // KERN_DEBUG("locked inode with ref %d to unlink: %s\n", ip->ref, path);

    if (ip->nlink < 1)
        KERN_PANIC("unlink: nlink < 1");
    if (ip->type == T_DIR && !isdirempty(ip))
    {
        inode_unlockput(ip);
        goto bad;
    }

    memset(&de, 0, sizeof(de));
    if (inode_write(dp, (char *)&de, off, sizeof(de)) != sizeof(de))
        KERN_PANIC("unlink: writei");
    if (ip->type == T_DIR)
    {
        dp->nlink--;
        inode_update(dp);
    }
    inode_unlockput(dp);

    ip->nlink--;
    inode_update(ip);
    inode_unlockput(ip);

    commit_trans();

    syscall_set_errno(tf, E_SUCC);
    return;

bad:
    inode_unlockput(dp);
    commit_trans();
    syscall_set_errno(tf, E_DISK_OP);
    return;
}

static struct inode *create(char *path, short type, short major, short minor)
{
    uint32_t off;
    struct inode *ip, *dp;
    char name[DIRSIZ];

    if ((dp = nameiparent(path, name)) == 0)
        return 0;
    inode_lock(dp);

    if ((ip = dir_lookup(dp, name, &off)) != 0)
    {
        inode_unlockput(dp);
        inode_lock(ip);
        if (type == T_FILE && ip->type == T_FILE)
            return ip;
        inode_unlockput(ip);
        return 0;
    }

    if ((ip = inode_alloc(dp->dev, type)) == 0)
        KERN_PANIC("create: ialloc");

    inode_lock(ip);
    ip->major = major;
    ip->minor = minor;
    ip->nlink = 1;
    inode_update(ip);

    if (type == T_DIR)
    {                // Create . and .. entries.
        dp->nlink++; // for ".."
        inode_update(dp);
        // No ip->nlink++ for ".": avoid cyclic ref count.
        if (dir_link(ip, ".", ip->inum) < 0 || dir_link(ip, "..", dp->inum) < 0)
            KERN_PANIC("create dots");
    }

    if (dir_link(dp, name, ip->inum) < 0)
        KERN_PANIC("create: dir_link");

    inode_unlockput(dp);
    return ip;
}

void sys_open(tf_t *tf)
{
    int length = syscall_get_arg4(tf);
    char path[length + 1];
    int fd, omode;
    struct file *f;
    struct inode *ip;

    static int first = TRUE;
    if (first)
    {
        first = FALSE;
        log_init();
    }

    pt_copyin(get_curid(), syscall_get_arg2(tf), path, length);
    path[length] = '\0';
    omode = syscall_get_arg3(tf);
    // KERN_DEBUG("opening: %s\n", path);

    if (omode & O_CREATE)
    {
        begin_trans();
        ip = create(path, T_FILE, 0, 0);
        commit_trans();
        //KERN_DEBUG("created file\n");
        if (ip == 0)
        {
            //KERN_DEBUG("file create failed\n");
            syscall_set_retval1(tf, -1);
            syscall_set_errno(tf, E_CREATE);
            return;
        }
    }
    else
    {
        if ((ip = namei(path)) == 0)
        {
            syscall_set_retval1(tf, -1);
            syscall_set_errno(tf, E_NEXIST);
            return;
        }
        inode_lock(ip);
        if (ip->type == T_DIR && omode != O_RDONLY)
        {
            inode_unlockput(ip);
            syscall_set_retval1(tf, -1);
            syscall_set_errno(tf, E_DISK_OP);
            return;
        }
    }

    if ((f = file_alloc()) == 0 || (fd = fdalloc(f)) < 0)
    {
        // KERN_DEBUG("disk operation error, f=%d, fd=%d\n", f, fd);
        if (f)
            file_close(f);
        inode_unlockput(ip);
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }
    inode_unlock(ip);

    // KERN_DEBUG("file creating\n");
    f->type = FD_INODE;
    f->ip = ip;
    f->off = 0;
    f->readable = !(omode & O_WRONLY);
    f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
    syscall_set_retval1(tf, fd);
    syscall_set_errno(tf, E_SUCC);
}

void sys_mkdir(tf_t *tf)
{
    int length = syscall_get_arg3(tf);
    char path[length + 1];
    struct inode *ip;

    pt_copyin(get_curid(), syscall_get_arg2(tf), path, length);
    path[length] = '\0';

    begin_trans();
    if ((ip = (struct inode *)create(path, T_DIR, 0, 0)) == 0)
    {
        commit_trans();
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }
    inode_unlockput(ip);
    commit_trans();
    syscall_set_errno(tf, E_SUCC);
}

void sys_chdir(tf_t *tf)
{
    int length = syscall_get_arg3(tf);
    char path[length + 1];
    struct inode *ip;
    int pid = get_curid();

    pt_copyin(get_curid(), syscall_get_arg2(tf), path, length);
    path[length] = '\0';

    if ((ip = namei(path)) == 0)
    {
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }
    inode_lock(ip);
    if (ip->type != T_DIR)
    {
        inode_unlockput(ip);
        syscall_set_errno(tf, E_DISK_OP);
        return;
    }
    inode_unlock(ip);
    inode_put(tcb_get_cwd(pid));
    tcb_set_cwd(pid, ip);
    syscall_set_errno(tf, E_SUCC);
}

/**
 * Return Value: Upon successful completion, 0 shall be returned; otherwise, -1
 * shall be returned and errno E_BADF set to indicate the error.
 */

#define LS_BUFF_SIZE 10000
char ls_buff[LS_BUFF_SIZE];
void sys_ls(tf_t *tf)
{
    // zero out the ls buffer
    memset(ls_buff, 0, LS_BUFF_SIZE);
    unsigned int uva = syscall_get_arg2(tf); //check user address doesn't falls in kernel mem
    int path_length = syscall_get_arg3(tf);
    if (uva < VM_USERLO || uva + LS_BUFF_SIZE > VM_USERHI)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_INVAL_ADDR);
        return;
    }
    // copy user buffer into ls buffer
    pt_copyin(get_curid(), syscall_get_arg2(tf), ls_buff, path_length);
    ls_buff[path_length] = '\0';

    // find the inode corresponding to given path
    struct inode *dp = (struct inode *)namei(ls_buff);

    // zero out the ls_buff
    memset(ls_buff, 0, LS_BUFF_SIZE);
    // overwrite user buffer with 0
    pt_copyout(ls_buff, get_curid(), uva, path_length + 1);

    struct dirent de;
    unsigned int de_size = sizeof(de);
    int file_length;
    char *front = ls_buff; // char buffer to store file output i.e. "README.md file.py ..."
    for (unsigned int off = 0; off < dp->size; off += de_size)
    {
        if (inode_read(dp, (char *)&de, off, de_size) != de_size)
        {
            KERN_PANIC("sys_ls: readi");
        }
        if (de.inum == 0)
        {
            continue;
        }

        file_length = strnlen(de.name, DIRSIZ);
        strncpy(front, de.name, file_length);
        front += file_length;
        *(front++) = ' ';
    }

    int buff_length = front - ls_buff;
    if (buff_length >= LS_BUFF_SIZE)
    {
        buff_length = LS_BUFF_SIZE - 1;
    }
    ls_buff[buff_length] = '\0';
    pt_copyout(ls_buff, get_curid(), uva, LS_BUFF_SIZE);
    syscall_set_retval1(tf, 0);
    syscall_set_errno(tf, E_SUCC);
}

#define IS_DIR_BUFF_SIZE 10000
char is_dir_buff[IS_DIR_BUFF_SIZE];
void sys_is_dir(tf_t *tf)
{
    unsigned int uva = syscall_get_arg2(tf); //check user address doesn't falls in kernel mem
    int path_length = syscall_get_arg3(tf);
    if (uva < VM_USERLO || uva + path_length + 1 > VM_USERHI)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_INVAL_ADDR);
        return;
    }

    pt_copyin(get_curid(), syscall_get_arg2(tf), is_dir_buff, path_length);
    is_dir_buff[path_length] = '\0';

    struct inode *dp = (struct inode *)namei(is_dir_buff);

    if (dp == 0)
    {
        syscall_set_errno(tf, E_BADF);
        syscall_set_retval1(tf, -1);
    }
    syscall_set_errno(tf, E_SUCC);

    if (dp->type == T_DIR)
    {
        syscall_set_retval1(tf, 1);
        return;
    }
    else if (dp->type == T_FILE)
    {
        syscall_set_retval1(tf, 0);
        return;
    }
    else if (dp->type == T_DEV)
    {
        syscall_set_retval1(tf, 0);
        return;
    }
    else
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_BADF);
        return;
    }
    return;
}

void reverse_append(char *dst, char *src, int len)
{
    int dst_i, src_i;
    // search for next empty character in the destination
    for (dst_i = 0; dst[dst_i] != '\0'; dst_i++)
        ;
    // copy in reverse
    for (src_i = len - 1; src_i >= 0; src_i--, dst_i++)
    {
        dst[dst_i] = src[src_i];
    }
    dst[dst_i++] = '/';
}
void reverse_string(char *string)
{
    int length = strnlen(string, 10000);
    char reverse[length + 1];
    int i;
    for (i = 0; i < length; i++)
    {
        reverse[i] = string[length - i - 1];
    }
    reverse[length] = '\0';

    for (i = 0; i < length; i++)
    {
        string[i] = reverse[i];
    }
}

void sys_pwd(tf_t *tf)
{
    unsigned int uva = syscall_get_arg2(tf); //check user address doesn't falls in kernel mem
    int path_length = syscall_get_arg3(tf);
    if (uva < VM_USERLO || uva + path_length > VM_USERHI)
    {
        syscall_set_retval1(tf, -1);
        syscall_set_errno(tf, E_INVAL_ADDR);
        return;
    }
    char final_name[path_length];

    //   /root/sub1/sub2
    // pwd = sub2
    // curr = sub2
    // parent = sub1
    // final name = ""
    // temp name = ""
    // final = "2bus/1bus/toor/
    // final = "/root/sub1/sub2"

    struct inode *current_inode = (struct inode *)tcb_get_cwd(get_curid());
    struct inode *original_inode = current_inode;
    struct inode *parent_inode = namei("..");

    if (current_inode->inum == parent_inode->inum)
    {
        final_name[0] = '/';
        final_name[1] = '\0';
        pt_copyout(final_name, get_curid(), uva, path_length);
        syscall_set_errno(tf, E_SUCC);
        syscall_set_retval1(tf, 0);
        return;
    }

    memset(final_name, 0, path_length);
    struct dirent de;
    unsigned int de_size = sizeof(de);
    unsigned int off;

    while (parent_inode->inum != current_inode->inum)
    {
        for (off = 0; off < parent_inode->size; off += de_size)
        {
            if (inode_read(parent_inode, (char *)&de, off, sizeof(de)) != sizeof(de))
            {
                KERN_PANIC("can't read enough bytes in sys_pwd");
            }

            if (de.inum == current_inode->inum)
            {
                reverse_append(final_name, de.name, strnlen(de.name, DIRSIZ));
                break;
            }
        }
        current_inode = parent_inode;
        tcb_set_cwd(get_curid(), current_inode);
        parent_inode = namei("..");
    }

    reverse_string(final_name);
    pt_copyout(final_name, get_curid(), uva, path_length);
    syscall_set_errno(tf, E_SUCC);
    syscall_set_retval1(tf, 0);
    tcb_set_cwd(get_curid(), original_inode);
    return;
}
