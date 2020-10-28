#include <lib/debug.h>
#include <lib/x86.h>
#include <lib/thread.h>
#include <lib/scheduler.h>
#include <pcpu/PCPUIntro/export.h>
#include <thread/PThread/export.h>

#include "cvar.h"

/**
 *  // Monitor lock held by current thread. 
 *  void CV::wait(Lock *lock) {     
 *      assert(lock.isHeld()); 
 *      waiting.add(myTCB);  
 *      // Switch to new thread & release lock. 
 *      scheduler.suspend(&lock);     
 *      lock->acquire(); 
 *  } 
*/
void cvar_wait(cvar_t *cvar, qlock_t *lock)
{
    // put current process on the cvar's waiting queue
    multiq_enqueue(&(cvar->waiting), get_curid(), get_pcpu_idx());
    scheduler_suspend_qlock(&shared_scheduler, lock);
    // re-acquire lock
    qlock_acquire(lock);
}

/**
 *  // Monitor lock held by current thread. 
 *  void CV::signal() {     
 *      if (waiting.notEmpty()) {         
 *          thread = waiting.remove(); 
 *          scheduler.makeReady(thread);     
 *      } 
 *  } 
*/
void cvar_signal(cvar_t *cvar)
{
    // return if waiting queue is empty
    if (multiq_is_empty(&(cvar->waiting)))
    {
        return;
    }
    // get waiting pid off of queue
    multiq_elt_t *elt = multiq_dequeue(&(cvar->waiting));
    scheduler_make_ready(&shared_scheduler, elt->pid, elt->pcpu_idx);
    // KERN_DEBUG("Putting CPU %d Process %d on the ready queue\n", elt->pcpu_idx, elt->pid);
}

/**
 *  void CV::broadcast() {     
 *      while (waiting.notEmpty()) {         
 *          thread = waiting.remove(); 
 *          scheduler.makeReady(thread);     
 *      } 
 *  }
*/
void cvar_broadcast(cvar_t *cvar)
{
    // while "waiting" queue is not empty
    multiq_elt_t *elt;
    while (!multiq_is_empty(&(cvar->waiting)))
    {
        // get waiting pid off of queue
        elt = multiq_dequeue(&(cvar->waiting));
        scheduler_make_ready(&shared_scheduler, elt->pid, elt->pcpu_idx);
    }
}
