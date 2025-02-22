#include <stdio.h>
#include <syscalls.h>

int main(void) {
    printf("[SLEEP TEST]: Going to sleep for 0 usec\n");
    usleep(0);
    printf("[SLEEP TEST]: Going to sleep for 1 second\n");
    usleep(1000000);
    printf("[SLEEP TEST]:Done!\n");

    if (fork() == 0) {
        printf("[SLEEP TEST]: Child going to sleep for 2 seconds\n");
        usleep(2000000);
        printf("[SLEEP TEST]: Child done!\n");
        return 0;
    } else {
        printf("[SLEEP TEST]: Parent going to sleep for 3 seconds\n");
        usleep(3000000);
        printf("[SLEEP TEST]: Parent done!\n");
    }

    // count 1 - 100 using sleeping children processes
    for (int i = 1; i <= 100; i++) {
        if (fork() == 0) {
            printf("[SLEEP TEST]: Child %d\n", i);
            usleep(100000 * (100-i));
            printf("#%d finished, PID: %d\n", (100-i), getpid());
            return 0;
        }
    }

    return 0;
}
