#include <kernel/boot.h>

#include <kernel/printk.h>
#include <kernel/sched.h>
#include <kernel/uart.h>
#include <kernel/paging.h>

#include <kernel/heap.h>
#include <kernel/mmu.h>
#include <kernel/mm.h>
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

#define TEST_FILE "bin/initial"

int kernel_init(bootloader_t* bootloader_info) {
    timer_init(KERNEL_HEARTBEAT_TIMER, TIMER0_IDX);
    
    
    intc_init();

    // these 2 can be combined when we rewrite drivers
    uart_init_interrupts();
    request_irq(1, uart_handler, NULL);
    enable_irqs();
    kernel_mmu_init(bootloader_info);
    init_page_allocator(&kpage_allocator, bootloader_info);
    kernel_heap_init();
    printk("Kernel initialized\n");

    
    return 0;
}

void kernel_mmu_init(bootloader_t* bootloader_info) {
    mmu_init_page_table(bootloader_info); // Populate L1 table
    mmu_set_domains();     // Configure domains
    mmu_enable();          // Load TTBR0 and enable MMU
    printk("MMU enabled!\n");
}

void switch_to_user_pages(process_t* proc) {
    uint32_t ttbr0 = (uint32_t)proc->ttbr0 |
                     (proc->asid << 6) |    // ASID in bits 6-13
                     0x1;                   // Inner cacheable
    
    asm volatile("mcr p15, 0, %0, c2, c0, 0" : : "r"(ttbr0));
    
    // Ensure TLB is flushed for the new ASID
    asm volatile("mcr p15, 0, %0, c8, c7, 0" : : "r"(0));  // Flush TLB
}

// Helper function to write a single byte to user space
static inline int __put_user(char *dst, char val) {
    int ret;
    asm volatile(
        "1: strbt %1, [%2]\n"    // Try to store byte
        "   mov %0, #0\n"        // Success
        "2:\n"
        : "=r"(ret)
        : "r"(val), "r"(dst)
        : "memory");
    return ret;
}

int copy_to_user(void *user_dest, const void *kernel_src, size_t len) {
    // We need to handle page faults that might occur
    char *src = (char *)kernel_src;
    char *dst = (char *)user_dest;
    
    // Copy one byte at a time to handle page boundaries
    for (size_t i = 0; i < len; i++) {
        // Try to write - might page fault
        if (__put_user(dst[i], src[i]) != 0) {
            return -1;
        }
    }
    
    return 0;
}

void test_process_creation() {
    fat32_fs_t sd_card;
    fat32_file_t userspace_application;
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
    if (userspace_application.file_size == 0) {
        printk("Empty file\n");
        while(1);
    }
    
    uint32_t bytes[1024];
    fat32_read(&userspace_application, bytes, userspace_application.file_size);


    process_t* new_process = create_process(code_page, data_page, bytes, userspace_application.file_size);
    // set up the page table for user pages
    map_page(new_process->ttbr0, MEMORY_USER_CODE_BASE, code_page, MMU_NORMAL_MEMORY | MMU_AP_RO | MMU_CACHEABLE | MMU_SHAREABLE | MMU_TEX_NORMAL);
    map_page(new_process->ttbr0, MEMORY_USER_DATA_BASE, data_page, MMU_NORMAL_MEMORY | MMU_AP_RW | MMU_CACHEABLE | MMU_SHAREABLE | MMU_TEX_NORMAL);

    switch_to_user_pages(new_process);
    printk("Switched to user pages\n");
    copy_to_user((void*)MEMORY_USER_CODE_BASE, bytes, userspace_application.file_size);



    printk("Switching to user mode\n");
    printk("First instruction: %p\n", *(uint32_t*)new_process->regs.pc);
    // context_restore(&new_process->regs);
}


__attribute__((section(".text.kernel_main")))
int kernel_main(bootloader_t* bootloader_info) { // we can pass a different struct once we decide what the bootloader should fully do.
    if (bootloader_info->magic != 0xFEEDFACE) {
        printk("Invalid bootloader magic: %x\n", bootloader_info->magic);
        return -1;
    }
    // should be done in kernel_main
    setup_stacks();
    kernel_init(bootloader_info);

    scheduler_init();
    __builtin_unreachable();
}
