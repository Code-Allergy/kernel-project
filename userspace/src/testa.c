#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

int main(void) {
    int i = 50000;
    uint32_t pid = getpid();
    while(i--) {
        printf("Hello world pid: %d: %d!\n", pid, i);
    }
    return 0;
}
