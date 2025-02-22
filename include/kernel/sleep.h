#ifndef KERNEL_SLEEP_H
#define KERNEL_SLEEP_H

#include <stdint.h>
#include <kernel/sched.h>

typedef struct sleep_queue {
    process_t* procs[MAX_PROCESSES];
    uint32_t count;
} sleep_queue_t;

extern sleep_queue_t sleep_queue;

void check_sleep_expiry(void);

#endif
