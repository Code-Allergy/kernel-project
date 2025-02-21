#include <stddef.h>
#include <stdint.h>
#include <kernel/heap.h>
#include <kernel/paging.h>
#include <kernel/printk.h>
#include <kernel/sched.h>
#include <kernel/paging.h>
#include <kernel/mmu.h>
#include <kernel/string.h>


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
        mmu_driver.map_page(NULL, (void*)addr, page, L2_KERNEL_DATA_PAGE);
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

// returns a 4 byte aligned address in the heap
void* kmalloc(uint32_t size) {
    // Round up size to multiple of 4 bytes
    size = (size + 3) & ~3;

    // Ensure returned address is 4-byte aligned
    void* ptr = simple_block_alloc(size);
    return (void*)(((uintptr_t)ptr + 3) & ~3);
}

void kfree(void* ptr) {
    (void)ptr;
    // Do nothing for now
}

char* strdup(const char* s) {
    size_t len = strlen(s) + 1;
    char* p = kmalloc(len);
    if (p) {
        memcpy(p, s, len);
    }
    p[len] = '\0';
    return p;
}
