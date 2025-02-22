int main() {
    // Initialize signal handling
    // signal(SIGCHLD, SIG_IGN);

    // // Fork and execute shell
    // pid = fork();
    // if (pid < 0) {
    //     perror("fork");
    //     exit(1);
    // }
    // else if (pid == 0) {
    //     // Child process
    //     execl("/bin/sh", "/bin/sh", NULL);
    //     perror("execl");
    //     exit(1);
    // }

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
