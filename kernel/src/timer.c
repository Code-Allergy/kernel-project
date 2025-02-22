#include <kernel/sched.h>
#include <kernel/timer.h>

// system clock
void system_clock(void) {
    scheduler_driver.tick();
}

void start_kernel_clocks(void) {
    clock_timer.start_idx_callback(0, KERNEL_HEARTBEAT_TIMER, system_clock);
}
