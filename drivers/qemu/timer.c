#include <stddef.h>

#include <kernel/printk.h>
#include <kernel/intc.h>
#include <kernel/timer.h>
#include <kernel/panic.h>

#include "timer.h"
#include "intc.h"


#define SLEEP_TIMER 5

volatile uint64_t sleep_until = 0;

void handle_irq(int irq, void* data) {
    if (TIMER0->irq_status & (1 << 0)) {
        TIMER0->irq_status |= (1 << 0);   // Clear interrupt
        clock_timer.global_ticks += 0x100000000ULL;    // Increment high bits
    }
}

static void system_tick_clock(int idx) {
    TIMER0->irq_enable = (1 << idx);
    TIMER0->timer[idx].interval = 0xFFFFFFFF; // Max interval (32-bit)
    // TIMER0->timer[idx].control = (1 << 7) | // Auto-reload
    //                     (0 << 4) | // Prescaler = 1 (24 MHz clock)
    //                     TIMER_RELOAD | // Start timer
    //                     TIMER_ENABLE | (1 << 2);  // Enable timer

    TIMER0->timer[idx].control = TIMER_ENABLE | TIMER_RELOAD | (1 << 2);

    interrupt_controller.register_irq(get_timer_irq_idx(idx), handle_irq, NULL);
    interrupt_controller.enable_irq(get_timer_irq_idx(idx));
}

static void timer_init(void) {
    clock_timer.global_ticks = 0;
    clock_timer.initialized = 1;
    system_tick_clock(TIMER1_IDX);

    // whatever else at runtime
}

__attribute__((noreturn)) void system_clock(int irq, void* data);

void timer_start(int timer_idx, uint32_t interval_us) {
    AW_Timer *t = (AW_Timer*) TIMER_BASE;

    t->irq_enable = 0x3F; // Enable timer 1 and 2 interrupts
    t->timer[timer_idx].control = 0; // Disable first
    t->timer[timer_idx].interval = interval_us * 24; // 24MHz

    // Control: Enable + Reload + 24MHz clock (bits 2-3 = 0b01) + IRQ
    t->timer[timer_idx].control = TIMER_ENABLE | TIMER_RELOAD | (1 << 2);

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
    t->timer[timer_idx].control = TIMER_ENABLE | TIMER_RELOAD | (1 << 2);

    clock_timer.callbacks[timer_idx] = callback;
    clock_timer.available--;

    /* setup interrupt handler */
    interrupt_controller.register_irq(get_timer_irq_idx(timer_idx), handle_callback, NULL);
    interrupt_controller.enable_irq(get_timer_irq_idx(timer_idx));
}



void handle_oneshot_callback(int timer_idx, void (*callback)(void)) {
    // todo
}

uint64_t get_ticks(void) {
    uint32_t high_old, high_new;
    uint32_t low;

    do {
        high_old = (clock_timer.global_ticks >> 32);   // Read high bits
        low = TIMER0->timer[TIMER1_IDX].count;              // Read current timer value (lower 32 bits)
        high_new = (clock_timer.global_ticks >> 32);   // Re-read high bits (check for overflow)
    } while (high_old != high_new);        // Repeat if overflow occurred during read

    return ((uint64_t)high_new << 32) | (0xFFFFFFFFULL - low);
}

uint64_t ticks_to_ns(uint64_t ticks) {
    uint64_t seconds = ticks / TIMER_FREQ;
    uint64_t remainder_ticks = ticks % TIMER_FREQ;
    return (seconds * 1000000000ULL) + ((remainder_ticks * 1000000000ULL) / TIMER_FREQ);
}

timer_t clock_timer = {
    .available = 6,
    .total = 6,
    .init = timer_init,
    .start_idx = timer_start, // todo fix
    .start_idx_callback = timer_start_callback, // todo fix
    .get_ticks = get_ticks,
    .ticks_to_ns = ticks_to_ns,
};
