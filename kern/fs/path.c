// File system implementation.  Five layers:
//   + Blocks: allocator for raw disk blocks.
//   + Log: crash recovery for multi-step updates.
//   + Files: inode allocator, reading, writing, metadata.
//   + Directories: inode with special contents (list of other inodes!)
//   + Names: paths like /usr/fs.c for convenient naming.
//
// This file contains the low-level file system manipulation
// routines.  The (higher-level) system call implementations
// are in sysfile.c.

#include <kern/lib/types.h>
#include <kern/lib/debug.h>
#include <kern/lib/spinlock.h>
#include <thread/PTCBIntro/export.h>
#include <thread/PCurID/export.h>
#include "inode.h"
#include "dir.h"
#include "log.h"

// Paths

/**
 * Copy the next path element from path into name.
 * If the length of name is larger than or equal to DIRSIZ, then only
 * (DIRSIZ - 1) # characters should be copied into name.
 * This is because you need to save '\0' in the end.
 * You should still skip the entire string in this case.
 * Return a pointer to the element following the copied one.
 * The returned path has no leading slashes,
 * so the caller can check *path == '\0' to see if the name is the last one.
 * If no name to remove, return 0.
 *
 * Examples :
 *   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
 *   skipelem("///a//bb", name) = "bb", setting name = "a"
 *   skipelem("a", name) = "", setting name = "a"
 *   skipelem("", name) = skipelem("////", name) = 0
 */
static char *skipelem(char *path, char *name)
{
    if (path == 0 || *path == '\0')
    {
        return 0;
    }

    char *front, *end;
    int len;
    front = path;
    while (*front == '/') //find first non-slash element
    {
        front++;
    }

    end = front;
    while (*end != '/' && *end != '\0') //find the next slash or hit the end
    {
        end++;
    }

    len = end - front; //calculate length of path element
    if (len <= 0)      // if there is not path element
    {
        return 0;
    }
    if (len > DIRSIZ) //if path element exceeds size
    {
        len = DIRSIZ - 1;
    }

    memmov(name, front, len); //move from path to name
    name[len] = '\0';         //add string terminator
    while (*end == '/')       //ignore trailing slashes
    {
        end++;
    }
    return end;
}

/**
 * Look up and return the inode for a path name.
 * If nameiparent is true, return the inode for the parent and copy the final
 * path element into name, which must have room for DIRSIZ bytes.
 * Returns 0 in the case of error.
 */
static struct inode *namex(char *path, bool nameiparent, char *name)
{
    struct inode *ip, *next;

    // If path is a full path, get the pointer to the root inode. Otherwise get
    // the inode corresponding to the current working directory.
    if (*path == '/')
    {
        ip = inode_get(ROOTDEV, ROOTINO);
    }
    else
    {
        ip = inode_dup((struct inode *)tcb_get_cwd(get_curid()));
    }
    /*
        namex(/usr/var/lib, false, name)

        pre:
            path = "/usr/var/lib"
            name = 0
            ip = /
        loop 0
            condition: path = "var/lib", name = usr => TRUE
            body: ip = /usr
        loop 1 
            condition: path = "lib", name = var => TRUE
            body: ip = /usr/var
        loop 2
            condition: path = "", name = lib => TRUE
            body1: unlock /usr/var and return it
            body2: ip = usr/var/lib
        loop 3
            condition: path = 0, name = lib => FALSE
            return ip = usr/var/lib

        namex("", true, name)

        ip = cwd/
        path = ""
        name = 0
        loop 0
            condition: path = 0, name = 0 => FALSE

        name("/", true, name)
        
        pre:
            path = "/"
            name = 0
            ip = /
        loop 0
            condition: path = "", name = 0 => TRUE
            body: 
    */
    while ((path = skipelem(path, name)) != 0)
    {
        inode_lock(ip);
        if (ip->type != T_DIR)
        {
            inode_unlockput(ip);
            return 0;
        }
        if (nameiparent && *path == '\0')
        {
            inode_unlockput(ip);
            return ip;
        }

        next = dir_lookup(ip, name, 0);
        inode_unlockput(ip);
        if (next == 0)
        {
            return 0;
        }
        ip = next;
    }
    if (nameiparent)
    {
        inode_put(ip);
        return 0;
    }
    inode_put(ip); // TODO: if namei is SUPPOSED to increment ref, then delete this
    return ip;
}

/**
 * Return the inode corresponding to path.
 */
struct inode *namei(char *path)
{
    char name[DIRSIZ];
    return namex(path, FALSE, name);
}

/**
 * Return the inode corresponding to path's parent directory and copy the final
 * element into name.
 */
struct inode *nameiparent(char *path, char *name)
{
    return namex(path, TRUE, name);
}
