#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

int main(void) {
    for (int i = 0; i < 100; i++) {
        int ret = fork();
        if (ret == 0) {
            int i = 1000;
            uint32_t pid = getpid();
            while(i--) {
                printf("Hello world pid: %d: %d!\n", pid, i);
            }
            printf("PID %d is done.\n", pid);
            return 0;
        } else {
            printf("I am the parent and i have a child %d\n", ret);
        }
    }


    return 0;
}
