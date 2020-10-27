#include <lib/debug.h>
#include <lib/x86.h>

#include "multiq.h"

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
void multiq_enqueue(multiq_t *q, unsigned int pid, unsigned int pcpu_idx)
{
    // add to buff and increment tail
    q->buff[q->tail % BUFF_SIZE] = (multiq_elt_t){pid, pcpu_idx};
    q->tail++;
}
/**
 * Pop the head off of the queue, or return NULL if empty
*/
multiq_elt_t *multiq_dequeue(multiq_t *q)
{
    if (multiq_is_empty(q))
    {
        return NULL;
    }
    // remove from buff and increment head
    multiq_elt_t *elt = &(q->buff[q->head % BUFF_SIZE]);
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
bool multiq_is_empty(multiq_t *q)
{
    return q->head % BUFF_SIZE == q->tail % BUFF_SIZE;
}