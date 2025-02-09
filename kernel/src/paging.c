#include <kernel/sched.h>
#include <kernel/paging.h>
#include <kernel/boot.h>
#include <kernel/printk.h>

extern uint32_t kernel_end; // Defined in linker script, end of kernel memory space
extern uint32_t kernel_code_end; // Defined in linker script, end of kernel code space, pages should be RO

page_allocator_t kpage_allocator;
#define MB_ALIGN_DOWN(addr) ((addr) & ~0xFFFFF)

void init_page_allocator(struct page_allocator *alloc) {
    // start at end of kernel code space
    uint32_t kernel_end_phys = ((uint32_t)&kernel_end - KERNEL_ENTRY) + DRAM_BASE;

    // Compute total pages correctly
    uint32_t dram_start = DRAM_BASE;
    uint32_t dram_size = DRAM_SIZE;
    alloc->total_pages = dram_size / PAGE_SIZE;
    alloc->reserved_pages = ((kernel_end_phys - dram_start) / PAGE_SIZE + 1);
    alloc->free_pages = alloc->total_pages - alloc->reserved_pages;
    alloc->free_list = NULL;

    // Start loop from the last and work to the front
    for (uint32_t i = alloc->total_pages-1; i >= alloc->reserved_pages; i--) {
        // Physical address = DRAM start + page index * PAGE_SIZE
        uint32_t paddr = dram_start + (i * PAGE_SIZE);
        alloc->pages[i].paddr = (void*)paddr;
        alloc->pages[i].next = alloc->free_list;
        alloc->free_list = &alloc->pages[i];
    }

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

    while(current) {
        uint32_t paddr = (uint32_t)current->paddr;

        if((paddr & 0x3FFF) == 0) {  // 16KB aligned
            struct page *check = current;
            uint32_t valid = 1;
            struct page *last_checked = current;

            for(size_t i=0; i<count-1; i++) {
                if(!check->next ||
                   (uint32_t)check->next->paddr != (uint32_t)check->paddr + PAGE_SIZE) {
                    valid = 0;
                    break;
                }
                last_checked = check->next;
                check = check->next;
            }

            if(valid) {
                start_block = current;
                // Update the free list to skip all pages we're allocating
                if(prev) prev->next = last_checked->next;
                else     alloc->free_list = last_checked->next;
                last_checked->next = NULL; // Mark end of allocated block
                break;
            }
        }

        prev = current;
        current = current->next;
    }

    if(!start_block) {
        return NULL;
    }

    alloc->free_pages -= count;
    return start_block->paddr;
}

void free_page(struct page_allocator *alloc, void *ptr) {
    if (!ptr) return;

    uint32_t page_index = (((uint32_t)ptr) - DRAM_BASE) / PAGE_SIZE;

    // Validate the page index
    if (page_index < alloc->reserved_pages || page_index >= alloc->total_pages) {
        printk("Invalid page free attempt: %p\n", ptr);
        return;
    }

    struct page *page = &alloc->pages[page_index];

    // Verify this is a valid allocated page
    if (page->paddr != ptr) {
        printk("Corrupted page free attempt: %p\n", ptr);
        return;
    }

    page->next = alloc->free_list;
    alloc->free_list = page;
    alloc->free_pages++;
}

void free_aligned_pages(struct page_allocator *alloc, void *ptr, size_t count) {
    if (!ptr) return;

    uint32_t page_index = (((uint32_t)ptr) - DRAM_BASE) / PAGE_SIZE;

    // Validate the page index
    if (page_index < alloc->reserved_pages || page_index >= alloc->total_pages) {
        printk("Invalid aligned page free attempt: %p\n", ptr);
        return;
    }

    struct page *start = &alloc->pages[page_index];

    // Verify these are valid allocated pages
    for (size_t i = 0; i < count; i++) {
        if (alloc->pages[page_index + i].paddr != ptr + (i * PAGE_SIZE)) {
            printk("Corrupted aligned page free attempt: %p\n", ptr);
            return;
        }
    }

    // Link the pages together
    for (size_t i = 0; i < count - 1; i++) {
        alloc->pages[page_index + i].next = &alloc->pages[page_index + i + 1];
    }

    // Add to free list
    alloc->pages[page_index + count - 1].next = alloc->free_list;
    alloc->free_list = start;
    alloc->free_pages += count;
}
