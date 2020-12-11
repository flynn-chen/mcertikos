#ifndef _USER_PROC_H_
#define _USER_PROC_H_

#include <types.h>

pid_t spawn(unsigned int elf_id, unsigned int quota);
pid_t debug_spawn(unsigned int elf_id, unsigned int quota);
int debug_start(unsigned int pid);
void debug_end(unsigned int pid);
int add_breakpoint(unsigned int pid, unsigned int vaddr);
int read_address(unsigned int pid, unsigned int dst, unsigned int vaddr, unsigned int len);
void yield(void);

#endif /* !_USER_PROC_H_ */
