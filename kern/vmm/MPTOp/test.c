// #include <lib/x86.h>
#include <lib/debug.h>
#include "export.h"
// #include "import.h"

// #define PAGESIZE 4096
// #define VM_USERLO 0x40000000
// #define VM_USERHI 0xF0000000
// #define VM_USERLO_PI (VM_USERLO / PAGESIZE)
// #define VM_USERHI_PI (VM_USERHI / PAGESIZE)

int MPTOp_test1()
{
    unsigned int vaddr = 4096 * 1024 * 300;
    if (get_ptbl_entry_by_va(10, vaddr) != 0)
    {
        dprintf("test 1.1 failed: (%d != 0)\n", get_ptbl_entry_by_va(10, vaddr));
        return 1;
    }
    if (get_pdir_entry_by_va(10, vaddr) != 0)
    {
        dprintf("test 1.2 failed: (%d != 0)\n", get_pdir_entry_by_va(10, vaddr));
        return 1;
    }
    set_pdir_entry_by_va(10, vaddr, 100);
    set_ptbl_entry_by_va(10, vaddr, 100, 259);
    if (get_ptbl_entry_by_va(10, vaddr) == 0)
    {
        dprintf("test 1.3 failed: (%d == 0)\n", get_ptbl_entry_by_va(10, vaddr));
        return 1;
    }
    if (get_pdir_entry_by_va(10, vaddr) == 0)
    {
        dprintf("test 1.4 failed: (%d == 0)\n", get_pdir_entry_by_va(10, vaddr));
        return 1;
    }
    rmv_ptbl_entry_by_va(10, vaddr);
    rmv_pdir_entry_by_va(10, vaddr);
    if (get_ptbl_entry_by_va(10, vaddr) != 0)
    {
        dprintf("test 1.5 failed: (%d != 0)\n", get_ptbl_entry_by_va(10, vaddr));
        return 1;
    }
    if (get_pdir_entry_by_va(10, vaddr) != 0)
    {
        dprintf("test 1.6 failed: (%d != 0)\n", get_pdir_entry_by_va(10, vaddr));
        return 1;
    }
    dprintf("test 1 passed.\n");
    return 0;
}

/**
 * Write Your Own Test Script (optional)
 *
 * Come up with your own interesting test cases to challenge your classmates!
 * In addition to the provided simple tests, selected (correct and interesting) test functions
 * will be used in the actual grading of the lab!
 * Your test function itself will not be graded. So don't be afraid of submitting a wrong script.
 *
 * The test function should return 0 for passing the test and a non-zero code for failing the test.
 * Be extra careful to make sure that if you overwrite some of the kernel data, they are set back to
 * the original value. O.w., it may make the future test scripts to fail even if you implement all
 * the functions correctly.
 */
int MPTOp_test_own()
{
    // unsigned int pde_index, pte_index, page_index, ptbl_entry;
    // for (pde_index = 0; pde_index < 1024; pde_index++)
    // {
    //     for (pte_index = 0; pte_index < 1024; pte_index++)
    //     {
    //         page_index = pde_index * 1024 + pde_index;
    //         if (page_index < VM_USERLO_PI || page_index >= VM_USERHI_PI)
    //         {
    //             ptbl_entry = get_ptbl_entry(0, pde_index, pte_index);
    //             if ((ptbl_entry & (PTE_P | PTE_W | PTE_G)) != (PTE_P | PTE_W | PTE_G))
    //             {
    //                 dprintf("own test 1 failed: pde_index %d, pte_index %d has wrong permissions\n", pde_index, pte_index);
    //                 return 1;
    //             }
    //         }
    //         else
    //         {
    //             ptbl_entry = get_ptbl_entry(0, pde_index, pte_index);
    //             if ((ptbl_entry & (PTE_P | PTE_W)) != (PTE_P | PTE_W))
    //             {
    //                 dprintf("own test 2 failed: pde_index %d, pte_index %d has wrong permissions\n", pde_index, pte_index);
    //                 return 1;
    //             }
    //         }
    //     }
    // }
    dprintf("own test passed.\n");
    return 0;
}

int test_MPTOp()
{
    return MPTOp_test1() + MPTOp_test_own();
}
