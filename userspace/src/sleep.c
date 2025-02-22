#include <stdio.h>
#include <stdint.h>
#include <syscalls.h>

int main(void) {
    printf("[SLEEP TEST]: Going to sleep for 0 usec\n");
    usleep(0);
    printf("[SLEEP TEST]: Going to sleep for 1 second\n");
    usleep(1000000);
    printf("[SLEEP TEST]: Done!\n");

    return 0;
}
