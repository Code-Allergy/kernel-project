// #include "sysc.h"

#include <stdint.h>
#include <stdio.h>
#include <syscalls.h>


#define open(path, flags, mode) syscall_3(SYSCALL_OPEN_NO, (uint32_t)path, flags, mode)
#define close(fd) syscall_1(SYSCALL_CLOSE_NO, fd)
#define fork() syscall_0(SYSCALL_FORK_NO)
#define readdir(fd, buf, len) syscall_3(SYSCALL_READDIR_NO, fd, buf, len);



// userspace dirent structure
typedef struct dirent {
    uint32_t d_ino;    // Inode number
    char d_name[256];  // Filename
} dirent_t;

dirent_t some[4];

int main(void) {
    int fd = open("/", 0, 0);
    if (fd == 0) {
        printf("READ OK!");
    } else {
        printf("READ FAIL! %d", fd);
    }

    int read_dirs = readdir(fd, some, sizeof(some));
    printf("Read %d entries!\n", read_dirs);
    for (int i = 0; i < read_dirs; i++) {
        printf("Entry %d: %s\n", i, some[i].d_name);
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
