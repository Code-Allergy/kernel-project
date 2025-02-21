#include <kernel/boot.h>
#include <kernel/printk.h>
#include <kernel/sched.h>
#include <kernel/uart.h>
#include <kernel/paging.h>
#include <kernel/panic.h>
#include <kernel/heap.h>
#include <kernel/mmu.h>
#include <kernel/mm.h>
#include <kernel/fat32.h>
#include <kernel/intc.h>
#include <kernel/timer.h>
#include <kernel/vfs.h>
#include <kernel/string.h>
#include <kernel/time.h>
#include <kernel/rtc.h>
#include <elf32.h>

#include <stdint.h>


extern uint32_t __bss_start;
extern uint32_t __bss_end;

#define KERNEL_HEARTBEAT_TIMER 2000 // usec

bootloader_t bootloader_info;

// system clock
void system_clock() {
    scheduler_driver.tick();
}

void init_kernel_hardware(void) {
    clock_timer.init();
    interrupt_controller.init();

    // these 2 can be combined when we rewrite drivers
    interrupt_controller.enable_irq_global();
    uart_driver.enable_interrupts();
}


#define PADDR(addr) ((uint32_t)((addr) - KERNEL_START) + DRAM_BASE)


// TODO TMP
void init_kernel_pages(void) {
    extern uint32_t kernel_code_end;
    extern uint32_t kernel_end;

    mmu_driver.init();

    const uint32_t dram_offset = KERNEL_VIRTUAL_DRAM - DRAM_BASE;
    // map all dram into kernel space (KERNEL_VIRTUAL_DRAM)
    // TODO - we can use section mapping for this, it will be faster.
    for (uint32_t vaddr = KERNEL_VIRTUAL_DRAM; vaddr < (KERNEL_VIRTUAL_DRAM + DRAM_SIZE); vaddr += PAGE_SIZE) {
        mmu_driver.unmap_page(NULL, (void*)vaddr);
        mmu_driver.map_page(NULL, (void*)vaddr, (void*)(vaddr - dram_offset), L2_KERNEL_DATA_PAGE);
    }

    // map kernel code pages, 4k aligned
    for (uint32_t vaddr = KERNEL_START; vaddr < (uint32_t)&kernel_code_end; vaddr += PAGE_SIZE) {
        uint32_t paddr = PADDR(vaddr);
        mmu_driver.map_page(NULL, (void*)vaddr, (void*)paddr, L2_KERNEL_CODE_PAGE);
    }

    // map kernel data pages
    for (uint32_t vaddr = (uint32_t)&kernel_code_end; vaddr < (uint32_t)&kernel_end; vaddr += PAGE_SIZE) {
        uint32_t paddr = DRAM_BASE + (vaddr - KERNEL_ENTRY);
        mmu_driver.map_page(NULL, (void*)vaddr, (void*)paddr, L2_KERNEL_DATA_PAGE);
    }

    mmu_driver.flush_tlb();
    set_ttbr1(((uint32_t)l1_page_table - KERNEL_START + DRAM_BASE));
    ttbcr_configure_2gb_split();
    ttbcr_enable_ttbr0();
    ttbcr_enable_ttbr1();
    mmu_driver.kernel_mem = 1;
}

void init_stack_canary(void) {
    extern uint32_t canary_value_start;

   canary_value_start = STACK_CANARY_VALUE;
}

// void clear_bss(void) {
//     uint32_t* l1_page_table_end = l1_page_table + 0x4000;

//     // clear BSS but skip page table regions
//     uint32_t *bss = &__bss_start;
//     while (bss < &__bss_end) {
//         if (!((bss >= l1_page_table && bss < l1_page_table_end) ||
//               (bss >= &l2_tables_start && bss < &l2_tables_end))) {
//             *bss = 0;
//         }
//         bss++;
//     }
// }

void copy_and_remap_page_tables(uint32_t old_l1_base, uint32_t old_l2_base) {
    // Assuming l1_page_table and l2_pages are the original tables passed from bootloader

    // 1. Copy L1 table to new location
    memcpy((void*)l1_page_table, (void*)old_l1_base, 0x4000);

    // 2. Copy all L2 tables to new location
    memcpy((void*)l2_tables, (void*)old_l2_base, 0x400000);

    // 3. Update each L1 entry to point to new L2 tables
    for (int i = 0; i < 4096; i++) {
        uint32_t *entry = (uint32_t *)(l1_page_table + (i * 4));
        if ((*entry & 0x3) == 0x1) { // Check if it's an L2 page table descriptor
            // Extract original L2 physical address
            uint32_t old_l2_addr = (*entry & 0xFFFFFC00) << 10;

            // Calculate offset from original L2 base
            uint32_t offset = old_l2_addr - (uint32_t)old_l2_base;

            // Compute new L2 address
            uint32_t new_l2_addr = (uint32_t) l2_tables + offset;

            // Update the L1 entry with the new address, preserving flags
            *entry = (*entry & 0x3FF) | (new_l2_addr & 0xFFFFFC00);
        }
    }

    // 4. Ensure all writes are completed
    asm volatile("dsb");

    // 5. Invalidate TLB
    asm volatile("mcr p15, 0, %0, c8, c7, 0" : : "r" (0));

    // 6. Update TTBR0 to the new L1 table
    asm volatile("mcr p15, 0, %0, c2, c0, 0" : : "r" (l1_page_table));

    // 7. Ensure the update is visible
    asm volatile("dsb");
    asm volatile("isb");
}

#ifndef BOOTLOADER
__attribute__((section(".text.kernel_main"), noreturn)) void kernel_main(bootloader_t* _bootloader_info) {
    // copy_and_remap_page_tables(_bootloader_info->l1_table_base, _bootloader_info->l2_table_base);
    init_kernel_pages();
    // clear_bss();
    setup_stacks();

    for (size_t i = 0; i < sizeof(bootloader_t); i++) ((char*)&bootloader_info)[i] = ((char*)_bootloader_info)[i];
    // if (calculate_checksum((void*)(intptr_t)kernel_main, bootloader_info.kernel_size) != bootloader_info.kernel_checksum) panic("Checksum check failed!");
    if (bootloader_info.magic != 0xFEEDFACE) panic("Invalid bootloader magic: %x\n", bootloader_info.magic);
    init_stack_canary();



    printk("Kernel starting - version %s\n", GIT_VERSION);
    printk("Kernel base address %p\n", kernel_main);
    init_kernel_hardware();
    printk("Finished initializing hardware\n");

    init_page_allocator(&kpage_allocator);
    kernel_heap_init();
    vfs_init();
    zero_device_init();
    ones_device_init();
    uart0_vfs_device_init();
    init_mount_fat32();
    scheduler_init();

    printk("Kernel initialized at time %d\n", epoch_now());

    // start the system clock, then start the scheduler
    clock_timer.start_idx_callback(0, KERNEL_HEARTBEAT_TIMER, system_clock);
    scheduler();

    panic("Reached end of kernel_main, something bad happened");
    __builtin_unreachable();
}
#endif
