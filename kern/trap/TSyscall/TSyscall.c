#include <lib/debug.h>
#include <lib/pmap.h>
#include <lib/types.h>
#include <lib/x86.h>
#include <lib/trap.h>
#include <lib/string.h>
#include <lib/syscall.h>
#include <dev/console.h>
#include <dev/intr.h>
#include <pcpu/PCPUIntro/export.h>

#include "import.h"

#define BUFLEN 1024 // from kern/dev/console.c
static char sys_buf[NUM_IDS][PAGESIZE];

/**
 * Copies a string from user into buffer and prints it to the screen.
 * This is called by the user level "printf" library as a system call.
 */
void sys_puts(tf_t *tf)
{
    unsigned int cur_pid;
    unsigned int str_uva, str_len;
    unsigned int remain, cur_pos, nbytes;

    cur_pid = get_curid();
    str_uva = syscall_get_arg2(tf);
    str_len = syscall_get_arg3(tf);

    if (!(VM_USERLO <= str_uva && str_uva + str_len <= VM_USERHI))
    {
        syscall_set_errno(tf, E_INVAL_ADDR);
        return;
    }

    remain = str_len;
    cur_pos = str_uva;

    while (remain)
    {
        if (remain < PAGESIZE - 1)
            nbytes = remain;
        else
            nbytes = PAGESIZE - 1;

        if (pt_copyin(cur_pid, cur_pos, sys_buf[cur_pid], nbytes) != nbytes)
        {
            syscall_set_errno(tf, E_MEM);
            return;
        }

        sys_buf[cur_pid][nbytes] = '\0';
        KERN_INFO("%s", sys_buf[cur_pid]);

        remain -= nbytes;
        cur_pos += nbytes;
    }

    syscall_set_errno(tf, E_SUCC);
}

void sys_readline(tf_t *tf)
{
    char *buf;
    int read;
    unsigned int curid = get_curid();
    uintptr_t line = syscall_get_arg2(tf);
    uintptr_t len = syscall_get_arg3(tf);

    if (!(VM_USERLO <= line && line + len <= VM_USERHI) || len >= BUFLEN)
    {
        syscall_set_errno(tf, E_INVAL_ADDR);
        return;
    }

    buf = readline("$> ");
    len = min(strnlen(buf, BUFLEN - 1), len);
    buf[len] = '\0';
    read = pt_copyout(buf, curid, line, len + 1);
    if (len > 0 && read == 0)
    {
        syscall_set_errno(tf, E_MEM);
        return;
    }

    syscall_set_errno(tf, E_SUCC);
    syscall_set_retval1(tf, read);
}

extern uint8_t _binary___obj_user_pingpong_ping_start[];
extern uint8_t _binary___obj_user_pingpong_pong_start[];
extern uint8_t _binary___obj_user_pingpong_ding_start[];
extern uint8_t _binary___obj_user_fstest_fstest_start[];
extern uint8_t _binary___obj_user_shell_shell_start[];
extern uint8_t _binary___obj_user_debugger_debugger_start[];
extern uint8_t _binary___obj_user_debuggee_debuggee_start[];

/**
 * Spawns a new child process.
 * The user level library function sys_spawn (defined in user/include/syscall.h)
 * takes two arguments [elf_id] and [quota], and returns the new child process id
 * or NUM_IDS (as failure), with appropriate error number.
 * Currently, we have three user processes defined in user/pingpong/ directory,
 * ping, pong, and ding.
 * The linker ELF addresses for those compiled binaries are defined above.
 * Since we do not yet have a file system implemented in mCertiKOS,
 * we statically load the ELF binaries into the memory based on the
 * first parameter [elf_id].
 * For example, ping, pong, and ding correspond to the elf_ids
 * 1, 2, 3, and 4, respectively.
 * If the parameter [elf_id] is none of these, then it should return
 * NUM_IDS with the error number E_INVAL_PID. The same error case apply
 * when the proc_create fails.
 * Otherwise, you should mark it as successful, and return the new child process id.
 */
