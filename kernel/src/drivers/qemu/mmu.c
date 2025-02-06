#include <stdint.h>
#include <kernel/boot.h>

#include <kernel/printk.h>
#include <kernel/mmu.h>
#include <kernel/string.h>
#include <kernel/panic.h>

#define UART0_BASE 0x01C28000
#define UART4_BASE 0x01c29000
#define MMC0_BASE 0x01C0F000

extern uint32_t kernel_end;
extern uint32_t kernel_code_end;



// L1 Page Table (4096 entries, 4KB each for 4GB address space)
uint32_t l1_page_table[4096] MMU_TABLE_ALIGN;
l2_page_table_t l2_tables[4096] __attribute__((aligned(1024)));

// void mmu_set_domains(void) {
//     uint32_t dacr = 0;
//     // every domain is set to client
//     for (int d = 0; d < 16; d++) {
//         dacr |= (0x1 << (d * 2));
//     }

//     __asm__ volatile("mcr p15, 0, %0, c3, c0, 0" : : "r"(dacr));
// }
//


void mmu_set_domains(void) {
    // Domain 0 (Kernel): Manager (bypass permissions)
    // Domain 1 (User): Client (enforce page table AP bits)
    uint32_t dacr = (0x3 << (MMU_DOMAIN_KERNEL * 2)) | (0x1 << (MMU_DOMAIN_USER * 2));
    asm volatile("mcr p15, 0, %0, c3, c0, 0" : : "r"(dacr));
}

int is_kernel_page(uint32_t paddr) {
    return paddr >= KERNEL_ENTRY && paddr < (uint32_t)&kernel_end;
}

int mmu_init_page_table(void) {
    /* Verify page table alignment */
    if (((uintptr_t)l1_page_table & 0x3FFF) != 0) {
        printk("Page table not 16KB aligned\n");
        return -1;
    }

    if (((uintptr_t)l2_tables & 0x3FF) != 0) {
        printk("L2 tables not 1KB aligned\n");
        return -1;
    }

    if (bootloader_info.kernel_size > 1024*1024) {
        printk("Kernel too large to fit in 1MB: TODO\n");
        return -1;
    }

    // map all l1 entries to l2 tables
    for (uint32_t section = 0; section < 4096; section++) {
        l1_page_table[section] = ((uint32_t)&l2_tables[section] & ~0x3FF)
                                | MMU_PAGE_DESCRIPTOR | (1 << 5);
    }

    // Map 4KB for UART0-UART3
    l2_tables[SECTION_INDEX(UART0_BASE)][PAGE_INDEX(UART0_BASE)] = UART0_BASE | L2_DEVICE_PAGE;
    // Map 4KB for UART4-UART7 (not used)
    l2_tables[SECTION_INDEX(UART4_BASE)][PAGE_INDEX(UART4_BASE)] = UART4_BASE | L2_DEVICE_PAGE;
    // Map 4KB for MMC0
    l2_tables[SECTION_INDEX(MMC0_BASE)][PAGE_INDEX(MMC0_BASE)]   = MMC0_BASE  | L2_DEVICE_PAGE;
    // Map 4KB for other io, CCM, IRQ, PIO, timer, pwm
    l2_tables[SECTION_INDEX(0x01c20000)][PAGE_INDEX(0x01c20000)] = 0x01c20000 | L2_DEVICE_PAGE;

    for (uint32_t addr = KERNEL_ENTRY; addr < (uint32_t)&kernel_end; addr += 4096) {
        if (addr < (uint32_t)&kernel_code_end) {
            l2_tables[SECTION_INDEX(addr)][PAGE_INDEX(addr)] =  addr | L2_KERNEL_CODE_PAGE;
        } else {
            l2_tables[SECTION_INDEX(addr)][PAGE_INDEX(addr)] = addr | L2_KERNEL_DATA_PAGE;
        }
    }

    // identity map all of DRAM for now
    for (uint32_t paddr = DRAM_BASE; paddr < DRAM_BASE + DRAM_SIZE; paddr += 4096) {
        // Skip already mapped kernel code/data
        if (!is_kernel_page(paddr)) {
            l2_tables[SECTION_INDEX(paddr)][PAGE_INDEX(paddr)] =
                paddr | L2_KERNEL_DATA_PAGE;
        }
    }
    return 0;
}

void mmu_enable(void) {
    // Load TTBR1 with the physical address of the L1 table
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
}

