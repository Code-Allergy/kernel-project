#ifndef KERNEL_INT_H
#define KERNEL_INT_H


static inline void disable_interrupts(void) {
    __asm__ volatile("cpsid i");
}

static inline void enable_interrupts(void) {
    __asm__ volatile("cpsie i");
}


#endif // KERNEL_INT_H
