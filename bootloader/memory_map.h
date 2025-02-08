#ifndef MEMORY_MAP_H
#define MEMORY_MAP_H

#if defined(PLATFORM) && (PLATFORM == BBB)
    #include "memory_map_bbb.h"
#else
    #include "memory_map_qemu.h"
#endif

#endif