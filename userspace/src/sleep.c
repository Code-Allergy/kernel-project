#include <stdio.h>
#include <syscalls.h>

#define NUM_CHILDREN 1000

int main(void) {
    printf("[USER SLEEP TEST] Going to sleep for 0 usec\n");
    usleep(0);
    printf("[USER SLEEP TEST] Done!\n");
    printf("[USER SLEEP TEST] Going to sleep for 0.1 second\n");
    usleep(100000);
    printf("[USER SLEEP TEST] Done!\n");
    printf("[USER SLEEP TEST] Going to sleep for 0.5 second\n");
    usleep(500000);
    printf("[USER SLEEP TEST] Done!\n");

    printf("[USER SLEEP TEST] Counting 1-1000 using sleeping children processes\n");
    printf("[USER SLEEP TEST] Spawning processes backwards so we can verify it's not just the order of the processes being scheduled\n");
    for (int i = 0; i < NUM_CHILDREN; i++) {
        int pid = fork();
        if (pid < 0) {
            fprintf(stderr, "[USER SLEEP TEST] Fork failed, exiting\n");
            return 1;
        } else
        if (pid == 0) {
            printf("[SLEEP TEST]: Child %d\n", i+1);
            usleep(10000 * (NUM_CHILDREN-i));
            printf("#%d finished, PID: %d\n", (NUM_CHILDREN-i), getpid());
            return 0;
        }
    }

    return 0;
}
