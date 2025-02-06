#ifndef KERNEL_PANIC_H
#define KERNEL_PANIC_H

#define panic(fmt, ...) do { \
    printk("PANIC: " fmt, ##__VA_ARGS__); \
    printk(" at %s:%d\n", __FILE__, __LINE__); \
    printk("System halted.\n"); \
    while(1); \
} while(0)

#endif
