#include <stdint.h>
#include <kernel/boot.h>

#include "mmu.h"

// Align to 16KB (ARMv7 requirement for TTBR0)
#define MMU_TABLE_ALIGN  __attribute__((aligned(16384)))

#define MMU_SECTION_DESCRIPTOR (2 << 0)     // Section descriptor (b10)
#define MMU_DOMAIN_KERNEL      0            // Domain 0 for kernel
#define MMU_AP_RW              (3 << 10)    // Read/write access
#define MMU_CACHEABLE          (1 << 3)     // Enable caching
#define MMU_BUFFERABLE         (1 << 2)     // Enable write buffering
#define MMU_DEVICE_MEMORY (0 << 3)          // Disable caching for device memory
#define MMU_SHAREABLE (1 << 10)             // Mark as shareable

#define DACR_CLIENT_DOMAIN0 (1 << 0)

#define UART0_BASE 0x01C28000



void mmu_set_domains() {
    asm volatile("mcr p15, 0, %0, c3, c0, 0" : : "r"(DACR_CLIENT_DOMAIN0));

    // Verify domain access control
    uint32_t dacr_readback;
    asm volatile("mrc p15, 0, %0, c3, c0, 0" : "=r"(dacr_readback));
    
    if (dacr_readback != DACR_CLIENT_DOMAIN0) {
        // Handle domain access error
        printk("Failed to set domain access control\n");
        return;
    }
}

// L1 Page Table (4096 entries, 4KB each for 4GB address space)
static uint32_t l1_page_table[4096] MMU_TABLE_ALIGN;

typedef uint32_t l2_page_table_t[256];
l2_page_table_t l2_tables[4096] __attribute__((aligned(1024)));


// void mmu_init_page_table(bootloader_t* bootloader_info) {
//     if (((uintptr_t)l1_page_table & 0x3FFF) != 0) {
//         printk("Page table not 16KB aligned\n");
//         return;
//     }

//     // L2 page table entries, map first 4 as 4KB pages
//     for (int i = 0; i < 4096; i++) {
//         uint32_t l2_table_phys = (uint32_t)&l2_tables[i];
//         l1_page_table[i] = l2_table_phys | MMU_PAGE_DESCRIPTOR | MMU_DOMAIN_KERNEL;


//         for (int j = 0; j < 256; j++) {
//             uint32_t page_phys = (i << 20) + (j << 12); // 4KB-aligned physical address
//             l2_tables[i][j] = page_phys | L2_SMALL_PAGE | MMU_AP_RW | MMU_CACHEABLE;
//         }
//         printk("Mapping L2 table %d\n", i);
//     }
    
// }

