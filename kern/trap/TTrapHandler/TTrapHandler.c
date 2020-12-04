#include <lib/string.h>
#include <lib/trap.h>
#include <lib/syscall.h>
#include <lib/debug.h>
#include <lib/kstack.h>
#include <lib/x86.h>
#include <dev/intr.h>
#include <pcpu/PCPUIntro/export.h>
#include <kern/proc/PProc/export.h>
#include <vmm/MPTOp/export.h>
#include <thread/PThread/export.h>

#include "import.h"

void ide_intr(void);

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
    // KERN_DEBUG("Page fault: VA 0x%08x, errno 0x%08x, process %d.\n",
    //            fault_va, errno, cur_pid);

    // breakpoint handling
    // if ((errno & 0x4) == 0x4)
    // {
    //     // error for writing to a read-only page
    //     unsigned int pte_entry = get_ptbl_entry_by_va(cur_pid, fault_va);
    //     if (pte_entry & PTE_BRK)
    //     {
    //         // handling copy-on-write
    //         KERN_DEBUG("WE FOUND A BREAKPOINT!!!!!!\n");
    //         // suspend the running process
    //         // start up the debugger
    //         unsigned int debugger_pid = tcb_get_debugger_id(cur_pid);
    //         thread_yield_to(debugger_pid);
    //         KERN_DEBUG("returned from debugger, revalidating the address 0x%08x\n", fault_va);
    //         validate_address(cur_pid, fault_va);
    //         return;
    //         // Before debugger restarts the debuggee, we have to validate this address
    //     }
    // }

    if (errno & PFE_PR)
    {
        trap_dump(tf);
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

unsigned int last_vaddr[NUM_IDS];

void breakpoint_handler(tf_t *tf)
{
    if (tf->trapno != T_BRKPT)
    {
        return;
    }

    unsigned int cur_pid;
    unsigned int errno;
    unsigned int fault_va;

    cur_pid = get_curid();
    errno = tf->err;
    fault_va = rcr2();
    unsigned int intr_addr = tf->eip - 1;
    KERN_DEBUG("Handling breakpoint at addr 0x%08x\n", intr_addr);

    // error for writing to a read-only page
    // KERN_DEBUG("getting breakpoint intr addr\n");
    unsigned int pte_entry = get_ptbl_entry_by_va(cur_pid, intr_addr);
    // KERN_DEBUG("got breakpoint intr addr\n");

    // suspend the running process
    // start up the debugger
    // KERN_DEBUG("getting debugger id\n");
    unsigned int debugger_pid = tcb_get_debugger_id(cur_pid);
    // KERN_DEBUG("got debugger id\n");
    thread_yield_to(debugger_pid);
    KERN_DEBUG("returned from debugger, revalidating the address 0x%08x\n", intr_addr);
    // Before we return to the debuggee, we have to validate this address
    validate_address(cur_pid, intr_addr);
    KERN_DEBUG("finished validating\n");
    // set the trap flag on eflags register (should enable single step)
    // proc_enable_single_step(cur_pid);
    KERN_DEBUG("enabled single step in pid: %d 0x%08x\n", cur_pid, intr_addr);
    last_vaddr[cur_pid] = intr_addr;

    /*
        currently arr CURR instruction
        1. write a breakpoint at NEXT instruction
        2. when we break at NEXT instr, restore the CURR instr's breakpoint
        3. erase NEXT breakpoint and continue

        1. single step (^)
        2. restore old breakpoint
    */
    return;
}

void single_step_handler(tf_t *tf)
{
    unsigned int cur_pid = get_curid();
    KERN_DEBUG("got a single step interrupt, invalidating 0x%08x\n", last_vaddr[cur_pid]);
    // restore the breakpoint that got erased when it was handled
    invalidate_address(cur_pid, last_vaddr[cur_pid]);
    proc_disable_single_step(cur_pid);
    return;
}
/**
 * We currently only handle the page fault exception.
 * All other exceptions should be routed to the default exception handler.
 */
void exception_handler(tf_t *tf)
{
    if (tf->trapno == T_PGFLT)
        pgflt_handler(tf);
    else if (tf->trapno == T_DEBUG)
        single_step_handler(tf);
    else if (tf->trapno == T_BRKPT)
        breakpoint_handler(tf);
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
    case T_IRQ0 + IRQ_IDE2:
        spurious_intr_handler();
        break;
    case T_IRQ0 + IRQ_TIMER:
        timer_intr_handler();
        break;
    // handle the disk interrupts here
    case T_IRQ0 + IRQ_IDE1:
        ide_intr();
        intr_eoi();
        break;
    default:
        default_intr_handler();
    }
}

unsigned int last_active[NUM_CPUS];

void trap(tf_t *tf)
{
    unsigned int cur_pid = get_curid();
    unsigned int cpu_idx = get_pcpu_idx();
    trap_cb_t handler;

    unsigned int last_pid = last_active[cpu_idx];

    if (last_pid != 0)
    {
        set_pdir_base(0); // switch to the kernel's page table
        last_active[cpu_idx] = 0;
    }

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

    if (last_pid != 0)
    {
        kstack_switch(cur_pid);
        set_pdir_base(cur_pid);
        last_active[cpu_idx] = last_pid;
    }
    trap_return((void *)tf);
}
