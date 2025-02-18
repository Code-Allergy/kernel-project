#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

int main(void) {
    int i = 50;
    uint32_t pid = getpid();
    while(i--) {
        printf("Hello world pid: %d: %d!\n", pid, i);
    }

    int ret = fork();
    if (ret == 0) {
        printf("I am the child\n");
    } else {
        printf("I am the parent\n");
    }

    return 0;
}
