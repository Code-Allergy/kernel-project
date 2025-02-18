#include "sysc.h"
int main(void) {
    debug("Hello, World 1!\n");
    yield();
    debug("Hello, World 1!\n");
    yield();
    debug("Hello, World 1!\n");
    yield();
    debug("Hello, World 1!\n");
    yield();
    debug("Hello, World 1!\n");

    return 0;
}
