#ifndef ALLWINNER_A10_INTC_H
#define ALLWINNER_A10_INTC_H

#include <stdint.h>

#define INTC_BASE 0x01C20400

typedef struct {
    volatile uint32_t VECTOR;         // 0x00 - Interrupt Vector
    volatile uint32_t BASE_ADDR;      // 0x04 - Interrupt Base Address
    volatile uint32_t PROT_EN;        // 0x08 - Interrupt Protection
    volatile uint32_t NMI_CTRL;       // 0x0C - NMI Control
    
    // Pending registers (0x10-0x1C)
    volatile uint32_t IRQ_PEND[3];    // 0x10, 0x14, 0x18
    uint32_t _reserved0[1];           // 0x1C padding
    
    // FIQ pending (0x20-0x28)
    volatile uint32_t FIQ_PEND[3];    // 0x20, 0x24, 0x28
    uint32_t _reserved1[1];           // 0x2C padding
    
    // Interrupt type selection (0x30-0x38)
    volatile uint32_t IRQ_TYPE_SEL[3];// 0x30, 0x34, 0x38
    uint32_t _reserved2[1];           // 0x3C padding
    
    // Enable registers (0x40-0x48)
    volatile uint32_t ENABLE[3];      // 0x40, 0x44, 0x48
    uint32_t _reserved3[1];           // 0x4C padding
    
    // Mask registers (0x50-0x58)
    volatile uint32_t MASK[3];        // 0x50, 0x54, 0x58
    uint32_t _reserved4[1];           // 0x5C padding
} AllwinnerINTC;

#define INTC ((AllwinnerINTC *)INTC_BASE)
#define PIC ((AllwinnerINTC *)INTC_BASE)

// Macros for common operations
#define INTC_IRQ_ENABLE(n)    (INTC->ENABLE[(n)/32] |= (1U << ((n)%32)))
#define INTC_IRQ_DISABLE(n)   (INTC->ENABLE[(n)/32] &= ~(1U << ((n)%32)))
#define INTC_IRQ_MASK(n)      (INTC->MASK[(n)/32] |= (1U << ((n)%32)))
#define INTC_IRQ_UNMASK(n)    (INTC->MASK[(n)/32] &= ~(1U << ((n)%32)))
#define INTC_IRQ_IS_PENDING(n) (INTC->IRQ_PEND[(n)/32] & (1U << ((n)%32)))
#define INTC_IRQ_SET_TYPE(n,t) (INTC->IRQ_TYPE_SEL[(n)/32] = \
                                (INTC->IRQ_TYPE_SEL[(n)/32] & ~(3U << (2*((n)%32)))) | \
                                ((t) << (2*((n)%32))))

// Priority level macros
#define INTC_PRIORITY_LEVEL0 0
#define INTC_PRIORITY_LEVEL1 1
#define INTC_PRIORITY_LEVEL2 2
#define INTC_PRIORITY_LEVEL3 3

#define INTC_SET_PRIORITY(n, level) \
    do { \
        const uint32_t reg_idx = (n) / 8; \
        const uint32_t shift = ((n) % 8) * 4; \
        INTC->PRIORITY[reg_idx] = (INTC->PRIORITY[reg_idx] & ~(0xFU << shift)) | \
                                  ((level) << shift); \
    } while(0)

void setup_stacks();
void setup_cspr();
void uart_handler(int irq, void *data);

// Interrupt types
#define INTC_TYPE_LEVEL_LOW    0
#define INTC_TYPE_LEVEL_HIGH   1
#define INTC_TYPE_EDGE_FALLING 2
#define INTC_TYPE_EDGE_RISING  3

#define MAX_IRQ_HANDLERS 96
typedef void (*irq_handler_t)(int irq, void *data);

struct irq_entry {
    irq_handler_t handler;
    void *data;
};

int register_irq_handler(uint32_t irq, irq_handler_t handler, void *data);
int enable_irq(uint32_t irq);
int disable_irq(uint32_t irq);
void irq_handler(void);
int intc_init(void);

#endif // ALLWINNER_A10_INTC_H