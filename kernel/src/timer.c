#include <kernel/sched.h>
#include <kernel/timer.h>
#include <kernel/sleep.h> // Added include for timing_wheel_tick and kernel_timing_wheel

// system clock
void system_clock(void) {
    timing_wheel_tick(&kernel_timing_wheel); // Process any due sleep events
    scheduler_driver.tick();
}

void start_kernel_clocks(void) {
    clock_timer.start_idx_callback(0, KERNEL_HEARTBEAT_TIMER, system_clock);
}
