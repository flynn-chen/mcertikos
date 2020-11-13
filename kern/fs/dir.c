#include <kern/lib/types.h>
#include <kern/lib/debug.h>
#include <kern/lib/string.h>
#include "inode.h"
#include "dir.h"

// Directories

int dir_namecmp(const char *s, const char *t)
{
    return strncmp(s, t, DIRSIZ);
}

/**
 * Look for a directory entry in a directory.
 * If found, set *poff to byte offset of entry.
 * /
 * /dir1
 * /dir1/dir2
 * /dir1/dir3
 * 
 * pwd = /dir1
 * 
 * dir_lookup(dir1, "dir2", [])
 * 
 * 
 * BSIZE = 16
 * 
 * inode:
 *  1: byte 0       [(dirent)(dirent)]
 *  2: byte 16      [(dirent)(dirent)]
 *  3: byte 32      [(dirent)(dirent)]
 *  IND:    inode
 *              1:
 *              2:
 *              3:
 * 
 */
struct inode *dir_lookup(struct inode *dp, char *name, uint32_t *poff)
{
    uint32_t off;
    struct dirent *d;
    int num_read;
    if (dp->type != T_DIR)
        KERN_PANIC("dir_lookup not DIR");

    size_t dirent_size = sizeof(struct dirent);
    char dst[dirent_size];
    for (off = 0; off < dp->size; off += dirent_size)
    {
        num_read = inode_read(dp, dst, off, dirent_size);
        KERN_ASSERT(num_read == dirent_size);

        d = (struct dirent *)dst; // cast dst pointer to a dirent

        // two things must be true:
        //  1. dir_namecmp(dst, name)
        //  2. inum is not 0 (find an entry that is allocated)
        if (!dir_namecmp(d->name, name) & (d->inum != 0))
        {
            if (poff != 0)
                *poff = off;
            struct inode *ip = inode_get(dp->dev, d->inum);
            return ip;
        }
    }
    return 0;
}

// Write a new directory entry (name, inum) into the directory dp.
int dir_link(struct inode *dp, char *name, uint32_t inum)
{
    // Check that name is not present.
    uint32_t poff, off;
    struct inode *ip;
    struct dirent *d;
    int num_read;
    ip = dir_lookup(dp, name, &poff);
    if (ip != 0)
    {
        inode_put(ip);
        return -1;
    }

    // TODO: Look for an empty dirent.
    size_t dirent_size = sizeof(struct dirent);
    char dst[dirent_size];
    for (off = 0; off < dp->size; off += dirent_size)
    {
        num_read = inode_read(dp, dst, off, dirent_size);
        KERN_ASSERT(num_read == dirent_size);

        d = (struct dirent *)dst;
        if (d->inum == 0)
        {
            break;
        }
    }

    d->inum = inum;
    memmove(d->name, name, DIRSIZ);
    if (inode_write(dp, dst, off, dirent_size) != dirent_size)
    {
        KERN_PANIC("inode write failed, did not write enough bytes for dirent_size");
    }

    return 0;
}
