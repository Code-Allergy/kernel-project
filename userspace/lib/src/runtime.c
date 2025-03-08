// ctr0
// TODO :
// environ
// argc, argv passed on stack in main
// set heap pointer, initialize simple allocator
#include <syscalls.h>

extern int main(void);
void _start(void);
void _exit(int return_val);
#define debug(value) syscall_debug(value, sizeof(value))


__attribute__((noreturn)) void _exit(int return_val) {
    syscall_1(SYSCALL_EXIT_NO, return_val);
    __builtin_unreachable();
}

void __attribute__((section(".text.startup"))) _start(void) {
    int ret = main();

    _exit(ret);
}
