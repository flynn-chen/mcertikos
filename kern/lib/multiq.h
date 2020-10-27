#ifndef _KERN_LIB_MULTIQ_H_
#define _KERN_LIB_MULTIQ_H_

#ifdef _KERN_

#include <lib/gcc.h>
#include <lib/types.h>
#include <lib/x86.h>

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
} multiq_elt_t;

typedef struct
{
    multiq_elt_t buff[BUFF_SIZE];
    unsigned int head;
    unsigned int tail;
} multiq_t;

void multiq_enqueue(multiq_t *q, unsigned int pid, unsigned int pcpu_idx);
multiq_elt_t *multiq_dequeue(multiq_t *q);
bool multiq_is_empty(multiq_t *q);

#endif /* _KERN_ */

#endif /* !_KERN_LIB_MULTIQ_H_ */
