#include <lib/x86.h>
#include <lib/string.h>
#include <lib/debug.h>
#include <kern/thread/PCurID/export.h>
#include <kern/lib/pmap.h>

#include "import.h"

#define PDE_ADDR(x) (x >> 22)
#define PTE_ADDR(x) ((x >> 12) & 0x3ff)

#define PAGESIZE 4096
#define PDIRSIZE (PAGESIZE * 1024)
#define VM_USERLO 0x40000000
#define VM_USERHI 0xF0000000
#define VM_USERLO_PDE (VM_USERLO / PDIRSIZE)
#define VM_USERHI_PDE (VM_USERHI / PDIRSIZE)

/**
 * Returns the page table entry corresponding to the virtual address,
 * according to the page structure of process # [proc_index].
 * Returns 0 if the mapping does not exist.
 */
unsigned int get_ptbl_entry_by_va(unsigned int proc_index, unsigned int vaddr)
{
    unsigned int pde_index = PDE_ADDR(vaddr);
    if (get_pdir_entry(proc_index, pde_index) != 0)
    {
        return get_ptbl_entry(proc_index, pde_index, PTE_ADDR(vaddr));
    }
    else
    {
        return 0;
    }
}

// Returns the page directory entry corresponding to the given virtual address.
unsigned int get_pdir_entry_by_va(unsigned int proc_index, unsigned int vaddr)
{
    return get_pdir_entry(proc_index, PDE_ADDR(vaddr));
}

// Removes the page table entry for the given virtual address.
void rmv_ptbl_entry_by_va(unsigned int proc_index, unsigned int vaddr)
{
    unsigned int pde_index = PDE_ADDR(vaddr);
    if (get_pdir_entry(proc_index, pde_index) != 0)
    {
        rmv_ptbl_entry(proc_index, pde_index, PTE_ADDR(vaddr));
    }
}

// Removes the page directory entry for the given virtual address.
void rmv_pdir_entry_by_va(unsigned int proc_index, unsigned int vaddr)
{
    rmv_pdir_entry(proc_index, PDE_ADDR(vaddr));
}

// Maps the virtual address [vaddr] to the physical page # [page_index] with permission [perm].
// You do not need to worry about the page directory entry. just map the page table entry.
void set_ptbl_entry_by_va(unsigned int proc_index, unsigned int vaddr,
                          unsigned int page_index, unsigned int perm)
{
    set_ptbl_entry(proc_index, PDE_ADDR(vaddr), PTE_ADDR(vaddr), page_index, perm);
}

// Registers the mapping from [vaddr] to physical page # [page_index] in the page directory.
void set_pdir_entry_by_va(unsigned int proc_index, unsigned int vaddr,
                          unsigned int page_index)
{
    set_pdir_entry(proc_index, PDE_ADDR(vaddr), page_index);
}

// Initializes the identity page table.
// The permission for the kernel memory should be PTE_P, PTE_W, and PTE_G,
// While the permission for the rest should be PTE_P and PTE_W.
void idptbl_init(unsigned int mbi_addr)
{
    unsigned int pde_index, pte_index, perm;
    container_init(mbi_addr);

    // Set up IDPTbl
    for (pde_index = 0; pde_index < 1024; pde_index++)
    {
        if ((pde_index < VM_USERLO_PDE) || (VM_USERHI_PDE <= pde_index))
        {
            // kernel mapping
            perm = PTE_P | PTE_W | PTE_G;
        }
        else
        {
            // normal memory
            perm = PTE_P | PTE_W;
        }

        for (pte_index = 0; pte_index < 1024; pte_index++)
        {
            set_ptbl_entry_identity(pde_index, pte_index, perm);
        }
    }
}

/*
    addr_arr = [0x4000, 0x135432]
    byte_arr = [0xcd, 0xt5]

    byte_arr[x] = the original first byte of addr_arr[x]

    revalidate y: search addr_arr to find y (at index i) restore byte_arr[i]
*/
#define MAX_BREAKPOINT 100
unsigned int addr_arr[NUM_IDS][MAX_BREAKPOINT];
char byte_arr[NUM_IDS][MAX_BREAKPOINT];
unsigned int used_arr[NUM_IDS][MAX_BREAKPOINT];
unsigned int breakpoint_number = 0;
/*
    add a breakpoint to the given virtual address

    Return 1 (true) if successfully added
    Otherwise return 0 (false)
*/
unsigned int add_breakpoint(unsigned int proc_index, unsigned int vaddr)
{
    unsigned int brk_idx = breakpoint_number;
    // verify this location doesn't already have a breakpoint
    for (unsigned int i = 0; i < breakpoint_number; i++)
    {
        if (addr_arr[proc_index][i] == vaddr)
        {
            if (used_arr[proc_index][i] == 1)
            {
                return 1; // breakpoint already exists, do nothing
            }
            else
            {
                brk_idx = i;
            }
        }
    }


    char old_byte;
    // copy in the old byte
    pt_copyin(proc_index, vaddr, (void *)&old_byte, 1);
    // overwrite it with int 3 op code
    pt_memset(proc_index, vaddr, (char) 0xcc, 1);

    // save the old_byte to the vaddr
    addr_arr[proc_index][brk_idx] = vaddr;
    byte_arr[proc_index][brk_idx] = old_byte;
    used_arr[proc_index][brk_idx] = 1;
    if (brk_idx == breakpoint_number)
    {
        breakpoint_number += 1;
    }
    // KERN_DEBUG("added breakpoint to 0x%08x. replaced %d\n", vaddr, old_byte);
    return 1;
}

/*
    remove breakpoint from the given virtual address

    the given address MUST have previously had a breakpoint added for it to work

    Return 1 (true) if successfully removed
    Otherwise return 0 (false)
*/
unsigned int remove_breakpoint(unsigned int proc_index, unsigned int vaddr)
{
    char original_instruction;
    unsigned int found = 0;
    unsigned int brk_idx;
    // search for the entry corresponding to this vaddr
    for (brk_idx = 0; brk_idx < breakpoint_number; brk_idx++)
    {   
        if (addr_arr[proc_index][brk_idx] == vaddr && used_arr[proc_index][brk_idx] == 1)
        {
            found = 1;
            original_instruction = byte_arr[proc_index][brk_idx];
            break;
        }
    }

    if (found == 0)
    {
        KERN_PANIC("couldn't find original instruction for address 0x%08x\n", vaddr);
    }

    // rewrite the original instruction
    pt_memset(proc_index, vaddr, original_instruction, 1);
    // mark this entry as unused (so it can be added to later)
    used_arr[proc_index][brk_idx] = 0;
    return 1;
}

void remove_all_breakpoints(unsigned int pid)
{
    for (unsigned int i = 0; i < breakpoint_number; i++)
    {
        if(used_arr[pid][i] == 1)
        {
            remove_breakpoint(pid, addr_arr[pid][i]);
        }
    }
}