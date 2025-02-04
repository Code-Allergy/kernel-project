#include <stdint.h>
#include <kernel/printk.h>
#include <kernel/int.h>
#include <kernel/uart.h>
#include <kernel/syscall.h>

#include "intc.h"
#include "int.h"
#include "../../../team-repo/src/drivers/qemu/uart.h"

static struct irq_entry handlers[MAX_IRQ_HANDLERS] = {0};
static uint32_t svc_handlers[NR_SYSCALLS] = {0};

/* Initial vector table for ARM, memory is pointed to here in register */
extern uint32_t _vectors[];

#define INTC_IRQ_NO_HANDLER -1
#define INTC_IRQ_TOO_LARGE -2

void handle_irq_c(void) {
    for(int reg = 0; reg < 3; reg++) {
        uint32_t pending = INTC->IRQ_PEND[reg];
        while(pending) {
            int irq = 32 * reg + __builtin_ctz(pending);
            if(handlers[irq].handler) {
                handlers[irq].handler(irq, handlers[irq].data);
            }
            pending &= pending - 1;
        }
    }
}

void handle_svc_c(void) {
    printk("SVC handler\n");
}

void data_abort_handler() {
    uint32_t dfsr, dfar;
    asm volatile("mrc p15, 0, %0, c5, c0, 0" : "=r"(dfsr)); // DFSR
    asm volatile("mrc p15, 0, %0, c6, c0, 0" : "=r"(dfar)); // DFAR
    printk("Data abort! Addr: 0x%x, Status: 0x%x\n", dfar, dfsr);
    while (1); // Halt
}

void uart_handler(int irq, void *data) {
    // Read UART IIR to check interrupt type and clear it
    uint32_t iir = UART0->IIR_FCR & 0x0F;  // Mask IIR bits

    if (iir == 0x04) {  // Received data available
        char c = UART0->RBR_THR_DLL;  // Read the character (clears interrupt)
        printk("Received: %c\n", c);
    } else if (iir == 0x0C) {  // Timeout (FIFO non-empty)
        // Handle FIFO timeout
    }

    // Acknowledge interrupt in INTC (platform-specific)
    INTC->IRQ_PEND[0] = (irq << 1); 

    printk("IRQ %d: UART fired!\n", irq);
}

int register_irq_handler(uint32_t irq, irq_handler_t handler, void *data) {
    if(irq >= MAX_IRQ_HANDLERS) return -1;
    handlers[irq].handler = handler;
    handlers[irq].data = data;
    return 0;
}

int enable_irq(uint32_t irq) {
    if(irq >= MAX_IRQ_HANDLERS) return INTC_IRQ_TOO_LARGE;
    if (!handlers[irq].handler) return INTC_IRQ_NO_HANDLER;
    INTC_IRQ_ENABLE(irq);
    return 0;
}

int disable_irq(uint32_t irq) {
    if(irq >= MAX_IRQ_HANDLERS) return INTC_IRQ_TOO_LARGE;
    INTC_IRQ_DISABLE(irq);
    return 0;
}

int request_irq(int irq, irq_handler_t handler, void *data) {
    if(irq >= MAX_IRQ_HANDLERS) return -1;
    handlers[irq].handler = handler;
    handlers[irq].data = data;
    INTC_IRQ_ENABLE(irq);
    return 0;
}


int intc_init() {
    uint32_t vbar_addr = (uint32_t)_vectors;
    if(vbar_addr & 0x1F) {
        printk("VBAR unaligned! Fixing 0x%08x → 0x%08x\n", 
               vbar_addr, vbar_addr & ~0x1F);
        vbar_addr &= ~0x1F;
    }
    __asm__ volatile(
        "mcr p15, 0, %0, c12, c0, 0 \n"  /* Write VBAR */
        "dsb \n"
        "isb \n"
        : 
        : "r" (vbar_addr)
    );

    // setup stacks for IRQ and SVC modes
    __asm__ volatile(
        "cps #0x12 \n"        /* IRQ mode */
        "ldr sp, =irq_stack_top \n"
        "cps #0x13 \n"        /* SVC mode */
        "ldr sp, =svc_stack_top \n"
        "cps #0x1F \n"        /* System mode */
    );

    // setup CSPR for IRQ mode
    __asm__ volatile(
        "mrs r0, cpsr \n"
        "bic r0, r0, #0x80 \n"
        "msr cpsr_c, r0 \n"
    );
}
