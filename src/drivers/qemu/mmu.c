#include <stdint.h>
#include <kernel/boot.h>

#include <kernel/printk.h>
#include <kernel/mmu.h>
#define UART0_BASE 0x01C28000

#define panic(fmt, ...) do { printk(fmt, ##__VA_ARGS__); while(1); } while(0)

uint32_t make_section_entry(uint32_t phys_addr) {
    return (phys_addr & 0xFFF00000)          // physical base address
           | MMU_SECTION_DESCRIPTOR         // descriptor type bits
           | (MMU_DOMAIN_KERNEL << 5)         // domain bits (bits 5-8)
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
uint32_t l1_page_table[4096] MMU_TABLE_ALIGN;
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

    uintptr_t next_avail_addr = bootloader_info->kernel_entry + (8 << 20);

    // map all sections as L2 pages by default
    for (uint32_t section = 0; section < 4096; section++) {

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

    uint32_t mmc_section = 0x01C0F000 >> 20;
    uint32_t mmc_page_index = (0x01C0F000 & 0xFFFFF) >> 12;
    for (uint32_t i = 0; i < 1; i++) { // Map 4KB for MMC
        uint32_t phys_addr = 0x01C0F000 + (i << 12);
        l2_tables[mmc_section][mmc_page_index + i] = phys_addr | L2_SMALL_PAGE
                                                    | MMU_AP_RW
                                                    | MMU_DEVICE_MEMORY
                                                    | MMU_SHAREABLE;
    }


    if (bootloader_info->kernel_size > 1024*1024) {
        printk("Kernel too large to fit in 1MB: TODO\n");
        return;
    }

    uint32_t kernel_section = bootloader_info->kernel_entry >> 20;
    // l1_page_table[kernel_section] = set_domain_l1_page_table_entry((uint32_t)&l2_tables[kernel_section], MMU_DOMAIN_KERNEL);
    l1_page_table[kernel_section] |= (MMU_DOMAIN_KERNEL << 5);
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

uint32_t alloc_l1_table(struct page_allocator *alloc) {
    // Allocate 4 contiguous 4KB pages (16KB total)
    void *addr = alloc_aligned_pages(alloc, 4);
    if(!addr) return 0;

    // Initialize L1 table
    memset(addr, 0, 16*1024);
    return (uint32_t)addr;
}


// Map a 4KB page into virtual memory using process-specific page tables
void map_page(uint32_t *ttbr0, uint32_t vaddr, uint32_t paddr, uint32_t flags) {
    // --- Sanity Checks ---
    // Verify 4KB alignment (last 12 bits must be 0)
    if ((vaddr & 0xFFF) != 0 || (paddr & 0xFFF) != 0) {
        panic("Unaligned vaddr(%08x) or paddr(%08x)", vaddr, paddr);
    }

    // --- L1 Table Lookup ---
    uint32_t l1_index = vaddr >> 20;       // Bits [31:20]
    uint32_t *l1_entry = &ttbr0[l1_index]; // Pointer to L1 entry

    // --- L2 Table Management ---
    uint32_t *l2_table;
    
    if ((*l1_entry & 0x3) != 0x1) {        // Check if L2 table exists
        // Allocate new L2 table if needed
        l2_table = alloc_page(&kpage_allocator);
        if (!l2_table) panic("Out of L2 tables");
        
        // Initialize L2 table (4096 entries * 4 bytes = 16KB)
        memset(l2_table, 0, 4096 * sizeof(uint32_t));
        
        // Update L1 entry (Section 3.5.1 of ARM ARM)
        *l1_entry = (uint32_t)l2_table | 0x1; // Set as page table
    } else {
        // Existing L2 table
        l2_table = (uint32_t*)(*l1_entry & ~0x3FF);
    }

    // --- L2 Entry Setup ---
    uint32_t l2_index = (vaddr >> 12) & 0xFF; // Bits [19:12]
    l2_table[l2_index] = paddr | flags | 0x2; // Small page descriptor

    // --- Cache/TLB Maintenance ---
    // dsb();          // Ensure writes complete
    __asm__ volatile(
        "mcr p15, 0, %0, c8, c7, 1" // Invalidate TLB entry (MVA=va)
        : : "r" (vaddr)
    );
    // isb();
}


// // Example: Map a 4 KB page with the given flags (attributes).
// // 'alloc' is used if a new L2 table needs to be allocated.
// void map_page(struct page_allocator *alloc, uint32_t vaddr, uint32_t paddr, uint32_t flags) {
//     // --- Step 1: Determine the L1 index for this virtual address ---
//     uint32_t l1_index = vaddr >> 20;  // Each L1 entry covers 1 MB
//     uint32_t *l1_entry = &l1_page_table[l1_index];

//     // --- Step 2: Check if this L1 entry already points to an L2 table ---
//     if ((*l1_entry & 0x3) != MMU_PAGE_DESCRIPTOR) {
//         // No L2 table exists here yet. Link to the corresponding L2 table
//         if (!l2_tables[l1_index]) {
//             printk("No L2 table available for vaddr %x\n", vaddr);
//             return;
//         }
        
//         // Update the L1 entry to point to our L2 table
//         *l1_entry = ((uint32_t)l2_tables[l1_index] & ~0x3FF) | MMU_PAGE_DESCRIPTOR;
//     }
//     // --- Step 3: Get a pointer to the L2 table ---
//     l2_page_table_t *l2_table = (l2_page_table_t *)(*l1_entry & ~0x3FF);
    
//     // --- Step 4: Determine the L2 index ---
//     // Bits 19:12 of the virtual address (since each L2 entry maps 4 KB).
//     uint32_t l2_index = (vaddr >> 12) & 0xFF;
    
//     // --- Step 5: Set the L2 entry ---
//     // The entry should contain the physical base address (aligned to 4 KB)
//     // plus the flags and the small page descriptor.
//     (*l2_table)[l2_index] = (paddr & ~0xFFF) | flags | L2_SMALL_PAGE;
    
//     // Optionally, you might want to flush the TLB for this entry.
// }
