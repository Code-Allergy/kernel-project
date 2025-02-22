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
#include <kernel/log.h>
#include <elf32.h>

#include <stdint.h>


extern uint32_t __bss_start;
extern uint32_t __bss_end;

bootloader_t bootloader_info;



void init_kernel_hardware(void) {
    interrupt_controller.init();

    // these 2 can be combined when we rewrite drivers
    interrupt_controller.enable_irq_global();
    uart_driver.enable_interrupts();
    init_kernel_time();
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

void clear_bss(void) {
    uint32_t* l1_page_table_end = l1_page_table + 0x4000;

    // clear BSS but skip page table regions
    uint32_t *bss = &__bss_start;
    printk("Would clear from %p to %p\n", &__bss_start, &__bss_end);
    // while (bss < &__bss_end) {
    //     if (!((bss >= l1_page_table && bss < l1_page_table_end) ||
    //           (bss >= &l2_tables_start && bss < &l2_tables_end))) {
    //         *bss = 0;
    //     }
    //     bss++;
    // }
}

#ifndef BOOTLOADER
__attribute__((section(".text.kernel_main"), noreturn)) void kernel_main(bootloader_t* _bootloader_info) {
    init_kernel_pages();
    clear_bss();
    setup_stacks();

    for (size_t i = 0; i < sizeof(bootloader_t); i++) ((char*)&bootloader_info)[i] = ((char*)_bootloader_info)[i]; // copy bootloader into memory controlled by the kernel
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

    printk("Kernel ready at time %llu\n", epoch_now());
    LOG(INFO, "Hello from logging function!\n");
    scheduler();

    panic("Reached end of kernel_main, something bad happened");
    __builtin_unreachable();
}
#endif
