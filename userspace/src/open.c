#include "sysc.h"

#include <stdio.h>

#define SYSCALL_OPEN 4
#define open(path, flags, mode) syscall_3(SYSCALL_OPEN, (uint32_t)path, flags, mode)

int main(void) {


    int res = open("/", 0, 0);
    if (res == 0) {
        printf("READ OK!");
    } else {
        printf("READ FAIL! %d", res);
    }

    return 0;
}
