#ifndef KERNEL_PANIC_H
#define KERNEL_PANIC_H
#include <kernel/string.h>
#include <kernel/log.h>
#include <kernel/printk.h>

// TODO - this can be a function and we should dump the stack and registers
// we could use log here, we would just need to dump the log buffer to the console before halting
__attribute((noreturn)) void panic(const char *fmt, ...);


static inline void unimplemented_driver(void) {
    panic("Unimplemented driver: %s\n", __func__);
}

#endif
