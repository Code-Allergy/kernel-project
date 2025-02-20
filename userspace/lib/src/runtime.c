#include <syscalls.h>

extern int main(void);
void _start(void);
void _exit(int return_val);


__attribute__((noreturn)) void _exit(int return_val) {
    syscall_1(SYSCALL_EXIT_NO, return_val);
    __builtin_unreachable();
}

void __attribute__((section(".text.startup"))) _start(void) {
    int ret = main();

    _exit(ret);
}
