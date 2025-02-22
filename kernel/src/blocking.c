#include <kernel/sched.h>


typedef struct blocked_queue {
    struct pcb* procs[MAX_PROCESSES];
    uint32_t count;
} blocked_queue_t;

blocked_queue_t blocked_queue;

// wake up the first process that is blocked on an event
void check_blocked_events(void) {
    for (uint32_t i = 0; i < blocked_queue.count; ) {
//         if (event_occurred(blocked_queue.procs[i]->wait_event)) {
//             wake_process(blocked_queue.procs[i]);
//             remove_from_blocked_queue(i);
        // } else {
            // i++;
        // }
    }
}
