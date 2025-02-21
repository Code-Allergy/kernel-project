#include <kernel/printk.h>
#include <kernel/time.h>
#include <kernel/panic.h>
#include <kernel/rtc.h>

#include "timer.h"
#include "ccm.h"
#include "rtc.h"

#define CCU_TIMER_CLK_REG (CCM_BASE + 0x64)  // APB1 Clock Control Register
#define CCU_TIMER_GATE_BIT (1 << 6)          // Bit 6 enables timer clock

struct timer_regs {
    volatile uint32_t ctrl;    // 0xA0
    volatile uint32_t low;     // 0xA4
    volatile uint32_t high;    // 0xA8
};

struct timer_regs* timer = (struct timer_regs*)(TIMER_BASE + 0xA0);

#define CCU_APB1_CLK_DIV_REG (CCM_BASE + 0x058)  // APB1 Clock Divider Register


void init_timer() {
    // Enable clock gate first
    *(volatile uint32_t*)CCU_TIMER_CLK_REG |= CCU_TIMER_GATE_BIT;
    // Clear counter first
    timer->ctrl |= 2;  // Set clear bit

    // Wait a bit for clear to take effect
    for(volatile int i = 0; i < 1000; i++);

    // Set OSC24M source (bit 2 = 0) and clear other control bits
    timer->ctrl = 0;
}

#define A10_RTC_BASE_YEAR 2010

// Define bit masks and shifts based on the register layout
#define YEAR_MASK   0x003F0000  // Bits 21:16
#define YEAR_SHIFT  16

#define MONTH_MASK  0x00000F00  // Bits 11:8
#define MONTH_SHIFT 8

#define DAY_MASK    0x0000001F  // Bits 4:0
#define DAY_SHIFT   0

// Define bit masks and shifts based on the register layout
#define HOUR_MASK   0x001F0000  // Bits 20:16
#define HOUR_SHIFT  16

#define MINUTE_MASK 0x00003F00  // Bits 13:8
#define MINUTE_SHIFT 8

#define SECOND_MASK 0x0000003F  // Bits 5:0
#define SECOND_SHIFT 0

// Function to extract day, month, and year
void extract_date(rtc_date_t* date) {
    uint32_t reg_value = *(uint32_t*)RTC_YYMMDD;
    date->year  = ((reg_value & YEAR_MASK)  >> YEAR_SHIFT) + A10_RTC_BASE_YEAR; // A10 RTC stores relative year
    date->month = (reg_value & MONTH_MASK) >> MONTH_SHIFT;
    date->day   = (reg_value & DAY_MASK)   >> DAY_SHIFT;
}


// Function to extract hour, minute, and second
void extract_time(rtc_time_t* time) {
    uint32_t reg_value = *(uint32_t*)RTC_HHMMSS;
    time->hour   = (reg_value & HOUR_MASK)   >> HOUR_SHIFT;
    time->minute = (reg_value & MINUTE_MASK) >> MINUTE_SHIFT;
    time->second = (reg_value & SECOND_MASK) >> SECOND_SHIFT;
}

// PIT register offsets
#define AW_A10_PIT_COUNT_LO  0xA4
#define AW_A10_PIT_COUNT_HI  0xA8
#define AW_A10_PIT_COUNT_CTL 0xA0

#define REG32(addr) (*(volatile uint32_t *)(addr))

uint64_t read_64bit_counter() {
    // Latch the current count into LO/HI registers
    REG32(TIMER_BASE + AW_A10_PIT_COUNT_CTL) = (1 << 0); // Set RL_EN
    printk("value: %d\n", REG32(TIMER_BASE + AW_A10_PIT_COUNT_LO));

    // Read LO/HI
    uint32_t lo = REG32(TIMER_BASE + AW_A10_PIT_COUNT_LO);
    uint32_t hi = REG32(TIMER_BASE + AW_A10_PIT_COUNT_HI);
    // return hi;
    return ((uint64_t)hi << 32) | lo;
}

void rtc_init() {
    while (1) {
      uint64_t cnt = read_64bit_counter();
      printk("Counter: %u\n", cnt);
    }


    panic("unimplemented!");
}


rtc_driver_t rtc_driver =  {
    .init = rtc_init,
    .get_date = extract_date,
    .get_time = extract_time,
};
