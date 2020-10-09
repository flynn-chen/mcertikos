#include <lib/trap.h>
#include <lib/x86.h>

#include "import.h"

extern tf_t uctx_pool[NUM_IDS];

/**
 * Retrieves the system call arguments from uctx_pool that get
 * passed in from the current running process' system call.
 */

/**
 * The system call number will go in %eax, and the arguments (up to five of them) will go in %ebx, %ecx, %edx, %esi, and %edi, respectively.
 * 
 */
unsigned int syscall_get_arg1(void)
{
    unsigned int pid = get_curid();
    return uctx_pool[pid].regs.eax;
}

unsigned int syscall_get_arg2(void)
{
    unsigned int pid = get_curid();
    return uctx_pool[pid].regs.ebx;
}

unsigned int syscall_get_arg3(void)
{
    unsigned int pid = get_curid();
    return uctx_pool[pid].regs.ecx;
}

unsigned int syscall_get_arg4(void)
{
    unsigned int pid = get_curid();
    return uctx_pool[pid].regs.edx;
}

unsigned int syscall_get_arg5(void)
{
    unsigned int pid = get_curid();
    return uctx_pool[pid].regs.esi;
}

unsigned int syscall_get_arg6(void)
{
    unsigned int pid = get_curid();
    return uctx_pool[pid].regs.edi;
}

/**
 * Sets the error number in uctx_pool that gets passed
 * to the current running process when we return to it.
 * 
 * A system call always returns with an error number via register EAX.
 * All valid error numbers are listed in __error_nr defined in kern/lib/syscall.h.
 * E_SUCC indicates success (no errors).
 * A system call can return at most 5 32-bit values via registers EBX, ECX, EDX, ESI and EDI.
 */
void syscall_set_errno(unsigned int errno)
{
    unsigned int pid = get_curid();
    uctx_pool[pid].regs.eax = errno;
}

/**
 * Sets the return values in uctx_pool that get passed
 * to the current running process when we return to it.
 */
void syscall_set_retval1(unsigned int retval)
{
    unsigned int pid = get_curid();
    uctx_pool[pid].regs.ebx = retval;
}

void syscall_set_retval2(unsigned int retval)
{
    unsigned int pid = get_curid();
    uctx_pool[pid].regs.ecx = retval;
}

void syscall_set_retval3(unsigned int retval)
{
    unsigned int pid = get_curid();
    uctx_pool[pid].regs.edx = retval;
}

void syscall_set_retval4(unsigned int retval)
{
    unsigned int pid = get_curid();
    uctx_pool[pid].regs.esi = retval;
}

void syscall_set_retval5(unsigned int retval)
{
    unsigned int pid = get_curid();
    uctx_pool[pid].regs.edi = retval;
}
