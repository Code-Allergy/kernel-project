// #include "sysc.h"

#include <stdint.h>
#include <stdio.h>
#include <syscalls.h>


#define open(path, flags) syscall_2(SYSCALL_OPEN_NO, (uint32_t)path, flags)
#define close(fd) syscall_1(SYSCALL_CLOSE_NO, fd)
#define fork() syscall_0(SYSCALL_FORK_NO)
#define readdir(fd, buf, len) syscall_3(SYSCALL_READDIR_NO, fd, buf, len);

#define O_CREAT 0x40
#define O_RDONLY 0x00
#define O_WRONLY 0x01
#define O_RDWR 0x02
#define O_APPEND 0x08
#define O_TRUNC 0x10
#define O_EXCL 0x20

// userspace dirent structure
typedef struct dirent {
    uint32_t d_ino;    // Inode number
    char d_name[256];  // Filename
} dirent_t;

dirent_t some[8];

int main(void) {
    printf("Hello World!\n");
    printf("Hello World over through user VFS over UART0!\n");
    int fd = open("/mnt/elf", 0);
    if (fd < 0) {
        printf("Error opening file! %d\n", fd);
    } else {
        printf("Opened file! %d\n", fd);
    }

    while(1) {
        ssize_t bytes;
        char buffer[256];
        if ((bytes = read(stdin, buffer, 256)) == -EAGAIN) {
            usleep(1000); // sleep for 1ms
        } else {
            printf("Read %d bytes: %s\n", bytes, buffer);
        }
    }

    // if (write(fd, "Hello world! From VFS!\n", 23) != 23) {
    //     printf("Error!\n");
    // }

    // int read_dirs = readdir(fd, (uint32_t)some, sizeof(some));
    // printf("Reading dir /mnt/bin:\n", read_dirs);
    // for (int i = 0; i < read_dirs; i++) {
    //     printf("Entry %d: %s\n", i, some[i].d_name);
    // }

    // printf("Trying to open null.elf\n");
    // int fd2 = open("/mnt/elf/sh.elf", 0x10101010);
    // if (fd2 < 0) {
    //     printf("Error opening file! %d\n", fd2);
    // } else {
    //     printf("Opened file! %d\n", fd2);
    // }

    // // printf("Seeking 2 bytes\n");
    // // int offset = lseek(fd2, 2, SEEK_SET);
    // // printf("Offset: %d\n", offset);


    // printf("Reading first byte\n");
    // char buffer[12];
    // int read_bytes = read(fd2, buffer, 2);
    // if (read_bytes != 2) {
    //     printf("Error reading file! %d\n", read_bytes);
    // } else {
    //     printf("Read string: %s\n", buffer);
    // }

    // printf("Reading a second\n");
    // read_bytes = read(fd2, buffer, 2);
    // if (read_bytes != 2) {
    //     printf("Error reading file! %d\n", read_bytes);
    // } else {
    //     printf("Read string: %s\n", buffer);
    // }

    // int fd1 = open("/mnt/log.txt", O_CREAT);
    // if (fd1 < 0) {
    //     printf("Error opening file! %d\n", fd1);
    // } else {
    //     printf("Opened log file! %d\n", fd1);
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
