#ifndef _KERN_VMM_MPTCOPY_H_
#define _KERN_VMM_MPTCOPY_H_

#ifdef _KERN_

void shallow_copy_mem(unsigned int from_pid, unsigned int to_pid);
void deep_copy_mem(unsigned int id, unsigned int vaddr);

#endif /* _KERN_ */

#endif /* !_KERN_VMM_MPTINTRO_H_ */
