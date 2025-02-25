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
        int res = exec("/mnt/elf/sh");
        fprintf(stderr, "exec failed with code %d\n", res);
        exit(1);
    }

    int retval = waitpid(pid);
    printf("Shell process ended with return code %d, hanging indefinitely!\n", retval);

    return 0;
}
