#include <kernel/boot.h>

#include <kernel/printk.h>
#include <kernel/sched.h>
#include <kernel/uart.h>
#include <kernel/paging.h>

#include <kernel/heap.h>
#include <kernel/mmu.h>

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


static inline void enable_irqs(void) {
    __asm__ volatile("cpsie i");
}

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

    // void* page = alloc_page(&kpage_allocator);
    // map_page(&kpage_allocator, 0xF0000000, (uint32_t) page, (uint32_t)(3 << 10));

    // test out writing to the page
    // *(uint32_t*) 0xF0000000 = 0xDEADBEEF;

    // printk("Value at 0xF0000000: %x\n", *(uint32_t*) 0x8000);

    // printk("Kernel busy loop\n");

    // printk("res: %u\n", kernel_heap_curr);

    // int i = 4;


    scheduler_init();
    __builtin_unreachable();
}
