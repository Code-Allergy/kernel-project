#include <kernel/paging.h>
#include <kernel/boot.h>
#include <kernel/printk.h>

extern uint32_t kernel_end; // Defined in linker script, end of kernel memory space
extern uint32_t kernel_code_end; // Defined in linker script, end of kernel code space, pages should be RO

void init_page_allocator(struct page_allocator *alloc, bootloader_t* bootloader_info) {
    // Compute total pages correctly
    uint32_t dram_start = DRAM_BASE;
    uint32_t dram_size = DRAM_SIZE;
    alloc->total_pages = dram_size / PAGE_SIZE;
    alloc->free_pages = alloc->total_pages - alloc->reserved_pages;
    alloc->free_list = NULL;

    // start at end and loop to front, stopping at reserved pages
    for (uint32_t i = 0; i < alloc->total_pages; i++) {
        // Physical address = DRAM start + page index * PAGE_SIZE
        uint32_t paddr = DRAM_BASE + (i * PAGE_SIZE);
        alloc->pages[i].paddr = (void*)paddr;
        alloc->pages[i].next = alloc->free_list;
        alloc->free_list = &alloc->pages[i];
    }

    // mmu_driver.map


    // Start loop from the first FREE page (after reserved pages)
    // for (uint32_t i = alloc->reserved_pages; i < alloc->total_pages; i++) {
    //     // Physical address = DRAM start + page index * PAGE_SIZE
    //     uint32_t paddr = dram_start + (i * PAGE_SIZE);
    //     alloc->pages[i].paddr = (void*)paddr;
    //     // Add to free list (insert at head for simplicity)
    //     alloc->pages[i].next = alloc->free_list;
    //     alloc->free_list = &alloc->pages[i];
    // }

    // Debug prints
    printk("Total pages: %d (%dKB)\n", alloc->total_pages, alloc->total_pages * PAGE_SIZE / 1024);
    printk("Reserved pages: %d (%dKB)\n", alloc->reserved_pages, alloc->reserved_pages * PAGE_SIZE / 1024);
    printk("Free pages: %d (%dKB)\n", alloc->free_pages, alloc->free_pages * PAGE_SIZE / 1024);
}
