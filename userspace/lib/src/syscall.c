#include "runtime.h"
#include "../include/syscall.h"

void syscall_debug(const char *str, uint32_t len) {
    asm volatile (
        "mov r7, %[syscall_num]   \n"  // Set syscall number (r7)
        "mov r0, %[arg1]          \n"  // Set arg1 (r0)
        "mov r1, %[arg2]          \n"  // Set arg2 (r1)
        "svc #0                   \n"  // Trigger the syscall
        :  // No output operands
        : [syscall_num] "r" (SYSCALL_DEBUG_NO), [arg1] "r" (str), [arg2] "r" (len)
        : "r0", "r1", "r7"  // Clobbered registers
    );
    // syscall_2(SYSCALL_DEBUG_NO, (uint32_t) str, len);
}

uint32_t getpid(void) {
    uint32_t retval;
    __asm__ volatile (
        "mov r7, %[syscall_num]   \n"  // Set syscall number (r7)
        "svc #0                   \n"  // Trigger the syscall
        "mov %[retval], r0        \n"  // Move return value from r0 to retval
        : [retval] "=r" (retval)    // Output operand: store the value of r0 in retval
        : [syscall_num] "r" (SYSCALL_GET_PID_NO)  // Input operand: pass syscall number in r7
        : "r7"  // Clobbered register: r7 is used for syscall number
    );

    return retval;  // Return the value stored in retval, which was in r0 after syscall
}

void exit(int return_val) {
    _exit(return_val); // actually do the exit (in runtime.c)
    __builtin_unreachable();
}
