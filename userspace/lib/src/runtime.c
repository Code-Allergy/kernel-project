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
    for (fd = 0; fd < 3; fd++) { // all 3 std fds will point to /dev/uart0
        fd = open("/dev/uart0", 0, 0);
        if (fd < 0) {
            debug("Failed to load fds for process\n");
            _exit(-1);
        }
    }
     // TODO we should pass a w/r flag


    int ret = main();

    _exit(ret);
}
