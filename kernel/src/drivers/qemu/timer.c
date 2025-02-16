#include <stddef.h>

#include <kernel/printk.h>
#include <kernel/intc.h>
#include <kernel/timer.h>
#include <kernel/panic.h>

#include "timer.h"
#include "intc.h"


#define SLEEP_TIMER 5

volatile uint64_t sleep_until = 0;


static void timer_init(void) {
    clock_timer.initialized = 1;
    // whatever else at runtime
}

static void system_clock(int irq, void* data) {
    (void)data;
    printk("Clock activated! irq %d\n", irq);
    AW_Timer *t = (AW_Timer*) TIMER_BASE;
    t->irq_status ^= (get_timer_idx_from_irq(irq) << 0);
}

void timer_start(int timer_idx, uint32_t interval_us) {
    AW_Timer *t = (AW_Timer*) TIMER_BASE;

    t->irq_enable = 0x3F; // Enable timer 1 and 2 interrupts
    t->timer[timer_idx].control = 0; // Disable first
    t->timer[timer_idx].interval = interval_us * 24; // 24MHz

    // Control: Enable + Reload + 24MHz clock (bits 2-3 = 0b01) + IRQ
    t->timer[timer_idx].control = TIMER_ENABLE | TIMER_RELOAD | (1 << 2) | TIMER_IRQ_EN;

    /* setup interrupt handler */
    interrupt_controller.register_irq(get_timer_irq_idx(timer_idx), system_clock, NULL);
    interrupt_controller.enable_irq(get_timer_irq_idx(timer_idx));
}

void handle_callback(int irq, void* __attribute__((unused)) data) {
    uint32_t timer_idx = get_timer_idx_from_irq(irq);
    if (clock_timer.callbacks[timer_idx] == NULL) {
        panic("No callback set for timer %d\n", timer_idx);
    }
    // reset the timer
    AW_Timer *t = (AW_Timer*) TIMER_BASE;
    t->irq_status ^= (get_timer_idx_from_irq(irq) << 0);
    // call the callback
    clock_timer.callbacks[timer_idx]();

}

void timer_start_callback(int timer_idx, uint32_t interval_us, void (*callback)(void)) {
    AW_Timer *t = (AW_Timer*) TIMER_BASE;
    if (timer_idx >= (int)clock_timer.total) panic("Timer index out of bounds\n");

    t->irq_enable = (1 << timer_idx);
    t->timer[timer_idx].control = 0;
    t->timer[timer_idx].interval = interval_us * 24; // 24MHz clock selected

    // Control: Enable + Reload + 24MHz clock (bits 2-3 = 0b01) + IRQ
    t->timer[timer_idx].control = TIMER_ENABLE | TIMER_RELOAD | (1 << 2) | TIMER_IRQ_EN;

    clock_timer.callbacks[timer_idx] = callback;
    clock_timer.available--;

    /* setup interrupt handler */
    interrupt_controller.register_irq(get_timer_irq_idx(timer_idx), handle_callback, NULL);
    interrupt_controller.enable_irq(get_timer_irq_idx(timer_idx));
}



void handle_oneshot_callback(int timer_idx, void (*callback)(void)) {
    // todo
}

timer_t clock_timer = {
    .available = 6,
    .total = 6,
    .init = timer_init,
    .start_idx = timer_start, // todo fix
    .start_idx_callback = timer_start_callback, // todo fix
};
