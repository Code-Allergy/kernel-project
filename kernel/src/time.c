#include <kernel/time.h>
#include <kernel/rtc.h>
#include <kernel/timer.h>
#include <kernel/time.h>
#include <kernel/printk.h>

uint64_t base_epoch = 0; // epoch at the time the system was started
uint64_t base_ticks = 0; // base ticks when initializing module
int timezone_offset = 0; // offset in minutes west of UTC
int daylight_savings = 0; // daylight savings time active


void init_kernel_time(void) {
    clock_timer.init();
    start_kernel_clocks();
    rtc_date_t date;
    rtc_time_t time;
    rtc_driver.get_date(&date);
    rtc_driver.get_time(&time);
    base_epoch = rtc_to_epoch(&date, &time);
    base_ticks = clock_timer.get_ticks();
    timezone_offset = 0;
    daylight_savings = 0;
}


// Lookup table for number of days before each month in normal years
const int days_before_month[12] = {
    0,   31,  59,  90, 120, 151, // Jan - Jun
    181, 212, 243, 273, 304, 334 // Jul - Dec
};


// Function to check if a year is a leap year
int is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// Function to calculate days since 1970
uint64_t days_since_epoch(rtc_date_t* date) {
    uint64_t days = 0;

    // Count full years since 1970
    for (uint64_t y = 1970; y < date->year; y++) {
        days += is_leap_year(y) ? 366 : 365;
    }

    // Add days from previous months of the current year
    days += days_before_month[date->month - 1];

    // Adjust for leap year if current year is leap and month is after February
    if (date->month > 2 && is_leap_year(date->year)) {
        days += 1;
    }

    // Add the day of the month
    days += (date->day - 1);

    return days;
}

uint64_t rtc_to_epoch(rtc_date_t* date, rtc_time_t* time) {
    uint32_t total_days = days_since_epoch(date);
    return (total_days * SECONDS_PER_DAY) + (time->hour * SECONDS_PER_HOUR) +
           (time->minute * SECONDS_PER_MINUTE) + time->second;
}

inline uint64_t epoch_now(void) {
    uint64_t elapsed = clock_timer.get_ticks() - base_ticks;
    return base_epoch + (clock_timer.ticks_to_ms(elapsed) / 1000);
}

// this could be better optimized if we export the ticks freq from the driver, then we do 1 division instead of 2.
inline void tv_now(struct timeval* tv) {
    uint64_t elapsed = clock_timer.get_ticks() - base_ticks;
    uint64_t usec = clock_timer.ticks_to_us(elapsed);
    tv->tv_sec = base_epoch + (usec / 1000000);
    tv->tv_usec = usec % 1000000;
}

inline void fill_tz(struct timezone* tz) {
    tz->tz_minuteswest = timezone_offset;
    tz->tz_dsttime = daylight_savings;
}
