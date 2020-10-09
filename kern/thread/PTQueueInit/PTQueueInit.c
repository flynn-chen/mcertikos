#include "lib/x86.h"

#include "import.h"

/**
 * Initializes all the thread queues with tqueue_init_at_id.
 */
void tqueue_init(unsigned int mbi_addr)
{

    tcb_init(mbi_addr);

    for (unsigned int chid = 0; chid <= NUM_IDS; chid++)
    {
        tqueue_init_at_id(chid);
    }
}

/**
 * Insert the TCB #pid into the tail of the thread queue #chid.
 * Recall that the doubly linked list is index based.
 * So you only need to insert the index.
 * Hint: there are multiple cases in this function.
 */
void tqueue_enqueue(unsigned int chid, unsigned int pid)
{

    /* 
    get the tqueue tail of chid called tq_tail

    if tqueue is just intialized with tq_tail at NUM_IDS
        - set the tqueue head of chid to pid (NUM_IDS -> pid)
        - set the tqueue tail of chid to pid (NUM_IDS -> pid)
        - set the previous tcb of pid to NUM_IDS
        - set the next tcb of pid to NUM_IDS

    if tqueue is already initialized with tq_tail at [0, NUM_IDS-1]
        - set the tqueue tail of chid to pid
        - set the previous tcb of pid to tq_tail
        - set the next tcb of tq_tail to pid
    */

    unsigned int tq_tail = tqueue_get_tail(chid);
    // if queue is empty
    if (tq_tail == NUM_IDS)
    {
        tqueue_set_head(chid, pid);
        tqueue_set_tail(chid, pid);
        tcb_set_prev(pid, NUM_IDS);
        tcb_set_next(pid, NUM_IDS);
        // previous tcb = NUM_IDS
        // next tcb = NUM_IDS (TODO: is it necessary?)
    }
    // if queue has values already
    else
    {
        tqueue_set_tail(chid, pid);
        tcb_set_prev(pid, tq_tail);
        tcb_set_next(tq_tail, pid);
        // previous tcb of pid = tq_tail
        // next tcb of tq_tail = pid
    }
}

/**
 * Reverse action of tqueue_enqueue, i.e. pops a TCB from the head of the specified queue.
 * It returns the popped thread's id, or NUM_IDS if the queue is empty.
 * Hint: there are multiple cases in this function.
 */
unsigned int tqueue_dequeue(unsigned int chid)
{

    /* 
    get the tqueue head of chid called tq_head

    if tqueue is empty with tq_head at NUM_IDS
        - return NUM_IDS

    if tqueue has items in it with tq_head at [0, NUM_IDS-1]
        - get the next tcb of first tcb (second tcb)
        - if there is only one item in it (AKA next tcb of first item = NUM_IDS):
            - set the tqueue head of chid to be NUM_IDS
            - set the tqueue tail of chid to be NUM_IDS
            - set the next tcb of first item to be NUM_IDS (popped off)?

        - if there are other items in it:
            - set the tqueue head of the chid to be second tcb
            - set the previous tcb of the second tcb to be NUM_IDS 
            - set the next tcb of first item to be NUM_IDS (popped off)?
    */

    unsigned int tq_head = tqueue_get_head(chid);
    //check if it is empty queue, if it is, return NUM_IDS
    if (tq_head == NUM_IDS)
    {
        return NUM_IDS;
    }

    //check if there is only one item in the queue
    unsigned int second_tcb = tcb_get_next(tq_head);
    if (second_tcb == NUM_IDS)
    {
        tqueue_set_head(chid, NUM_IDS);
        tqueue_set_tail(chid, NUM_IDS);

        // TODO: is this necessary?
        // redundant with line 50 (tcb_set_next(pid, NUM_IDS);)
        tcb_set_next(tq_head, NUM_IDS);
    }
    else
    {
        tqueue_set_head(chid, second_tcb);
        tcb_set_prev(second_tcb, NUM_IDS);

        // TODO: is this necessary?
        // redundant with line 50 (tcb_set_next(pid, NUM_IDS);)
        tcb_set_next(tq_head, NUM_IDS);
    }

    return tq_head;
}

/**
 * Removes the TCB #pid from the queue #chid.
 * Hint: there are many cases in this function.
 */
void tqueue_remove(unsigned int chid, unsigned int pid)
{
    /* 
    get the tqueue head
    get the tqueue tail
    get the previous tcb of pid
    get the next tcb of pid

    if there is no pid in tqueue chid
        - do we need to consider this case?

    if pid is at the head or tail tqueue chid (or is the only thread in the queue)
        - if pid is at tqueue head
            - set next tcb as tqueue head
            - set previous tcb of tqueue head to NUM_IDS
            - set next tcb of pid to NUM_IDS
            - set previous tcb of pid to NUM_IDS
        - if pid is at tqueue tail
            - set previous tcb as tqueue tail
            - set next tcb of tqueue tail to NUM_IDS
            - set next tcb of pid to NUM_IDS
            - set previous tcb of pid to NUM_IDS

    if pid is in the middle of tqueue chid (or is not the only thread in the queue)
        - set previous tcb of pid_next_tcb to pid_prev_tcb
        - set next tcb of pid_prev_tcb to pid_next_tcb
        - set next tcb of pid to NUM_IDS
        - set previous tcb of pid to NUM_IDS
    */

    unsigned int tq_head, tq_tail;
    unsigned int pid_previous_tcb, pid_next_tcb;
    tq_head = tqueue_get_head(chid);
    tq_tail = tqueue_get_tail(chid);
    pid_previous_tcb = tcb_get_prev(pid);
    pid_next_tcb = tcb_get_next(pid);

    // if deleting the only thing on the queue
    if (tq_head == pid && tq_tail == pid)
    {
        tqueue_set_head(chid, NUM_IDS);
        tqueue_set_tail(chid, NUM_IDS);
        tcb_set_next(pid, NUM_IDS);
        tcb_set_prev(pid, NUM_IDS);
        return;
    }
    // if deleting the head
    else if (tq_head == pid)
    {
        tqueue_set_head(chid, pid_next_tcb);
        tcb_set_prev(pid_next_tcb, NUM_IDS);
        tcb_set_next(pid, NUM_IDS);
        tcb_set_prev(pid, NUM_IDS);
        return;
    }
    // if deleting the tail
    else if (tq_tail == pid)
    {
        tqueue_set_tail(chid, pid_previous_tcb);
        tcb_set_next(pid_previous_tcb, NUM_IDS);
        tcb_set_next(pid, NUM_IDS);
        tcb_set_prev(pid, NUM_IDS);
        return;
    }

    // if deleting from the middle of the queue
    tcb_set_prev(pid_next_tcb, pid_previous_tcb);
    tcb_set_next(pid_previous_tcb, pid_next_tcb);
    tcb_set_next(pid, NUM_IDS);
    tcb_set_prev(pid, NUM_IDS);
    return;
}
