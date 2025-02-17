#include "uart.h"
#include <kernel/mmu.h>
// drivers/mmu.c
extern int mmu_init_l1_page_table(void);
extern void mmu_set_domains(void);
extern void mmu_enable(void);
extern void map_page(void *ttbr0, void* vaddr, void* paddr, uint32_t flags);
extern void unmap_page(void* tbbr0, void* vaddr);
extern inline void invalidate_all_tlb(void);
extern void set_l1_page_table(uint32_t *l1_page_table);
extern void* get_physical_address(void *vaddr);

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

void mmu_init(void) {
    mmu_init_l1_page_table(); // Populate L1 table
    mmu_map_hw_pages();
    mmu_set_domains();     // Configure domains
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
