// #include "sysc.h"

#include <stdio.h>
#include <syscalls.h>


#define open(path, flags, mode) syscall_3(SYSCALL_OPEN_NO, (uint32_t)path, flags, mode)
#define close(fd) syscall_1(SYSCALL_CLOSE_NO, fd)
#define fork() syscall_0(SYSCALL_FORK_NO)

int main(void) {
    int fd = open("/", 0, 0);
    if (fd == 0) {
        printf("READ OK!");
    } else {
        printf("READ FAIL! %d", fd);
    }

    int res = close(fd);
    if (res == 0) {
        printf("CLOSE OK!");
    } else {
        printf("CLOSE FAIL! %d", res);
    }

    // if (fork() == 0) {
    //     printf("I am the child\n");
    // } else {
    //     printf("I am the parent\n");
    // }

    return 0;
}
