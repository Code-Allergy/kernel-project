#include <kernel/boot.h>

#include <kernel/printk.h>
#include <kernel/sched.h>
#include <kernel/uart.h>

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



uint32_t read_vbar(void) {
    uint32_t vbar;
    __asm__ volatile("mrc p15, 0, %0, c12, c0, 0" : "=r"(vbar));
    return vbar;
}

// void intc_init() {
//     PIC->ENABLE[1] |= (1 << (UART_IRQ - 32));
//     PIC->MASK[1] &= ~(1 << (UART_IRQ - 32));

//     PIC->PROT_EN = 0x01;  // Enable protection
//     PIC->NMI_CTRL = 0x00;
// }


int kernel_init() {
    timer_init(KERNEL_HEARTBEAT_TIMER);
    
    request_irq(1, uart_handler, NULL);

    
    return 0;
}


__attribute__((section(".text.kernel_main")))
int kernel_main(bootloader_t* bootloader_info) { // we can pass a different struct once we decide what the bootloader should fully do.
    if (bootloader_info->magic != 0xFEEDFACE) {
        printk("Invalid bootloader magic: %x\n", bootloader_info->magic);
        return -1;
    }
    // intc_init();
    
    intc_init();
    kernel_init();

    uart_init_interrupts();
    enable_irqs();

    printk("Kernel busy loop\n");
    while(1) {
        asm volatile("wfi");
    }
    __builtin_unreachable();
}
