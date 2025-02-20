#include <stddef.h>
#include "../include/syscalls.h"

// from include/syyscall.h
typedef __builtin_va_list va_list;

#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type)   __builtin_va_arg(ap, type)
#define va_end(ap)         __builtin_va_end(ap)


#define BUFFER_SIZE 1024

static char buffer[BUFFER_SIZE];  // Static buffer of 1024 bytes

// Helper function to convert an integer to a string in base 10
static int itoa(int num, char *str, int base) {
    int i = 0;
    int isNegative = 0;
    // Handle 0 explicitly, otherwise empty string is printed
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return i;
    }

    // Handle negative numbers only if base is 10
    if (num < 0 && base == 10) {
        isNegative = 1;
        num = -num;
    }

    // Process individual digits
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }

    // Append negative sign for negative numbers
    if (isNegative) {
        str[i++] = '-';
    }

    str[i] = '\0';  // Null-terminate string

    // Reverse the string
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }

    return i;
}

// Basic vsnprintf implementation for embedded systems
int vsnprintf(char *str, size_t size, const char *format, va_list args) {
    int i = 0;
    const char *ptr;

    for (ptr = format; *ptr != '\0'; ++ptr) {
        if (*ptr != '%') {
            // Copy normal characters
            if (i < size - 1) {
                str[i++] = *ptr;
            }
        } else {
            // Skip the '%' character
            ++ptr;
            switch (*ptr) {
                case 'c': {  // Character
                    char c = (char)va_arg(args, int);
                    if (i < size - 1) {
                        str[i++] = c;
                    }
                    break;
                }
                case 's': {  // String
                    char *s = va_arg(args, char*);
                    while (*s != '\0' && i < size - 1) {
                        str[i++] = *s++;
                    }
                    break;
                }
                case 'd': {  // Signed integer
                    int num = va_arg(args, int);
                    char numStr[32];
                    int len = itoa(num, numStr, 10);
                    for (int j = 0; j < len && i < size - 1; ++j) {
                        str[i++] = numStr[j];
                    }
                    break;
                }
                case 'x': {  // Hexadecimal integer
                    int num = va_arg(args, int);
                    char numStr[32];
                    int len = itoa(num, numStr, 16);
                    for (int j = 0; j < len && i < size - 1; ++j) {
                        str[i++] = numStr[j];
                    }
                    break;
                }
                case '%': {  // Literal '%' character
                    if (i < size - 1) {
                        str[i++] = '%';
                    }
                    break;
                }
                default:
                    // Handle unsupported format specifiers by ignoring them
                    break;
            }
        }

        if (i >= size - 1) {
            break;  // Avoid buffer overflow
        }
    }

    str[i] = '\0';  // Null-terminate the string
    return i;
}

int printf(const char *format, ...) {
    va_list args;
    va_start(args, format);

    // Call vsnprintf to format the string
    int len = vsnprintf(buffer, sizeof(buffer), format, args);

    va_end(args);

    // Output the formatted string (syscall_debug, for example)
    syscall_debug(buffer, len);

    return len;
}
