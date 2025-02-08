#ifndef UTILS_H
#define UTILS_H
#include <stdint.h>
#include <stddef.h>


void REG32_write(unsigned int base, unsigned int offset, unsigned int value);
void REG32_write_masked(unsigned int base, unsigned int offset, unsigned int mask, unsigned int value);

unsigned int REG32_read(unsigned int base, unsigned int offset);
unsigned int REG32_read_masked(unsigned int base, unsigned int offset, unsigned int mask);

static inline uint32_t cpu_to_le32(uint32_t v) {
    return (((v & 0xFF000000) >> 24) |
            ((v & 0x00FF0000) >> 8)  |
            ((v & 0x0000FF00) << 8)  |
            ((v & 0x000000FF) << 24));
}

static inline void __switch_stack(void* new_sp) {
    __asm__ volatile (
        "mov sp, %0"
        :
        : "r" (new_sp)
        : "sp"
    );
}

static inline void* __switch_stack_save(void* new_sp) {
    void* old_sp;
    __asm__ volatile (
        "mov %0, sp\n\t"
        "mov sp, %1"
        : "=r" (old_sp)
        : "r" (new_sp)
        : "sp"
    );
    return old_sp;
}

uint32_t calculate_checksum(const void *data, size_t len);

#endif /*UTILS_H*/
