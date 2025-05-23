#include <kernel/sleep.h>
#include <kernel/timer.h>
#include <kernel/list.h>
#include <kernel/sched.h>
#include <kernel/heap.h>
#include <kernel/printk.h>
#include <stdint.h>
#include <stddef.h> // For NULL

// Global instance of the delta sleep queue head
// LIST_HEAD is a macro from kernel/list.h that initializes the list head.
LIST_HEAD(delta_sleep_queue);

// Initialize the delta list sleep queue
void delta_list_init(void) {
    // INIT_LIST_HEAD is already called by LIST_HEAD macro,
    // but calling it again ensures it's explicitly initialized if LIST_HEAD wasn't used
    // or if re-initialization is desired. Given LIST_HEAD is used, this is redundant
    // but harmless. If delta_sleep_queue were just `struct list_head delta_sleep_queue;`
    // then INIT_LIST_HEAD(&delta_sleep_queue); would be essential here.
    // Since LIST_HEAD does the init, this function can be empty or simply ensure it.
    // For clarity and safety if LIST_HEAD changes:
    INIT_LIST_HEAD(&delta_sleep_queue);
}

// Add a process to the delta list sleep queue
void delta_list_add(process_t *proc, uint64_t delay_us) {
    uint64_t total_delay_ticks = clock_timer.us_to_ticks(delay_us);

    // Ensure minimum 1 tick sleep if delay_us was > 0 but resulted in 0 ticks,
    // or if delay_us was 0. This simplifies logic by always having a positive delta.
    // This choice makes usleep(0) behave like a short sleep until the next tick.
    if (total_delay_ticks == 0) {
        total_delay_ticks = 1;
    }

    delta_list_entry_t *new_entry = (delta_list_entry_t *)kmalloc(sizeof(delta_list_entry_t));
    if (!new_entry) {
        printk("delta_list_add: kmalloc failed for new_entry\n");
        // Process state remains unchanged from before syscall, or set to ready if appropriate.
        // For now, if kmalloc fails, the process just won't sleep.
        // If it was already marked sleeping by syscall, it should be woken.
        // However, syscall marks it sleeping *after* this call.
        return; 
    }

    new_entry->process = proc;
    // proc->state will be set by the caller (e.g., sys_usleep) after this function returns successfully.

    struct list_head *iter_link;
    delta_list_entry_t *current_entry;
    uint64_t cumulative_ticks = 0;

    // list_for_each_entry is suitable here as we don't modify list structure during this iteration part
    // type for list_for_each_entry is delta_list_entry_t
    list_for_each_entry(current_entry, delta_list_entry_t, &delta_sleep_queue, link) {
        if (cumulative_ticks + current_entry->delta_ticks > total_delay_ticks) {
            // Insert new_entry before current_entry
            new_entry->delta_ticks = total_delay_ticks - cumulative_ticks;
            current_entry->delta_ticks -= new_entry->delta_ticks;
            // list_add_tail adds 'new' before 'head'. So, to add new_entry before current_entry,
            // we use current_entry->link as the 'head' for list_add_tail.
            list_add_tail(&new_entry->link, &current_entry->link);
            proc->state = PROCESS_SLEEPING; // Mark process as sleeping
            return;
        }
        cumulative_ticks += current_entry->delta_ticks;
    }

    // If loop finishes, new_entry is to be added at the tail of the actual list.
    new_entry->delta_ticks = total_delay_ticks - cumulative_ticks;
    list_add_tail(&new_entry->link, &delta_sleep_queue);
    proc->state = PROCESS_SLEEPING; // Mark process as sleeping
}

// Process the delta list on each system tick
void delta_list_tick(void) {
    if (list_empty(&delta_sleep_queue)) {
        return;
    }

    // Get the first entry (head of the queue)
    // list_first_entry is often list_entry((head)->next, type, member)
    delta_list_entry_t *head_entry = list_entry(delta_sleep_queue.next, delta_list_entry_t, link);

    if (head_entry->delta_ticks > 0) {
        head_entry->delta_ticks--;
    }

    // Wake up all processes whose delta_ticks have reached 0
    // Need to use _safe version if we are deleting from the list while iterating,
    // but here we are always processing from the head.
    while (!list_empty(&delta_sleep_queue)) {
        // Re-fetch head_entry in each iteration as the list changes
        head_entry = list_entry(delta_sleep_queue.next, delta_list_entry_t, link);
        if (head_entry->delta_ticks == 0) {
            head_entry->process->state = PROCESS_READY;
            // Optional: Add to scheduler's ready queue explicitly if needed by OS design
            // scheduler_add_ready(head_entry->process);
            list_del(&head_entry->link);
            kfree(head_entry);
            // Continue, as the new head might also be ready (delta 0 if it was the next one)
        } else {
            // Head of the list is not yet 0, so nothing else later in the list can be 0.
            break;
        }
    }
}
