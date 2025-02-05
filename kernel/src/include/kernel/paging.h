#ifndef KERNEL_PAGING_H
#define KERNEL_PAGING_H

#include <kernel/boot.h>

#define PAGE_SIZE 4096

struct page {
    struct page *next;
    uint32_t flags;
    void* paddr;
    uint32_t used;

    // pointer to the page table entry
    uint32_t* pte;
};

struct page_allocator {
    struct page pages[DRAM_SIZE / PAGE_SIZE];
    struct page *free_list;
    uint32_t total_pages;
    uint32_t free_pages;
    uint32_t reserved_pages;
};


void* alloc_page(struct page_allocator *alloc);

typedef struct page_allocator page_allocator_t;
extern page_allocator_t kpage_allocator;

void init_page_allocator(struct page_allocator *alloc, bootloader_t* bootloader_info);

#define ALIGN_16KB(addr) ((addr) & ~0x3FFF)  // 16KB alignment

void* alloc_aligned_pages(struct page_allocator *alloc, size_t count);

#endif
