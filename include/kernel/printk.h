#ifndef _PRINTK_H
#define _PRINTK_H

// Some helpful macro strings

#define DATE_FMT    "%02d/%02d/%04d"
#define TIME_FMT    "%02d:%02d:%02d"

void printk(const char *fmt, ...);


#endif // _PRINTK_H
