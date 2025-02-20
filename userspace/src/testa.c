#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

int main(void) {
    fork();
    fork();
    // int i = 20000;
    // uint32_t pid = getpid();
    // // while(i--) {
    // //     printf("Hello world pid: %d: %d!\n", pid, i);
    // // }

    for (int i = 0; i < 10; i++) {
        int ret = fork();
        if (ret == 0) {
            printf("I am the child\n");
            return 0;
        } else {
            printf("I am the parent\n");
        }
    }


    return 0;
}
