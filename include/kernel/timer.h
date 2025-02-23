#ifndef KERNEL_TIMER_H
#define KERNEL_TIMER_H
#include <stdint.h>


#define KERNEL_HEARTBEAT_TIMER 3000 // should be in USEC and should be universal for all platforms
#define MAX_TIMERS 16

typedef void (*timer_callback_t)(void);

typedef struct {
    uint64_t global_ticks; // NEVER READ THIS DIRECTLY! it is updated by interrupt. Current tick count since uptime

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

    // get global ticks
    uint64_t (*get_ticks)(void);

    // get global ticks as nanoseconds
    uint64_t (*ticks_to_ns)(uint64_t ticks);
    uint64_t (*ns_to_ticks)(uint64_t ticks);
    uint64_t (*ticks_to_us)(uint64_t ticks);
    uint64_t (*us_to_ticks)(uint64_t us);
    uint64_t (*ticks_to_ms)(uint64_t ticks);
    uint64_t (*ms_to_ticks)(uint64_t ms);
} timer_t;

extern timer_t clock_timer;

// outer interface
void start_kernel_clocks(void); // timer.c


#endif
