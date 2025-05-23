#ifndef KERNEL_SLEEP_H
#define KERNEL_SLEEP_H

#include <stdint.h>
#include <kernel/list.h> // Required for list_head_t
#include <kernel/sched.h> // Required for process_t

#define NUM_SLOTS 256 // Example size, can be tuned

// Entry for the timing wheel
typedef struct timing_wheel_entry {
    list_head_t link;         // Link for the list in the slot
    uint64_t target_wake_tick; // Tick value when the process should wake up
    process_t *process;       // Pointer to the process
    uint32_t num_rotations;   // Number of wheel rotations before this entry is considered
} timing_wheel_entry_t;

// Timing wheel structure
typedef struct timing_wheel {
    list_head_t slots[NUM_SLOTS]; // Array of linked list heads
    uint32_t current_slot;        // Index of the current slot
    uint32_t wheel_size;          // Total number of slots (NUM_SLOTS)
    uint32_t resolution_us;       // Time duration each slot represents in microseconds
} timing_wheel_t;

extern struct timing_wheel kernel_timing_wheel; // Declare global instance

// Function prototypes for timing wheel operations
void timing_wheel_init(struct timing_wheel *tw, uint32_t resolution_us);
void timing_wheel_add(struct timing_wheel *tw, process_t *proc, uint64_t delay_us);
void timing_wheel_tick(struct timing_wheel *tw);

// extern sleep_queue_t sleep_queue; // Commented out old structure

// void check_sleep_expiry(void); // This function is now obsolete and removed.

#endif
