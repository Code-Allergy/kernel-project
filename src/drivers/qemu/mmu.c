#include <stdint.h>
#include <kernel/boot.h>

#include <kernel/printk.h>
#include <kernel/mmu.h>

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

uint32_t make_section_entry(uint32_t phys_addr) {
    return (phys_addr & 0xFFF00000)          // physical base address
           | MMU_SECTION_DESCRIPTOR         // descriptor type bits
           | (MMU_DOMAIN_KERNEL << 5)         // domain bits (bits 5-8)
           | MMU_AP_RW                      // access permissions
           | MMU_CACHEABLE                  // cacheable
           | MMU_BUFFERABLE;                 // bufferable
}

uint32_t make_l1_page_table_entry(uint32_t phys_addr) {
    return (phys_addr & 0xFFFFFC00)          // physical base address
           | MMU_PAGE_DESCRIPTOR            // descriptor type bits
           | MMU_AP_RW                      // access permissions
           | MMU_CACHEABLE                  // cacheable
           | MMU_BUFFERABLE;                 // bufferable
}

uint32_t make_small_page_kernel(uint32_t phys_addr) {
    return (phys_addr & 0xFFFFF000)          // physical base address
           | L2_SMALL_PAGE                  // descriptor type bits
           | MMU_AP_RW                      // access permissions
           | MMU_CACHEABLE                  // cacheable
           | MMU_BUFFERABLE;                 // bufferable
}

void mmu_set_domains(void) {
    // Set domain 0 to client (0b01) and all others to manager (0b11)
    uint32_t dacr = 0;
    // Domain 0: client (01)
    dacr |= (0x1 << (0 * 2));
    // Domains 1-15: manager (11)
    for (int d = 1; d < 16; d++) {
        dacr |= (0x3 << (d * 2));
    }
    
    asm volatile("mcr p15, 0, %0, c3, c0, 0" : : "r"(dacr));
}

// L1 Page Table (4096 entries, 4KB each for 4GB address space)
static uint32_t l1_page_table[4096] MMU_TABLE_ALIGN;

typedef uint32_t l2_page_table_t[256];
l2_page_table_t l2_tables[4096] __attribute__((aligned(1024)));

void mmu_init_page_table(bootloader_t* bootloader_info) {
    /* Verify page table alignment */
    if (((uintptr_t)l1_page_table & 0x3FFF) != 0) {
        printk("Page table not 16KB aligned\n");
        return;
    }

    if (((uintptr_t)l2_tables & 0x3FF) != 0) {
        printk("L2 tables not 1KB aligned\n");
        return;
    }

    // Map kernel section to L1 table page
    l1_page_table[bootloader_info->kernel_entry >> 20] = make_section_entry(bootloader_info->kernel_entry);
    // l1_page_table[0x5FFE0000 >> 20] = make_section_entry(0x5FFE0000);

    // map the first 10 pages on and after the kernek as L1 table pages 1MB
    for (int i = 0; i < 8; i++) {
        l1_page_table[(bootloader_info->kernel_entry >> 20) + i] = make_section_entry(bootloader_info->kernel_entry + (i << 20));
    }

    uintptr_t next_avail_addr = bootloader_info->kernel_entry + (8 << 20);

    // map all sections as L2 pages by default
    for (uint32_t section = 0; section < 4096; section++) {
        // Check if this section is already initialized by the kernel mapping
        if (!((l1_page_table[section] & 0x3) != MMU_PAGE_DESCRIPTOR)) {
            printk("Skipping section %u\n", section);
            continue;
        }

        // Set up the L1 entry to point to the L2 table for this 1MB region.
        // The physical address of the L2 table must be aligned to 1KB.
        l1_page_table[section] = ((uint32_t)&l2_tables[section] & ~0x3FF)
                                | MMU_PAGE_DESCRIPTOR;
        
        // Fill in all 256 L2 entries for this section.
        for (uint32_t page = 0; page < 256; page++) {
            uint32_t phys_addr = (section << 20) | (page << 12);
            l2_tables[section][page] = phys_addr | L2_SMALL_PAGE
                                        | MMU_AP_RW
                                        | MMU_SHAREABLE
                                        | MMU_CACHEABLE
                                        | MMU_BUFFERABLE;
        }
    }

    uint32_t uart_section = UART0_BASE >> 20;
    uint32_t uart_page_index = (UART0_BASE & 0xFFFFF) >> 12;
    for (uint32_t i = 0; i < 1; i++) { // Map 4KB for UART
        uint32_t phys_addr = UART0_BASE + (i << 12);
        l2_tables[uart_section][uart_page_index + i] = phys_addr | L2_SMALL_PAGE
                                                    | MMU_AP_RW
                                                    | MMU_DEVICE_MEMORY
                                                    | MMU_SHAREABLE;
    }

    // for (int i = 0; i < 4; i++) {
    //     uint32_t virt_addr = DRAM_BASE + (i << 20);
    //     uint32_t l1_index = virt_addr >> 20;
    //     l1_page_table[l1_index] = (virt_addr & ~0xFFFFF) | MMU_PAGE_DESCRIPTOR | MMU_DOMAIN_KERNEL;

    //     // Fill L2 table for this 1MB section
    //     uint32_t *l2_table = &l2_tables[i];
    //     for (int j = 0; j < 256; j++) {
    //         uint32_t phys_addr = virt_addr + (j << 12);
    //         l2_table[j] = phys_addr | L2_SMALL_PAGE | MMU_AP_RW | MMU_CACHEABLE;
    //     }
    // }

    /*
     * Map UART0 as device memory using small pages.
     *
     * The UART0 region is assumed to be 0x1000 bytes.
     */


    // /*
    //  * Now override the entries that cover the UART0 physical address range.
    //  * The index inside the 1MB section is determined by:
    //  *     (UART0_BASE & 0xFFFFF) >> 12
    //  *
    //  * Since the UART0 region is 0x1000 (4KB * 1) or possibly 4 pages (if 0x4000),
    //  * adjust the count accordingly.
    //  */
    // uint32_t uart_page_index = (UART0_BASE & 0xFFFFF) >> 12;
    // // For example, if UART0 spans 0x1000 bytes (one 4KB page), map one page:
    // // If it spans more, adjust 'pages_to_map'
    // uint32_t pages_to_map =  (0x1000) >> 12;  // 0x1000 / 0x1000 = 1 page.
    // // In many cases UART devices might require several pages (e.g. 4 pages). Adjust:
    // // uint32_t pages_to_map = 4;

    // for (uint32_t i = 0; i < pages_to_map; i++) {
    //     uint32_t phys_addr = UART0_BASE + (i << 12);
    //     l2_tables[uart_section][uart_page_index + i] = phys_addr | L2_SMALL_PAGE
    //                                                    | MMU_AP_RW
    //                                                    | MMU_DEVICE_MEMORY
    //                                                    | MMU_SHAREABLE;
    // }
    // printk("Mapped UART0 at section %u, pages %u-%u\n", uart_section,
    //        uart_page_index, uart_page_index + pages_to_map - 1);

}

