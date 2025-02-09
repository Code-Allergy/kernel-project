#include <stdint.h>

#include <kernel/boot.h>
#include <kernel/mm.h>
#include <kernel/printk.h>
#include <kernel/mmu.h>
#include <kernel/string.h>
#include <kernel/paging.h>

#include "uart.h"
#include "mmc.h"

#define PADDR(addr) ((void*)((addr) - KERNEL_START) + DRAM_BASE)


#define panic(fmt, ...) do { printk(fmt, ##__VA_ARGS__); while(1); } while(0)

// L1 Page Table (4096 entries, 4KB each for 4GB address space)
uint32_t l1_page_table[4096] MMU_TABLE_ALIGN;
l2_page_table_t l2_tables[4096] __attribute__((aligned(1024)));

void mmu_map_hw_pages(void) {
// #ifdef BOOTLOADER
    // Map 4KB for UART0-UART3
    l2_tables[SECTION_INDEX(UART0_BASE)][PAGE_INDEX(UART0_BASE)] = UART0_BASE | L2_DEVICE_PAGE;
    // Map 4KB for UART4-UART7 (not used)
    l2_tables[SECTION_INDEX(UART4_BASE)][PAGE_INDEX(UART4_BASE)] = UART4_BASE | L2_DEVICE_PAGE;
    // Map 4KB for MMC0
    l2_tables[SECTION_INDEX(MMC0_BASE)][PAGE_INDEX(MMC0_BASE)]   = MMC0_BASE  | L2_DEVICE_PAGE;
    // Map 4KB for other io, CCM, IRQ, PIO, timer, pwm
    l2_tables[SECTION_INDEX(0x01c20000)][PAGE_INDEX(0x01c20000)] = 0x01c20000 | L2_DEVICE_PAGE;
// #else
//     // Map 4KB for UART0-UART3
//     l2_tables[SECTION_INDEX(UART0_BASE | MEMORY_KERNEL_DEVICE_BASE)][PAGE_INDEX(UART0_BASE)] = UART0_BASE | L2_DEVICE_PAGE;
//     // Map 4KB for UART4-UART7 (not used)
//     l2_tables[SECTION_INDEX(UART4_BASE | MEMORY_KERNEL_DEVICE_BASE)][PAGE_INDEX(UART4_BASE)] = UART4_BASE | L2_DEVICE_PAGE;
//     // Map 4KB for MMC0
//     l2_tables[SECTION_INDEX(MMC0_BASE | MEMORY_KERNEL_DEVICE_BASE)][PAGE_INDEX(MMC0_BASE)]   = MMC0_BASE  | L2_DEVICE_PAGE;
//     // Map 4KB for other io, CCM, IRQ, PIO, timer, pwm
//     l2_tables[SECTION_INDEX(0x01c20000 | MEMORY_KERNEL_DEVICE_BASE)][PAGE_INDEX(0x01c20000)] = 0x01c20000 | L2_DEVICE_PAGE;
// #endif
}

static void* get_physical_address(void *vaddr, uint32_t *ttbr0) {
    // If no ttbr0 provided, use kernel page table
    if (ttbr0 == NULL) {
        ttbr0 = (uint32_t*)((uint32_t)l1_page_table - KERNEL_START + DRAM_BASE);
    }

    // DRAM is mapped 1:1 with physical addresses for kernel space
    if ((uint32_t)vaddr >= DRAM_BASE && (uint32_t)vaddr < DRAM_BASE + DRAM_SIZE) {
        return vaddr;
    }

    // Get L1 entry from the provided ttbr0
    uint32_t *l1_entry = &ttbr0[SECTION_INDEX((uint32_t)vaddr)];

    // Check L1 entry type
    switch (*l1_entry & 0x3) {
        case 0:  // Invalid
            return NULL;

        case 1:  // Page Table (points to L2)
            {
                // Get L2 table physical address (clear the control bits)
                uint32_t *l2_table = (uint32_t*)(*l1_entry & ~0x3FF);

                // If L2 table is in kernel virtual space, convert to physical
                if ((uint32_t)l2_table >= KERNEL_START) {
                    l2_table = (uint32_t*)(((uint32_t)l2_table - KERNEL_START) + DRAM_BASE);
                }

                // Get L2 entry
                uint32_t l2_entry = l2_table[PAGE_INDEX((uint32_t)vaddr)];

                // Check if L2 entry is valid (small page)
                if ((l2_entry & 0x2) == 0x2) {
                    // Combine physical page address with page offset
                    return (void*)((l2_entry & ~0xFFF) | ((uint32_t)vaddr & 0xFFF));
                }
                return NULL;
            }

        case 2:  // Section (1MB)
            // Return section base address + offset
            return (void*)((*l1_entry & ~0xFFFFF) | ((uint32_t)vaddr & 0xFFFFF));

        default:  // Reserved
            return NULL;
    }
}

