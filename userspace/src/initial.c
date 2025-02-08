#include "sysc.h"

void _start(void) {
    // while (1) {
    //     debug("Hello, World!\n");
    //     yield();
    // }
    debug("Hello, World!\n");
    // yield();

    debug("Exiting...\n");
    exit(0);
    // Exit the program (you can implement this with another syscall if needed)
    while (1);
}
