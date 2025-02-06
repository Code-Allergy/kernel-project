#ifndef KERNEL_INTC_H
#define KERNEL_INTC_H

#include <stdint.h>

#define INTC_IRQ_NO_HANDLER -1
#define INTC_IRQ_TOO_LARGE -2

typedef void (*irq_handler_t)(int irq, void *data);
struct irq_entry {
    irq_handler_t handler;
    void *data;
};

typedef struct InterruptController {
    void (*init)(void);           // Initialize the controller
    void (*register_irq)(uint32_t, irq_handler_t, void*); // Register an IRQ handler
    void (*enable_irq_global)(void); // Enable IRQs globally
    void (*enable_irq)(uint32_t); // Enable a specific IRQ
    void (*disable_irq)(uint32_t);// Disable a specific IRQ
    void (*eoi)(uint32_t);        // Send End-Of-Interrupt signal
} interrupt_controller_t;

extern interrupt_controller_t interrupt_controller;

#endif /* KERNEL_INTC_H */
