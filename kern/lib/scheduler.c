#include <lib/debug.h>
#include <lib/x86.h>
#include <lib/thread.h>
#include <dev/intr.h>
#include <pcpu/PCPUIntro/export.h>
#include <thread/PThread/export.h>

#include "scheduler.h"

scheduler_t shared_scheduler;

void scheduler_init(scheduler_t *sched){
    spinlock_init(&(sched->spinlock));
}

void scheduler_make_ready(scheduler_t *sched, unsigned int pid, unsigned int pcpu_idx){
    intr_local_disable();
    spinlock_acquire(&(sched->spinlock));

    // remove from sleeping queue, put on ready queue
    tqueue_remove(pid, pid);
    tcb_set_state(pid, TSTATE_READY);
    tqueue_enqueue(NUM_IDS + pcpu_idx, pid);

    spinlock_release(&(sched->spinlock));
    intr_local_enable();
}

void scheduler_suspend_spinlock(scheduler_t *sched, spinlock_t *lock){
    intr_local_disable();
    spinlock_acquire(&(sched->spinlock));

    spinlock_release(lock);

    unsigned int pid = get_curid();
    unsigned int next_pid = tqueue_dequeue(NUM_IDS + get_pcpu_idx());
    KERN_ASSERT(next_pid != NUM_IDS);

    // set state to sleeping, and waiting on its own queue
    tcb_set_state(pid, TSTATE_SLEEP);
    tqueue_enqueue(pid, pid);

    // set next_pid to running
    tcb_set_state(next_pid, TSTATE_RUN);
    set_curid(next_pid);

    KERN_DEBUG("scheduler changing from %d to %d on CPU %d\n", pid, next_pid, get_pcpu_idx());
    // context switch
    kctx_switch(pid, next_pid);

    spinlock_release(&(sched->spinlock));
    intr_local_enable();
}


void scheduler_suspend_qlock(scheduler_t *sched, qlock_t *lock){
    intr_local_disable();
    spinlock_acquire(&(sched->spinlock));

    qlock_release(lock);

    unsigned int pid = get_curid();
    unsigned int next_pid = tqueue_dequeue(NUM_IDS + get_pcpu_idx());
    KERN_ASSERT(next_pid != NUM_IDS);

    // set state to sleeping, and waiting on its own queue
    tcb_set_state(pid, TSTATE_SLEEP);
    tqueue_enqueue(pid, pid);

    // set next_pid to running
    tcb_set_state(next_pid, TSTATE_RUN);
    set_curid(next_pid);

    KERN_DEBUG("scheduler changing from %d to %d on CPU %d\n", pid, next_pid, get_pcpu_idx());
    // context switch
    kctx_switch(pid, next_pid);

    spinlock_release(&(sched->spinlock));
    intr_local_enable();
}
