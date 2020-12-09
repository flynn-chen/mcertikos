#include <proc.h>
#include <syscall.h>
#include <types.h>

pid_t spawn(uintptr_t exec, unsigned int quota)
{
    return sys_spawn(exec, quota);
}

pid_t debug_spawn(uintptr_t exec, unsigned int quota)
{
    return sys_debug_spawn(exec, quota);
}

void debug_start(unsigned int pid)
{
    return sys_debug_start(pid);
}

int add_breakpoint(unsigned int pid, unsigned int addr)
{
    return sys_add_breakpoint(pid, addr);
}

int read_address(unsigned int pid, unsigned int dst, unsigned int vaddr, unsigned int len)
{
    return sys_read_address(pid, dst, vaddr, len);
}

void yield(void)
{
    sys_yield();
}
