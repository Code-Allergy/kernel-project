#ifndef KERNEL_PAGING_H
#define KERNEL_PAGING_H

#include <kernel/boot.h>

#define PAGE_SIZE 4096

struct page {
    struct page *next;
    uint32_t flags;
    void* paddr;

    // pointer to the page table entry
    uint32_t* pte;
};

struct page_allocator {
    struct page pages[DRAM_SIZE / PAGE_SIZE];
    struct page *kernel_list;
    struct page *free_list;
    uint32_t total_pages;
    uint32_t free_pages;
};


void* alloc_page(struct page_allocator *alloc);

typedef struct page_allocator page_allocator_t;
extern page_allocator_t kpage_allocator;

void init_page_allocator(struct page_allocator *alloc, bootloader_t* bootloader_info);

#endif