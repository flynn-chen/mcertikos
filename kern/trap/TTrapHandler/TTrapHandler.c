#include <lib/string.h>
#include <lib/trap.h>
#include <lib/syscall.h>
#include <lib/debug.h>
#include <lib/kstack.h>
#include <lib/x86.h>
#include <dev/intr.h>
#include <pcpu/PCPUIntro/export.h>

#include <vmm/MPTOp/export.h>
#include <thread/PThread/export.h>

#include "import.h"

static void trap_dump(tf_t *tf)
{
    if (tf == NULL)
        return;

    uintptr_t base = (uintptr_t)tf;

    KERN_DEBUG("trapframe at %x\n", base);
    KERN_DEBUG("\t%08x:\tedi:   \t\t%08x\n", &tf->regs.edi, tf->regs.edi);
    KERN_DEBUG("\t%08x:\tesi:   \t\t%08x\n", &tf->regs.esi, tf->regs.esi);
    KERN_DEBUG("\t%08x:\tebp:   \t\t%08x\n", &tf->regs.ebp, tf->regs.ebp);
    KERN_DEBUG("\t%08x:\tesp:   \t\t%08x\n", &tf->regs.oesp, tf->regs.oesp);
    KERN_DEBUG("\t%08x:\tebx:   \t\t%08x\n", &tf->regs.ebx, tf->regs.ebx);
    KERN_DEBUG("\t%08x:\tedx:   \t\t%08x\n", &tf->regs.edx, tf->regs.edx);
    KERN_DEBUG("\t%08x:\tecx:   \t\t%08x\n", &tf->regs.ecx, tf->regs.ecx);
    KERN_DEBUG("\t%08x:\teax:   \t\t%08x\n", &tf->regs.eax, tf->regs.eax);
    KERN_DEBUG("\t%08x:\tes:    \t\t%08x\n", &tf->es, tf->es);
    KERN_DEBUG("\t%08x:\tds:    \t\t%08x\n", &tf->ds, tf->ds);
    KERN_DEBUG("\t%08x:\ttrapno:\t\t%08x\n", &tf->trapno, tf->trapno);
    KERN_DEBUG("\t%08x:\terr:   \t\t%08x\n", &tf->err, tf->err);
    KERN_DEBUG("\t%08x:\teip:   \t\t%08x\n", &tf->eip, tf->eip);
    KERN_DEBUG("\t%08x:\tcs:    \t\t%08x\n", &tf->cs, tf->cs);
    KERN_DEBUG("\t%08x:\teflags:\t\t%08x\n", &tf->eflags, tf->eflags);
    KERN_DEBUG("\t%08x:\tesp:   \t\t%08x\n", &tf->esp, tf->esp);
    KERN_DEBUG("\t%08x:\tss:    \t\t%08x\n", &tf->ss, tf->ss);
}

void default_exception_handler(tf_t *tf)
{
    unsigned int cur_pid;

    cur_pid = get_curid();
    trap_dump(tf);

    KERN_PANIC("Trap %d @ 0x%08x.\n", tf->trapno, tf->eip);
}

void pgflt_handler(tf_t *tf)
{
    unsigned int cur_pid;
    unsigned int errno;
    unsigned int fault_va;

    cur_pid = get_curid();
    errno = tf->err;
    fault_va = rcr2();

    // Uncomment this line to see information about the page fault
    // KERN_DEBUG("Page fault: VA 0x%08x, errno 0x%08x, process %d, EIP 0x%08x.\n",
    //            fault_va, errno, cur_pid, uctx_pool[cur_pid].eip);

    if (errno & PFE_PR)
    {
        KERN_PANIC("Permission denied: va = 0x%08x, errno = 0x%08x.\n",
                   fault_va, errno);
        return;
    }

    if (alloc_page(cur_pid, fault_va, PTE_W | PTE_U | PTE_P) == MagicNumber)
    {
        KERN_PANIC("Page allocation failed: va = 0x%08x, errno = 0x%08x.\n",
                   fault_va, errno);
    }
}

/**
 * We currently only handle the page fault exception.
 * All other exceptions should be routed to the default exception handler.
 */
void exception_handler(tf_t *tf)
{
    if (tf->trapno == T_PGFLT)
        pgflt_handler(tf);
    else
        default_exception_handler(tf);
}

static int spurious_intr_handler(void)
{
    return 0;
}

static int timer_intr_handler(void)
{
    intr_eoi();
    sched_update();
    return 0;
}

static int default_intr_handler(void)
{
    intr_eoi();
    return 0;
}

/**
 * Any interrupt request other than the spurious or timer should be
 * routed to the default interrupt handler.
 */
void interrupt_handler(tf_t *tf)
{
    switch (tf->trapno)
    {
    case T_IRQ0 + IRQ_SPURIOUS:
        spurious_intr_handler();
        break;
    case T_IRQ0 + IRQ_TIMER:
        timer_intr_handler();
        break;
    default:
        default_intr_handler();
    }
}

void trap(tf_t *tf)
{
    unsigned int cur_pid = get_curid();
    KERN_ASSERT(cur_pid != 0);
    trap_cb_t handler;

    // if (cur_pid != get_previous_id())
    set_pdir_base(0); // switch to the kernel's page table

    handler = TRAP_HANDLER[get_pcpu_idx()][tf->trapno];

    if (handler)
    {
        handler(tf);
    }
    else
    {
        KERN_WARN("No handler for user trap 0x%x, process %d, eip 0x%08x.\n",
                  tf->trapno, cur_pid, tf->eip);
    }
    /*

    PID 1 is enjoying life
        cur_id = 1, prev_id = NUM_IDS
    
    timer interrupt, yield to PID 2
        cur_id = 2, prev_id = 1

    page fault
        trap
            if prev_id != NUM_IDS (prev_id == 1)
                set page directory to 0

            prev_id = NUM_IDS

            handle the trap

            prev_id = 1

            if prev_id != NUM_IDS (prev_id == 1)
                set page directory back to 2

            return


    page fault
        trap
            if prev_id != NUM_IDS (prev_id == 1)
                set page directory to 0

            prev_id = NUM_IDS

            timer interrupt
                trap
                    if prev_id != NUM_IDS (prev_id == 1)
                        set page directory to 0

            handle the trap
            if prev_id != NUM_IDS (prev_id == 1)
                set page directory back to 2
            return


    user accesses bad mem (CPU 1 PID 4)
    page fault
        trap1   (switch to CPU 1 PID 0)
        timer interrupt
            trap2   goal: avoid switching to CPU 1 PID 0 because we're already here?
            yield to CPU 1 PID 5
            ...
            we eventually get yielded to and woken up
            our kstack is CPU 1 PID 0?
            we should trap_return from trap2 and continue in trap1
        finish handling the page fault
        trap_return to user process
                    .
                    .
                    .


    PID 1
    page fault
        trap1
        timer
                  
    */

    // check for per CPU previous_id in PThread.c
    if (cur_pid != get_previous_id())
    {
        // KERN_ASSERT(previous_id() == 0);
        kstack_switch(cur_pid);
    }
    set_pdir_base(cur_pid);
    // invalidate the previous array
    // invalidate_previous_id();
    trap_return((void *)tf);
}
