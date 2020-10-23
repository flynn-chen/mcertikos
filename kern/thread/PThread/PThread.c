#include <lib/x86.h>
#include <lib/thread.h>
#include <lib/spinlock.h>
#include <lib/debug.h>
#include <dev/lapic.h>
#include <pcpu/PCPUIntro/export.h>

#include "import.h"

spinlock_t cpu_lock[NUM_CPUS];
unsigned int ms_elapsed[NUM_CPUS];
spinlock_t sched_update_lock[NUM_CPUS];

unsigned int prev_id[NUM_CPUS];

void thread_init(unsigned int mbi_addr)
{
    for (unsigned int i = 0; i < NUM_CPUS; i++)
    {
        spinlock_init(&cpu_lock[i]);
        spinlock_init(&sched_update_lock[NUM_CPUS]);
    }
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
    spinlock_acquire(&cpu_lock[get_pcpu_idx()]);
    unsigned int pid = kctx_new(entry, id, quota);
    if (pid != NUM_IDS)
    {
        tcb_set_cpu(pid, get_pcpu_idx());
        tcb_set_state(pid, TSTATE_READY);
        tqueue_enqueue(NUM_IDS + get_pcpu_idx(), pid);
    }
    spinlock_release(&cpu_lock[get_pcpu_idx()]);
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
    unsigned int new_cur_pid;
    unsigned int old_cur_pid = get_curid();

    // TODO: spinlock_try_acquire?
    // another thread may yield to this thread
    // while yielding (holding the lock)
    // we could be interrupted by the timer - though only once
    spinlock_acquire(&cpu_lock[get_pcpu_idx()]);

    prev_id[get_pcpu_idx()] = old_cur_pid;

    tcb_set_state(old_cur_pid, TSTATE_READY);
    tqueue_enqueue(NUM_IDS + get_pcpu_idx(), old_cur_pid);

    new_cur_pid = tqueue_dequeue(NUM_IDS + get_pcpu_idx());

    KERN_DEBUG("CPU %d is yielding from %d to %d\n", get_pcpu_idx(), old_cur_pid, new_cur_pid);

    tcb_set_state(new_cur_pid, TSTATE_RUN);
    set_curid(new_cur_pid);

    spinlock_release(&cpu_lock[get_pcpu_idx()]);
    if (old_cur_pid != new_cur_pid)
    {
        kctx_switch(old_cur_pid, new_cur_pid);
    }
}

void sched_update(void)
{
    unsigned int current_cpu = get_pcpu_idx();
    // KERN_DEBUG("CPU %d just got interrupted\n", current_cpu);
    spinlock_acquire(&sched_update_lock[current_cpu]);
    ms_elapsed[current_cpu] += (1000 / LAPIC_TIMER_INTR_FREQ);
    if (ms_elapsed[current_cpu] >= SCHED_SLICE)
    {
        ms_elapsed[current_cpu] = 0;
        spinlock_release(&sched_update_lock[current_cpu]);
        // KERN_DEBUG("CPU %d is yielding\n", current_cpu);
        thread_yield();
        return;
    }
    spinlock_release(&sched_update_lock[current_cpu]);
}

unsigned int previous_id(void)
{
    spinlock_acquire(&cpu_lock[get_pcpu_idx()]);
    unsigned int prev_pid = prev_id[get_pcpu_idx()];
    spinlock_release(&cpu_lock[get_pcpu_idx()]);
    return prev_pid;
}