#include <kernel/spinlock.h>

void spinlock_init(spinlock_t *lock) {
    lock->lock = 0;
    lock->cpsr = 0;
}

void spinlock_acquire(spinlock_t *lock) {
    // save current cspr and disable IRQs
    uint32_t cpsr;
    __asm__ __volatile__("mrs %0, cpsr\n cpsid i" : "=r" (cpsr) : : "memory");
    lock->cpsr = cpsr;

    // stomically acquire the lock
    while (__atomic_test_and_set(&lock->lock, __ATOMIC_ACQUIRE)) {
        // does nothing on single core machine
        __asm__ __volatile__("yield");
    }
}

void spinlock_release(spinlock_t *lock) {
    __atomic_clear(&lock->lock, __ATOMIC_RELEASE);
    __asm__ __volatile__("msr cpsr_c, %0" : : "r" (lock->cpsr));
}
