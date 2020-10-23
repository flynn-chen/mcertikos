#ifndef _KERN_LIB_CVAR_H_
#define _KERN_LIB_CVAR_H_

#ifdef _KERN_

#include <lib/gcc.h>
#include <lib/types.h>
#include <lib/x86.h>
#include <lib/spinlock.h>

/**
 * Queue implementation using an array
 * Assumes there can be at most NUM_IDS
 * cpus waiting simultaneously
*/
#define BUFF_SIZE NUM_IDS

typedef struct
{
    unsigned int pid;
    unsigned int pcpu_idx;
} elt_t;

typedef struct
{
    elt_t buff[BUFF_SIZE];
    unsigned int head;
    unsigned int tail;
} queue_t;

void enqueue(queue_t *q, unsigned int pid, unsigned int pcpu_idx);
elt_t *dequeue(queue_t *q);
bool is_empty(queue_t *q);

/**
 * Conditional variable implementation using queue_t
*/
typedef struct
{
    queue_t waiting;
} cvar_t;

void cvar_wait(cvar_t *cvar, spinlock_t *lock);
void cvar_signal(cvar_t *cvar);
void cvar_broadcast(cvar_t *cvar);

/**
 * Other functions that need to be imported
*/
unsigned int get_curid(void);
void set_curid(unsigned int curid);
void kctx_switch(unsigned int from_pid, unsigned int to_pid);
void tcb_set_state(unsigned int pid, unsigned int state);
void tqueue_enqueue(unsigned int chid, unsigned int pid);
unsigned int tqueue_dequeue(unsigned int chid);
void tqueue_remove(unsigned int chid, unsigned int pid);

#endif /* _KERN_ */

#endif /* !_KERN_LIB_CVAR_H_ */
