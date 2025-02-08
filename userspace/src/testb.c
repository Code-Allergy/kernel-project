#include "sysc.h"
void _start(void) {
    debug("Hello, World 2!\n");
    yield();
    exit(0);
}
