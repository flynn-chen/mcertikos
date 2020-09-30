#include <lib/gcc.h>
#include <lib/x86.h>
#include <lib/debug.h>

#include "import.h"

#define PT_PERM_UP 0
#define PT_PERM_PTU (PTE_P | PTE_W | PTE_U)

/**
 * Page directory pool for NUM_IDS processes.
 * mCertiKOS maintains one page structure for each process.
 * Each PDirPool[index] represents the page directory of the page structure
 * for the process # [index].
 * In mCertiKOS, we statically allocate page directories, and maintain the second
 * level page tables dynamically.
 * The unsigned int * type is meant to suggest that the contents of the array
 * are pointers to page tables. In reality they are actually page directory
 * entries, which are essentially pointers plus permission bits. The functions
 * in this layer will require casting between integers and pointers anyway and
 * in fact any 32-bit type is fine, so feel free to change it if it makes more
 * sense to you with a different type.
 */
unsigned int *PDirPool[NUM_IDS][1024] gcc_aligned(PAGESIZE);

/**
 * In mCertiKOS, we use identity page table mappings for the kernel memory.
 * IDPTbl is an array of statically allocated identity page tables that will be
 * reused for all the kernel memory.
 * That is, in every page directory, the entries that fall into the range of
 * addresses reserved for the kernel will point to an entry in IDPTbl.
 */
unsigned int IDPTbl[1024][1024] gcc_aligned(PAGESIZE);

/**
 * Validate the give process ID to ensure it's < NUM_IDS
 * Return 0 if invalid process index, else return 1
*/

// unsigned int check_proc_idx(unsigned int proc_index)
// {
//     if (proc_index >= NUM_IDS)
//     {
//         return 0;
//     }
//     return 1;
// }
/**
 * Validate the given index to ensure it's < 1024
 * Return 0 if invalid index, else return 1
*/
// unsigned int check_page_idx(unsigned int index)
// {
//     if (index >= 1024)
//     {
//         return 0;
//     }
//     return 1;
// }

void set_pdir_base(unsigned int index)
{
    // if (check_proc_idx(index) == 0)
    // {
    //     return;
    // }
    set_cr3(PDirPool[index]);
}

// Returns the page directory entry # [pde_index] of the process # [proc_index].
// This can be used to test whether the page directory entry is mapped.
unsigned int get_pdir_entry(unsigned int proc_index, unsigned int pde_index)
{
    // if (check_proc_idx(proc_index) == 0 || check_page_idx(pde_index) == 0)
    // {
    //     return 0;
    // }
    return (unsigned int)PDirPool[proc_index][pde_index];
}

// Sets the specified page directory entry with the start address of physical
// page # [page_index].
// You should also set the permissions PTE_P, PTE_W, and PTE_U.
void set_pdir_entry(unsigned int proc_index, unsigned int pde_index,
                    unsigned int page_index)
{
    // if (check_proc_idx(proc_index) == 0 || check_page_idx(pde_index) == 0)
    // {
    //     return;
    // }
    unsigned int new_frame_address = page_index << 12;
    PDirPool[proc_index][pde_index] = (unsigned int *)(new_frame_address | PT_PERM_PTU);
}

// Sets the page directory entry # [pde_index] for the process # [proc_index]
// with the initial address of page directory # [pde_index] in IDPTbl.
// You should also set the permissions PTE_P, PTE_W, and PTE_U.
// This will be used to map a page directory entry to an identity page table.
void set_pdir_entry_identity(unsigned int proc_index, unsigned int pde_index)
{
    // if (check_proc_idx(proc_index) == 0 || check_page_idx(pde_index) == 0)
    // {
    //     return;
    // }
    unsigned int pde_int = (unsigned int)IDPTbl[pde_index];
    pde_int |= PT_PERM_PTU;
    PDirPool[proc_index][pde_index] = (unsigned int *)pde_int;
    // PDirPool[proc_index][pde_index] = (unsigned int *)((unsigned int)(IDPTbl[pde_index]) | PT_PERM_PTU);
}

// Removes the specified page directory entry (sets the page directory entry to 0).
// Don't forget to cast the value to (unsigned int *).
void rmv_pdir_entry(unsigned int proc_index, unsigned int pde_index)
{
    // if (check_proc_idx(proc_index) == 0 || check_page_idx(pde_index) == 0)
    // {
    //     return;
    // }
    PDirPool[proc_index][pde_index] = (unsigned int *)0;
}

// Returns the specified page table entry.
// Do not forget that the permission info is also stored in the page directory entries.
unsigned int get_ptbl_entry(unsigned int proc_index, unsigned int pde_index,
                            unsigned int pte_index)
{
    // if (check_proc_idx(proc_index) == 0 || check_page_idx(pde_index) == 0 || check_page_idx(pte_index) == 0)
    // {
    //     return 0;
    // }
    unsigned int pde_int = (unsigned int)PDirPool[proc_index][pde_index];

    // mask lower 12 bits of the page directory entry
    unsigned int page_frame = pde_int & ~(0x00000fff);
    unsigned int pte_int = ((unsigned int *)page_frame)[pte_index];
    return pte_int;
}

// Sets the specified page table entry with the start address of physical page # [page_index]
// You should also set the given permission.
void set_ptbl_entry(unsigned int proc_index, unsigned int pde_index,
                    unsigned int pte_index, unsigned int page_index,
                    unsigned int perm)
{
    // if (check_proc_idx(proc_index) == 0 || check_page_idx(pde_index) == 0 || check_page_idx(pte_index) == 0)
    // {
    //     return;
    // }
    unsigned int new_frame_address = page_index << 12;
    unsigned int page_frame = ((unsigned int)(PDirPool[proc_index][pde_index])) & ~(0x00000fff);
    ((unsigned int *)page_frame)[pte_index] = new_frame_address | perm;
}

// Sets up the specified page table entry in IDPTbl as the identity map.
// You should also set the given permission.
void set_ptbl_entry_identity(unsigned int pde_index, unsigned int pte_index,
                             unsigned int perm)
{
    /*
    The identity map is supposed to map the virtual addresses
    back to themselves, which can now be interpreted as physical addresses

    IDPTbl[1024][1024] so it can address 2^20 physical addresses (the exact
    number of physical pages in the AT struct)
    
    so for IDPTbl[i][j], the page_index is i * 1024 + j in the AT struct
    */

    // if (check_page_idx(pde_index) == 0 || check_page_idx(pte_index) == 0)
    // {
    //     return;
    // }

    unsigned int page_index = (pde_index * 1024) + pte_index;
    IDPTbl[pde_index][pte_index] = (page_index << 12) | perm;
}

// Sets the specified page table entry to 0.
void rmv_ptbl_entry(unsigned int proc_index, unsigned int pde_index,
                    unsigned int pte_index)
{
    // if (check_proc_idx(proc_index) == 0 || check_page_idx(pde_index) == 0 || check_page_idx(pte_index) == 0)
    // {
    //     return;
    // }
    unsigned int page_frame = ((unsigned int)(PDirPool[proc_index][pde_index])) & ~(0x00000fff);
    ((unsigned int *)page_frame)[pte_index] = 0;
}
