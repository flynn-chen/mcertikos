#include <lib/x86.h>
#include <lib/string.h>
#include <lib/debug.h>
#include <kern/thread/PCurID/export.h>

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
unsigned int breakpoint_number;
/*
    Invalidate the given virtual address by removing its present bit
    and setting the special PTE_BRK bit.

    Return 1 (true) if successfully invalidated.
    Otherwise return 0 (false)
*/
unsigned int invalidate_address(unsigned int proc_index, unsigned int vaddr)
{
    unsigned int ptbl_entry = get_ptbl_entry_by_va(proc_index, vaddr);
    unsigned int frame_addr, instruction_addr;
    char original_instruction;
    unsigned int offset = vaddr & 0xfff;
    char debug[16];
    if (ptbl_entry & PTE_P)
    {
        // get the physical address that we want to insert the debug int instruction
        frame_addr = ptbl_entry & 0xfffff000;
        instruction_addr = frame_addr + offset;

        // record the byte at that address
        memcpy((void *)&original_instruction, (void *)instruction_addr, 1);
        memcpy((void *)debug, (void *)instruction_addr, 16);
        KERN_DEBUG("before invalidation: \n");
        for (unsigned int i = 0; i < 16; i++)
        {
            KERN_DEBUG("\t%d\n", debug[i]);
        }
        // original_instruction = *(char *)instruction_addr;
        addr_arr[get_curid()][breakpoint_number] = vaddr;
        byte_arr[get_curid()][breakpoint_number] = original_instruction;
        breakpoint_number += 1;

        // overwrite first byte with int 3
        // add the int 3 opcode to the instruction
        // data_with_int = (data & 0xffffff00) | 0xcc;
        KERN_DEBUG("writing to 0x%08x (frame addr = 0x%08x, offset = 0x%08x)\n", instruction_addr, frame_addr, offset);
        KERN_DEBUG("overwrote 0x%08x == %c == %d\n", original_instruction, original_instruction, original_instruction);
        memset((void *)instruction_addr, 0xcc, 1);

        memcpy((void *)debug, (void *)instruction_addr, 16);
        KERN_DEBUG("after invalidation: \n");
        for (unsigned int i = 0; i < 16; i++)
        {
            KERN_DEBUG("\t%d\n", debug[i]);
        }

        return 1;
    }
    else
    {
        return 0;
    }
}

/*
    Re-validate the given virtual address by removing the PTE_BRK bit
    and setting the present bit.

    The given address MUST have previously been invalidated for this to work.

    Return 1 (true) if successfully validated.
    Otherwise return 0 (false)
*/
unsigned int validate_address(unsigned int proc_index, unsigned int vaddr)
{
    unsigned int ptbl_entry = get_ptbl_entry_by_va(proc_index, vaddr);
    unsigned int perm = ptbl_entry & (0xfff);
    unsigned int data_with_int, frame_addr, instruction_addr;
    char original_instruction = '\0';
    unsigned int offset = vaddr & 0xfff;

    for (unsigned int i = breakpoint_number - 1; i >= 0; i--)
    {
        if (addr_arr[get_curid()][i] == vaddr)
        {
            KERN_DEBUG("original breakpoint found\n");
            original_instruction = byte_arr[get_curid()][i];
            break;
        }
    }
    if (original_instruction == '\0')
    {
        KERN_PANIC("couldn't find original instruction for address 0x%08x\n", vaddr);
    }
    // get the physical address that we want to insert the debug int instruction
    frame_addr = ptbl_entry & 0xfffff000;
    instruction_addr = frame_addr + offset;
    KERN_DEBUG("removing breakpoint at va: 0x%08x (pa: 0x%08x) by rewriting byte %d\n", vaddr, instruction_addr, original_instruction);

    char debug[16];

    memcpy((void *)debug, (void *)instruction_addr, 16);

    KERN_DEBUG("before re-validation: \n");
    for (unsigned int i = 0; i < 16; i++)
    {
        KERN_DEBUG("\t%d\n", debug[i]);
    }

    memset((void *)instruction_addr, (int)original_instruction, 1);

    memcpy((void *)debug, (void *)instruction_addr, 16);
    KERN_DEBUG("after re-validation: \n");
    for (unsigned int i = 0; i < 16; i++)
    {
        KERN_DEBUG("\t%d\n", debug[i]);
    }

    return 1;
}