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


int kernel_heap_init(void) {
    for (uint32_t addr = kernel_heap_start; addr < kernel_heap_end; addr += PAGE_SIZE) {
        void* page = alloc_page(&kpage_allocator);
        if (!page) {
            printk("Failed to allocate page for kernel heap\n");
            return -1;
        }
        l2_tables[SECTION_INDEX(addr)][PAGE_INDEX(addr)] = (uint32_t)page | L2_KERNEL_DATA_PAGE;
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
    return simple_block_alloc(size);
}

void kfree(void* ptr) {
    (void)ptr;
    // Do nothing for now
}
