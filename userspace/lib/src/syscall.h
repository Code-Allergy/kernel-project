#ifndef __LIB__SYSCALL_H__
#define __LIB__SYSCALL_H__
#include <stdint.h>

#define SYSCALL_DEBUG_NO 0
#define SYSCALL_EXIT_NO 1

static inline void syscall_0(uint32_t syscall_num) {
    asm volatile (
        "mov r7, %[syscall_num]   \n"  // Set syscall number (r7)
        "svc #0                   \n"  // Trigger the syscall
        :  // No output operands
        : [syscall_num] "r" (syscall_num)
        : "r7"  // Clobbered registers
    );
}


static inline void syscall_1(uint32_t syscall_num, uint32_t arg1) {
    asm volatile (
        "mov r7, %[syscall_num]   \n"  // Set syscall number (r7)
        "mov r0, %[arg1]          \n"  // Set arg1 (r0)
        "svc #0                   \n"  // Trigger the syscall
        :  // No output operands
        : [syscall_num] "r" (syscall_num), [arg1] "r" (arg1)
        : "r0", "r7"  // Clobbered registers
    );
}

static inline void syscall_2(uint32_t syscall_num, uint32_t arg1, uint32_t arg2) {
    asm volatile (
        "mov r7, %[syscall_num]   \n"  // Set syscall number (r7)
        "mov r0, %[arg1]          \n"  // Set arg1 (r0)
        "mov r1, %[arg2]          \n"  // Set arg2 (r1)
        "svc #0                   \n"  // Trigger the syscall
        :  // No output operands
        : [syscall_num] "r" (syscall_num), [arg1] "r" (arg1), [arg2] "r" (arg2)
        : "r0", "r1", "r7"  // Clobbered registers
    );
}


#endif
