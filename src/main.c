#include <kernel/boot.h>

#include <kernel/printk.h>
#include <kernel/sched.h>
#include <kernel/uart.h>
#include <kernel/paging.h>

#include <kernel/heap.h>
#include <kernel/mmu.h>
#include <kernel/fat32.h>

#include <stdint.h>

#include "drivers/qemu/intc.h"
#include "drivers/qemu/timer.h"
#include "drivers/qemu/uart.h"

#define KERNEL_HEARTBEAT_TIMER 1000000 // us


#define TEST_IRQ 23
#define UART_IRQ 1

#define TIMER0_CTRL      ((volatile uint32_t*) 0x1c20c10)
#define TIMER0_INTV      ((volatile uint32_t*) 0x1c20c14)
#define TIMER0_CUR       ((volatile uint32_t*) 0x1c20c18)
#define TIMER_IRQ_ENABLE (1 << 2)  // Bit for enabling interrupt

#define PIC_IRQ_ENABLE ((volatile uint32_t*) 0x1c20410)
#define PIC_IRQ_STATUS ((volatile uint32_t*) 0x1c20400)
#define TIMER_IRQ  (1 << 22)

extern void syscall_test(void);

static inline void enable_irqs(void) {
    __asm__ volatile("cpsie i");
}

#define TEST_FILE "bin"

int kernel_init() {
    // timer_init(KERNEL_HEARTBEAT_TIMER);
    
    request_irq(1, uart_handler, NULL);

    
    return 0;
}

void kernel_mmu_init(bootloader_t* bootloader_info) {
    mmu_init_page_table(bootloader_info); // Populate L1 table
    mmu_set_domains();     // Configure domains
    mmu_enable();          // Load TTBR0 and enable MMU
    printk("MMU enabled!\n");
}


__attribute__((section(".text.kernel_main")))
int kernel_main(bootloader_t* bootloader_info) { // we can pass a different struct once we decide what the bootloader should fully do.
    fat32_fs_t sd_card;
    fat32_file_t userspace_application;
    if (bootloader_info->magic != 0xFEEDFACE) {
        printk("Invalid bootloader magic: %x\n", bootloader_info->magic);
        return -1;
    }
    setup_stacks();
    intc_init();
    kernel_init();
    uart_init_interrupts();
    enable_irqs();
    kernel_mmu_init(bootloader_info);
    init_page_allocator(&kpage_allocator, bootloader_info);
    kernel_heap_init();
    printk("Kernel initialized\n");
    
    if (fat32_mount(&sd_card, &mmc_fat32_diskio) != 0) {
        printk("Failed to mount SD card\n");
        return -1;
    }

    if (fat32_open(&sd_card, TEST_FILE, &userspace_application)) {
        printk("Failed to open file\n");
        return -1;
    }


    // get 2 pages, one for code, one for data
    void* code_page = alloc_page(&kpage_allocator);
    void* data_page = alloc_page(&kpage_allocator);

    // read the file into the code page
    printk("file size = %d\n", userspace_application.file_size);

    // set up the page table for user pages
    map_page(&kpage_allocator, 0x80000000, code_page, MMU_NORMAL_MEMORY | MMU_AP_RO | MMU_CACHEABLE | MMU_SHAREABLE | MMU_TEX_NORMAL);
    fat32_read(&userspace_application, 0x80000000, userspace_application.file_size);
    printk("first bytes = %p\n", *(uint32_t*)code_page);

    // set up the data page    
    map_page(&kpage_allocator, 0x80001000, data_page, MMU_NORMAL_MEMORY | MMU_AP_RW | MMU_CACHEABLE | MMU_SHAREABLE | MMU_TEX_NORMAL);

    // create a new process

    struct cpu_regs kregs;
    get_kernel_regs(&kregs);
    process_t* new_process = create_process(0x80000000, 0x80001000);


    printk("Switching to user mode\n");
    printk("First instruction: %p\n", *(uint32_t*)new_process->regs.pc);
    context_restore(&new_process->regs);
    
    printk("Done, halting!\n");
    scheduler_init();
    __builtin_unreachable();
}
