#ifndef _PRINTK_H
#define _PRINTK_H
#include <stdarg.h>

// Some helpful macro strings

#define DATE_FMT    "%02d/%02d/%04d"
#define TIME_FMT    "%02d:%02d:%02d"

void printk(const char *fmt, ...);
void vprintk(const char *fmt, va_list args);

#endif // _PRINTK_H
