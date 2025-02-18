#include "sysc.h"
int main(void) {
    while (1) {
        yield();
    }

    return 0;
}
