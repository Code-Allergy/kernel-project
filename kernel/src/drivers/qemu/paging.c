#include <kernel/boot.h>
#include <kernel/printk.h>
// #include <kernel/sched.h>
#include <kernel/paging.h>
#include <kernel/mmu.h>
#include <kernel/string.h>

// this should be src/paging.c, needs to be refactored again

extern uint32_t kernel_code_end; // Defined in linker script, end of kernel code space, pages should be RO
extern uint32_t kernel_end; // Defined in linker script, end of kernel memory space
#define MB_ALIGN_DOWN(addr) ((addr) & ~0xFFFFF)

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
