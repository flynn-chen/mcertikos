#ifndef _KERN_LIB_SCHEDULER_H_
#define _KERN_LIB_SCHEDULER_H_

#ifdef _KERN_

#include <lib/gcc.h>
#include <lib/types.h>
#include <lib/x86.h>
#include <lib/qlock.h>
#include <lib/spinlock.h>

typedef struct
{
    spinlock_t spinlock;
} scheduler_t;

extern scheduler_t shared_scheduler;

void scheduler_init(scheduler_t *sched);
void scheduler_make_ready(scheduler_t *sched, unsigned int pid, unsigned int pcpu_idx);
void scheduler_suspend_spinlock(scheduler_t *sched, spinlock_t *lock);
void scheduler_suspend_qlock(scheduler_t *sched, qlock_t *lock);

unsigned int get_curid(void);
void set_curid(unsigned int curid);
void kctx_switch(unsigned int from_pid, unsigned int to_pid);
void tcb_set_state(unsigned int pid, unsigned int state);
void tqueue_enqueue(unsigned int chid, unsigned int pid);
unsigned int tqueue_dequeue(unsigned int chid);
void tqueue_remove(unsigned int chid, unsigned int pid);

#endif /* _KERN_ */

#endif /* !_KERN_LIB_SCHEDULER_H_ */
