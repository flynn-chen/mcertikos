#ifndef _USER_PROC_H_
#define _USER_PROC_H_

#include <types.h>

pid_t spawn(unsigned int elf_id, unsigned int quota);
pid_t debug_spawn(unsigned int elf_id, unsigned int quota);
void debug_start(unsigned int pid);
void yield(void);

#endif /* !_USER_PROC_H_ */
