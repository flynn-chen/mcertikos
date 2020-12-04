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

int debug_invalidate(unsigned int pid, unsigned int addr)
{
    return sys_debug_invalidate(pid, addr);
}

void yield(void)
{
    sys_yield();
}
