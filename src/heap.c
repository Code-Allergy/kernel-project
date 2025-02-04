#include <stddef.h>
#include <stdint.h>
#include <kernel/heap.h>
#include <kernel/paging.h>
#include <kernel/printk.h>
#include <kernel/sched.h>
#include <kernel/paging.h>
#include <kernel/mmu.h>


uint32_t kernel_heap_start = KHEAP_START;
uint32_t kernel_heap_end   = KHEAP_START + KHEAP_SIZE;
uint32_t kernel_heap_curr  = KHEAP_START;


int kernel_heap_init() {
    for (uint32_t i = kernel_heap_start; i < kernel_heap_end; i += PAGE_SIZE) {
        void* page = alloc_page(&kpage_allocator);
        if (!page) {
            printk("Failed to allocate page for kernel heap\n");
            return -1;
        }
        map_page(&kpage_allocator, i, page, MMU_NORMAL_MEMORY | MMU_AP_RW | MMU_CACHEABLE | MMU_SHAREABLE | MMU_TEX_NORMAL);
    }

    return 0;
}

void* simple_block_alloc(uint32_t size) {
    uint32_t new_heap = kernel_heap_curr + size;
    if (new_heap > kernel_heap_end) {
        printk("Out of heap space\n");
        return NULL;
    }

    void* block = (void*)kernel_heap_curr;
    kernel_heap_curr = new_heap;

    return block;
}

void* kmalloc(uint32_t size) {
    simple_block_alloc(size);
}



