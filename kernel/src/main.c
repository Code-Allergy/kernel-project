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
    init_kernel_time();

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

    // TODO - we can use section mapping for this, it will be faster.
    // map all dram into kernel space (KERNEL_VIRTUAL_DRAM)
    for (uint32_t vaddr = KERNEL_VIRTUAL_DRAM; vaddr < (KERNEL_VIRTUAL_DRAM + DRAM_SIZE); vaddr += PAGE_SIZE) {
        mmu_driver.map_page(NULL, (void*)vaddr, (void*)(vaddr - (KERNEL_VIRTUAL_DRAM - DRAM_BASE)), L2_KERNEL_DATA_PAGE);
    }

    // map kernel code pages, 4k aligned
    for (uint32_t vaddr = KERNEL_START; vaddr < (uint32_t)&kernel_code_end; vaddr += PAGE_SIZE) {
        mmu_driver.map_page(NULL, (void*)vaddr, (void*)PADDR(vaddr), L2_KERNEL_CODE_PAGE);
    }

    // map kernel data pages
    for (uint32_t vaddr = (uint32_t)&kernel_code_end; vaddr < (uint32_t)&kernel_end; vaddr += PAGE_SIZE) {
        mmu_driver.map_page(NULL, (void*)vaddr, (void*)PADDR(vaddr), L2_KERNEL_DATA_PAGE);
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

__attribute__((noreturn))void enter_userspace(void) {
    uint64_t ticks;
    uint32_t s,ms;

    LOG(INFO, "Starting userspace\n");
    scheduler_init();
    ticks = clock_timer.get_ticks();
    ms = clock_timer.ticks_to_ms(ticks);
    s = ms / 1000;
    ms %= 1000;
    LOG(INFO, "Kernel ready after %u.%04u seconds!\n", s, ms);
    LOG(INFO, "Jumping to PID 0\n");
    log_consume(); // flush the log buffer

    scheduler();

    panic("Reached end of start_userspace, something bad happened in scheduler!");
    __builtin_unreachable();
}

static fat32_fs_t fs;
static fat32_file_t file;

#ifndef BOOTLOADER
__attribute__((section(".text.kernel_main"), noreturn))
void kernel_main(bootloader_t* _bootloader_info) {
    setup_stacks();      // switches mode to SVC
    init_kernel_pages();

    for (size_t i = 0; i < sizeof(bootloader_t); i++) ((char*)&bootloader_info)[i] = ((char*)_bootloader_info)[i]; // copy bootloader into memory controlled by the kernel
    if (bootloader_info.magic != 0xFEEDFACE) panic("Invalid bootloader magic: %x\n", bootloader_info.magic);
    init_stack_canary();
    init_kernel_hardware(); // initialize the most basic hardware

    LOG(INFO, "Kernel starting - version %s\n", GIT_VERSION);
    LOG(INFO, "Kernel base address %p\n", kernel_main);
    LOG(INFO, "Finished initializing critical hardware\n");

    // kernel memory
    init_page_allocator(&kpage_allocator);
    kernel_heap_init();
    // setup dynamic managed stacks better


    fat32_mount(&fs, &mmc_fat32_diskio);
    printk("entering create\n");
    fat32_create(&fs, "test1");
    if (fat32_open(&fs, "test1", &file)) {
        LOG(ERROR, "Failed to open file!\n");
    }
    printk("Done!\n");

    int bytes;
    if ((bytes = fat32_write(&file, "Hello W!\n", 9, 0)) < 0) {
        LOG(ERROR, "Failed to write to file!\n");
    };
    printk("Done #2, wrote %d bytes\n", bytes);

    while(1);
    // setup vfs
    vfs_init();

    // make the jump to starting the scheduler and starting our init process
    enter_userspace();
}
#endif
