#ifndef KERNEL_PANIC_H
#define KERNEL_PANIC_H
#include <kernel/string.h>
#include <kernel/log.h>
#include <kernel/printk.h>

// TODO - this can be a function and we should dump the stack and registers
// we could use log here, we would just need to dump the log buffer to the console before halting
#define panic(fmt, ...) do {\
    _panic(__FILE__, __LINE__, fmt, ##__VA_ARGS__);\
    __builtin_unreachable(); \
} while(0)
__attribute__((noreturn)) void _panic(const char* file, int line, const char *fmt, ...);


static inline void unimplemented_driver(void) {
    panic("Unimplemented driver: %s\n", __func__);
}

#endif
