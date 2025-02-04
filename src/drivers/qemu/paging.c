#include <kernel/boot.h>
#include <kernel/printk.h>
#include <kernel/sched.h>
#include <kernel/paging.h>

#include "mmu.h"

#define MB_ALIGN_DOWN(addr) ((addr) & ~0xFFFFF)

page_allocator_t kpage_allocator;

void init_page_allocator(struct page_allocator *alloc, bootloader_t* bootloader_info) {
    uint32_t kernel_pages = bootloader_info->kernel_size / PAGE_SIZE + 1;
    
    alloc->total_pages = bootloader_info->total_memory / PAGE_SIZE;
    alloc->free_pages = alloc->total_pages - bootloader_info->reserved_memory / PAGE_SIZE; // kernel pages are not free.
    alloc->free_list = NULL;

    printk("Kernel pages: %d\n", kernel_pages);
    printk("Total pages: %d\n", alloc->total_pages);

    for (uint32_t i = 0; i < alloc->total_pages; i++) {
        alloc->pages[i].paddr = (void*)(bootloader_info->kernel_entry + i * PAGE_SIZE);
        
        if (i < kernel_pages) {
            // Add to kernel list
            alloc->pages[i].next = alloc->kernel_list;
            alloc->kernel_list = &alloc->pages[i];
        } else {
            // Add to free list
            printk("Adding page %d to free list\n", i);
            alloc->pages[i].next = alloc->free_list;
            alloc->free_list = &alloc->pages[i];
        }
    }
}

void* alloc_page(struct page_allocator *alloc) {
    if (!alloc->free_list) return NULL;
    
    struct page *desc = alloc->free_list;
    alloc->free_list = desc->next;
    alloc->free_pages--;
    
    // Clear the page before returning
    memset(desc->paddr, 0, PAGE_SIZE);
    return desc->paddr;
}

void free_page(struct page_allocator *alloc, void *ptr) {
    struct page *page = (struct page*)ptr;
    page->next = alloc->free_list;
    alloc->free_list = page;
    alloc->free_pages++;
}