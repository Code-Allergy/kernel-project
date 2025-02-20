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

#include "../drivers/qemu/i2c.h"


extern uint32_t __bss_start;
extern uint32_t __bss_end;

#define KERNEL_HEARTBEAT_TIMER 80000 // usec

bootloader_t bootloader_info;

// system clock
void system_clock() {
    scheduler_driver.tick();
}

void init_kernel_hardware(void) {
    clock_timer.init();
    interrupt_controller.init();

    // these 2 can be combined when we rewrite drivers
    uart_driver.enable_interrupts();
    interrupt_controller.enable_irq(1);
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
    // mmu_driver.set_l1_table(NULL);
}

void init_stack_canary(void) {
    extern uint32_t canary_value_start;

   canary_value_start = STACK_CANARY_VALUE;
}

#ifndef BOOTLOADER
__attribute__((section(".text.kernel_main"), noreturn)) void kernel_main(bootloader_t* _bootloader_info) { // we can pass a different struct once we decide what the bootloader should fully do.
    for (size_t i = 0; i < sizeof(bootloader_t); i++) ((char*)&bootloader_info)[i] = ((char*)_bootloader_info)[i];

    if (calculate_checksum((void*)kernel_main, bootloader_info.kernel_size) != bootloader_info.kernel_checksum) panic("Checksum check failed!"); // TODO this can't log here!!
    init_kernel_pages();
    setup_stacks();
    init_stack_canary();
    if (bootloader_info.magic != 0xFEEDFACE) panic("Invalid bootloader magic: %x\n", bootloader_info.magic);

    printk("Kernel starting - version %s\n", GIT_VERSION);
    printk("Kernel base address %p\n", kernel_main);
    init_kernel_hardware();
    printk("Finished initializing hardware\n");

    // we already have vm from the bootloader, but we should switch to our own tables

    init_page_allocator(&kpage_allocator);
    kernel_heap_init();
    vfs_init();


    scheduler_init();
    interrupt_controller.enable_irq_global();

    printk("Kernel initialized at time %d\n", epoch_now());

    struct cubie_twi twi;
    twi.base = (uint32_t*)TWI0_BASE;
    printk("TWI base: %p\n", twi.base);
    // while(1);
    // rtc_i2c_read(&twi, 0);


    clock_timer.start_idx_callback(0, KERNEL_HEARTBEAT_TIMER, system_clock);

    // test_elf32();
    // while(1);

    scheduler();

    panic("Reached end of kernel_main, something bad happened");
    __builtin_unreachable();
}
#endif
