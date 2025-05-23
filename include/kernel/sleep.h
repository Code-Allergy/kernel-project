#ifndef KERNEL_SLEEP_H
#define KERNEL_SLEEP_H

#include <stdint.h>
#include <kernel/list.h> // Required for list_head_t
#include <kernel/sched.h> // Required for process_t

// Timing wheel related definitions are removed.

// Function prototypes for timing wheel operations (removed)
// void timing_wheel_init(struct timing_wheel *tw, uint32_t resolution_us);
// void timing_wheel_add(struct timing_wheel *tw, process_t *proc, uint64_t delay_us);
// void timing_wheel_tick(struct timing_wheel *tw);

// Delta list entry structure
typedef struct delta_list_entry {
    process_t *process;
    uint64_t delta_ticks; // Relative ticks to wait after the previous entry expires
    struct list_head link;
} delta_list_entry_t;

// Declaration for the delta sleep queue head
extern struct list_head delta_sleep_queue;

// Function prototypes for delta list operations
void delta_list_init(void);
void delta_list_add(process_t *proc, uint64_t delay_us);
void delta_list_tick(void);

// extern sleep_queue_t sleep_queue; // Commented out old structure (remains commented)

// void check_sleep_expiry(void); // This function is now obsolete and removed.

#endif
