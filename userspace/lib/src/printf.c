#include <stddef.h>
#include <stdio.h>
#include "../include/syscalls.h"

// from include/syyscall.h
typedef __builtin_va_list va_list;

#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type)   __builtin_va_arg(ap, type)
#define va_end(ap)         __builtin_va_end(ap)

#define ANSI_RED "\033[31m"
#define ANSI_RESET "\033[0m"

#define BUFFER_SIZE 1024

static char colored_buffer[2048];
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

    int len = vsnprintf(buffer, sizeof(buffer), format, args);

    va_end(args);

    if (write(stdout, buffer, len) < len) {
        syscall_debug("IOFAIL", 6);
        exit(-1);
    }

    return len;
}

// int fprintf(int fd, const char *format, ...) {
//     va_list args;
//     va_start(args, format);

//     int len = vsnprintf(buffer, sizeof(buffer), format, args);

//     va_end(args);

//     if (write(fd, buffer, len) < len) {
//         syscall_debug("IOFAIL", 6);
//         exit(-1);
//     }

//     return len;
// }

// these 3 should be elsewhere
int sprintf(char *str, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vsnprintf(str, 1024, format, args);
    va_end(args);
    return ret;
}

int strcpy(char *dest, const char *src) {
    int i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return i;
}

int strlen(const char *str) {
    int i = 0;
    while (str[i] != '\0') {
        i++;
    }
    return i;
}



int fprintf(int fd, const char *format, ...) {
    static int red_active = 0;  // Track whether we are inside a red-colored block
    char buffer[1024];

    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (fd == 2) {  // Apply ANSI red color if writing to stderr
        char colored_buffer[2048];  // Ensure we have space for added escape codes
        char *out = colored_buffer;
        const char *in = buffer;

        if (!red_active) {
            out += sprintf(out, "%s", ANSI_RED);
            red_active = 1;
        }

        while (*in) {
            *out++ = *in;
            if (*in == '\n') {
                strcpy(out, ANSI_RESET ANSI_RED);
                out += strlen(ANSI_RESET ANSI_RED);
            }
            in++;
        }

        strcpy(out, ANSI_RESET);
        out += strlen(ANSI_RESET);
        red_active = 0;  // Reset after each write

        len = out - colored_buffer;
        if (write(fd, colored_buffer, len) < len) {
            syscall_debug("IOFAIL", 6);
            exit(-1);
        }
    } else {
        if (write(fd, buffer, len) < len) {
            syscall_debug("IOFAIL", 6);
            exit(-1);
        }
    }

    return len;
}
