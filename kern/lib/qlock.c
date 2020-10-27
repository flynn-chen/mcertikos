#include <lib/debug.h>
#include <lib/x86.h>
#include <lib/spinlock.h>
#include <lib/thread.h>
#include <dev/intr.h>
#include <pcpu/PCPUIntro/export.h>
#include <thread/PThread/export.h>

#include "qlock.h"

void qlock_init(qlock_t *qlock){
    spinlock_init(&(qlock->spinlock));
}


void qlock_acquire(qlock_t *qlock){
    intr_local_disable();
    // acquire spinlock
    spinlock_acquire(&(qlock->spinlock));

    if(qlock->value == BUSY){
        // put current process on the qlock's waiting queue
        multiq_enqueue(&(qlock->waiting), get_curid(), get_pcpu_idx());
        scheduler_suspend_spinlock(&shared_scheduler, &(qlock->spinlock));
    }
    else{
        // if free, set to busy and release lock
        qlock->value = BUSY;
        spinlock_release(&(qlock->spinlock));
    }
    intr_local_enable();
}

void qlock_release(qlock_t *qlock){
    intr_local_disable();
    spinlock_acquire(&(qlock->spinlock));
    if(!multiq_is_empty(&(qlock->waiting))){
        multiq_elt_t *elt = multiq_dequeue(&(qlock->waiting));
        scheduler_make_ready(&shared_scheduler, elt->pid, elt->pcpu_idx);
    }
    else{
        qlock->value = FREE;
    }
    spinlock_release(&(qlock->spinlock));
    intr_local_enable();
}
