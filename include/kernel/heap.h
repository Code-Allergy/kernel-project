#ifndef KERNEL_HEAP_H
#define KERNEL_HEAP_H
#include <stdint.h>

#define KHEAP_START 0xE0000000
#define KHEAP_SIZE 0x2000000     // initial size for now.

extern uint32_t kernel_heap_start;
extern uint32_t kernel_heap_end;
extern uint32_t kernel_heap_curr;

int kernel_heap_init(void);

void* kmalloc(uint32_t size);
void kfree(void* ptr);
char* strdup(const char* s);

uint32_t kernel_heap_usage_get(void);
uint32_t kernel_heap_total_get(void);

#endif // KERNEL_HEAP_H
