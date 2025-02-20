// universal functions for bbb/qemu
#include "kernel/panic.h"
#include <stdint.h>
#include <kernel/mmu.h>
#include <kernel/mm.h>

#include <kernel/printk.h>
#include <kernel/boot.h>
#include <kernel/paging.h>
#include <kernel/string.h>




uint32_t read_dacr(void) {
    uint32_t dacr;
    __asm__ volatile("mrc p15, 0, %0, c3, c0, 0" : "=r"(dacr));
    return dacr;
}

uint32_t alloc_l1_table(struct page_allocator *alloc) {
    // Allocate 4 contiguous 4KB pages (16KB total)
    void *addr = alloc_aligned_pages(alloc, 4);
    if(!addr) return 0;

    // Initialize L1 table
    memset(PHYS_TO_KERNEL_VIRT(addr), 0, 16*1024);
    return (uint32_t)addr;
}

void mmu_set_domains(void) {
    // Set domain 0 (kernel) to manager access (0x3)
    uint32_t dacr = 0x3;  // Just set domain 0 to manager (0x3)
    __asm__ volatile("mcr p15, 0, %0, c3, c0, 0" : : "r"(dacr));
}


int mmu_init_l1_page_table(void) {
    /* Verify page table alignment */
    if (((uintptr_t)l1_page_table & 0x3FFF) != 0) {
        printk("Page table not 16KB aligned\n");
        return -1;
    }

    if (((uintptr_t)l2_tables & 0x3FF) != 0) {
        printk("L2 tables not 1KB aligned\n");
        return -1;
    }
    // map all l1 entries to l2 tables
    for (uint32_t section = 0; section < 4096; section++) {
        uint32_t l2_table = ((uint32_t)&l2_tables[section] > KERNEL_ENTRY) ?
                    DRAM_BASE + ((uint32_t)&l2_tables[section] - KERNEL_ENTRY) :
                            (uint32_t)&l2_tables[section];

        l1_page_table[section] = l2_table | MMU_PAGE_DESCRIPTOR | (MMU_DOMAIN_KERNEL << 5);
    }

    return 0;
}

void mmu_enable(void) {
    // Load TTBR1 with the physical address of the L1 table
    mmu_driver.ttbr0 = l1_page_table;
    __asm__ volatile(
        "mcr p15, 0, %0, c2, c0, 0 \n" // TTBR0
        "dsb \n"
        "isb \n"
        : : "r"(mmu_driver.ttbr0)
    );

    // Invalidate TLB and caches
    __asm__ volatile("mcr p15, 0, %0, c8, c7, 0" : : "r"(0)); // Invalidate TLB
    __asm__ volatile("mcr p15, 0, %0, c7, c5, 0" : : "r"(0)); // Invalidate I-cache
    __asm__ volatile("mcr p15, 0, %0, c7, c6, 0" : : "r"(0)); // Invalidate data cache
    __asm__ volatile("mcr p15, 0, %0, c7, c10, 0" : : "r"(0)); // Clean data cache
    __asm__ volatile("mcr p15, 0, %0, c7, c10, 4" : : "r"(0)); // Drain write buffer

    // Enable MMU, caches, and branch prediction
    uint32_t sctlr;
    __asm__ volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(sctlr));
    sctlr |= (1 << 0)  // Enable MMU
           | (1 << 2)  // Enable D-cache
           | (1 << 12) // Enable I-cache
           | (1 << 11) // Enable branch prediction
           ;
    __asm__ volatile("mcr p15, 0, %0, c1, c0, 0" : : "r"(sctlr));
}

#ifdef BOOTLOADER
void map_page(void *ttbr0, void* vaddr, void* paddr, uint32_t flags) {
    if (ttbr0 == NULL) ttbr0 = l1_page_table;

    if (((uint32_t)vaddr & 0xFFF) != 0 || ((uint32_t)paddr & 0xFFF) != 0) {
        panic("Unaligned vaddr(%p) or paddr(%p)", vaddr, paddr);
    }

    uint32_t* l1_entry = &((uint32_t*)ttbr0)[SECTION_INDEX((uint32_t)vaddr)];
    if ((*l1_entry & 0x3) != 0x1) {
        panic("Page not allocated, aborting %p (%p->%p)\n", *l1_entry, vaddr, paddr);
    }

    uint32_t *l2_table = (uint32_t*)(*l1_entry & ~0x3FF);
    uint32_t existing = l2_table[PAGE_INDEX((uint32_t)vaddr)];
    if (existing & L2_SMALL_PAGE) { // check if we are overwriting an existing mapping
        panic("Warning: Overwriting existing page mapping at vaddr %p (old paddr: %p, new paddr: %p)\n",
               vaddr, (void*)(existing & ~0xFFF), paddr);
    }

    // Map the page
    l2_table[PAGE_INDEX((uint32_t)vaddr)] = (uint32_t)paddr | L2_SMALL_PAGE | flags;
}

