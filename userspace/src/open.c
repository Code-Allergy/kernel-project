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

dirent_t some[8];

int main(void) {
    printf("Hello World!\n");
    printf("Hello World over through user VFS over UART0!\n");
    int fd = open("/mnt/elf", 0, 0);
    if (fd < 0) {
        printf("Error opening file! %d\n", fd);
    } else {
        printf("Opened file! %d\n", fd);
    }



    // if (write(fd, "Hello world! From VFS!\n", 23) != 23) {
    //     printf("Error!\n");
    // }

    int read_dirs = readdir(fd, (uint32_t)some, sizeof(some));
    printf("Reading dir /mnt/bin:\n", read_dirs);
    for (int i = 0; i < read_dirs; i++) {
        printf("Entry %d: %s\n", i, some[i].d_name);
    }

    printf("Trying to open null.elf\n");
    int fd2 = open("/mnt/elf/hello.txt", 0, 0);
    if (fd2 < 0) {
        printf("Error opening file! %d\n", fd2);
    } else {
        printf("Opened file! %d\n", fd2);
    }

    printf("Reading first bytes\n");
    char elf_header[12];
    int read_bytes = read(fd2, elf_header, 12);
    if (read_bytes != 12) {
        printf("Error reading file! %d\n", read_bytes);
    } else {
        printf("Read string: %s\n", elf_header);
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
