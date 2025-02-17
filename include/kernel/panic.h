#ifndef KERNEL_PANIC_H
#define KERNEL_PANIC_H
#include <kernel/printk.h>

#define panic(fmt, ...) do { \
    printk("PANIC: " fmt, ##__VA_ARGS__); \
    printk(" at %s:%d\n", __FILE__, __LINE__); \
    printk("System halted.\n"); \
    while(1); \
} while(0)

static inline void unimplemented_driver(void) {
    panic("Unimplemented driver: %s\n", __func__);
}

#endif
