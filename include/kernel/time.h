#ifndef KERNEL_TIME_H
#define KERNEL_TIME_H

#include <stdint.h>

#define SECONDS_PER_MINUTE 60
#define SECONDS_PER_HOUR   3600
#define SECONDS_PER_DAY    86400

typedef uint64_t epoch_t;

typedef struct {
    uint32_t hour;
    uint32_t minute;
    uint32_t second;
} rtc_time_t;

typedef struct {
    uint32_t year;
    uint32_t month;
    uint32_t day;
} rtc_date_t;

typedef struct timeval {
    uint64_t tv_sec; // seconds since epoch
    uint64_t tv_usec;// microseconds
} timeval_t;

typedef struct timezone {
    int tz_minuteswest; // Minutes west of UTC (not recommended)
    int tz_dsttime;     // Daylight savings time status (obsolete)
} timezone_t;


void init_kernel_time(void);

int is_leap_year(int year);
epoch_t epoch_now(void);
void tv_now(struct timeval* tv);
void fill_tz(struct timezone* tz);
epoch_t rtc_to_epoch(rtc_date_t* date, rtc_time_t* time);
uint64_t days_since_epoch(rtc_date_t* date);

#endif // KERNEL_TIME_H
