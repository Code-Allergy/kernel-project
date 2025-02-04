#include <kernel/boot.h>
#include <kernel/printk.h>
#include <kernel/sched.h>
#include <kernel/paging.h>
#include <kernel/mmu.h>

#define MB_ALIGN_DOWN(addr) ((addr) & ~0xFFFFF)

page_allocator_t kpage_allocator;

#define PAGING_START 0x40250000
#define DDR_SIZE 0x20000000 // 512MB
#define MEMORY_SIZE (DDR_SIZE - 0x250000)

void init_page_allocator(struct page_allocator *alloc, bootloader_t* bootloader_info) {
    printk("Memory size: %d\n", MEMORY_SIZE);
    alloc->total_pages = MEMORY_SIZE / PAGE_SIZE;
    alloc->free_pages = alloc->total_pages;
    alloc->free_list = NULL;

    printk("Total pages: %d\n", alloc->total_pages);

    for (uint32_t i = 0; i < alloc->total_pages; i++) {
        alloc->pages[i].paddr = (void*)(PAGING_START + i * PAGE_SIZE);
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

void free_page(struct page_allocator *alloc, void *ptr) {
    struct page *page = (struct page*)ptr;
    page->next = alloc->free_list;
    alloc->free_list = page;
    alloc->free_pages++;
}