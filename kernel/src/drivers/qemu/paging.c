#include <kernel/boot.h>
#include <kernel/printk.h>
#include <kernel/sched.h>
#include <kernel/paging.h>
#include <kernel/mmu.h>
#include <kernel/boot.h>
#include <kernel/string.h>

extern uint32_t kernel_code_end; // Defined in linker script, end of kernel code space, pages should be RO
extern uint32_t kernel_end; // Defined in linker script, end of kernel memory space
#define MB_ALIGN_DOWN(addr) ((addr) & ~0xFFFFF)

page_allocator_t kpage_allocator;

// void init_page_allocator(struct page_allocator *alloc, bootloader_t* bootloader_info) {
//     alloc->total_pages = DRAM_SIZE / PAGE_SIZE;
//     alloc->reserved_pages = ((uint32_t)&kernel_end - DRAM_BASE) / PAGE_SIZE + 1;
//     printk("Reserved by kernel: %d\n", alloc->reserved_pages);
//     printk("Kernel end: %p end: %p\n", &kernel_end, DRAM_BASE);
//     alloc->free_pages = alloc->total_pages - alloc->reserved_pages;
//     alloc->free_list = NULL;


//     // Map all pages after reserved kernel pages to the free list
//     for (uint32_t i = alloc->total_pages; i >= alloc->reserved_pages; i--) {
//         uint32_t paddr = (uint32_t)&kernel_end + i * PAGE_SIZE;
//         printk("Mapping reserve paddr at %p\n", paddr);
//         alloc->pages[i].paddr = (void*)paddr;
//         alloc->pages[i].next = alloc->free_list;
//         alloc->free_list = &alloc->pages[i];
//     }

//     // printk("Total pages: %d (%dKB) %d\n", alloc->total_pages, alloc->free_pages, PAGE_SIZE);
//     printk("Free pages: %d (%dKB)\n", alloc->free_pages, alloc->free_pages * PAGE_SIZE);
//     printk("Reserved pages: %d (%dKB)\n", alloc->reserved_pages, alloc->reserved_pages * PAGE_SIZE / 1024);
//     printk("Free list: %p\n", alloc->free_list);
//     printk("First page: %p\n", alloc->free_list->paddr);
//     printk("First page next: %p\n", alloc->free_list->next);
//     printk("First page next paddr: %p\n", alloc->free_list->next->paddr);
// }
//

void init_page_allocator(struct page_allocator *alloc, bootloader_t* bootloader_info) {
    // Compute total pages correctly
    uint32_t dram_start = DRAM_BASE;
    uint32_t dram_size = DRAM_SIZE;
    alloc->total_pages = dram_size / PAGE_SIZE;

    // Compute reserved pages: kernel occupies from DRAM_BASE to kernel_end
    uint32_t kernel_size = (uint32_t)&kernel_end - dram_start;
    alloc->reserved_pages = (kernel_size + PAGE_SIZE - 1) / PAGE_SIZE; // Round up

    alloc->free_pages = alloc->total_pages - alloc->reserved_pages;
    alloc->free_list = NULL;

    // start at end and loop to front, stopping at reserved pages
    for (uint32_t i = alloc->total_pages - 1; i >= alloc->reserved_pages; i--) {
        // Physical address = DRAM start + page index * PAGE_SIZE
        uint32_t paddr = DRAM_BASE + (i * PAGE_SIZE);
        alloc->pages[i].paddr = (void*)paddr;
        alloc->pages[i].next = alloc->free_list;
        alloc->free_list = &alloc->pages[i];
    }


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

void* alloc_page(struct page_allocator *alloc) {
    if (!alloc->free_list)  {
        printk("Allocator was null!\n");
        return NULL;
    }

    struct page *desc = alloc->free_list;
    alloc->free_list = desc->next;
    alloc->free_pages--;

    return desc->paddr;
}

void* alloc_aligned_pages(struct page_allocator *alloc, size_t count) {
    struct page *prev = NULL;
    struct page *current = alloc->free_list;
    struct page *start_block = NULL;

    // Find contiguous 16KB-aligned block (4 pages)
    while(current) {
        uint32_t paddr = (uint32_t)current->paddr;

        // Check alignment and continuity
        if((paddr & 0x3FFF) == 0) {  // 16KB aligned
            printk("Checking page: %p\n", current->paddr);
            struct page *check = current;
            uint32_t valid = 1;

            // Verify next 3 pages are contiguous
            for(size_t i=0; i<count-1; i++) {
                if(!check->next ||
                   (uint32_t)check->next->paddr != (uint32_t)check->paddr + PAGE_SIZE) {
                    valid = 0;
                    break;
                }
                check = check->next;
            }

            if(valid) {
                start_block = current;
                break;
            }
        }

        prev = current;
        current = current->next;
    }

    if(!start_block) {
        printk("Failed to find %d contiguous aligned pages\n", count);
        return NULL;
    }

    // Remove block from free list
    if(prev) {
        prev->next = start_block->next;
    } else {
        alloc->free_list = start_block->next;
    }

    // Mark pages as used
    struct page *p = start_block;
    for(size_t i=0; i < count; i++) {
        p->used = 1;
        p = p->next;
    }

    alloc->free_pages -= count;
    printk("Returning end: %p\n", start_block->paddr);
    return start_block->paddr;
}

void free_page(struct page_allocator *alloc, void *ptr) {
    struct page *page = (struct page*)ptr;
    page->next = alloc->free_list;
    alloc->free_list = page;
    alloc->free_pages++;
}