void mmu_enable() {
    // Load TTBR0 with the physical address of the L1 table
    uint32_t ttbr0 = (uint32_t)l1_page_table;
    asm volatile("mcr p15, 0, %0, c2, c0, 0" : : "r"(ttbr0));
    asm volatile("mcr p15, 0, %0, c3, c0, 0" : : "r"(0)); // DACR
    // TODO: disable this, this is for testing. this disables DACR, we need to setup domains for the kernel and other regions.
    asm volatile("mcr p15, 0, %0, c3, c0, 0" : : "r"(0x33333333));

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
        //    | (1 << 11) // Enable branch prediction
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

// Example: Map a 4 KB page with the given flags (attributes).
// 'alloc' is used if a new L2 table needs to be allocated.
void map_page(struct page_allocator *alloc, uint32_t vaddr, uint32_t paddr, uint32_t flags) {
    // --- Step 1: Determine the L1 index for this virtual address ---
    uint32_t l1_index = vaddr >> 20;  // Each L1 entry covers 1 MB
    uint32_t *l1_entry = &l1_page_table[l1_index];

    // --- Step 2: Check if this L1 entry already points to an L2 table ---
    if ((*l1_entry & 0x3) != MMU_PAGE_DESCRIPTOR) {
        // No L2 table exists here yet. Link to the corresponding L2 table
        if (!l2_tables[l1_index]) {
            printk("No L2 table available for vaddr %x\n", vaddr);
            return;
        }
        
        // Update the L1 entry to point to our L2 table
        *l1_entry = ((uint32_t)l2_tables[l1_index] & ~0x3FF) | MMU_PAGE_DESCRIPTOR;
    }

    printk("L1 ENTRY: %p\n", l1_entry);
    // --- Step 3: Get a pointer to the L2 table ---
    l2_page_table_t *l2_table = (l2_page_table_t *)(*l1_entry & ~0x3FF);
    
    // --- Step 4: Determine the L2 index ---
    // Bits 19:12 of the virtual address (since each L2 entry maps 4 KB).
    uint32_t l2_index = (vaddr >> 12) & 0xFF;
    
    // --- Step 5: Set the L2 entry ---
    // The entry should contain the physical base address (aligned to 4 KB)
    // plus the flags and the small page descriptor.
    (*l2_table)[l2_index] = (paddr & ~0xFFF) | flags | L2_SMALL_PAGE;
    
    // Optionally, you might want to flush the TLB for this entry.
}
