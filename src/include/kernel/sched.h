#ifndef KERNEL_SCHED_H
#define KERNEL_SCHED_H


typedef struct {
    uint32_t pid;
    uint32_t priority;
    uint32_t state;
    uint32_t stack;
    uint32_t sp;
    uint32_t pc;
    uint32_t lr;
    uint32_t cpsr;

    // rest of registers from user mode
} process_t;

int scheduler_init(void);

#endif // KERNEL_SCHED_H
