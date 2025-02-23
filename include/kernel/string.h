#ifndef KERNEL_STRING_H
#define KERNEL_STRING_H
#include <stdarg.h>
#include <stddef.h>

char* strtok(char* str, const char* delimiters);
unsigned int strlen(const char* str);
char* strchr(const char* str, int c);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, unsigned int n);
int toupper(int c);
void* memset(void* ptr, int value, unsigned long num);
int memcmp(const void* ptr1, const void* ptr2, unsigned long num);
void* memcpy(void* dest, const void* src, unsigned long size);
int strcmp(const char* str1, const char* str2);
int strncmp(const char* str1, const char* str2, unsigned int n);
char* strcat(char* dest, const char* src);
char* strncat(char* dest, const char* src, unsigned int n);
// string.c
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
int snprintf(char *buf, size_t size, const char *fmt, ...);

#endif
