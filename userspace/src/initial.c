#include <stdint.h>
#define SYS_write 0  // Syscall number for writing (debugging)

void sys_write(const char* ptr, uint32_t len);

#define debug(fmt) syscall_2(0, fmt, sizeof(fmt))
#define sleep(len) syscall_1(1, len)

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

void _start(void) {
    debug("Hello, World!\n");

    debug("Exiting...\n");

    // Exit the program (you can implement this with another syscall if needed)
    while (1);
}