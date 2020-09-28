#include <lib/x86.h>
#include <lib/debug.h>

#include "import.h"

#define VA_PDE_MASK 0xFFC00000 // 11111111110000000000000000000000
#define VA_PTE_MASK 0x003ff000 // 00000000001111111111000000000000
#define PAGESIZE 4096
#define VM_USERLO 0x40000000
#define VM_USERHI 0xF0000000
#define VM_USERLO_PI (VM_USERLO / PAGESIZE)
#define VM_USERHI_PI (VM_USERHI / PAGESIZE)
/**
 * Returns the page table entry corresponding to the virtual address,
 * according to the page structure of process # [proc_index].
 * Returns 0 if the mapping does not exist.
 */
unsigned int get_ptbl_entry_by_va(unsigned int proc_index, unsigned int vaddr)
{
    unsigned int pde_index = (vaddr & VA_PDE_MASK) >> 22;
    unsigned int pte_index = (vaddr & VA_PTE_MASK) >> 12;

    unsigned int pde = get_pdir_entry(proc_index, pde_index);
    if ((pde & 1) != 1) // check for present bit
    {
        return 0;
    }
    unsigned int pte = get_ptbl_entry(proc_index, pde_index, pte_index);
    if ((pte & 1) != 1)
    {
        return 0;
    }
    return pte;
}

// Returns the page directory entry corresponding to the given virtual address.
unsigned int get_pdir_entry_by_va(unsigned int proc_index, unsigned int vaddr)
{
    unsigned int pde_index = (vaddr & VA_PDE_MASK) >> 22;

    unsigned int pde = get_pdir_entry(proc_index, pde_index);
    if (pde & 1 != 1) // check for present bit
    {
        return 0;
    }
    return pde;
}

// Removes the page table entry for the given virtual address.
void rmv_ptbl_entry_by_va(unsigned int proc_index, unsigned int vaddr)
{
    unsigned int pde_index = (vaddr & VA_PDE_MASK) >> 22;
    unsigned int pte_index = (vaddr & VA_PTE_MASK) >> 12;
    unsigned int pde = get_pdir_entry(proc_index, pde_index);
    if (pde & 1 != 1) // check for present bit
    {
        return;
    }
    rmv_ptbl_entry(proc_index, pde_index, pte_index);
}

// Removes the page directory entry for the given virtual address.
void rmv_pdir_entry_by_va(unsigned int proc_index, unsigned int vaddr)
{
    unsigned int pde_index = (vaddr & VA_PDE_MASK) >> 22;
    rmv_pdir_entry(proc_index, pde_index);
}

// Maps the virtual address [vaddr] to the physical page # [page_index] with permission [perm].
// You do not need to worry about the page directory entry. just map the page table entry.
void set_ptbl_entry_by_va(unsigned int proc_index, unsigned int vaddr,
                          unsigned int page_index, unsigned int perm)
{
    unsigned int pde_index = (vaddr & VA_PDE_MASK) >> 22;
    unsigned int pte_index = (vaddr & VA_PTE_MASK) >> 12;
    set_ptbl_entry(proc_index, pde_index, pte_index, page_index, perm);
}

// Registers the mapping from [vaddr] to physical page # [page_index] in the page directory.
void set_pdir_entry_by_va(unsigned int proc_index, unsigned int vaddr,
                          unsigned int page_index)
{
    unsigned int pde_index = (vaddr & VA_PDE_MASK) >> 22;
    set_pdir_entry(proc_index, pde_index, page_index);
}

// Initializes the identity page table.
// The permission for the kernel memory should be PTE_P, PTE_W, and PTE_G,
// While the permission for the rest should be PTE_P and PTE_W.
void idptbl_init(unsigned int mbi_addr)
{
    unsigned int pde_index, pte_index, page_index;

    container_init(mbi_addr);

    // set kernel's pdir entries to be the identity
    for (pde_index = 0; pde_index < 1024; pde_index++)
    {
        for (pte_index = 0; pte_index < 1024; pte_index++)
        {
            page_index = pde_index * 1024 + pde_index;
            if (page_index < VM_USERLO_PI || page_index >= VM_USERHI_PI)
            {
                set_ptbl_entry_identity(pde_index, pte_index, PTE_P | PTE_W | PTE_G);
            }
            else
            {
                set_ptbl_entry_identity(pde_index, pte_index, PTE_P | PTE_W);
            }
        }
    }
}