void sys_spawn(tf_t *tf)
{
    unsigned int new_pid;
    unsigned int elf_id, quota;
    void *elf_addr;
    unsigned int curid = get_curid();

    elf_id = syscall_get_arg2(tf);
    quota = syscall_get_arg3(tf);

    if (!container_can_consume(curid, quota))
    {
        syscall_set_errno(tf, E_EXCEEDS_QUOTA);
        syscall_set_retval1(tf, NUM_IDS);
        return;
    }
    else if (NUM_IDS < curid * MAX_CHILDREN + 1 + MAX_CHILDREN)
    {
        syscall_set_errno(tf, E_MAX_NUM_CHILDEN_REACHED);
        syscall_set_retval1(tf, NUM_IDS);
        return;
    }
    else if (container_get_nchildren(curid) == MAX_CHILDREN)
    {
        syscall_set_errno(tf, E_INVAL_CHILD_ID);
        syscall_set_retval1(tf, NUM_IDS);
        return;
    }

    switch (elf_id)
    {
    case 1:
        elf_addr = _binary___obj_user_pingpong_ping_start;
        break;
    case 2:
        elf_addr = _binary___obj_user_pingpong_pong_start;
        break;
    case 3:
        elf_addr = _binary___obj_user_pingpong_ding_start;
        break;
    case 4:
        elf_addr = _binary___obj_user_fstest_fstest_start;
        break;
    case 5:
        elf_addr = _binary___obj_user_shell_shell_start;
        break;
    case 6:
        elf_addr = _binary___obj_user_debugger_debugger_start;
        break;
    default:
        syscall_set_errno(tf, E_INVAL_PID);
        syscall_set_retval1(tf, NUM_IDS);
        return;
    }

    new_pid = proc_create(elf_addr, quota);

    if (new_pid == NUM_IDS)
    {
        syscall_set_errno(tf, E_INVAL_PID);
        syscall_set_retval1(tf, NUM_IDS);
    }
    else
    {
        syscall_set_errno(tf, E_SUCC);
        syscall_set_retval1(tf, new_pid);
    }
}

void sys_debug_spawn(tf_t *tf)
{
    unsigned int new_pid;
    unsigned int elf_id, quota;
    void *elf_addr;
    unsigned int curid = get_curid();

    elf_id = syscall_get_arg2(tf);
    quota = syscall_get_arg3(tf);

    if (!container_can_consume(curid, quota))
    {
        syscall_set_errno(tf, E_EXCEEDS_QUOTA);
        syscall_set_retval1(tf, NUM_IDS);
        // KERN_DEBUG("debuggee cannot consume\n");
        return;
    }
    else if (NUM_IDS < curid * MAX_CHILDREN + 1 + MAX_CHILDREN)
    {
        syscall_set_errno(tf, E_MAX_NUM_CHILDEN_REACHED);
        syscall_set_retval1(tf, NUM_IDS);
        // KERN_DEBUG("debuggee exceeded max pid\n");
        return;
    }
    else if (container_get_nchildren(curid) == MAX_CHILDREN)
    {
        syscall_set_errno(tf, E_INVAL_CHILD_ID);
        syscall_set_retval1(tf, NUM_IDS);
        // KERN_DEBUG("debuggee exceeded max children\n");
        return;
    }

    switch (elf_id)
    {
    case 1:
        elf_addr = _binary___obj_user_pingpong_ping_start;
        break;
    case 2:
        elf_addr = _binary___obj_user_pingpong_pong_start;
        break;
    case 3:
        elf_addr = _binary___obj_user_pingpong_ding_start;
        break;
    case 4:
        elf_addr = _binary___obj_user_fstest_fstest_start;
        break;
    case 5:
        elf_addr = _binary___obj_user_shell_shell_start;
        break;
    case 6:
        elf_addr = _binary___obj_user_debugger_debugger_start;
        break;
    case 7:
        elf_addr = _binary___obj_user_debuggee_debuggee_start;
        break;
    default:
        syscall_set_errno(tf, E_INVAL_PID);
        syscall_set_retval1(tf, NUM_IDS);
        return;
    }

    // KERN_DEBUG("proc debug creating\n");
    new_pid = proc_debug_create(elf_addr, quota);
    // KERN_DEBUG("proc debug created\n");

    if (new_pid == NUM_IDS)
    {
        syscall_set_errno(tf, E_INVAL_PID);
        syscall_set_retval1(tf, NUM_IDS);
    }
    else
    {
        syscall_set_errno(tf, E_SUCC);
        syscall_set_retval1(tf, new_pid);
    }
}