#else
// TODO instead of checking if kernel mem is set, we should just make 2 functions, and replace the function pointer
// Map a 4KB page into virtual memory using process-specific page tables
void map_page(void *ttbr0, void* vaddr, void* paddr, uint32_t flags) {
    // printk("Kernel mem? %d\n", mmu_driver.kernel_mem);
    if (ttbr0 == NULL) ttbr0 = l1_page_table;
    else ttbr0 = PHYS_TO_KERNEL_VIRT(ttbr0);
    // Verify 4KB alignment (last 12 bits must be 0)
    if (((uint32_t)vaddr & 0xFFF) != 0 || ((uint32_t)paddr & 0xFFF) != 0) {
        printk("Unaligned vaddr(%p) or paddr(%p)", vaddr, paddr);
        while (1);
    }

    // Clear the unused bits from paddr (keeping only the physical frame number)
    paddr = (void*)((uint32_t)paddr & ~0xFFF);

    // --- L1 Table Lookup ---
    uint32_t* l1_entry = &((uint32_t*)ttbr0)[SECTION_INDEX((uint32_t)vaddr)];

    // Create L2 table if it doesn't exist
    if ((*l1_entry & 0x3) != 0x1) {
        void* l2_table = alloc_page(&kpage_allocator);
        if (mmu_driver.kernel_mem) {
            memset(PHYS_TO_KERNEL_VIRT(l2_table), 0, 4096);
        } else {
            memset(l2_table, 0, 4096);
        }
        // Clear the new L2 table
        *l1_entry = ((uint32_t)l2_table | 0x1 | (MMU_DOMAIN_KERNEL << 5));
    }

    uint32_t *l2_table;
    if (mmu_driver.kernel_mem) {
        l2_table = PHYS_TO_KERNEL_VIRT((uint32_t*)(*l1_entry & ~0x3FF));
    } else {
        l2_table = (uint32_t*)(*l1_entry & ~0x3FF);
    }

    // Check if page is already mapped
    uint32_t existing = l2_table[PAGE_INDEX((uint32_t)vaddr)];
    if (existing & L2_SMALL_PAGE) {
        // panic for now, but we probably should never let this happen, probably indicates some corruption
        panic("Warning: Overwriting existing page mapping at vaddr %p (old paddr: %p, new paddr: %p)\n",
               vaddr, (void*)(existing & ~0xFFF), paddr);
    }

    // Map the page
    l2_table[PAGE_INDEX((uint32_t)vaddr)] = (uint32_t)paddr | L2_SMALL_PAGE | flags;
}

#endif

void unmap_page(void* ttbr0, void* vaddr) {
    if (ttbr0 == NULL) ttbr0 = l1_page_table;
    else ttbr0 = PHYS_TO_KERNEL_VIRT(ttbr0);
    // --- Sanity Checks ---
    // Verify 4KB alignment (last 12 bits must be 0)
    if (((uint32_t)vaddr & 0xFFF) != 0) {
        printk("Unaligned vaddr(%p)", vaddr);
        while (1);
    }

    // --- L1 Table Lookup ---
    uint32_t* l1_entry = &((uint32_t*)ttbr0)[SECTION_INDEX((uint32_t)vaddr)]; // Pointer to L1 entry
    if ((*l1_entry & 0x3) != 0x1) {
        printk("Page not allocated, aborting %p (%p)\n", *l1_entry, vaddr);
        while (1);
    }

    uint32_t *l2_table;
    if (mmu_driver.kernel_mem) {
        l2_table = PHYS_TO_KERNEL_VIRT((uint32_t*)(*l1_entry & ~0x3FF));
    } else {
        l2_table = (uint32_t*)(*l1_entry & ~0x3FF);
    }

    l2_table[PAGE_INDEX((uint32_t)vaddr)] = 0;
}

void set_l1_page_table(uint32_t *l1_page_table) {
    mmu_driver.ttbr0 = l1_page_table;
    __asm__ volatile(
        "mcr p15, 0, %0, c2, c0, 0 \n" // TTBR0
        "dsb \n"
        "isb \n"
        : : "r"(mmu_driver.ttbr0)
    );
}

void invalidate_all_tlb(void) {
    __asm__ volatile (
        "mcr p15, 0, %0, c8, c7, 0\n"
        "dsb\n"
        "isb\n"
        : : "r" (0) : "memory"
    );
}

static inline uint32_t read_ttbr0(void) {
    uint32_t ttbr0;
    __asm__ volatile (
        "mrc p15, 0, %0, c2, c0, 0\n"
        : "=r" (ttbr0)
    );
    return ttbr0;
}

int check_if_user_addr(uint32_t vaddr, uint32_t len) {
    // count how many page boundaries we cross
    size_t start = (size_t)vaddr;
    size_t end = (size_t)vaddr + len;
    size_t start_page = start & ~0xFFF;
    size_t end_page = (end + 0xFFF) & ~0xFFF;
    size_t num_pages = (end_page - start_page) / 0x1000;

    // for each page, check if it is in the user space by checking the L1 table
    uint32_t* ttbr0 = PHYS_TO_KERNEL_VIRT(read_ttbr0());
    for (size_t i = 0; i < num_pages; i++) {
        uint32_t* l1_entry = &(ttbr0)[SECTION_INDEX(start_page + i * 0x1000)];
        l1_entry = PHYS_TO_KERNEL_VIRT((uint32_t)l1_entry);
        if ((*l1_entry & 0x3) != MMU_PAGE_DESCRIPTOR) {
            return 0;
        }

        uint32_t l2_table = (*l1_entry & ~0x3FF);
        if (l2_table == 0) {
            return 0;
        }
    }

    return 1;
}
