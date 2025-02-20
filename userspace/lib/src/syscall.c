#include "runtime.h"
#include <syscalls.h>

void syscall_debug(const char *str, uint32_t len) {
    syscall_2(SYSCALL_DEBUG_NO, (uint32_t) str, len);
}

uint32_t getpid(void) {
    return syscall_0(SYSCALL_GET_PID_NO);
}

int fork(void) {
    return syscall_0(SYSCALL_FORK_NO);
}

void exit(int return_val) {
    _exit(return_val); // actually do the exit (in runtime.c)
    __builtin_unreachable();
}