/**
 * Start the debuggee process
 */
void sys_debug_start(tf_t *tf)
{
    // KERN_DEBUG("yielding in sys_debug_start\n");
    unsigned int debuggee_pid = syscall_get_arg2(tf);
    thread_yield_to(debuggee_pid);
    syscall_set_errno(tf, E_SUCC);
}

/**
 * Invalidate address in the debuggee
 */
void sys_add_breakpoint(tf_t *tf)
{

    unsigned int succ;
    // KERN_DEBUG("yielding in sys_debug_start\n");
    unsigned int debuggee_pid = syscall_get_arg2(tf);
    unsigned int debuggee_addr = syscall_get_arg3(tf);

    if (debuggee_addr < VM_USERLO || debuggee_addr > VM_USERHI)
    {
        syscall_set_errno(tf, E_INVAL_ADDR);
        return;
    }

    //invalidate debuggee addr
    succ = add_breakpoint(debuggee_pid, debuggee_addr);
    if (succ == 1)
    {
        syscall_set_errno(tf, E_SUCC);
    }
    else
    {
        syscall_set_errno(tf, E_INVAL_ADDR);
    }
}

char read_addr_buff[100];
void sys_read_address(tf_t *tf)
{
    memzero(read_addr_buff, 100);
    uint32_t pid = syscall_get_arg2(tf);
    uintptr_t dst = syscall_get_arg3(tf);
    uintptr_t vaddr = syscall_get_arg4(tf);
    unsigned int len = syscall_get_arg5(tf);
    
    if (vaddr < VM_USERLO || vaddr + 1 > VM_USERHI || dst < VM_USERLO || dst + 1 > VM_USERHI)
    {
        syscall_set_errno(tf, E_INVAL_ADDR);
        return;
    }
    // define kernel buffer to store memory in
    // unsigned int data[len];
    // copy from user's vaddr into kernel
    size_t num_read = pt_copyin(pid, vaddr, (void *)read_addr_buff, len);
    // verify it worked
    if (num_read == 0) {
        syscall_set_errno(tf, E_INVAL_ADDR);
        return;
    }
    else if (num_read != len)
    {
        syscall_set_errno(tf, E_MEM);
        return;
    }

    // copy from kernel's buffer to user's destination buffer
    // size_t pt_copyout(void *kva, uint32_t pmap_id, uintptr_t uva, size_t len)
    size_t num_wrote = pt_copyout((void *)read_addr_buff, get_curid(), dst, len);

    if (num_wrote == 0) {
        syscall_set_errno(tf, E_INVAL_ADDR);
        return;
    }
    else if (num_wrote != len)
    {
        syscall_set_errno(tf, E_MEM);
        return;
    }
    syscall_set_errno(tf, E_SUCC);
    return;
}

/**
 * Yields to another thread/process.
 * The user level library function sys_yield (defined in user/include/syscall.h)
 * does not take any argument and does not have any return values.
 * Do not forget to set the error number as E_SUCC.
 */
void sys_yield(tf_t *tf)
{
    thread_yield();
    syscall_set_retval1(tf, 0);
    syscall_set_errno(tf, E_SUCC);
}