uint32_t read_dacr(void) {
    uint32_t dacr;
    asm volatile("mrc p15, 0, %0, c3, c0, 0" : "=r"(dacr));
    return dacr;
}

void mmu_set_domains(void) {
    // Set domain 0 (kernel) to manager access (0x3)
    uint32_t dacr = 0x3;  // Just set domain 0 to manager (0x3)
    asm volatile("mcr p15, 0, %0, c3, c0, 0" : : "r"(dacr));

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

    // map all l1 entries to l2 tables
    for (uint32_t section = 0; section < 4096; section++) {
        uint32_t l2_table = ((uint32_t)&l2_tables[section] > KERNEL_ENTRY) ?
                    DRAM_BASE + ((uint32_t)&l2_tables[section] - KERNEL_START) :
                            (uint32_t)&l2_tables[section];

        l1_page_table[section] = (l2_table & ~0x3FF) | MMU_PAGE_DESCRIPTOR | (MMU_DOMAIN_KERNEL << 5);
    }

    return 0;
}



void debug_l1_entry(uint32_t *va, uint32_t* ttbr0) {
    uint32_t va_val = (uint32_t)va;

    // Get L1 index (bits 31-20)
    uint32_t l1_index = va_val >> 20;
    uint32_t l1_entry = ttbr0[l1_index];

    printk("L1 Entry [%p] @ %p: %p\n", l1_index, &l1_page_table[l1_index], l1_entry);

    // Decode L1 descriptor
    if((l1_entry & 0x3) == 0x2) {
        printk("  Section: Phys=%p\n", l1_entry & 0xFFFFF000);
        printk("  AP=%p, Domain=%p, C=%d, B=%d\n",
              (l1_entry >> 10) & 0x3,
              (l1_entry >> 5) & 0xF,
              (l1_entry >> 3) & 0x1,
              (l1_entry >> 2) & 0x1);
    } else if((l1_entry & 0x3) == 0x1) {
        printk("  Page Table: L2 @ %p, Domain: %d\n", l1_entry & 0xFFFFFC00, (l1_entry >> 5) & 0xF);
    } else {
        printk("  INVALID ENTRY\n");
    }
}

void debug_l2_entry(uint32_t *va, uint32_t* ttbr0) {
    uint32_t va_val = (uint32_t)va;

    // Get L1 index first
    uint32_t l1_index = va_val >> 20;
    uint32_t l1_entry = ttbr0[l1_index];

    if((l1_entry & 0x3) != 0x1) {
        printk("No L2 table for address %p\n", va);
        return;
    }

    // Get L2 table address
    uint32_t *l2_table = (uint32_t*)(l1_entry & 0xFFFFFC00);

    // Get L2 index (bits 19-12)
    uint32_t l2_index = (va_val >> 12) & 0xFF;
    uint32_t l2_entry = l2_table[l2_index];

    printk("L2 Entry [%p] @ %p: %p\n",
          l2_index, &l2_table[l2_index], l2_entry);

    if((l2_entry & 0x3) == 0x2) {
        printk("  Small Page: Phys=%p\n", l2_entry & 0xFFFFF000);
        printk("  AP=%p, C=%d, B=%d, TEX=%p, S=%d\n",
              (l2_entry >> 4) & 0x3,
              (l2_entry >> 3) & 0x1,
              (l2_entry >> 2) & 0x1,
              (l2_entry >> 6) & 0x7,
              (l2_entry >> 10) & 0x1);
    } else {
        printk("  INVALID ENTRY\n");
    }
}

