// #include <utils.h>
// #include <stdint.h>
#include <kernel/string.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Write a value to a register
 *
 * @param base   Base address of the register
 * @param offset Offset of the register
 * @param value  Value to write to the register
 */
void REG32_write(unsigned int base, unsigned int offset, unsigned int value) {
    volatile unsigned int *reg = (volatile unsigned int *)(base + offset);
    *reg = value;
}

/**
 * Write a value to a register with a mask (only writes to the bits specified by the mask)
 *
 * @param base   Base address of the register
 * @param offset Offset of the register
 * @param mask   Mask to apply to the value
 * @param value  Value to write to the register (masked bits only)
 */
void REG32_write_masked(unsigned int base, unsigned int offset, unsigned int mask, unsigned int value) {
    volatile unsigned int *reg = (volatile unsigned int *)(base + offset);
    *reg = (*reg & ~mask) | (value & mask);
}

/**
 * Read a value from a register
 *
 * @param base   Base address of the register
 * @param offset Offset of the register
 * @return       Value read from the register
 */
unsigned int REG32_read(unsigned int base, unsigned int offset) {
    volatile unsigned int *reg = (volatile unsigned int *)(base + offset);
    return *reg;
}

/**
 * Read a value from a register with a mask
 *
 * @param base   Base address of the register
 * @param offset Offset of the register
 * @param mask   Mask to apply to the value
 * @return       Value read from the register (masked bits only)
 */
unsigned int REG32_read_masked(unsigned int base, unsigned int offset, unsigned int mask) {
    volatile unsigned int *reg = (volatile unsigned int *)(base + offset);
    return *reg & mask;
}

// here for now
//

unsigned int strlen(const char* str) {
    unsigned int len = 0;
    while (*str != '\0') {
        len++;str++;
    }
    return len;
}

char* strcpy(char* dest, const char* src) {
    char* start = dest;
    while (*src != '\0') {
        *dest = *src;
        dest++;
        src++;
    }
    *dest = '\0';
    return start;
}

char* strncpy(char* dest, const char* src, unsigned int n) {
    char* start = dest;
    while (*src != '\0' && n > 0) {
        *dest = *src;
        dest++;
        src++;
        n--;
    }
    *dest = '\0';
    return start;
}

char* strchr(const char* str, int c) {
    while (*str != '\0') {
        if (*str == c) {
            return (char*)str;
        }
        str++;
    }
    if (c == '\0') {  // Handle null terminator case
        return (char*)str;
    }
    return NULL;
}

char* strtok(char* str, const char* delim) {
    static char* buffer = NULL;

    // Handle initial case or explicit new string
    if (str != NULL) {
        buffer = str;
    }

    // Return NULL if we've reached the end or have invalid input
    if (buffer == NULL) {
        return NULL;
    }

    // Skip leading delimiters
    while (*buffer && strchr(delim, *buffer) != NULL) {
        buffer++;
    }

    // If we've reached the end after skipping delimiters
    if (*buffer == '\0') {
        return NULL;
    }

    // Find start of token
    char* start = buffer;

    // Find end of token
    while (*buffer && strchr(delim, *buffer) == NULL) {
        buffer++;
    }

    if (*buffer) {
        *buffer = '\0';
        buffer++;
    }

    return start;
}

int toupper(int c) {
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 'A';
    }
    return c;
}

void* memset(void* ptr, int value, unsigned long num) {
    unsigned char* p = (unsigned char*)ptr;
    while (num > 0) {
        *p = (unsigned char)value;
        p++;
        num--;
    }
    return ptr;

}

// void* memcpy(void* dest, const void* src, unsigned long size) {
//     char* d = (char*)dest;
//     const char* s = (const char*)src;
//     while (size > 0) {
//         *d = *s;
//         d++;
//         s++;
//         size--;
//     }
//     return dest;
// }

void* memcpy(void* dest, const void* src, unsigned long size) {
    char* d = (char*)dest;
    const char* s = (const char*)src;

    // Copy in 4-byte chunks
    while (size >= 4) {
        *(int*)d = *(const int*)s;
        d += 4;
        s += 4;
        size -= 4;
    }

    // Copy any remaining bytes one-by-one
    while (size > 0) {
        *d = *s;
        d++;
        s++;
        size--;
    }

    return dest;
}

int memcmp(const void* ptr1, const void* ptr2, unsigned long num) {
    const unsigned char* p1 = (const unsigned char*)ptr1;
    const unsigned char* p2 = (const unsigned char*)ptr2;
    while (num > 0) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
        num--;
    }
    return 0;
}

uint32_t calculate_checksum(const void *data, size_t len) {
    uint32_t checksum = 0;
    const uint8_t *ptr = (const uint8_t*)data;

    for (size_t i = 0; i < len; i++) {
        checksum ^= ptr[i];
    }
    return checksum;
}

int strcmp(const char* str1, const char* str2) {
    while (*str1 != '\0' && *str2 != '\0') {
        if (*str1 != *str2) {
            return *str1 - *str2;
        }
        str1++;
        str2++;
    }
    return *str1 - *str2;
}

int strncmp(const char* str1, const char* str2, unsigned int n) {
    while (*str1 != '\0' && *str2 != '\0' && n > 0) {
        if (*str1 != *str2) {
            return *str1 - *str2;
        }
        str1++;
        str2++;
        n--;
    }
    return 0;
}

char* strcat(char* dest, const char* src) {
    char* start = dest;
    while (*dest != '\0') {
        dest++;
    }
    while (*src != '\0') {
        *dest = *src;
        dest++;
        src++;
    }
    *dest = '\0';
    return start;
}

char* strncat(char* dest, const char* src, unsigned int n) {
    char* start = dest;
    while (*dest != '\0') {
        dest++;
    }
    while (*src != '\0' && n > 0) {
        *dest = *src;
        dest++;
        src++;
        n--;
    }
    *dest = '\0';
    return start;
}


// string format types into a buffer
