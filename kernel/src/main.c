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

static bootloader_t bootloader_info_kernel;

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

void switch_to_user_pages(process_t* proc) {
    uint32_t ttbr0 = (uint32_t)proc->ttbr1 |
                     (proc->asid << 6) |    // ASID in bits 6-13
                     0x1;                   // Inner cacheable

    __asm__ volatile("mcr p15, 0, %0, c2, c0, 0" : : "r"(ttbr0));

    // Ensure TLB is flushed for the new ASID
    __asm__ volatile("mcr p15, 0, %0, c8, c7, 0" : : "r"(0));  // Flush TLB
}

// Helper function to write a single byte to user space
static inline int __put_user(char *dst, char val) {
    int ret;
    __asm__ volatile(
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
        if (__put_user(&dst[i], src[i]) != 0) {
            return -1;
        }
    }

    return 0;
}

void enter_user_mode(process_t* proc) {
    __asm__ volatile(
        "mcr p15, 0, %0, c2, c0, 0     \n" // Load process page table
        "mcr p15, 0, %1, c3, c0, 0     \n" // Domains: Kernel(0)=Manager, User(1)=Client
        "dsb              \n"
        "isb              \n"
        :: "r"(proc->ttbr1), "r"(0x30000001)
    );

    __asm__ volatile(
        "mov r0, %0 \n"
        "ldmia r0, {r1-r12, sp, lr} \n"
        "ldr pc, [r0, #60] \n"  // Jump to proc->regs.pc
        :: "r"(&proc->regs)
        : "r0"
    );
}

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

    printk("File opened\n");
    // get 2 pages, one for code, one for data
    void* code_page = alloc_page(&kpage_allocator);
    void* data_page = alloc_page(&kpage_allocator);
    printk("Code page: %p\n", code_page);
    printk("Data page: %p\n", data_page);

    // read the file into the code page
    printk("file size = %d\n", userspace_application.file_size);
    if (userspace_application.file_size == 0) {
        printk("Empty file\n");
        while(1);
    }
    printk("Reading file\n");
    uint32_t bytes[1024];
    fat32_read(&userspace_application, bytes, userspace_application.file_size);


    process_t* new_process = create_process(code_page, data_page, bytes, userspace_application.file_size);
    printk("Created process\n");

    printk("About to switch to user pages\n");
    // switch_to_user_pages(new_process);


    // printk("Switched to user pages\n");


    // printk("Switching to user mode\n");
    // printk("First instruction: %p\n", *(uint32_t*)new_process->regs.pc);
    //
    enter_user_mode(new_process);
}

__attribute__((noreturn)) void kernel_panic(void) {
    printk("Kernel panic\n");
    while(1);
}


__attribute__((section(".text.kernel_main")))
int kernel_main(bootloader_t* _bootloader_info) { // we can pass a different struct once we decide what the bootloader should fully do.
    setup_stacks();
    printk("Kernel starting - version %s\n", GIT_VERSION);

    char *src = (char *)_bootloader_info;
    char *dest = (char *)&bootloader_info_kernel;
    for (size_t i = 0; i < sizeof(bootloader_t); i++) {
        dest[i] = src[i];
    }



    bootloader_t* bootloader_info = &bootloader_info_kernel;
    if (bootloader_info->magic != 0xFEEDFACE) {
        printk("Invalid bootloader magic: %x\n", bootloader_info->magic);
        return -1;
    }

    if (bootloader_info->kernel_checksum !=
    calculate_checksum((void*)kernel_main, bootloader_info->kernel_size)) {
        printk("Kernel checksum failed\n");
        return -1;
    }

    kernel_init(bootloader_info);
    test_domain_protection();


    char* some_array;
    some_array = (char*) kmalloc(2048);
    if (some_array == NULL) {
        printk("Failed to allocate memory\n");
    }

    some_array[0] = 'c';
    some_array[1] = 'a';
    some_array[2] = 't';
    some_array[3] = 't';
    some_array[4] = 'l';
    some_array[5] = 'e';
    some_array[6] = '\0';
    printk("Array: %s\n", some_array);

    printk("Address of memory: %p\n", some_array);

    scheduler_init();
    __builtin_unreachable();
}