void debug_l1_l2_entries(void *va, uint32_t* ttbr0) {
    printk("\n=== MMU Debug for %p, TTBR0: %p ======================\n", va);
    debug_l1_entry(va, ttbr0);
    debug_l2_entry(va, ttbr0);
    printk("=============================================\n\n");
}




uint32_t alloc_l1_table(struct page_allocator *alloc) {
    // Allocate 4 contiguous 4KB pages (16KB total)
    void *addr = alloc_aligned_pages(alloc, 4);
    if(!addr) return 0;

    // Initialize L1 table
    memset(addr, 0, 16*1024);
    return (uint32_t)addr;
}

// Map a 4KB page into virtual memory using process-specific page tables
static void map_page(void *ttbr0, void* vaddr, void* paddr, uint32_t flags) {
    if (ttbr0 == NULL) {
#ifdef BOOTLOADER
        ttbr0 = l1_page_table;
#else
        uint32_t kernel_table = (uint32_t)l1_page_table - KERNEL_START + DRAM_BASE;
        ttbr0 = (uint32_t*)kernel_table;
#endif
    }
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
        if (!l2_table) {
            printk("Failed to allocate L2 table\n");
            while (1);
        }

        memset(l2_table, 0, PAGE_SIZE);
        *l1_entry = (uint32_t)l2_table | 0x1;
    }


    uint32_t *l2_table = (uint32_t*)(*l1_entry & ~0x3FF);
    l2_table[PAGE_INDEX((uint32_t)vaddr)] = ((uint32_t)paddr & ~0xFFF) | L2_SMALL_PAGE | flags;
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

void mmu_init(void) {
    mmu_init_l1_page_table(); // Populate L1 table
    mmu_map_hw_pages();
    mmu_set_domains();     // Configure domains
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
    asm volatile("mcr p15, 0, %0, c8, c7, 0" : : "r"(0)); // Invalidate TLB
    asm volatile("mcr p15, 0, %0, c7, c5, 0" : : "r"(0)); // Invalidate I-cache
    asm volatile("mcr p15, 0, %0, c7, c6, 0" : : "r"(0)); // Invalidate data cache
    asm volatile("mcr p15, 0, %0, c7, c10, 0" : : "r"(0)); // Clean data cache
    asm volatile("mcr p15, 0, %0, c7, c10, 4" : : "r"(0)); // Drain write buffer

    // Enable MMU, caches, and branch prediction
    uint32_t sctlr;
    asm volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(sctlr));
    sctlr |= (1 << 0)  // Enable MMU
           | (1 << 2)  // Enable D-cache
           | (1 << 12) // Enable I-cache
           | (1 << 11) // Enable branch prediction
           ;
    asm volatile("mcr p15, 0, %0, c1, c0, 0" : : "r"(sctlr));
}

static void set_l1_page_table(uint32_t *l1_page_table) {
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

static inline void invalidate_all_tlb(void) {
    __asm__ __volatile__ (
        "mcr p15, 0, %0, c8, c7, 0\n"
        "dsb\n"
        "isb\n"
        : : "r" (0) : "memory"
    );
}

void set_ttbr0_with_asid(uint32_t* table, uint8_t asid) {
    uint32_t ttbr0 = (uint32_t)table;
    ttbr0 |= asid;  // ASID goes in bits [7:0] for ARMv7

    __asm__ volatile(
        "mcr p15, 0, %0, c2, c0, 0"  // Write TTBR0
        : : "r" (ttbr0) : "memory"
    );

    // Ensure the write is complete
    __asm__ volatile("isb" ::: "memory");
}




mmu_t mmu_driver = {
    .init = mmu_init,
    .enable = mmu_enable,
    .map_page = map_page,
    .unmap_page = unmap_page,
    .flush_tlb = invalidate_all_tlb,
    .set_l1_table = set_l1_page_table,
    .get_physical_address = get_physical_address,
    .map_hardware_pages = mmu_map_hw_pages,
    .set_l1_with_asid = set_ttbr0_with_asid,
    // .disable = mmu_disable,
    // .debug_l1_entry = debug_l1_entry,
    // .debug_l2_entry = debug_l2_entry,
    // .debug_l1_l2_entries = debug_l1_l2_entries,
    // .verify_domain_access = verify_domain_access,
    // .test_domain_protection = test_domain_protection,

    // device info for qemu - cubieboard
    .page_size = 4096,
};
