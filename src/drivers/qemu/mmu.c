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


void mmu_init_page_table(bootloader_t* bootloader_info) {
    if (((uintptr_t)l1_page_table & 0x3FFF) != 0) {
        printk("Page table not 16KB aligned\n");
        return;
    }

    // Initialize all entries as unmapped
    for (uint32_t i = 0; i < 4096; i++) {
        l1_page_table[i] = 0; // Invalid descriptor
    }

    /* Map DRAM */
    for (uint32_t i = REAL_BASE >> 20; i < (REAL_BASE + 0x20000000) >> 20; i++) {
        uint32_t base_addr = i << 20;
        l1_page_table[i] = base_addr | MMU_SECTION_DESCRIPTOR | MMU_AP_RW | 
                            MMU_DOMAIN_KERNEL | MMU_CACHEABLE | MMU_BUFFERABLE;
    }

    /* Map UART0 */
    for (uint32_t i = UART0_BASE >> 20; i <= (UART0_BASE + 0x1000) >> 20; i++) {
        uint32_t base_addr = i << 20;
        l1_page_table[UART0_BASE >> 20] = base_addr | MMU_SECTION_DESCRIPTOR | 
                            MMU_AP_RW | MMU_DOMAIN_KERNEL | 
                            MMU_DEVICE_MEMORY | MMU_SHAREABLE;
    }

}

void mmu_enable() {
    // Load TTBR0 with the physical address of the L1 table
    uint32_t ttbr0 = (uint32_t)l1_page_table;
    asm volatile("mcr p15, 0, %0, c2, c0, 0" : : "r"(ttbr0));

    // Invalidate TLB and caches
    asm volatile("mcr p15, 0, %0, c8, c7, 0" : : "r"(0)); // Invalidate TLB
    asm volatile("mcr p15, 0, %0, c7, c5, 0" : : "r"(0)); // Invalidate I-cache
    asm volatile("mcr p15, 0, %0, c7, c5, 6" : : "r"(0)); // Invalidate BTB

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