void mmu_init_page_table(bootloader_t* bootloader_info) {
    if (((uintptr_t)l1_page_table & 0x3FFF) != 0) {
        printk("Page table not 16KB aligned\n");
        return;
    }

    // Invalidate all L1 entries
    for (uint32_t i = 0; i < 4096; i++) {
        l1_page_table[i] = 0;
    }
    
    /*
     * Map DRAM using small (4KB) pages.
     *
     * Each L1 entry covers 1MB. We compute the start and end sections for the DRAM.
     */
    // uint32_t dram_start_section = DRAM_BASE >> 20;
    // uint32_t dram_end_section = ((DRAM_BASE + bootloader_info->total_memory) - 1) >> 20;
    // for (uint32_t section = dram_start_section; section <= dram_end_section; section++) {
    //     // Set up the L1 entry to point to the L2 table for this 1MB region.
    //     // The physical address of the L2 table must be aligned to 1KB.
    //     l1_page_table[section] = ((uint32_t)&l2_tables[section] & ~0x3FF)
    //                               | MMU_PAGE_DESCRIPTOR;
        
    //     // Fill in all 256 L2 entries for this section.
    //     for (uint32_t page = 0; page < 256; page++) {
    //         /* 
    //          * The physical address for this 4KB page is computed as:
    //          *    section_base + (page * 4KB)
    //          *
    //          * In many cases you want an identity mapping for DRAM.
    //          * Adjust the physical address if a different mapping is desired.
    //          */
    //         uint32_t phys_addr = (section << 20) | (page << 12);
    //         l2_tables[section][page] = phys_addr | L2_SMALL_PAGE
    //                                     | MMU_AP_RW
    //                                     | MMU_CACHEABLE
    //                                     | MMU_BUFFERABLE;
    //     }
    //     // Optionally print mapping info:
    //     printk("Mapped DRAM section %u (%p - %p) with L2 table at %p\n",
    //            section, section << 20, (section << 20) | 0xFFFFF, &l2_tables[section]);
    // }

    for (int i = 0; i < 4; i++) {
        uint32_t virt_addr = DRAM_BASE + (i << 20);
        uint32_t l1_index = virt_addr >> 20;
        l1_page_table[l1_index] = (virt_addr & ~0xFFFFF) | MMU_PAGE_DESCRIPTOR | MMU_DOMAIN_KERNEL;

        // Fill L2 table for this 1MB section
        uint32_t *l2_table = &l2_tables[i];
        for (int j = 0; j < 256; j++) {
            uint32_t phys_addr = virt_addr + (j << 12);
            l2_table[j] = phys_addr | L2_SMALL_PAGE | MMU_AP_RW | MMU_CACHEABLE;
        }
    }

    /*
     * Map UART0 as device memory using small pages.
     *
     * The UART0 region is assumed to be 0x1000 bytes.
     */
    uint32_t uart_section = UART0_BASE >> 20;
    // If this section is not already initialized by the DRAM mapping,
    // set up its L1 entry (or you can re-use the same L2 table if that makes sense).
    if ((l1_page_table[uart_section] & 0x3) != MMU_PAGE_DESCRIPTOR) {
        l1_page_table[uart_section] = ((uint32_t)&l2_tables[uart_section] & ~0x3FF)
                                      | MMU_PAGE_DESCRIPTOR;
        // Initialize the L2 table entries to 0 (invalid)
        for (uint32_t page = 0; page < 256; page++) {
            l2_tables[uart_section][page] = 0;
        }
    }

    /*
     * Now override the entries that cover the UART0 physical address range.
     * The index inside the 1MB section is determined by:
     *     (UART0_BASE & 0xFFFFF) >> 12
     *
     * Since the UART0 region is 0x1000 (4KB * 1) or possibly 4 pages (if 0x4000),
     * adjust the count accordingly.
     */
    uint32_t uart_page_index = (UART0_BASE & 0xFFFFF) >> 12;
    // For example, if UART0 spans 0x1000 bytes (one 4KB page), map one page:
    // If it spans more, adjust 'pages_to_map'
    uint32_t pages_to_map =  (0x1000) >> 12;  // 0x1000 / 0x1000 = 1 page.
    // In many cases UART devices might require several pages (e.g. 4 pages). Adjust:
    // uint32_t pages_to_map = 4;

    for (uint32_t i = 0; i < pages_to_map; i++) {
        uint32_t phys_addr = UART0_BASE + (i << 12);
        l2_tables[uart_section][uart_page_index + i] = phys_addr | L2_SMALL_PAGE
                                                       | MMU_AP_RW
                                                       | MMU_DEVICE_MEMORY
                                                       | MMU_SHAREABLE;
    }
    printk("Mapped UART0 at section %u, pages %u-%u\n", uart_section,
           uart_page_index, uart_page_index + pages_to_map - 1);

}

void mmu_enable() {
    // Load TTBR0 with the physical address of the L1 table
    uint32_t ttbr0 = (uint32_t)l1_page_table;
    asm volatile("mcr p15, 0, %0, c2, c0, 0" : : "r"(ttbr0));

    // Invalidate TLB and caches
    asm volatile("mcr p15, 0, %0, c8, c7, 0" : : "r"(0)); // Invalidate TLB
    asm volatile("mcr p15, 0, %0, c7, c5, 0" : : "r"(0)); // Invalidate I-cache

    // Enable MMU, caches, and branch prediction
    uint32_t sctlr;
    asm volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(sctlr));
    sctlr |= (1 << 0)  // Enable MMU
           | (1 << 2)  // Enable D-cache
           | (1 << 12) // Enable I-cache
           | (1 << 11) // Enable branch prediction
           ;
    asm volatile("mcr p15, 0, %0, c1, c0, 0" : : "r"(sctlr));

        // Read back SCTLR to confirm settings
    uint32_t sctlr_readback;
    asm volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(sctlr_readback));
    // Check if critical bits are set
    if (!(sctlr_readback & (1 << 0)) ||  // MMU
        !(sctlr_readback & (1 << 2)) ||  // D-cache
        !(sctlr_readback & (1 << 12))) { // I-cache

        printk("Failed to enable MMU\n");
        // Handle configuration error
        return;
    }
}
