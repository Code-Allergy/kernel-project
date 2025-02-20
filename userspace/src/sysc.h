#ifndef _SYSC_H
#define _SYSC_H
#include <stdint.h>
#define SYS_write 0  // Syscall number for writing (debugging)

void sys_write(const char* ptr, uint32_t len);

static inline uint32_t syscall_0(uint32_t syscall_num) {
    uint32_t result;
    asm volatile (
        "mov r7, %[syscall_num]   \n"  // Set syscall number (r7)
        "svc #0                   \n"  // Trigger the syscall
        "mov %[result], r0        \n"  // Store return value from r0
        : [result] "=r" (result)       // Output: store r0 in result
        : [syscall_num] "r" (syscall_num)
        : "r7"  // Clobbered registers
    );
    return result;
}


static inline uint32_t syscall_1(uint32_t syscall_num, uint32_t arg1) {
    uint32_t result;
    asm volatile (
        "mov r7, %[syscall_num]   \n"  // Set syscall number (r7)
        "mov r0, %[arg1]          \n"  // Set arg1 (r0)
        "svc #0                   \n"  // Trigger the syscall
        "mov %[result], r0        \n"  // Store return value from r0
        : [result] "=r" (result)       // Output: store r0 in result
        : [syscall_num] "r" (syscall_num), [arg1] "r" (arg1)
        : "r0", "r7"  // Clobbered registers
    );
    return result;
}

static inline uint32_t syscall_2(uint32_t syscall_num, uint32_t arg1, uint32_t arg2) {
    uint32_t result;
    asm volatile (
        "mov r7, %[syscall_num]   \n"  // Set syscall number (r7)
        "mov r0, %[arg1]          \n"  // Set arg1 (r0)
        "mov r1, %[arg2]          \n"  // Set arg2 (r1)
        "svc #0                   \n"  // Trigger the syscall
        "mov %[result], r0        \n"  // Store return value from r0
        : [result] "=r" (result)       // Output: store r0 in result
        : [syscall_num] "r" (syscall_num), [arg1] "r" (arg1), [arg2] "r" (arg2)
        : "r0", "r1", "r7"  // Clobbered registers
    );
    return result;
}

static inline uint32_t syscall_3(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    uint32_t result;
    asm volatile (
        "mov r7, %[syscall_num]   \n"  // Set syscall number (r7)
        "mov r0, %[arg1]          \n"  // Set arg1 (r0)
        "mov r1, %[arg2]          \n"  // Set arg2 (r1)
        "mov r2, %[arg3]          \n"  // Set arg3 (r2)
        "svc #0                   \n"  // Trigger the syscall
        "mov %[result], r0        \n"  // Store return value from r0
        : [result] "=r" (result)       // Output: store r0 in result
        : [syscall_num] "r" (syscall_num), [arg1] "r" (arg1),
          [arg2] "r" (arg2), [arg3] "r" (arg3)  // Inputs
        : "r0", "r1", "r2", "r7", "memory"  // Clobbered registers
    );
    return result;
}


#define debug(fmt) syscall_2(0, fmt, sizeof(fmt))
// #define sleep(len) syscall_1(1, len)
#define exit(ret) syscall_1(1, ret)
#define getpid() syscall_0(2)
#define yield() syscall_0(3)

#endif
