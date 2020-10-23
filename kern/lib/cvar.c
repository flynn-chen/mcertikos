#include <lib/debug.h>
#include <lib/x86.h>
#include <lib/spinlock.h>
#include <lib/thread.h>
#include <pcpu/PCPUIntro/export.h>

#include "cvar.h"

/**
 * Queue implementation details
 * (every reference to head/tail is implied head % BUFF_SIZE and tail % BUFF_SIZE)
 *  - head points to head of queue, only empty if entire queue is empty
 *  - tail points to first empty position in the queue
 *  - head <= tail always
 *  - head == tail if and only if queue is empty
 *  - head < BUFF_SIZE always
 *      - if head >= BUFF_SIZE, decrement head and tail by BUFF_SIZE
 *  - tail <= 2 * BUFF_SIZE always
 *      - not possible for it not to have been decremented before
 *  - tail - head <= BUFF_SIZE
 *      - can't have more than BUFF_SIZE threads waiting
 *  - should always index buff with % BUFF_SIZE
*/

/**
 * Add an elt with the given pid and pcpu_idx to the queue
*/
void enqueue(queue_t *q, unsigned int pid, unsigned int pcpu_idx)
{
    // add to buff and increment tail
    q->buff[q->tail % BUFF_SIZE] = (elt_t){pid, pcpu_idx};
    q->tail++;
}
/**
 * Pop the head off of the queue, or return NULL if empty
*/
elt_t *dequeue(queue_t *q)
{
    if (is_empty(q))
    {
        return NULL;
    }
    // remove from buff and increment head
    elt_t *elt = &(q->buff[q->head % BUFF_SIZE]);
    q->head++;
    // shift pointers down if they've grown past BUFF_SIZE
    if (q->head >= BUFF_SIZE)
    {
        q->head -= BUFF_SIZE;
        q->tail -= BUFF_SIZE;
    }
    return elt;
}
/**
 * Check if queue is empty
*/
bool is_empty(queue_t *q)
{
    return q->head % BUFF_SIZE == q->tail % BUFF_SIZE;
}

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
void cvar_wait(cvar_t *cvar, spinlock_t *lock)
{
    if (!spinlock_holding(lock))
    {
        return;
    }

    // get next pid off of the ready queue
    unsigned int next_pid = tqueue_dequeue(NUM_IDS + get_pcpu_idx());
    if (next_pid == NUM_IDS)
    {
        // no clue what to do if this happens
        KERN_DEBUG("CPU %d: Process %d: has no one to give control to\n", get_pcpu_idx(), get_curid());
        return;
    }
    KERN_DEBUG("CPU %d: Process %d: is going to start up %d\n", get_pcpu_idx(), get_curid(), next_pid);

    unsigned int pid = get_curid();

    // put current process on the cvar's waiting queue
    enqueue(&(cvar->waiting), pid, get_pcpu_idx());

    // switch to new thread
    // set state to sleeping, and waiting on its own queue (might not need to do this)
    tcb_set_state(pid, TSTATE_SLEEP);
    tqueue_enqueue(pid, pid);

    // set next_pid to running
    tcb_set_state(next_pid, TSTATE_RUN);
    set_curid(next_pid);

    // release lock, switch processes, re-acquire lock
    spinlock_release(lock);
    kctx_switch(pid, next_pid);

    // re-acquire lock
    spinlock_acquire(lock);
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
    if (is_empty(&(cvar->waiting)))
    {
        return;
    }
    // get waiting pid off of queue
    elt_t *elt = dequeue(&(cvar->waiting));

    // take it off the sleeping queue
    tqueue_remove(elt->pid, elt->pid);
    // put waiting thread on the ready queue
    tcb_set_state(elt->pid, TSTATE_READY);
    tqueue_enqueue(NUM_IDS + elt->pcpu_idx, elt->pid);
    KERN_DEBUG("Putting CPU %d Process %d on the ready queue\n", elt->pcpu_idx, elt->pid);
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
    elt_t *elt;
    while (!is_empty(&(cvar->waiting)))
    {
        // get waiting pid off of queue
        elt = dequeue(&(cvar->waiting));

        // take it off the sleeping queue
        tqueue_remove(elt->pid, elt->pid);

        // put it on the ready queue (guaranteed not NULL by while loop condition)
        tcb_set_state(elt->pid, TSTATE_READY);
        tqueue_enqueue(NUM_IDS + elt->pcpu_idx, elt->pid);
    }
}
