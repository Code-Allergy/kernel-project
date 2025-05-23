#include <kernel/sleep.h>
#include <kernel/timer.h>
#include <kernel/list.h>
#include <kernel/sched.h>
#include <kernel/heap.h>
#include <kernel/printk.h> // For printk, if needed for error handling
#include <stdint.h>
#include <stddef.h> // For NULL and sizeof

// Global instance of the timing wheel
struct timing_wheel kernel_timing_wheel;

// Old sleep_queue_t related functions and data (if any) should be removed.
// The prompt asks to remove check_sleep_expiry(), which will be done by this overwrite.

void timing_wheel_init(struct timing_wheel *tw, uint32_t resolution_us) {
    if (!tw) {
        printk("timing_wheel_init: timing wheel pointer is NULL\n");
        return;
    }
    tw->wheel_size = NUM_SLOTS;
    tw->current_slot = 0;
    tw->resolution_us = resolution_us;

    for (uint32_t i = 0; i < tw->wheel_size; i++) {
        INIT_LIST_HEAD(&tw->slots[i]);
    }
    // printk("Timing wheel initialized. Resolution: %u us, Slots: %u\n", resolution_us, NUM_SLOTS);
}

void timing_wheel_add(struct timing_wheel *tw, process_t *proc, uint64_t delay_us) {
    if (!tw || !proc) {
        printk("timing_wheel_add: timing wheel or process pointer is NULL\n");
        return;
    }

    uint64_t current_ticks = clock_timer.get_ticks();
    uint64_t delay_ticks = clock_timer.us_to_ticks(delay_us);
    uint64_t target_wake_tick = current_ticks + delay_ticks;
    
    uint64_t ticks_per_slot = clock_timer.us_to_ticks(tw->resolution_us);

    if (ticks_per_slot == 0) {
        printk("timing_wheel_add: ticks_per_slot is zero. Resolution_us might be too low.\n");
        // Fallback or error: potentially add to current slot for immediate check, or handle error.
        // For now, let's add to current slot with 0 rotations, it will be checked in next tick.
        ticks_per_slot = 1; // Avoid division by zero, ensure it's processed soon.
    }

    uint64_t relative_ticks = delay_ticks; // ticks from now
    uint32_t relative_slots = relative_ticks / ticks_per_slot;
    uint32_t num_rotations = relative_slots / tw->wheel_size;
    uint32_t actual_slot_index = (tw->current_slot + relative_slots) % tw->wheel_size;

    timing_wheel_entry_t *entry = (timing_wheel_entry_t *)kmalloc(sizeof(timing_wheel_entry_t));
    if (!entry) {
        printk("timing_wheel_add: kmalloc failed for timing_wheel_entry_t\n");
        // Potentially set process to ready to avoid losing it, or handle error differently
        proc->state = PROCESS_READY; 
        return;
    }

    entry->process = proc;
    entry->target_wake_tick = target_wake_tick;
    entry->num_rotations = num_rotations;
    INIT_LIST_HEAD(&entry->link);

    list_add_tail(&entry->link, &tw->slots[actual_slot_index]);
    proc->state = PROCESS_SLEEPING;
    
    // printk("Process %d added to timing wheel. Slot: %u, Rotations: %u, Wake tick: %llu\n", proc->pid, actual_slot_index, num_rotations, target_wake_tick);
}

void timing_wheel_tick(struct timing_wheel *tw) {
    if (!tw) {
        printk("timing_wheel_tick: timing wheel pointer is NULL\n");
        return;
    }

    tw->current_slot = (tw->current_slot + 1) % tw->wheel_size;
    uint64_t current_ticks = clock_timer.get_ticks();

    struct list_head *current_list = &tw->slots[tw->current_slot];
    timing_wheel_entry_t *entry;
    timing_wheel_entry_t *tmp_entry; // n in the macro

    list_for_each_entry_safe(entry, timing_wheel_entry_t, tmp_entry, current_list, link) {
        if (entry->num_rotations > 0) {
            entry->num_rotations--;
        } else {
            // Check target_wake_tick to handle cases where a process might have been
            // scheduled for a very short duration, or if ticks_per_slot was very small.
            // Also handles the case where the tick interrupt might be delayed.
            if (current_ticks >= entry->target_wake_tick) {
                // printk("Waking up process %d from slot %u. Target: %llu, Current: %llu\n", entry->process->pid, tw->current_slot, entry->target_wake_tick, current_ticks);
                entry->process->state = PROCESS_READY;
                // TODO: Add to scheduler ready queue if applicable
                // scheduler_add_ready(entry->process); 
                list_del(&entry->link);
                kfree(entry);
            } else {
                // If current_ticks < target_wake_tick, it means we are in the correct slot
                // but not yet at the precise tick. This can happen if resolution_us is coarse.
                // It will be picked up in a subsequent tick if it remains in this slot
                // or handled by the num_rotations logic if it was for a future turn.
                // However, with current_ticks >= entry->target_wake_tick, this branch might not be hit
                // if the process is truly due. Consider if this logic is still needed.
                // For now, if it's not time yet, leave it. It will be re-checked if it stays in this slot
                // or if it was incorrectly placed due to tick resolution.
                // printk("Process %d in slot %u not yet ready. Target: %llu, Current: %llu, Rotations: %u\n", entry->process->pid, tw->current_slot, entry->target_wake_tick, current_ticks, entry->num_rotations);
            }
        }
    }
}

// The old check_sleep_expiry() function is now implicitly removed by overwriting the file.
// void check_sleep_expiry(void) { ... } // This is gone.
