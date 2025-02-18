#include "runtime.h"



void exit(int return_val) {
    _exit(return_val); // actually do the exit (in runtime.c)
    __builtin_unreachable();
}
