#include "sysc.h"
int res = 43;
const char* str = "Hello, World!\n";
int main(void) {
    while (1) {
        res++;
        yield();
        __asm__ volatile("wfi");
    }

    return 0;
}
