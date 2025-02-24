#include "runtime.h"
#include <syscalls.h>
#include <time.h>

void syscall_debug(const char *str, uint32_t len) {
    syscall_2(SYSCALL_DEBUG_NO, (uint32_t) str, len);
}

uint32_t getpid(void) {
    return syscall_0(SYSCALL_GET_PID_NO);
}

int yield(void) {
    return syscall_0(SYSCALL_YIELD_NO);
}

int fork(void) {
    return syscall_0(SYSCALL_FORK_NO);
}

void exit(int return_val) {
    _exit(return_val); // actually do the exit (in runtime.c)
    __builtin_unreachable();
}

// int readdir(int fd, void *buf, size_t len) {
//     return syscall_3(SYSCALL_READDIR_NO, fd, (uint32_t) buf, len);
// }

int open(const char *path, int flags, int mode) {
    return syscall_3(SYSCALL_OPEN_NO, (uint32_t) path, flags, mode);
}

ssize_t read(int fd, void *buf, size_t count) {
    return syscall_3(SYSCALL_READ_NO, fd, (uint32_t) buf, count);
}

ssize_t write(int fd, const void *buf, size_t count) {
    return syscall_3(SYSCALL_WRITE_NO, fd, (uint32_t) buf, count);
}

uint64_t time(void) {
    uint64_t epoch;
    if (syscall_1(SYSCALL_TIME_NO, (uint32_t) &epoch) != 0) return 0;
    return epoch;
}

int gettimeofday(struct timeval *tv, struct timezone *tz) {
    return syscall_2(SYSCALL_GETTIMEOFDAY_NO, (uint32_t) tv, (uint32_t) tz);
}

int usleep(uint64_t usec) {
    return syscall_2(SYSCALL_USLEEP_NO, (uint32_t)(usec >> 32), (uint32_t)usec);
}

int lseek(int fd, int offset, int mode) {
    return syscall_3(SYSCALL_LSEEK_NO, fd, offset, mode);
}

int exec(const char* path) {
    return syscall_1(SYSCALL_EXEC_NO, (uint32_t) path);
}
