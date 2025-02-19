#ifndef KERNEL_RTC_H
#define KERNEL_RTC_H

#include <kernel/time.h>

typedef struct {
    void (*init)(void);
    void (*get_time)(rtc_time_t* time);
    void (*get_date)(rtc_date_t* date);
} rtc_driver_t;

extern rtc_driver_t rtc_driver;

#endif
