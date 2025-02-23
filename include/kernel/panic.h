#ifndef KERNEL_PANIC_H
#define KERNEL_PANIC_H
#include <kernel/log.h>
#include <kernel/printk.h>

// TODO - this can be a function and we should dump the stack and registers
// we could use log here, we would just need to dump the log buffer to the console before halting
#define panic(fmt, ...) do { \
    __asm__("cpsid i"); \
    printk("PANIC: " fmt, ##__VA_ARGS__); \
    printk(" at %s:%d\n", __FILE__, __LINE__); \
    printk("System halted.\n"); \
    while (1) { \
        __asm__("wfi");  \
    } \
    __builtin_unreachable();\
} while(0)

static inline void unimplemented_driver(void) {
    panic("Unimplemented driver: %s\n", __func__);
}

#endif
