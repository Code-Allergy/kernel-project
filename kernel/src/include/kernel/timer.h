#ifndef KERNEL_TIMER_H
#define KERNEL_TIMER_H
#include <stdint.h>

#define MAX_TIMERS 256

typedef void (*timer_callback_t)(void);

typedef struct {
    uint32_t available;
    uint32_t total;
    uint32_t initialized;

    timer_callback_t callbacks[MAX_TIMERS];

    // Init whatever is needed for the timers before any can be started.
    void (*init)(void);

    // Start any timer with usec delay, then call the callback, and repeat every usec
    void (*start)(uint32_t usec, void (*callback)(void));

    // Start a timer idx with usec delay, and repeat every usec
    void (*start_idx)(uint32_t idx, uint32_t usec);

    // Start any timer with usec delay, then call the callback, and repeat every usec
    void (*start_idx_callback)(uint32_t idx, uint32_t usec, timer_callback_t callback);

    // Start a timer idx with usec delay, then call the callback once and unregister the timer
    void (*init_idx_oneshot)(uint32_t idx, uint32_t usec, timer_callback_t callback);
} timer_t;

extern timer_t clock_timer;

#endif
