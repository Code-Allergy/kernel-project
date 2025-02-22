#include <kernel/sleep.h>
#include <kernel/time.h>
#include <kernel/timer.h>

sleep_queue_t sleep_queue;

// only wake up the top process that is sleeping
void check_sleep_expiry(void) {
    uint64_t current_ticks = clock_timer.get_ticks();

    for (uint32_t i = 0; i < sleep_queue.count; ) {
        process_t* proc = sleep_queue.procs[i];

        if (current_ticks >= proc->wake_ticks) {
            proc->state = PROCESS_READY;

            // Remove from queue (swap with last)
            sleep_queue.procs[i] = sleep_queue.procs[--sleep_queue.count];
        } else {
            i++;  // Proceed if not expired
        }
    }
}
