#include <stdint.h>
#include <kernel/boot.h>

#include <kernel/printk.h>
#include <kernel/mmu.h>
#include <kernel/string.h>
#define UART0_BASE 0x01C28000

extern uint32_t __bss_end;

#define panic(fmt, ...) do { printk(fmt, ##__VA_ARGS__); while(1); } while(0)

#define SECTION_INDEX(addr) ((addr) >> 20)
#define PAGE_INDEX(addr) (((addr) & 0xFFFFF) >> 12)

#define L2_DEVICE_PAGE (L2_SMALL_PAGE | MMU_AP_RW | MMU_DEVICE_MEMORY | MMU_SHAREABLE)

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
    dacr |= (0x1 << (1 * 2));
    // Domains 2-15: manager (11)
    for (int d = 2; d < 16; d++) {
        dacr |= (0x3 << (d * 2));
    }

    asm volatile("mcr p15, 0, %0, c3, c0, 0" : : "r"(dacr));
}

// L1 Page Table (4096 entries, 4KB each for 4GB address space)
uint32_t l1_page_table[4096] MMU_TABLE_ALIGN;
l2_page_table_t l2_tables[4096] __attribute__((aligned(1024)));

int mmu_init_page_table(bootloader_t* bootloader_info) {
    /* Verify page table alignment */
    if (((uintptr_t)l1_page_table & 0x3FFF) != 0) {
        printk("Page table not 16KB aligned\n");
        return -1;
    }

    if (((uintptr_t)l2_tables & 0x3FF) != 0) {
        printk("L2 tables not 1KB aligned\n");
        return -1;
    }

    if (bootloader_info->kernel_size > 1024*1024) {
        printk("Kernel too large to fit in 1MB: TODO\n");
        return;
    }

    // map all sections as L2 pages by default
    for (uint32_t section = 0; section < 4096; section++) {
        // Set up the L1 entry to point to the L2 table for this 1MB region.
        // The physical address of the L2 table must be aligned to 1KB.
        l1_page_table[section] = ((uint32_t)&l2_tables[section] & ~0x3FF)
                                | MMU_PAGE_DESCRIPTOR;
    }

    l2_tables[SECTION_INDEX(UART0_BASE)][PAGE_INDEX(UART0_BASE)] = UART0_BASE | L2_DEVICE_PAGE | (MMU_DOMAIN_KERNEL << 5);
    // Map 4KB for MMC0
    l2_tables[SECTION_INDEX(0x01C0F000)][PAGE_INDEX(0x01C0F000)] = 0x01C0F000 | L2_DEVICE_PAGE | (MMU_DOMAIN_KERNEL << 5);
    // Map 4KB for other io, CCM, IRQ, PIO, timer, pwm
    l2_tables[SECTION_INDEX(0x01c20000)][PAGE_INDEX(0x01c20000)] = 0x01c20000 | L2_DEVICE_PAGE | (MMU_DOMAIN_KERNEL << 5);

    printk("BSS_END: %p\n", &__bss_end);
    for (uint32_t addr = KERNEL_ENTRY; addr < (uint32_t)&__bss_end; addr += 4096) {
        map_page(l1_page_table, (void*)addr, (void*)addr,
                L2_SMALL_PAGE | MMU_AP_RW | MMU_CACHEABLE | MMU_BUFFERABLE | (MMU_DOMAIN_KERNEL << 5));
    }

    return 0;
}

void mmu_enable(void) {
    // Load TTBR0 with the physical address of the L1 table
    uint32_t ttbr0 = (uint32_t)l1_page_table;
    asm volatile(
        "mcr p15, 0, %0, c2, c0, 0 \n" // TTBR0
        "dsb \n"
        "isb \n"
        : : "r"(ttbr0)
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
void map_page(uint32_t *ttbr0, void* vaddr, void* paddr, uint32_t flags) {
    // --- Sanity Checks ---
    // Verify 4KB alignment (last 12 bits must be 0)
    if (((uint32_t)vaddr & 0xFFF) != 0 || ((uint32_t)paddr & 0xFFF) != 0) {
        panic("Unaligned vaddr(%08x) or paddr(%08x)", vaddr, paddr);
    }

    // --- L1 Table Lookup ---
    uint32_t l1_index = (uint32_t) vaddr >> 20;       // Bits [31:20]
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
    uint32_t l2_index = ((uint32_t) vaddr >> 12) & 0xFF; // Bits [19:12]
    l2_table[l2_index] = (uint32_t) paddr | flags | 0x2; // Small page descriptor

    // --- Cache/TLB Maintenance ---
    // dsb();          // Ensure writes complete
    __asm__ volatile(
        "mcr p15, 0, %0, c8, c7, 1" // Invalidate TLB entry (MVA=va)
        : : "r" (vaddr)
    );
    // isb();
}

void kernel_mmu_init(bootloader_t* bootloader_info) {
    mmu_init_page_table(bootloader_info); // Populate L1 table
    mmu_set_domains();     // Configure domains
    mmu_enable();          // Load TTBR0 and enable MMU
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
