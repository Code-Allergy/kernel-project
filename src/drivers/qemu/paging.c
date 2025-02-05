#include <kernel/boot.h>
#include <kernel/printk.h>
#include <kernel/sched.h>
#include <kernel/paging.h>
#include <kernel/mmu.h>

#define MB_ALIGN_DOWN(addr) ((addr) & ~0xFFFFF)

page_allocator_t kpage_allocator;

#define PHYS_BASE 0x40000000
#define PAGING_START (PHYS_BASE + 0x250000)
#define DDR_SIZE 0x20000000 // 512MB
#define MEMORY_SIZE (DDR_SIZE - 0x250000)

void init_page_allocator(struct page_allocator *alloc, bootloader_t* bootloader_info) {
    printk("Memory size: %d\n", MEMORY_SIZE);
    alloc->total_pages = MEMORY_SIZE / PAGE_SIZE;
    alloc->free_pages = alloc->total_pages;
    alloc->free_list = NULL;

    printk("Total pages: %d\n", alloc->total_pages);

    for (uint32_t i = alloc->total_pages; i > 0 ; i--) {
        uint32_t paddr = PHYS_BASE + i * PAGE_SIZE;
        alloc->pages[i].paddr = (void*)paddr;
        // Verify natural alignment
        if((paddr & (PAGE_SIZE-1)) != 0) 
            panic("Misaligned physical page");
        alloc->pages[i].next = alloc->free_list;
        alloc->free_list = &alloc->pages[i];
    }

    printk("Free pages: %d\n", alloc->free_pages);
    printk("Free list: %p\n", alloc->free_list);
    printk("First page: %p\n", alloc->free_list->paddr);
    printk("First page next: %p\n", alloc->free_list->next);
    printk("First page next paddr: %p\n", alloc->free_list->next->paddr);


}

void* alloc_page(struct page_allocator *alloc) {
    if (!alloc->free_list)  {
        printk("Allocator was null!\n");
        return NULL;
    }
    
    struct page *desc = alloc->free_list;
    alloc->free_list = desc->next;
    alloc->free_pages--;
    
    // Clear the page before returning
    memset(desc->paddr, 0, PAGE_SIZE);
    return desc->paddr;
}

void* alloc_aligned_pages(struct page_allocator *alloc, size_t count) {
    struct page *prev = NULL;
    struct page *current = alloc->free_list;
    struct page *start_block = NULL;
    uint32_t found = 0;

    // Find contiguous 16KB-aligned block (4 pages)
    while(current) {
        uint32_t paddr = (uint32_t)current->paddr;
        
        // Check alignment and continuity
        if((paddr & 0x3FFF) == 0) {  // 16KB aligned
            printk("Checking page: %p\n", current->paddr);
            struct page *check = current;
            uint32_t valid = 1;
            
            // Verify next 3 pages are contiguous
            for(int i=0; i<count-1; i++) {
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
    for(int i=0; i<count; i++) {
        p->used = 1;
        p = p->next;
    }

    alloc->free_pages -= count;
    return start_block->paddr;
}

void free_page(struct page_allocator *alloc, void *ptr) {
    struct page *page = (struct page*)ptr;
    page->next = alloc->free_list;
    alloc->free_list = page;
    alloc->free_pages++;
}

int copy_from_user(void *dst, void *src, uint32_t size) {
    // for now, just works like memcpy, but we can add checks later
    return memcpy(dst, src, size);
}

int copy_to_user(void *dst, void *src, uint32_t size) {
    // for now, just works like memcpy, but we can add checks later
    return memcpy(dst, src, size);
}