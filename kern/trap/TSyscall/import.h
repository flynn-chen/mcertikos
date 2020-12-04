#ifndef _KERN_TRAP_TSYSCALL_H_
#define _KERN_TRAP_TSYSCALL_H_

#ifdef _KERN_

unsigned int get_curid(void);
unsigned int syscall_get_arg1(tf_t *tf);
unsigned int syscall_get_arg2(tf_t *tf);
unsigned int syscall_get_arg3(tf_t *tf);
unsigned int syscall_get_arg4(tf_t *tf);
unsigned int syscall_get_arg5(tf_t *tf);
unsigned int syscall_get_arg6(tf_t *tf);

void syscall_set_errno(tf_t *tf, unsigned int errno);
void syscall_set_retval1(tf_t *tf, unsigned int retval);
void syscall_set_retval2(tf_t *tf, unsigned int retval);
void syscall_set_retval3(tf_t *tf, unsigned int retval);
void syscall_set_retval4(tf_t *tf, unsigned int retval);
void syscall_set_retval5(tf_t *tf, unsigned int retval);

unsigned int container_can_consume(unsigned int curid, unsigned int quota);
unsigned int container_get_nchildren(unsigned int curid);
unsigned int proc_create(void *elf_addr, unsigned int quota);
unsigned int proc_debug_create(void *elf_addr, unsigned int quota);
void thread_yield(void);
void thread_yield_to(unsigned int pid);
unsigned int invalidate_address(unsigned int pid, unsigned int vaddr);

#endif /* _KERN_ */

#endif /* !_KERN_TRAP_TSYSCALL_H_ */
