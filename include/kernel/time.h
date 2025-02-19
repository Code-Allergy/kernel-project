#ifndef KERNEL_TIME_H
#define KERNEL_TIME_H

#include <stdint.h>

#define SECONDS_PER_MINUTE 60
#define SECONDS_PER_HOUR   3600
#define SECONDS_PER_DAY    86400

typedef uint32_t epoch_t;

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

// TODO timeval struct later for more precision, need to use other clock on top of rtc epoch
// struct timeval {
//     long tv_sec;
//     long tv_usec;
// };

int is_leap_year(int year);
epoch_t epoch_now();
epoch_t rtc_to_epoch(rtc_date_t* date, rtc_time_t* time);
uint32_t days_since_epoch(rtc_date_t* date);

#endif // KERNEL_TIME_H
