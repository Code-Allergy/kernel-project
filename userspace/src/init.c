#include <stdio.h>
#include <stdlib.h>
#include <syscalls.h>


int main() {
    // Initialize signal handling
    // signal(SIGCHLD, SIG_IGN);

    // // Fork and execute shell
    printf("Hello from init!\n");
    int pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    }
    else if (pid == 0) {
        // Child process
        int res = exec("/mnt/elf/sh.elf");
        fprintf(stderr, "exec failed with code %d\n", res);
        exit(1);
    }

    while (1) {
        // wait on child, for now, just while loop
    }

    // // Parent process becomes init
    // while (1) {
    //     pid = wait(NULL);
    //     if (pid < 0) {
    //         perror("wait");
    //         exit(1);
    //     }
    // }

    return 0;
}
