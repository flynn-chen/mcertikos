#ifndef _KERN_LIB_QLOCK_H_
#define _KERN_LIB_QLOCK_H_

#ifdef _KERN_

#include <lib/gcc.h>
#include <lib/types.h>
#include <lib/x86.h>
#include <lib/spinlock.h>
#include <lib/multiq.h>

#define FREE 0
#define BUSY 1

/**
 * Multiprocessor queuing lock implementation using multiq_t
*/
typedef struct
{
    unsigned int value;
    spinlock_t spinlock;
    multiq_t waiting;
} qlock_t;

void qlock_init(qlock_t *qlock);
void qlock_acquire(qlock_t *qlock);
void qlock_release(qlock_t *qlock);

#endif /* _KERN_ */

#endif /* !_KERN_LIB_QLOCK_H_ */