uint32_t alloc_l1_table(struct page_allocator *alloc) {
    // Allocate 4 contiguous 4KB pages (16KB total)
    void *addr = alloc_aligned_pages(alloc, 4);
    if(!addr) return 0;

    // initialize the 4 pages from the allocation
    for (int i = 0; i < 4; i++) {
        l2_tables[SECTION_INDEX((uint32_t)addr + i*4096)][PAGE_INDEX((uint32_t)addr + i*4096)] = ((uint32_t)addr + i*4096) | L2_KERNEL_DATA_PAGE;
    }

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

        l2_tables[SECTION_INDEX((uint32_t)l2_table)][PAGE_INDEX((uint32_t)l2_table)] = (uint32_t)l2_table | L2_KERNEL_DATA_PAGE; // map the page as itself and kernel data
        // Initialize L2 table (4096 entries * 4 bytes = 16KB)
        memset(l2_table, 0, 4096 * sizeof(uint32_t));
        printk("Here\n");

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
        : : "r" (0)
    );
    // isb();
}

void kernel_mmu_init(void) {
    mmu_init_page_table(); // Populate L1 table
    mmu_set_domains();     // Configure domains
    mmu_enable();          // Load TTBR0 and enable MMU
}

void debug_l1_entry(uint32_t *va) {
    uint32_t va_val = (uint32_t)va;

    // Get L1 index (bits 31-20)
    uint32_t l1_index = va_val >> 20;
    uint32_t l1_entry = l1_page_table[l1_index];

    printk("L1 Entry [%p] @ %p: %p\n",
          l1_index, &l1_page_table[l1_index], l1_entry);

    // Decode L1 descriptor
    if((l1_entry & 0x3) == 0x2) {
        printk("  Section: Phys=%p\n", l1_entry & 0xFFF00000);
        printk("  AP=%p, Domain=%p, C=%d, B=%d\n",
              (l1_entry >> 10) & 0x3,
              (l1_entry >> 5) & 0xF,
              (l1_entry >> 3) & 0x1,
              (l1_entry >> 2) & 0x1);
    } else if((l1_entry & 0x3) == 0x1) {
        printk("  Page Table: L2 @ %p\n", l1_entry & 0xFFFFFC00);
    } else {
        printk("  INVALID ENTRY\n");
    }
}

void debug_l2_entry(uint32_t *va) {
    uint32_t va_val = (uint32_t)va;

    // Get L1 index first
    uint32_t l1_index = va_val >> 20;
    uint32_t l1_entry = l1_page_table[l1_index];

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

void debug_l1_l2_entries(void *va) {
    printk("\n=== MMU Debug for %p ======================\n", va);
    debug_l1_entry(va);
    debug_l2_entry(va);
    printk("=============================================\n\n");
}



// Helper function to verify domain settings
void verify_domain_access(void) {
    uint32_t dacr;
    asm volatile("mrc p15, 0, %0, c3, c0, 0" : "=r"(dacr));

    printk("DACR: %p\n", dacr);

    // Check specific domain settings
    uint32_t kernel_domain = (dacr >> (MMU_DOMAIN_KERNEL * 2)) & 0x3;
    uint32_t device_domain = (dacr >> (MMU_DOMAIN_DEVICE * 2)) & 0x3;

    printk("Kernel domain access: %s\n",
           kernel_domain == MMU_DOMAIN_MANAGER ? "Manager" :
           kernel_domain == MMU_DOMAIN_CLIENT ? "Client" : "No Access");

    printk("Device domain access: %s\n",
           device_domain == MMU_DOMAIN_MANAGER ? "Manager" :
           device_domain == MMU_DOMAIN_CLIENT ? "Client" : "No Access");
}

// Add this to your mmu_init function after setting up page tables
void test_domain_protection(void) {
    // Test writing to a code page
    uint32_t *code_ptr = (uint32_t *)KERNEL_ENTRY;
    uint32_t old_value = *code_ptr;

    printk("Testing domain protection...\n");
    printk("Attempting to write to code section at %p\n", code_ptr);

    // Try to write - should cause domain fault if properly configured
    *code_ptr = 0xDEADBEEF;

    // If we get here, domain protection failed
    if (*code_ptr == 0xDEADBEEF) {
        printk("WARNING: Successfully wrote to code section - domain protection failed!\n");
        // Restore the original value
        *code_ptr = old_value;
    } else {
        printk("Domain protection working - write was prevented\n");
    }
}

mmu_t mmu_driver = {
    // .init = mmu_init,
    .enable = kernel_mmu_init,
    // .disable = mmu_disable,
    // .debug_l1_entry = debug_l1_entry,
    // .debug_l2_entry = debug_l2_entry,
    // .debug_l1_l2_entries = debug_l1_l2_entries,
    // .verify_domain_access = verify_domain_access,
    // .test_domain_protection = test_domain_protection,
};
