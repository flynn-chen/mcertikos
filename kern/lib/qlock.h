#ifndef _KERN_LIB_QLOCK_H_
#define _KERN_LIB_QLOCK_H_

#ifdef _KERN_

#include <lib/gcc.h>
#include <lib/types.h>
#include <lib/x86.h>
#include <lib/spinlock.h>
#include <lib/multiq.h>
#include <lib/scheduler.h>

#define FREE 0
#define BUSY 1

/**
 * Multiprocessor queuing lock implementation using queue_t
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

/**
 * Other functions that need to be imported
*/
unsigned int get_curid(void);
void set_curid(unsigned int curid);
void kctx_switch(unsigned int from_pid, unsigned int to_pid);
void tcb_set_state(unsigned int pid, unsigned int state);
void tqueue_enqueue(unsigned int chid, unsigned int pid);
unsigned int tqueue_dequeue(unsigned int chid);
void tqueue_remove(unsigned int chid, unsigned int pid);

#endif /* _KERN_ */

#endif /* !_KERN_LIB_QLOCK_H_ */
