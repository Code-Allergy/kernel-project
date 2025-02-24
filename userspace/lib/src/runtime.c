// ctr0

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
    // initialize file descriptors
    int fd;

    fd = open("/dev/uart0", OPEN_MODE_READ | OPEN_MODE_NOBLOCK, 0);
    if (fd < 0) {
        debug("Failed to load stdin for process\n");
        _exit(-1);
    }

    fd = open("/dev/uart0", OPEN_MODE_WRITE, 0);
    if (fd < 0) {
        debug("Failed to load stdout for process\n");
        _exit(-1);
    }

    fd = open("/dev/uart0", OPEN_MODE_WRITE, 0);
    if (fd < 0) {
        debug("Failed to load stderr for process\n");
        _exit(-1);
    }

    int ret = main();

    _exit(ret);
}
