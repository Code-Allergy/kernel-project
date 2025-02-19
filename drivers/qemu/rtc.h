#ifndef A10_ALLWINNER_RTC_H
#define A10_ALLWINNER_RTC_H
#include <kernel/time.h>
#include <kernel/boot.h>

#define RTC_BASE (0x01c20d00 + IO_KERNEL_OFFSET)

#define RTC_LOSC (RTC_BASE + 0x0)
#define RTC_YYMMDD (RTC_BASE + 0x4)
#define RTC_HHMMSS (RTC_BASE + 0x8)


void rtc_get_time(rtc_time_t* time);
void rtc_get_date(rtc_date_t* date);


#endif // A10_ALLWINNER_RTC_H
