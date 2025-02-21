#include "uart.h"
#include <kernel/mmu.h>
#include <kernel/boot.h>
// drivers/mmu.c
extern int mmu_init_l1_page_table(void);
extern void mmu_set_domains(void);
extern void mmu_enable(void);
extern void map_page(void *ttbr0, void* vaddr, void* paddr, uint32_t flags);
extern void unmap_page(void* tbbr0, void* vaddr);
extern inline void invalidate_all_tlb(void);
extern void set_l1_page_table(uint32_t *l1_page_table);

#define ALIGN_DOWN(addr, align) ((addr) & ~((align) - 1))
#define ALIGN_UP(addr, align)   ALIGN_DOWN((addr) + (align) - 1, (align))
#define L1_TABLE_BASE  0x90000000  // External RAM base for L1 page table
#define L2_TABLE_BASE  0x90004000  // External RAM base for L2 page tables (bootstrap)

#define L1_TABLE_SIZE  4096  // 4KB
#define L2_TABLE_SIZE  256   // 256 entries per L2 table

#define OCMC_BASE 0x40300000

// MMU Table Pointers (Do NOT allocate memory in the binary)
volatile uint32_t * const l1_page_table = (uint32_t *) L1_TABLE_BASE;
l2_page_table_t * const l2_tables = (l2_page_table_t *) L2_TABLE_BASE;


void mmu_map_hw_pages(void) {
    // Map UART0's exact 4KB page as device memory
    uint32_t uart_vaddr = UART0_BASE;
    uint32_t uart_paddr = uart_vaddr; // Identity mapping
    l2_tables[SECTION_INDEX(uart_vaddr)][PAGE_INDEX(uart_vaddr)] =
        (uart_paddr & 0xFFFFF000) | L2_DEVICE_PAGE;

    // Identity map internal SRAM (0x402F0400 to 0x402FFFFF)
    uint32_t sram_start = 0x402F0400;
    uint32_t sram_end = 0x402FFFFF;
    uint32_t sram_start_aligned = ALIGN_DOWN(sram_start, PAGE_SIZE);
    uint32_t sram_end_aligned = ALIGN_UP(sram_end, PAGE_SIZE);

    for (uint32_t addr = sram_start_aligned; addr < sram_end_aligned; addr += PAGE_SIZE) {
        l2_tables[SECTION_INDEX(addr)][PAGE_INDEX(addr)] =
            (addr & 0xFFFFF000) | L2_KERNEL_CODE_PAGE; // Executable for code
    }

    // Identity map L3 OCMC (0x40300000 to 0x4030FFFF)
    uint32_t ocmc_start = OCMC_BASE;
    uint32_t ocmc_end = OCMC_BASE + 0x10000; // 64KB
    uint32_t ocmc_start_aligned = ALIGN_DOWN(ocmc_start, PAGE_SIZE);
    uint32_t ocmc_end_aligned = ALIGN_UP(ocmc_end, PAGE_SIZE);

    for (uint32_t addr = ocmc_start_aligned; addr < ocmc_end_aligned; addr += PAGE_SIZE) {
        l2_tables[SECTION_INDEX(addr)][PAGE_INDEX(addr)] =
            (addr & 0xFFFFF000) | L2_KERNEL_DATA_PAGE; // Non-executable data
    }
}
// temp
#define L1_TABLE_ALIGNMENT 16384 // 16KB-aligned for ARMv7-A
#define L1_SECTION_DESCRIPTOR(pa, ap, tex, c, b, xn) \
    ((pa) & 0xFFF00000) | (2 << 0) | /* Section entry */      \
    ((ap) << 10) |                   /* AP bits (access permissions) */ \
    (0 << 5) |                       /* Domain 0 */           \
    ((tex) << 12) | ((c) << 3) | ((b) << 2) | /* TEX, C, B (memory attributes) */ \
    ((xn) << 4)                     /* XN (execute never) */

// Memory Type Settings
#define DEVICE_MEMORY     L1_SECTION_DESCRIPTOR(0, 0b11, 0, 0, 0, 1)
#define NORMAL_MEMORY     L1_SECTION_DESCRIPTOR(0, 0b11, 1, 1, 1, 0)
// Normal memory (WBWA): TEX=001, C=1, B=1
#define SECTION_ENTRY_NORMAL (0x2 | (0x3 << 10) | (0x1 << 12) | (1 << 3) | (1 << 2))
// Device memory: TEX=000, C=0, B=0
#define SECTION_ENTRY_DEVICE (0x2 | (0x3 << 10) | (0x0 << 12)

void mmu_identity_map_all(void) {
    // 1. Map entire 4GB as DEVICE (default)
    for (int i = 0; i < 4096; i++) {
        l1_page_table[i] = DEVICE_MEMORY | (i << 20);
    }

    // 2. Remap DRAM (0x80000000-0x9FFFFFFF) as NORMAL (cacheable)
    for (int i = 0x800; i < 0xA00; i++) { // 512MB DRAM (adjust if different)
        l1_page_table[i] = NORMAL_MEMORY | (i << 20);
    }

    // 3. Remap critical regions (e.g., code, SRAM, peripherals)
    // Example: Map SRAM (0x402F0400-0x40300000) as NORMAL
    for (int i = 0x402; i <= 0x403; i++) {
        l1_page_table[i] = NORMAL_MEMORY | (i << 20);
    }

    // 4. Map UART0 (0x44E09000) as DEVICE (already covered by step 1)
    // No action needed if already identity-mapped
}

void mmu_init(void) {
    // mmu_init_l1_page_table(); // Populate L1 table
    // mmu_map_hw_pages();
    mmu_identity_map_all();
    mmu_set_domains();     // Configure domains
}

void* get_physical_address(uint32_t* ttbr0, void *vaddr) {
    uint32_t *l1_entry = &ttbr0[SECTION_INDEX((uint32_t)vaddr)];
    if ((*l1_entry & 0x3) != 0x1) {
        return (void*)(((uint32_t)vaddr - KERNEL_ENTRY) + DRAM_BASE);
    }

    uint32_t *l2_table = (uint32_t*)(*l1_entry & 0xFFFFF000);
    uint32_t l2_index = ((uint32_t)vaddr >> 12) & 0xFF;

    return (void*)((l2_table[l2_index] & 0xFFFFF000) | ((uint32_t)vaddr & 0xFFF));
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
    // .disable = mmu_disable,
    // .debug_l1_entry = debug_l1_entry,
    // .debug_l2_entry = debug_l2_entry,
    // .debug_l1_l2_entries = debug_l1_l2_entries,
    // .verify_domain_access = verify_domain_access,
    // .test_domain_protection = test_domain_protection,

    // device info for qemu - cubieboard
    .page_size = 4096,
};
