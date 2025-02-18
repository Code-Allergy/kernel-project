#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

int main(void) {
    uint32_t pid = getpid();


    printf("Hello world! %d\n", pid);
    return 0;
}
