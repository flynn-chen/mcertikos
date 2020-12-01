#include <lib/x86.h>
#include <lib/thread.h>
#include <lib/spinlock.h>
#include <lib/debug.h>
#include <dev/lapic.h>
#include <pcpu/PCPUIntro/export.h>
#include <kern/thread/PTCBIntro/export.h>

#include "import.h"

static spinlock_t sched_lk;

unsigned int sched_ticks[NUM_CPUS];

void thread_init(unsigned int mbi_addr)
{
    unsigned int i;
    for (i = 0; i < NUM_CPUS; i++)
    {
        sched_ticks[i] = 0;
    }

    spinlock_init(&sched_lk);
    tqueue_init(mbi_addr);
    set_curid(0);
    tcb_set_state(0, TSTATE_RUN);
}

/**
 * Allocates a new child thread context, sets the state of the new child thread
 * to ready, and pushes it to the ready queue.
 * It returns the child thread id.
 */
unsigned int thread_spawn(void *entry, unsigned int id, unsigned int quota)
{
    unsigned int pid;

    spinlock_acquire(&sched_lk);

    pid = kctx_new(entry, id, quota);
    if (pid != NUM_IDS)
    {
        tcb_set_state(pid, TSTATE_READY);
        tqueue_enqueue(NUM_IDS, pid);
    }

    spinlock_release(&sched_lk);

    return pid;
}

/**
 * Allocates a new child thread context but does not execute it, 
 * so it sets the state of the new child thread to TSTATE_SLEEP
 * It returns the child thread id.
 */
unsigned int thread_debug_spawn(void *entry, unsigned int id, unsigned int quota)
{
    unsigned int pid, current_pid;

    spinlock_acquire(&sched_lk);

    pid = kctx_new(entry, id, quota);
    current_pid = get_curid();
    if (pid != NUM_IDS)
    {
        // KERN_DEBUG("spawn new debuggee process %d, registered %d as debugger\n", pid, current_pid);
        tcb_set_state(pid, TSTATE_SLEEP);
        // set the user proc's debugger_id to be the current id
        // (the debugger is the one who calls thread_debug_spawn)
        tcb_set_debugger_id(pid, current_pid);
        // user id is sleeping on the debugger id's sleep queue
        tqueue_enqueue(current_pid, pid);
    }

    spinlock_release(&sched_lk);

    return pid;
}

/**
 * Yield to the next thread in the ready queue.
 * You should set the currently running thread state as ready,
 * and push it back to the ready queue.
 * Then set the state of the popped thread as running, set the
 * current thread id, and switch to the new kernel context.
 * Hint: If you are the only thread that is ready to run,
 * do you need to switch to yourself?
 */
void thread_yield(void)
{
    unsigned int old_cur_pid;
    unsigned int new_cur_pid;

    spinlock_acquire(&sched_lk);

    old_cur_pid = get_curid();
    tcb_set_state(old_cur_pid, TSTATE_READY);
    tqueue_enqueue(NUM_IDS, old_cur_pid);

    new_cur_pid = tqueue_dequeue(NUM_IDS);
    tcb_set_state(new_cur_pid, TSTATE_RUN);
    set_curid(new_cur_pid);

    if (old_cur_pid != new_cur_pid)
    {
        spinlock_release(&sched_lk);
        kctx_switch(old_cur_pid, new_cur_pid);
    }
    else
    {
        spinlock_release(&sched_lk);
    }
}

void thread_yield_to(unsigned int debuggee_pid)
{
    unsigned int debugger_pid;
    spinlock_acquire(&sched_lk);

    debugger_pid = get_curid();
    // get sleeping debuggee off of debugger's sleeping queue
    unsigned int sleeping_pid = tqueue_dequeue(debugger_pid);
    // this assert might fail if there are multiple debuggee's sleeping
    KERN_ASSERT(sleeping_pid == debuggee_pid);
    // set debuggee to running
    tcb_set_state(debuggee_pid, TSTATE_RUN);
    set_curid(debuggee_pid);

    // set debugger to sleeping in debugger's sleeping queue
    tcb_set_state(debugger_pid, TSTATE_SLEEP);
    tqueue_enqueue(debugger_pid, debugger_pid);

    // KERN_DEBUG("yielding from debuggee %d to debugger %d\n", debuggee_pid, debugger_pid);

    if (debugger_pid != debuggee_pid)
    {
        spinlock_release(&sched_lk);
        kctx_switch(debugger_pid, debuggee_pid);
    }
    else
    {
        spinlock_release(&sched_lk);
    }
}

void sched_update(void)
{
    spinlock_acquire(&sched_lk);
    sched_ticks[get_pcpu_idx()] += 1000 / LAPIC_TIMER_INTR_FREQ;
    if (sched_ticks[get_pcpu_idx()] >= SCHED_SLICE)
    {
        sched_ticks[get_pcpu_idx()] = 0;
        spinlock_release(&sched_lk);
        thread_yield();
    }
    else
    {
        spinlock_release(&sched_lk);
    }
}

/**
 * Atomically release lock and sleep on chan.
 * Reacquires lock when awakened.
 */
void thread_sleep(void *chan, spinlock_t *lk)
{
    // TODO: your local variables here.
    unsigned int old_cur_pid;
    unsigned int new_cur_pid;

    if (lk == 0)
        KERN_PANIC("sleep without lock");

    // Must acquire sched_lk in order to change the current thread's state and
    // then switch. Once we hold sched_lk, we can be guaranteed that we won't
    // miss any wakeup (wakeup runs with sched_lk locked), so it's okay to
    // release lock.
    spinlock_acquire(&sched_lk);
    spinlock_release(lk);

    // Go to sleep.
    old_cur_pid = get_curid();
    new_cur_pid = tqueue_dequeue(NUM_IDS);
    KERN_ASSERT(new_cur_pid != NUM_IDS);
    tcb_set_chan(old_cur_pid, chan);
    tcb_set_state(old_cur_pid, TSTATE_SLEEP);
    tcb_set_state(new_cur_pid, TSTATE_RUN);
    set_curid(new_cur_pid);

    // Context switch.
    spinlock_release(&sched_lk);
    kctx_switch(old_cur_pid, new_cur_pid);
    spinlock_acquire(&sched_lk);

    // Tidy up.
    tcb_set_chan(old_cur_pid, NULL);

    // Reacquire original lock.
    spinlock_acquire(lk);
    spinlock_release(&sched_lk);
}

/**
 * Wake up all processes sleeping on chan.
 */
void thread_wakeup(void *chan)
{
    unsigned int pid;
    spinlock_acquire(&sched_lk);

    for (pid = 0; pid < NUM_IDS; pid++)
    {
        if (chan == tcb_get_chan(pid))
        {
            tcb_set_state(pid, TSTATE_READY);
            tqueue_enqueue(NUM_IDS, pid);
        }
    }

    spinlock_release(&sched_lk);
}
