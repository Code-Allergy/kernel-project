#ifndef KERNEL_SPINLOCK_H
#define KERNEL_SPINLOCK_H

#include <stdint.h>

// spinlock struct supporting both irqless and irqfull spinlocks
typedef struct {
    volatile uint32_t lock;
    uint32_t cpsr;
} spinlock_t;

// initial value that a spinlock should have
#define SPINLOCK_INIT { .lock = 0, .cpsr = 0 }

void spinlock_init(spinlock_t *lock);
void spinlock_acquire(spinlock_t *lock);
void spinlock_release(spinlock_t *lock);

#endif // KERNEL_SPINLOCK_H
