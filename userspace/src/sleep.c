#include <stdio.h>
#include <syscalls.h>

#define NUM_CHILDREN 64
int pids[NUM_CHILDREN];

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

    printf("[USER SLEEP TEST] Going to sleep for 2 seconds\n");
    usleep(2000000); // 2 million microseconds = 2 seconds
    printf("[USER SLEEP TEST] Done sleeping for 2 seconds!\n");

    printf("[USER SLEEP TEST] Counting 1-100 using sleeping children processes\n");
    printf("[USER SLEEP TEST] Spawning processes backwards so we can verify it's not just the order of the processes being scheduled\n");
    for (int i = 0; i < NUM_CHILDREN; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            fprintf(stderr, "[USER SLEEP TEST] Fork failed, exiting\n");
            return 1;
        } else if (pids[i] == 0) {
            // Inside the child process block (pids[i] == 0)
            uint64_t sleep_duration_us = 10000ULL * (NUM_CHILDREN - i); // Use ULL for uint64_t literal
            printf("[SLEEP TEST]: Child %d (PID: %d) intends to sleep for %llu us.\n", i + 1, getpid(), sleep_duration_us);
            usleep(sleep_duration_us);
            printf("#%d (PID: %d) finished sleeping.\n", (NUM_CHILDREN - i), getpid());
            return 0;
        }
    }

    printf("[USER SLEEP TEST] Waiting for all children to complete...\n");
    for (int i = 0; i < NUM_CHILDREN; i++) {
        if (pids[i] > 0) { // Ensure we only wait for valid PIDs
            int status = waitpid(pids[i]);
            printf("[USER SLEEP TEST] Child with PID %d finished with status %d. Waited for child %d.\n", pids[i], status, i + 1);
        }
    }
    printf("[USER SLEEP TEST] All children completed.\n");

    return 0;
}
