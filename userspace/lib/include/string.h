#ifndef __LIB_STRINGS_H__
#define __LIB_STRINGS_H__

#include <stddef.h>

int strlen(const char *str);
int strcpy(char *dest, const char *src);
int memcmp(const char* a, const char* b, size_t bytes);
int strcmp(const char *a, const char *b);

#endif
