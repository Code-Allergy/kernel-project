// NULL process (pid 0)
#include "sysc.h"
void _start(void) {
    debug("Started null process (0)\n");
    while (1) {
        // __asm__ volatile (
        //     "wfi\n"  // Wait For Interrupt: puts the CPU to sleep until an IRQ occurs
        // );
        // yield();  // Yield to the next process

    }
}
