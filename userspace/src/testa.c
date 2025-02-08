#include "sysc.h"
void _start(void) {
    debug("Hello, World 1!\n");
    yield();
    exit(0);
}
