#ifndef _KERN_VMM_MPTOP_H_
#define _KERN_VMM_MPTOP_H_

#ifdef _KERN_

unsigned int get_pdir_entry_by_va(unsigned int proc_index, unsigned int vaddr);
void set_pdir_entry_by_va(unsigned int proc_index, unsigned int vaddr,
                          unsigned int page_index);
void rmv_pdir_entry_by_va(unsigned int proc_index, unsigned int vaddr);
unsigned int get_ptbl_entry_by_va(unsigned int proc_index, unsigned int vaddr);
void set_ptbl_entry_by_va(unsigned int proc_index, unsigned int vaddr,
                          unsigned int page_index, unsigned int perm);
void rmv_ptbl_entry_by_va(unsigned int proc_index, unsigned int vaddr);
void idptbl_init(unsigned int mbi_addr);
unsigned int add_breakpoint(unsigned int proc_index, unsigned int vaddr);
unsigned int remove_breakpoint(unsigned int proc_index, unsigned int vaddr);
void remove_all_breakpoints(unsigned int pid);

#endif /* _KERN_ */

#endif /* !_KERN_VMM_MPTOP_H_ */
