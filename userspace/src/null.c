#include <syscalls.h>


int main(void) {
    // we should just yield in the future, but we need proper scheduling control first otherwise we waste time
    while (1) {
        // yield();
        __asm__ volatile("wfi");
    }

    return 0;
}
