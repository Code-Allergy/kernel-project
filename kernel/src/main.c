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

#define KERNEL_HEARTBEAT_TIMER 400000 // us
#define TEST_FILE "bin/while"
#define TEST_FILE2 "bin/testa"
#define STACK_CANARY_VALUE 0xDEADBEEF

bootloader_t bootloader_info;

// system clock
__attribute__((noreturn)) void system_clock(void) {
    extern uint32_t canary_value_start;
    if (canary_value_start != STACK_CANARY_VALUE) {
        uint32_t kernel_phys_l1 = (uint32_t)l1_page_table - KERNEL_START + DRAM_BASE;
        mmu_driver.set_l1_table((void*)kernel_phys_l1);
        printk("STACK CORRUPTION DETECTED\nHALT\n");
        while (1);
    }

    // fix stack pointer (64 bytes)
    __asm__ volatile ("add sp, sp, #64"); // TODO this is hacky but needed to reset the stack as schedule() doesn't reuturn


    scheduler();
}

void init_kernel_hardware(void) {
    clock_timer.init();
    clock_timer.start_idx_callback(0, KERNEL_HEARTBEAT_TIMER, system_clock);
    interrupt_controller.init();

    // these 2 can be combined when we rewrite drivers
    uart_driver.enable_interrupts();

    interrupt_controller.register_irq(1, uart_handler, NULL);
    interrupt_controller.enable_irq(1);
}

extern void context_restore(void);
extern void context_switch(struct cpu_regs* current_context, struct cpu_regs* next_context);
void context_switch_1(struct cpu_regs* next_context);
unsigned long __stack_chk_guard = STACK_CANARY_VALUE;

void __stack_chk_fail(void) {
    uint32_t kernel_phys_l1 = (uint32_t)l1_page_table - KERNEL_START + DRAM_BASE;
    mmu_driver.set_l1_table((void*)kernel_phys_l1);
    printk("STACK CORRUPTION DETECTED (__stack_chk_fail)\nHALT\n");
    while(1);  // Infinite loop or trigger a fault handler
}

#define PADDR(addr) ((uint32_t)((addr) - KERNEL_START) + DRAM_BASE)

void init_kernel_pages(void) {
    extern uint32_t kernel_code_end;
    extern uint32_t kernel_end;

    mmu_driver.init();

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

    // map all dram as identity mapped
    for (uint32_t vaddr = DRAM_BASE; vaddr < DRAM_BASE + DRAM_SIZE; vaddr += PAGE_SIZE) {
        mmu_driver.map_page(NULL, (void*)vaddr, (void*)vaddr, L2_KERNEL_DATA_PAGE);
    }

    mmu_driver.flush_tlb();
    mmu_driver.set_l1_table((uint32_t*)((uint32_t)l1_page_table - KERNEL_START + DRAM_BASE));
}

void init_stack_canary(void) {
    extern uint32_t canary_value_start;

   canary_value_start = STACK_CANARY_VALUE;
}

#ifndef BOOTLOADER
__attribute__((section(".text.kernel_main")))
int kernel_main(bootloader_t* _bootloader_info) { // we can pass a different struct once we decide what the bootloader should fully do.
    setup_stacks();
    init_stack_canary();
    for (size_t i = 0; i < sizeof(bootloader_t); i++) ((char*)&bootloader_info)[i] = ((char*)_bootloader_info)[i];
    if (bootloader_info.magic != 0xFEEDFACE) panic("Invalid bootloader magic: %x\n", bootloader_info.magic);
    if (calculate_checksum((void*)kernel_main, bootloader_info.kernel_size) != bootloader_info.kernel_checksum) panic("Checksum check failed!");

    printk("Kernel starting - version %s\n", GIT_VERSION);
    printk("Kernel base address %p\n", kernel_main);
    init_kernel_hardware();
    printk("Finished initializing hardware\n");

    // we already have vm from the bootloader, but we should switch to our own tables
    init_kernel_pages();
    init_page_allocator(&kpage_allocator);
    kernel_heap_init();
    interrupt_controller.enable_irq_global();
    scheduler_init();

    printk("Reached end of kernel_main, something bad happened!\nHalting\n");
    while (1)  __asm__ volatile("wfi");
    __builtin_unreachable();
}
#endif
