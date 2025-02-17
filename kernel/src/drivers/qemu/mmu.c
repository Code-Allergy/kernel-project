#include "kernel/mm.h"
#include <stdint.h>
#include <kernel/boot.h>

#include <kernel/printk.h>
#include <kernel/mmu.h>
#include <kernel/string.h>
// #include <kernel/panic.h>
#include <kernel/paging.h>


// drivers/mmu.c
extern int mmu_init_l1_page_table(void);
extern void mmu_set_domains(void);
extern void mmu_enable(void);
extern void map_page(void *ttbr0, void* vaddr, void* paddr, uint32_t flags);
extern void unmap_page(void* tbbr0, void* vaddr);
extern void invalidate_all_tlb(void);
extern void set_l1_page_table(uint32_t *l1_page_table);
extern void* get_physical_address(void *vaddr);

#define UART0_BASE 0x01C28000
#define UART4_BASE 0x01c29000
#define MMC0_BASE 0x01C0F000

#define PADDR(addr) ((void*)((addr) - KERNEL_START) + DRAM_BASE)

// L1 Page Table (4096 entries, 4KB each for 4GB address space)
// TODO
// uint32_t l1_page_table[4096] MMU_TABLE_ALIGN;
// l2_page_table_t l2_tables[4096] __attribute__((aligned(1024)));

#define panic(fmt, ...) do { printk(fmt, ##__VA_ARGS__); while(1); } while(0)

// for now all hw is identity mapped inside the kernel page table
void mmu_map_hw_pages(void) {
    // Map 4KB for UART0-UART3
    l2_tables[SECTION_INDEX(UART0_BASE)][PAGE_INDEX(UART0_BASE)] = UART0_BASE | L2_DEVICE_PAGE;
    // Map 4KB for UART4-UART7 (not used)
    l2_tables[SECTION_INDEX(UART4_BASE)][PAGE_INDEX(UART4_BASE)] = UART4_BASE | L2_DEVICE_PAGE;
    // Map 4KB for MMC0
    l2_tables[SECTION_INDEX(MMC0_BASE)][PAGE_INDEX(MMC0_BASE)]   = MMC0_BASE  | L2_DEVICE_PAGE;
    // Map 4KB for other io, CCM, IRQ, PIO, timer, pwm
    l2_tables[SECTION_INDEX(0x01c20000)][PAGE_INDEX(0x01c20000)] = 0x01c20000 | L2_DEVICE_PAGE;
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
