#include <kernel/intc.h>
#include <kernel/printk.h>
#include <kernel/int.h>
#include <kernel/panic.h>
#include <kernel/syscall.h>
#include <kernel/mmu.h>
#include <kernel/boot.h>
#include <kernel/sched.h>

#include "intc.h"

/* Initial vector table for ARM, memory is pointed to here in VBAR register */
extern uint32_t _vectors[];

struct irq_entry irq_handlers[MAX_IRQ_HANDLERS] = {0};
void handle_irq_c(uint32_t process_stack) {
    static uint32_t pending, irq;
    // switch to kernel page table
    // mmu_driver.set_l1_table((uint32_t*) (((uint32_t)l1_page_table - KERNEL_ENTRY) + DRAM_BASE));

    current_process->stack_top = (uint32_t*)process_stack;
    for(int reg = 0; reg < 3; reg++) {
        pending = INTC->IRQ_PEND[reg];
        while(pending) {
            irq = 32 * reg + __builtin_ctz(pending);
            if(irq_handlers[irq].handler) {
                irq_handlers[irq].handler(irq, irq_handlers[irq].data);
            }
            pending &= pending - 1;
        }
    }

    // if we were in a process, return to it
    // if (current_process) {
        // mmu_driver.set_l1_table((uint32_t*) current_process->ttbr0);
    // }
}

static void intc_init(void) {
    uint32_t vbar_addr = (uint32_t)_vectors;
    if(vbar_addr & 0x1F) {
        printk("VBAR unaligned! Fix %p → %p\n",
               vbar_addr, vbar_addr & ~0x1F);
        vbar_addr &= ~0x1F;
        panic("VBAR unaligned");
    }
    __asm__ volatile(
        "mcr p15, 0, %0, c12, c0, 0 \n"  /* Write VBAR */
        "dsb \n"
        "isb \n"
        :
        : "r" (vbar_addr)
    );

    // setup CSPR for IRQ mode
    __asm__ volatile(
        "mrs r0, cpsr \n"
        "bic r0, r0, #0x80 \n"
        "msr cpsr_c, r0 \n"
    );
}

static void enable_irq(uint32_t irq) {
    if(irq >= MAX_IRQ_HANDLERS) panic("IRQ too large");
    if (!irq_handlers[irq].handler) panic("No handler for IRQ %d", irq);
    INTC_IRQ_ENABLE(irq);
}

static void disable_irq(uint32_t irq) {
    if(irq >= MAX_IRQ_HANDLERS) panic("IRQ too large");
    INTC_IRQ_DISABLE(irq);
}

static void register_irq_handler(uint32_t irq, irq_handler_t handler, void *data) {
    if(irq >= MAX_IRQ_HANDLERS) panic("IRQ too large");
    irq_handlers[irq].handler = handler;
    irq_handlers[irq].data = data;
}

static void enable_irqs(void) {
    __asm__ volatile("cpsie i");
}

interrupt_controller_t interrupt_controller = {
    .init = intc_init,
    .register_irq = register_irq_handler,
    .enable_irq = enable_irq,
    .enable_irq_global = enable_irqs,
    .disable_irq = disable_irq,
};
