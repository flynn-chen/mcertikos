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

void yield(void)
{
    sys_yield();
}
