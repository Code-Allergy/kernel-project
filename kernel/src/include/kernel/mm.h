#ifndef KERNEL_MM_H
#define KERNEL_MM_H

// #define MEMORY_USER_CODE_BASE 0x00010000
// #define MEMORY_USER_DATA_BASE 0x00100000
// #define MEMORY_USER_HEAP_BASE 0x01000000
// #define MEMORY_USER_STACK_BASE 0x7F000000


// TODO

#define MEMORY_USER_CODE_BASE    0x00010000   // Keep user code start
#define MEMORY_USER_DATA_BASE    0x00100000   // Keep user data start
#define MEMORY_USER_HEAP_BASE    0x01000000   // Keep user heap start
#define MEMORY_USER_STACK_BASE   0x40000000   // Move stack below kernel
#define KERNEL_BASE             0x40120000   // TODO this should be virtual memory mapped instead of identity mapped

#endif // KERNEL_MMU_H