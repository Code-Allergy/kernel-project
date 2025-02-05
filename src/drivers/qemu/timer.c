#include "timer.h"
#include <kernel/printk.h>
#include <stddef.h>

#include "intc.h"


#define SLEEP_TIMER 5

volatile uint64_t sleep_until = 0;


// TEMP in here: this can be kernel heartbeat timer
void timer_handler(int irq, void *data) {
    AW_Timer *t = TIMER_BASE;
    
    // Clear timer interrupt
    t->irq_status ^= (1 << 1); // Toggle bit 1
    INTC->IRQ_PEND[0] = (1 << 1); 
    
    printk("IRQ %d: Timer fired!\n", irq);
}


void set_timer_usec(uint32_t usec, uint32_t timer_idx, uint32_t flags) {
    AW_Timer *t = TIMER_BASE;
    
    t->timer[timer_idx].control = 0; // Disable first
    t->timer[timer_idx].interval = 24 * usec; // 24MHz
    
    // Control: Enable + Reload + 24MHz clock (bits 2-3 = 0b01) + IRQ
    t->timer[timer_idx].control = flags;
}


void sleep_us(uint32_t usec) {
    AW_Timer *t = TIMER_BASE;
    const int timer_idx = 0;
    set_timer_usec(usec, SLEEP_TIMER, TIMER_ENABLE | TIMER_RELOAD | (1 << 2) | TIMER_IRQ_EN | TIMER_ONESHOT);
    
    // Wait for timer to finish
    while(!(t->irq_status & (1 << get_timer_irq_idx(SLEEP_TIMER))));
    
    // Clear timer interrupt
    t->irq_status = 1 << get_timer_irq_idx(SLEEP_TIMER);
}

void timer_init(uint64_t interval_us, int timer_idx) {
    AW_Timer *t = TIMER_BASE;

    t->irq_enable = 0x3F; // Enable timer 1 and 2 interrupts
    t->timer[timer_idx].control = 0; // Disable first
    t->timer[timer_idx].interval = interval_us * 24; // 24MHz
    
    // Control: Enable + Reload + 24MHz clock (bits 2-3 = 0b01) + IRQ
    t->timer[timer_idx].control = TIMER_ENABLE | TIMER_RELOAD | (1 << 2) | TIMER_IRQ_EN;
    
    /* setup interrupt handler */
    request_irq(get_timer_irq_idx(timer_idx), timer_handler, NULL);
}