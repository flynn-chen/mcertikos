#ifndef _KERN_LIB_BBQ_H_
#define _KERN_LIB_BBQ_H_

#ifdef _KERN_

#include <lib/gcc.h>
#include <lib/types.h>
#include <lib/x86.h>
#include <lib/spinlock.h>
#include <lib/cvar.h>

#define BBQ_SIZE 10

/**
 * Queue implementation using an array
 * Assumes there can be at most NUM_IDS
 * cpus waiting simultaneously
*/
#define BUFF_SIZE NUM_IDS

typedef struct
{
    unsigned int front;
    unsigned int next_empty;
    unsigned int buff[BBQ_SIZE];
    spinlock_t bbq_lock;
    cvar_t item_added;
    cvar_t item_removed;
} bbq_t;

extern bbq_t shared_bbq;

void bbq_init(bbq_t *bbq);
void bbq_insert(bbq_t *bbq, unsigned int item);
unsigned int bbq_remove(bbq_t *bbq);

#endif /* _KERN_ */

#endif /* !_KERN_LIB_CVAR_H_ */