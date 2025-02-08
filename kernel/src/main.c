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

#include <stdint.h>

#include "drivers/qemu/intc.h"

#define KERNEL_HEARTBEAT_TIMER 10000000 // us
#define TEST_FILE "bin/initial"

bootloader_t bootloader_info;

// system clock
void system_clock(void) {

}

void init_kernel_hardware(void) {
    clock_timer.init();
    clock_timer.start_idx_callback(0, KERNEL_HEARTBEAT_TIMER, system_clock);
    interrupt_controller.init();

    // these 2 can be combined when we rewrite drivers
    uart_driver.enable_interrupts();
    interrupt_controller.register_irq(1, uart_handler, NULL);
    interrupt_controller.enable_irq(1);
    interrupt_controller.enable_irq_global();

}

extern void context_restore(void);
extern void context_switch(struct cpu_regs* current_context, struct cpu_regs* next_context);
void context_switch_1(struct cpu_regs* next_context);

void test_process_creation(void) {
    fat32_fs_t sd_card;
    fat32_file_t userspace_application;
    if (fat32_mount(&sd_card, &mmc_fat32_diskio) != 0) {
        printk("Failed to mount SD card\n");
        return;
    }

    if (fat32_open(&sd_card, TEST_FILE, &userspace_application)) {
        printk("Failed to open file\n");
        return;
    }

    void* code_page = alloc_page(&kpage_allocator);
    void* data_page = alloc_page(&kpage_allocator);
    if (userspace_application.file_size == 0) {
        printk("Empty file\n");
        while(1);
    }
    uint32_t bytes[1024];
    fat32_read(&userspace_application, bytes, userspace_application.file_size);

    process_t* p = create_process(code_page, data_page, bytes, userspace_application.file_size);
    // clone_process(p);
    // create_process(code_page, data_page, bytes, userspace_application.file_size);

}


#define PADDR(addr) ((uint32_t)((addr) - KERNEL_START) + DRAM_BASE)

void init_kernel_pages(void) {
    extern uint32_t kernel_code_end;
    extern uint32_t kernel_end;

    // map kernel code pages, 4k aligned
    for (uint32_t vaddr = KERNEL_START; vaddr < (uint32_t)&kernel_code_end; vaddr += PAGE_SIZE) {
        uint32_t paddr = PADDR(vaddr);
        mmu_driver.map_page(NULL, (void*)vaddr, (void*)paddr, L2_KERNEL_CODE_PAGE);
        mmu_driver.flush_tlb();
    }

    // map kernel code pages
    for (uint32_t vaddr = (uint32_t)&kernel_code_end; vaddr < (uint32_t)&kernel_end; vaddr += PAGE_SIZE) {
        uint32_t paddr = DRAM_BASE + (vaddr - KERNEL_ENTRY);
        mmu_driver.map_page(NULL, (void*)vaddr, (void*)paddr, L2_KERNEL_DATA_PAGE);
        mmu_driver.flush_tlb();
    }

    // map all dram as identity mapped
    for (uint32_t vaddr = DRAM_BASE; vaddr < DRAM_BASE + DRAM_SIZE; vaddr += PAGE_SIZE) {
        mmu_driver.map_page(NULL, (void*)vaddr, (void*)vaddr, L2_KERNEL_DATA_PAGE);
        mmu_driver.flush_tlb();
    }
}

static inline uint32_t read_ttbr0(void) {
    uint32_t ttbr0;
    asm volatile ("mrc p15, 0, %0, c2, c0, 0" : "=r" (ttbr0));
    return ttbr0;
}

__attribute__((section(".text.kernel_main")))
int kernel_main(bootloader_t* _bootloader_info) { // we can pass a different struct once we decide what the bootloader should fully do.
    setup_stacks();
    for (size_t i = 0; i < sizeof(bootloader_t); i++)
        ((char*)&bootloader_info)[i] = ((char*)_bootloader_info)[i];

    printk("Kernel starting - version %s\n", GIT_VERSION);
    printk("Kernel base address %p\n", kernel_main);
    if (bootloader_info.magic != 0xFEEDFACE) panic("Invalid bootloader magic: %x\n", bootloader_info.magic);
    if (calculate_checksum((void*)kernel_main, bootloader_info.kernel_size) !=
        bootloader_info.kernel_checksum) panic("Checksum check failed!");
    printk("Passed initial checks!\n");

    init_kernel_hardware();
    printk("Finished initializing hardware\n");

    // we already have paging from the bootloader, but we should switch to our own
    mmu_driver.init();
    init_kernel_pages();
    mmu_driver.set_l1_table(mmu_driver.get_physical_address(l1_page_table));

    printk("Done!\n");
    ((volatile uint32_t*)0xC0000004)[0] = 0x12345678;

    init_page_allocator(&kpage_allocator);
    kernel_heap_init();

    scheduler_init();

    // create process but don't start
    test_process_creation();
    // test_process_creation();

    scheduler();
    printk("Reached end of kernel_main, halting\n");
    while (1);
    __builtin_unreachable();
}
