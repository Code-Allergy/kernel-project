#include "sysc.h"

#define SYSCALL_OPEN 4
#define open(path, flags, mode) syscall_3(SYSCALL_OPEN, (uint32_t)path, flags, mode)

int main(void) {


    int res = open("/", 0, 0);
    if (res == 0) {
        debug("READ OK!");
    } else {
        debug("READ FAIL!");
    }
}
