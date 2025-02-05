#ifndef ALLWINNER_A10_TIMER_H
#define ALLWINNER_A10_TIMER_H

#include <stdint.h>

typedef struct {
    // Global registers (0x00-0x0C)
    volatile uint32_t irq_enable;    // 0x00
    volatile uint32_t irq_status;    // 0x04
    volatile uint32_t reserved[2];   // 0x08-0x0C
    
    // Timer blocks (6 timers, 0x10-0x7F)
    struct {
        volatile uint32_t control;   // +0x00
        volatile uint32_t interval;  // +0x04
        volatile uint32_t count;     // +0x08
        volatile uint32_t reserved;  // +0x0C
    } timer[6];                      // 0x10-0x70
    
    // Padding between timers and watchdog (0x80-0x8F)
    volatile uint32_t reserved1[8];  // 0x70-0x8F

    // Watchdog and count registers (0x90-0xA8)
    volatile uint32_t wdog_control;  // 0x90
    volatile uint32_t wdog_mode;     // 0x94
    volatile uint32_t reserved2[3];  // 0x98-0xA4
    volatile uint32_t count_ctl;     // 0xA0
    volatile uint32_t count_lo;      // 0xA4
    volatile uint32_t count_hi;      // 0xA8

    volatile uint32_t reserved3[0x400/4 - 0xAC/4];
} AW_Timer;

#define TIMER_BASE 0x01C20C00
#define TIMER0 ((AW_Timer *)TIMER_BASE)

enum {
    TIMER0_IDX = 0,
    TIMER1_IDX = 1,
    TIMER2_IDX = 2,
    TIMER3_IDX = 3,
    TIMER4_IDX = 4,
    TIMER5_IDX = 5,
};

static inline uint32_t get_timer_irq_idx(uint32_t timer_idx) {
    static const uint32_t lookup_table[] = {22, 23, 24, 25, 67, 68};
    return (timer_idx < 6) ? lookup_table[timer_idx] : 0;
}

void timer_init(uint64_t interval_us, int timer_idx);

// Control register bits
#define TIMER_ENABLE    (1 << 0)
#define TIMER_RELOAD    (1 << 1)
#define TIMER_ONESHOT   (1 << 7)
#define TIMER_IRQ_EN    (1 << 4)

// Timer selection (use timer 1 for general purpose)
#define TIMER_SELECT 1
#define TIMER_IRQ_NUM TIMER_SELECT

#endif