#include <lib/cvar.h>
#include <lib/debug.h>
#include <pcpu/PCPUIntro/export.h>

#include "bbq.h"

bbq_t shared_bbq;

void dev_init(void)
{
    bbq_init(&shared_bbq);
}

void bbq_init(bbq_t *bbq)
{
    bbq->front = 0;
    bbq->next_empty = 0;
}

void bbq_insert(bbq_t *bbq, unsigned int item)
{
    spinlock_acquire(&(bbq->bbq_lock));

    while ((bbq->next_empty - bbq->front) == BBQ_SIZE)
    {
        KERN_DEBUG("\tCPU %d: Process %d: Waiting to insert %d (full buffer)\n", get_pcpu_idx(), get_curid(), item);
        cvar_wait(&(bbq->item_removed), &(bbq->bbq_lock));
        KERN_DEBUG("\tCPU %d: Process %d: I woke up (like dis) from waiting to insert %d\n", get_pcpu_idx(), get_curid(), item);
    }
    bbq->buff[bbq->next_empty % BBQ_SIZE] = item;
    bbq->next_empty++;

    if ((bbq->next_empty - bbq->front) == BBQ_SIZE)
    {
        KERN_DEBUG("\tCPU %d: Process %d: just filled the buffer\n", get_pcpu_idx(), get_curid());
    }

    cvar_signal(&(bbq->item_added));
    spinlock_release(&(bbq->bbq_lock));
}

unsigned int bbq_remove(bbq_t *bbq)
{
    unsigned int item;

    spinlock_acquire(&(bbq->bbq_lock));
    while (bbq->front == bbq->next_empty)
    {
        KERN_DEBUG("\tCPU %d: Process %d: Waiting to remove (empty buffer)\n", get_pcpu_idx(), get_curid());
        cvar_wait(&(bbq->item_added), &(bbq->bbq_lock));
        KERN_DEBUG("\tCPU %d: Process %d: woken up from waiting on empty buffer\n", get_pcpu_idx(), get_curid());
    }

    item = bbq->buff[bbq->front % BBQ_SIZE];
    bbq->front++;
    if (bbq->front >= BBQ_SIZE)
    {
        bbq->front -= BBQ_SIZE;
        bbq->next_empty -= BBQ_SIZE;
    }
    cvar_signal(&(bbq->item_removed));
    spinlock_release(&(bbq->bbq_lock));

    return item;
}