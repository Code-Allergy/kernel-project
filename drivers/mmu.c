// universal functions for bbb/qemu
#include <stdint.h>
#include <kernel/mmu.h>
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
    memset(addr, 0, 16*1024);
    return (uint32_t)addr;
}

void mmu_set_domains(void) {
    // Set domain 0 (kernel) to manager access (0x3)
    uint32_t dacr = 0x3;  // Just set domain 0 to manager (0x3)
    __asm__ volatile("mcr p15, 0, %0, c3, c0, 0" : : "r"(dacr));

    // Verify the setting
    uint32_t check = read_dacr();
    printk("DACR value: %p\n", check);
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
    printk("Dram base: %p\n", DRAM_BASE);
    // map all l1 entries to l2 tables
    for (uint32_t section = 0; section < 4096; section++) {
        uint32_t l2_table = ((uint32_t)&l2_tables[section] > KERNEL_ENTRY) ?
                    DRAM_BASE + ((uint32_t)&l2_tables[section] - KERNEL_ENTRY) :
                            (uint32_t)&l2_tables[section];

        l1_page_table[section] = l2_table | MMU_PAGE_DESCRIPTOR | (MMU_DOMAIN_KERNEL << 5);
    }

    printk("Done\n");
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

// Map a 4KB page into virtual memory using process-specific page tables
void map_page(void *ttbr0, void* vaddr, void* paddr, uint32_t flags) {
    if (ttbr0 == NULL) ttbr0 = l1_page_table;
    // --- Sanity Checks ---
    // Verify 4KB alignment (last 12 bits must be 0)
    if (((uint32_t)vaddr & 0xFFF) != 0 || ((uint32_t)paddr & 0xFFF) != 0) {
        printk("Unaligned vaddr(%p) or paddr(%p)", vaddr, paddr);
        while (1);
    }

    // --- L1 Table Lookup ---
    uint32_t* l1_entry = &((uint32_t*)ttbr0)[SECTION_INDEX((uint32_t)vaddr)]; // Pointer to L1 entry
    if ((*l1_entry & 0x3) != 0x1) {
#ifdef BOOTLOADER
        printk("Page not allocated, aborting %p (%p->%p)\n", *l1_entry, vaddr, paddr);
        while (1);
#endif
        void* l2_table = alloc_page(&kpage_allocator);
        *l1_entry = ((uint32_t)l2_table | 0x1 | (MMU_DOMAIN_KERNEL << 5));
    }
    // identity map the table so we can write to it
    uint32_t *l2_table = (uint32_t*)(*l1_entry & ~0x3FF);
    // void* l2_table_page = (void*) (*l1_entry & ~0xFFF);
    // mmu_driver.map_page(NULL, l2_table_page, l2_table_page, L2_KERNEL_DATA_PAGE);
    // mmu_driver.flush_tlb();
    l2_table[PAGE_INDEX((uint32_t)vaddr)] = (uint32_t)paddr | L2_SMALL_PAGE | flags;

    // unmap the table, we don't need it anymore
    // mmu_driver.unmap_page(NULL, l2_table);
}

void unmap_page(void* tbbr0, void* vaddr) {
    if (tbbr0 == NULL) tbbr0 = l1_page_table;
    // --- Sanity Checks ---
    // Verify 4KB alignment (last 12 bits must be 0)
    if (((uint32_t)vaddr & 0xFFF) != 0) {
        printk("Unaligned vaddr(%p)", vaddr);
        while (1);
    }

    // --- L1 Table Lookup ---
    uint32_t* l1_entry = &((uint32_t*)tbbr0)[SECTION_INDEX((uint32_t)vaddr)]; // Pointer to L1 entry
    if ((*l1_entry & 0x3) != 0x1) {
        printk("Page not allocated, aborting %p (%p)\n", *l1_entry, vaddr);
        while (1);
    }

    uint32_t *l2_table = (uint32_t*)(*l1_entry & ~0x3FF);
    l2_table[PAGE_INDEX((uint32_t)vaddr)] = 0;
}

void set_l1_page_table(uint32_t *l1_page_table) {
    mmu_driver.ttbr0 = l1_page_table;
    asm volatile(
        "mcr p15, 0, %0, c2, c0, 0 \n" // TTBR0
        "dsb \n"
        "isb \n"
        : : "r"(mmu_driver.ttbr0)
    );

    // Invalidate TLB and caches
    asm volatile("mcr p15, 0, %0, c8, c7, 0" : : "r"(0)); // Invalidate TLB
    asm volatile("mcr p15, 0, %0, c7, c5, 0" : : "r"(0)); // Invalidate I-cache
    asm volatile("mcr p15, 0, %0, c7, c6, 0" : : "r"(0)); // Invalidate data cache
    asm volatile("mcr p15, 0, %0, c7, c10, 0" : : "r"(0)); // Clean data cache
    asm volatile("mcr p15, 0, %0, c7, c10, 4" : : "r"(0)); // Drain write buffer
}

void invalidate_all_tlb(void) {
    __asm__ __volatile__ (
        "mcr p15, 0, %0, c8, c7, 0\n"
        "dsb\n"
        "isb\n"
        : : "r" (0) : "memory"
    );
}
