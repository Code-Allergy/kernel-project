#ifndef __LIB_STDIO_H__
#define __LIB_STDIO_H__
#define stdin 0
#define stdout 1
#define stderr 2
int printf(const char *restrict format, ...);
int fprintf(int fd, const char *format, ...);

#endif
