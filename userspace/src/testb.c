#include "sysc.h"
int main(void) {
    debug("Hello, World 2!\n");
    yield();
    debug("Hello, World 2!\n");
    yield();
    debug("Hello, World 2!\n");
    yield();
    debug("Hello, World 2!\n");
    yield();
    debug("Hello, World 2!\n");

    return 0;
}
