#include <stdio.h>
#include <stdint.h>
#include <syscalls.h>
#include <time.h>

int main(void) {
    uint64_t epoch = time();
    printf("Current time is: %d\n", (uint32_t) (epoch));


    printf("Creating timeval\n");
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);

    printf("S: %d U: %d\n", (uint32_t)tv.tv_sec, (uint32_t)tv.tv_usec);


    return 0;
}
