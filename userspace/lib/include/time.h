#ifndef __LIB__TIME_H__
#define __LIB__TIME_H__
#include <stdint.h>

struct timeval {
    uint64_t tv_sec;
    uint64_t tv_usec;
};

struct timezone {
    uint32_t tz_minuteswest;
    uint32_t tz_dsttime;
};

uint64_t time(void);
int gettimeofday(struct timeval* tv, struct timezone* tz);

#endif // __LIB__TIME_H__
