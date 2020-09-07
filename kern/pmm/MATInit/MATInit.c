#include <lib/debug.h>
#include "import.h"

#define PAGESIZE     4096
#define VM_USERLO    0x40000000
#define VM_USERHI    0xF0000000
#define VM_USERLO_PI (VM_USERLO / PAGESIZE)
#define VM_USERHI_PI (VM_USERHI / PAGESIZE)

/**
 * The initialization function for the allocation table AT.
 * It contains two major parts:
 * 1. Calculate the actual physical memory of the machine, and sets the number
 *    of physical pages (NUM_PAGES).
 * 2. Initializes the physical allocation table (AT) implemented in the MATIntro layer
 *    based on the information available in the physical memory map table.
 *    Review import.h in the current directory for the list of available
 *    getter and setter functions.
 */
void pmem_init(unsigned int mbi_addr)
{
    unsigned int nps;

    unsigned int table_size;
    unsigned int highest_addr;
    unsigned int page_start;
    unsigned int page_end;
    unsigned int entry_start;
    unsigned int entry_end;
    unsigned int page_idx;
    unsigned int entry_end_addr;

    // Calls the lower layer initialization primitive.
    // The parameter mbi_addr should not be used in the further code.
    devinit(mbi_addr);

    /**
     * Calculate the total number of physical pages provided by the hardware and
     * store it into the local variable nps.
     * Hint: Think of it as the highest address in the ranges of the memory map table,
     *       divided by the page size.
     */
    table_size = get_size();
    if (table_size == 0) {
        nps = 0;
    }
    
    //the last entry might not have the highest address
    highest_addr = 0;
    for (unsigned int i = 0; i < table_size; i++) {
        entry_end_addr = get_mms(i) + get_mml(i) - 1;
        if (entry_end_addr > highest_addr) {
            highest_addr = entry_end_addr;
        }
    }
    //KERN_DEBUG("highest_addr: %x\n", highest_addr);

    /*
    TBD: This formula of calculating nps might not be right...
    */
    
    nps = highest_addr / PAGESIZE;

    set_nps(nps);  // Setting the value computed above to NUM_PAGES.

    /**
     * Initialization of the physical allocation table (AT).
     *
     * In CertiKOS, all addresses < VM_USERLO or >= VM_USERHI are reserved by the kernel.
     * That corresponds to the physical pages from 0 to VM_USERLO_PI - 1,
     * and from VM_USERHI_PI to NUM_PAGES - 1.
     * The rest of the pages that correspond to addresses [VM_USERLO, VM_USERHI)
     * can be used freely ONLY IF the entire page falls into one of the ranges in
     * the memory map table with the permission marked as usable.
     *
     * Hint:
     * 1. You have to initialize AT for all the page indices from 0 to NPS - 1.
     * 2. For the pages that are reserved by the kernel, simply set its permission to 1.
     *    Recall that the setter at_set_perm also marks the page as unallocated.
     *    Thus, you don't have to call another function to set the allocation flag.
     * 3. For the rest of the pages, explore the memory map table to set its permission
     *    accordingly. The permission should be set to 2 only if there is a range
     *    containing the entire page that is marked as available in the memory map table.
     *    Otherwise, it should be set to 0. Note that the ranges in the memory map are
     *    not aligned by pages, so it may be possible that for some pages, only some of
     *    the addresses are in a usable range. Currently, we do not utilize partial pages,
     *    so in that case, you should consider those pages as unavailable.
     */

    //iterate over all pages
    for (unsigned int i = 0; i < nps; i++) {
        if (i < VM_USERLO_PI || i >= VM_USERHI_PI) {
            at_set_perm(i, 1);
        } else { 
            at_set_perm(i, 0);
        }
    }

    //iterate over each entry in the physical memory table
    for (unsigned int i = 0; i < table_size; i++) {

        //get the length of each physical memory entry
        entry_start = get_mms(i);
        entry_end = entry_start + get_mml(i) - 1;

        //see how many pages can fit in an entry 
        //also be ware since highest_addr + 1 = 0 (overflow for unsigned int) 
        for (unsigned int page_start = entry_start; page_start < entry_end - PAGESIZE; page_start += PAGESIZE) {

            //get page idx and align to beginning of page
            page_idx = page_start / PAGESIZE;
            if (page_start % PAGESIZE > 0) {
                page_idx++;
            }

            //change page_start to start of page
            page_start = page_idx * PAGESIZE;
            page_end = page_start + PAGESIZE - 1;

            //change permission for that page
            if (VM_USERLO_PI <= page_idx && page_idx < VM_USERHI_PI &&
            is_usable(i)) {
                at_set_perm(page_idx, 2);
            }
        }
    }
}
