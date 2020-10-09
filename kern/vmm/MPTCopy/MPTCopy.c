#include <lib/gcc.h>
#include <lib/x86.h>
#include <lib/debug.h>

#include "import.h"

#define PT_PERM_UP 0
#define PT_PERM_PTU (PTE_P | PTE_W | PTE_U)
#define PT_COW (PTE_P | PTE_U)

#define VA_PDE_MASK 0xFFC00000 // 11111111110000000000000000000000
#define VA_PTE_MASK 0x003ff000 // 00000000001111111111000000000000
#define PAGESIZE 4096
#define VM_USERLO 0X40000000
#define VM_USERHI 0xF0000000
#define VM_USERLO_PI (VM_USERLO / PAGESIZE)
#define VM_USERHI_PI (VM_USERHI / PAGESIZE)
/*
void set_pdir_base(unsigned int index);
unsigned int get_pdir_entry(unsigned int proc_index, unsigned int pde_index);
void set_pdir_entry(unsigned int proc_index, unsigned int pde_index,
                    unsigned int page_index);
void set_pdir_entry_identity(unsigned int proc_index, unsigned int pde_index);
void rmv_pdir_entry(unsigned int proc_index, unsigned int pde_index);
unsigned int get_ptbl_entry(unsigned int proc_index, unsigned int pde_index,
                            unsigned int pte_index);
void set_ptbl_entry(unsigned int proc_index, unsigned int pde_index,
                    unsigned int pte_index, unsigned int page_index,
                    unsigned int perm);
void set_ptbl_entry_identity(unsigned int pde_index, unsigned int pte_index,
                             unsigned int perm);
void rmv_ptbl_entry(unsigned int proc_index, unsigned int pde_index,
                    unsigned int pte_index);
*/

void shallow_copy_mem(unsigned int from_pid, unsigned int to_pid)
{
    unsigned int pde_val, page_index;
    // iterate through the page directory entry
    for (unsigned int pde_index = 0; pde_index < 1024; pde_index++)
    {
        // point to the same addr space
        pde_val = get_pdir_entry(from_pid, pde_index);
        page_index = pde_val >> 12;
        if (page_index < VM_USERLO_PI || VM_USERHI_PI >= page_index)
        {
            continue;
        }

        // point to the same page index for the page directory entry
        set_pdir_entry(to_pid, pde_index, page_index);

        // only update page table entry permission
        update_perm_ptbl(to_pid, pde_index);
    }
}

void update_perm_pbtl(unsigned int proc_index, unsigned int pde_index)
{
    unsigned int ptbl_entry, page_index;
    // iterate through each pbtl entry index
    for (unsigned int pte_index = 0; pte_index < 1024; pte_index++)
    {
        // get the entry that's currenty there
        ptbl_entry = get_ptbl_entry(proc_index, pde_index, pte_index);
        page_index = ptbl_entry >> 12;
        // don't modify it if it's not user memory
        if (page_index < VM_USERLO_PI || VM_USERHI_PI >= page_index)
        {
            continue;
        }
        // update the entry with new permissions
        set_ptbl_entry(proc_index, pde_index, pte_index, page_index, PT_COW);
    }
}