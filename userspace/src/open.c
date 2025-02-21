// #include "sysc.h"

#include <stdint.h>
#include <stdio.h>
#include <syscalls.h>
#include <errno.h>


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
    int fd = open("/dev/uart0", 0, 0);
    if (fd == 0) {
        printf("READ OK!\n");
    } else {
        printf("READ FAIL! %d\n", fd);
        return -1;
    }

    if (write(fd, "Hello world! From VFS!\n", 23) != 23) {
        printf("Error!\n");
    }

    // int read_dirs = readdir(fd, some, sizeof(some));
    // if (read_dirs == -ENOTDIR) {
    //     printf("Not a directory! going to read 100 bytes\n");
    //     char buf[100];
    //     if (read(fd, buf, 100) != 100) {
    //         printf("Read failed!\n");
    //     } else {
    //         // Ensure the last byte is null-terminated
    //         buf[99] = '\0';

    //         // Print out raw data as hex to show that 100 bytes were read (0x00 in this case)
    //         printf("Read 100 bytes: ");
    //         for (int i = 0; i < 100; ++i) {
    //             printf("%c ", (unsigned char)buf[i]);
    //         }
    //         printf("\n");

    //         // Optionally print out the null-terminated string if applicable (in case it's printable)
    //         printf("Null-terminated string: %s\n", buf);
    //     }
    // } else {
    //     printf("Read %d entries!\n", read_dirs);
    //     for (int i = 0; i < read_dirs; i++) {
    //         printf("Entry %d: %s\n", i, some[i].d_name);
    //     }
    // }


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
