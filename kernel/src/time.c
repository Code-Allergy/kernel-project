#include <kernel/time.h>
#include <kernel/rtc.h>

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
uint32_t days_since_epoch(rtc_date_t* date) {
    uint32_t days = 0;

    // Count full years since 1970
    for (uint32_t y = 1970; y < date->year; y++) {
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

uint32_t rtc_to_epoch(rtc_date_t* date, rtc_time_t* time) {
    uint32_t total_days = days_since_epoch(date);
    return (total_days * SECONDS_PER_DAY) + (time->hour * SECONDS_PER_HOUR) +
           (time->minute * SECONDS_PER_MINUTE) + time->second;
}


// TODO - we can improve this when we get the counter going, less calculations
uint32_t epoch_now() {
    rtc_date_t date;
    rtc_time_t time;
    rtc_driver.get_date(&date);
    rtc_driver.get_time(&time);
    return rtc_to_epoch(&date, &time);
}
