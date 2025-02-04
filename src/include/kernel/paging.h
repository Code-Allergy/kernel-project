#ifndef KERNEL_PAGING_H
#define KERNEL_PAGING_H

struct page {
    struct page *next;
    uint32_t flags;
    void* paddr;
};

struct page_allocator {
    struct page pages[4096];
    struct page *kernel_list;
    struct page *free_list;
    uint32_t total_pages;
    uint32_t free_pages;
};

void init_page_allocator(struct page_allocator *alloc, bootloader_t* bootloader_info);
void* alloc_page(struct page_allocator *alloc);

typedef struct page_allocator page_allocator_t;
extern page_allocator_t kpage_allocator;

#endif