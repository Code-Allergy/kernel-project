#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

int main(void) {
    int i = 50000;
    while(i--) {
        printf("Hello world: %d!\n", i);
    }
    return 0;
}
