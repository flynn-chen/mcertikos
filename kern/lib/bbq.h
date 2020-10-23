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
/*
CPU1: process ping1 1 is created.
CPU1: process ping2 2 is created.
[D] kern/thread/PThread/PThread.c:89: CPU 1 just got interrupted
From cpu 1: ping started.
[D] kern/trap/TSyscall/TSyscall.c:163: CPU 1: Process 1: Produced 0
[D] kern/trap/TSyscall/TSyscall.c:163: CPU 1: Process 1: Produced 1
[D] kern/trap/TSyscall/TSyscall.c:163: CPU 1: Process 1: Produced 2
[D] kern/trap/TSyscall/TSyscall.c:163: CPU 1: Process 1: Produced 3
[D] kern/trap/TSyscall/TSyscall.c:163: CPU 1: Process 1: Produced 4
[D] kern/thread/PThread/PThread.c:89: CPU 1 just got interrupted
[D] kern/trap/TSyscall/TSyscall.c:163: CPU 1: Process 1: Produced 0
[D] kern/trap/TSyscall/TSyscall.c:163: CPU 1: Process 1: Produced 1
[D] kern/trap/TSyscall/TSyscall.c:163: CPU 1: Process 1: Produced 2
[D] kern/trap/TSyscall/TSyscall.c:163: CPU 1: Process 1: Produced 3
[D] kern/lib/bbq.c:35:  CPU 1: Process 1: just filled the buffer
[D] kern/trap/TSyscall/TSyscall.c:163: CPU 1: Process 1: Produced 4
[D] kern/lib/bbq.c:26:  CPU 1: Process 1: Waiting to insert 0 (full buffer)
[D] kern/lib/cvar.c:87: CPU 1: Process 1: is going to start up 2
From cpu 1: ping started.
[D] kern/lib/bbq.c:26:  CPU 1: Process 2: Waiting to insert 0 (full buffer)
[D] kern/lib/cvar.c:87: CPU 1: Process 2: is going to start up 3
[D] kern/thread/PThread/PThread.c:89: CPU 1 just got interrupted
[D] kern/thread/PThread/PThread.c:89: CPU 1 just got interrupted
[D] kern/thread/PThread/PThread.c:89: CPU 1 just got interrupted
[D] kern/thread/PThread/PThread.c:96: CPU 1 is yielding
[D] kern/thread/PThread/PThread.c:74: CPU 1 is yielding from 3 to 3

^ that's all fine

[D] kern/dev/lapic.c:117: [1] Retry to calibrate internal timer of LAPIC.
[D] kern/dev/lapic.c:125: LAPIC timer freq = 999930000 Hz.
[D] kern/dev/lapic.c:129: Set LAPIC TICR = f41fa.
[AP2 KERN] INTR initialized.
[AP2 KERN] Register trap handlers...
[AP2 KERN] Done.
[AP2 KERN] Enabling interrupts...
[AP2 KERN] Done.
[AP2 KERN] kernel_main_ap
CPU2: process pong1 4 is created.
CPU2: process pong2 5 is created.
From cpu 2: pong started.
[D] kern/lib/cvar.c:135: Putting CPU 1 Process 1 on the ready queue    <----- at this point, if CPU 1 is ever interrupted, it should wake up Process 1,
[D] kern/trap/TSyscall/TSyscall.c:176: CPU 2: Process 4: Consumed 0           but CPU 1 never gets interrupted again for some reason
[D] kern/lib/cvar.c:135: Putting CPU 1 Process 2 on the ready queue
[D] kern/trap/TSyscall/TSyscall.c:176: CPU 2: Process 4: Consumed 1
[D] kern/trap/TSyscall/TSyscall.c:176: CPU 2: Process 4: Consumed 2
[D] kern/trap/TSyscall/TSyscall.c:176: CPU 2: Process 4: Consumed 3
[D] kern/trap/TSyscall/TSyscall.c:176: CPU 2: Process 4: Consumed 4
[D] kern/trap/TSyscall/TSyscall.c:176: CPU 2: Process 4: Consumed 0
[D] kern/trap/TSyscall/TSyscall.c:176: CPU 2: Process 4: Consumed 1
[D] kern/trap/TSyscall/TSyscall.c:176: CPU 2: Process 4: Consumed 2
[D] kern/trap/TSyscall/TSyscall.c:176: CPU 2: Process 4: Consumed 3
[D] kern/trap/TSyscall/TSyscall.c:176: CPU 2: Process 4: Consumed 4
[D] kern/lib/bbq.c:49:  CPU 2: Process 4: Waiting to remove (empty buffer)
[D] kern/lib/cvar.c:87: CPU 2: Process 4: is going to start up 5
From cpu 2: pong started.
[D] kern/lib/bbq.c:49:  CPU 2: Process 5: Waiting to remove (empty buffer)
[D] kern/lib/cvar.c:87: CPU 2: Process 5: is going to start up 6
[D] kern/dev/lapic.c:117: [0] Retry to calibrate internal timer of LAPIC.
[D] kern/thread/PThread/PThread.c:89: CPU 2 just got interrupted
[D] kern/thread/PThread/PThread.c:89: CPU 2 just got interrupted
[D] kern/thread/PThread/PThread.c:89: CPU 2 just got interrupted
[D] kern/thread/PThread/PThread.c:89: CPU 2 just got interrupted
[D] kern/thread/PThread/PThread.c:89: CPU 2 just got interrupted
[D] kern/thread/PThread/PThread.c:96: CPU 2 is yielding
[D] kern/thread/PThread/PThread.c:74: CPU 2 is yielding from 6 to 6
[D] kern/dev/lapic.c:117: [1] Retry to calibrate internal timer of LAPIC.
[D] kern/dev/lapic.c:125: LAPIC timer freq = 999930000 Hz.
[D] kern/dev/lapic.c:129: Set LAPIC TICR = f41fa.
[AP3 KERN] INTR initialized.
[AP3 KERN] Register trap handlers...
[AP3 KERN] Done.
[AP3 KERN] Enabling interrupts...
[AP3 KERN] Done.
[AP3 KERN] kernel_main_ap
*/

#endif /* _KERN_ */

#endif /* !_KERN_LIB_CVAR_H_ */