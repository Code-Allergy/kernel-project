#include "sysc.h"
int main(void) {
    while (1) {
        yield();
        __asm__ volatile("wfi");
    }

    return 0;
}
