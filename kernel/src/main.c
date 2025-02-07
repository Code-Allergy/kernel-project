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

void switch_to_user_pages(process_t* proc) {
    uint32_t ttbr0 = (uint32_t)proc->ttbr0 |
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
        :: "r"(proc->ttbr0), "r"(0x30000001)
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


    // process_t* new_process = create_process(code_page, data_page, bytes, userspace_application.file_size);
    printk("Created process\n");

    printk("About to switch to user pages\n");
    // switch_to_user_pages(new_process);


    // printk("Switched to user pages\n");


    // printk("Switching to user mode\n");
    // printk("First instruction: %p\n", *(uint32_t*)new_process->regs.pc);
    //
    // enter_user_mode(new_process);
}

// map additional pages for the kernel.
// void kernel_mmc_init(void) {
//     for (size_t i = 0; i < 8; i++) {
//         void* page = alloc_page(&kpage_allocator);
//         printk("Allocated page %p\n", page);
//     }

// }

// kernel will be in memory at DRAM_BASE, so we can map the kernel pages to the same location
// void init_kernel_data_pages(void) {
//     extern uint32_t kernel_code_end;
//     extern uint32_t kernel_end;

//     for (uint32_t i = (uint32_t)&kernel_code_end; i < (uint32_t)&kernel_end; i += PAGE_SIZE) {
//         mmu_driver.map_page(l1_page_table, (void*)i,
//             (void*)(DRAM_BASE + (i - KERNEL_ENTRY)), L2_KERNEL_DATA_PAGE);
//         mmu_driver.flush_tlb();
//     }
// }
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

    for (uint32_t vaddr = (uint32_t)&kernel_code_end; vaddr < (uint32_t)&kernel_end; vaddr += PAGE_SIZE) {
        uint32_t paddr = DRAM_BASE + (vaddr - KERNEL_ENTRY);
        mmu_driver.map_page(NULL, (void*)vaddr, (void*)paddr, L2_KERNEL_DATA_PAGE);
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

    init_page_allocator(&kpage_allocator);
    kernel_heap_init();

    scheduler_init();
    __builtin_unreachable();
}
