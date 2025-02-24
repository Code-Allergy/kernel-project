#ifndef __LIB_STDIO_H__
#define __LIB_STDIO_H__
#include <stddef.h>

#define stdin 0
#define stdout 1
#define stderr 2
int printf(const char *restrict format, ...);
int snprintf(char *buf, size_t size, const char *fmt, ...);
int fprintf(int fd, const char *format, ...);

#endif
