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

#include <stdint.h>

#include "../drivers/qemu/timer.h"
#include "../drivers/qemu/intc.h"


extern uint32_t __bss_start;
extern uint32_t __bss_end;

#define KERNEL_HEARTBEAT_TIMER 20000 // usec

bootloader_t bootloader_info;

// TODO remove -- testing only
void timer_start(int timer_idx, uint32_t interval_us);

// system clock
void system_clock(int irq, void* data) {
    scheduler_driver.tick();
    AW_Timer *t = (AW_Timer*) TIMER_BASE;
    t->irq_status ^= (get_timer_idx_from_irq(irq) << 0);
}

void init_kernel_hardware(void) {
    clock_timer.init();
    interrupt_controller.init();

    // these 2 can be combined when we rewrite drivers
    uart_driver.enable_interrupts();

    interrupt_controller.register_irq(1, uart_handler, NULL);
    interrupt_controller.enable_irq(1);
}

extern void context_restore(void);
extern void context_switch(struct cpu_regs* current_context, struct cpu_regs* next_context);
void context_switch_1(struct cpu_regs* next_context);


#define PADDR(addr) ((uint32_t)((addr) - KERNEL_START) + DRAM_BASE)


// TODO TMP
void init_kernel_pages(void) {
    extern uint32_t kernel_code_end;
    extern uint32_t kernel_end;

    // zero out all page tables
    for (int i = 0; i < 4096; i++) {
        l1_page_table[i] = 0;
    }

    mmu_driver.init();

    const uint32_t dram_offset = KERNEL_VIRTUAL_DRAM - DRAM_BASE;
    // map all dram into kernel space (KERNEL_VIRTUAL_DRAM)
    for (uint32_t vaddr = KERNEL_VIRTUAL_DRAM; vaddr < (KERNEL_VIRTUAL_DRAM + DRAM_SIZE); vaddr += PAGE_SIZE) {
        mmu_driver.unmap_page(NULL, (void*)vaddr);
        mmu_driver.map_page(NULL, (void*)vaddr, (void*)(vaddr - dram_offset), L2_KERNEL_DATA_PAGE);
    }

    // // map all dram as identity for now
    // for (uint32_t vaddr = DRAM_BASE; vaddr < DRAM_BASE + DRAM_SIZE; vaddr += PAGE_SIZE) {
    //     mmu_driver.map_page(NULL, (void*)vaddr, (void*)vaddr, L2_KERNEL_DATA_PAGE);
    // }

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
    mmu_driver.set_l1_table(NULL);
}

void init_stack_canary(void) {
    extern uint32_t canary_value_start;

   canary_value_start = STACK_CANARY_VALUE;
}

#ifndef BOOTLOADER
__attribute__((section(".text.kernel_main"), noreturn)) void kernel_main(bootloader_t* _bootloader_info) { // we can pass a different struct once we decide what the bootloader should fully do.
    for (size_t i = 0; i < sizeof(bootloader_t); i++) ((char*)&bootloader_info)[i] = ((char*)_bootloader_info)[i];
    // // clear bss
    // for (uint32_t* p = &__bss_start; p < &__bss_end; p++) *p = 0;

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

    printk("Kernel initialized\n");
    // timer_start(0, KERNEL_HEARTBEAT_TIMER);
    scheduler();

    panic("Reached end of kernel_main, something bad happened");
    __builtin_unreachable();
}
#endif
