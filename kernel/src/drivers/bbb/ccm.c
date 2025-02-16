#include <kernel/printk.h>
#include <kernel/ccm.h>
#include "mmc.h"
#include <stdint.h>
#include "gpio.h"

#include <kernel/ccm.h>

#define LED_PINS (0xF << 21)
#define CM_PER_BASE         0x44E00000
#define CM_WKUP_BASE         0x44E00400
#define CM_DPLL_BASE         0x44E00500
#define CTRL_MODULE_BASE    0x44E10000

// Power Management Registers

#define PRM_RSTCTRL         0x44E00F00


// Control Module Clock Registers
#define CM_PER_L3_CLKSTCTRL    (CM_PER_BASE + 0x0C)
#define CM_PER_L3_INSTR_CLKCTRL (CM_PER_BASE + 0xDC)
#define CM_PER_L3_CLKCTRL      (CM_PER_BASE + 0xE0)
#define CM_PER_OCPWP_L3_CLKSTCTRL (CM_PER_BASE + 0x12C)


#define CM_CLKSEL_DPLL_MPU      (CM_WKUP_BASE + 0x2C)
#define CM_CLKMODE_DPLL_CORE    (CM_WKUP_BASE + 0x90)
#define CM_IDLEST_DPLL_CORE     (CM_WKUP_BASE + 0x5C)
#define CM_CLKMODE_DPLL_MPU     (CM_WKUP_BASE + 0x88)
#define CM_IDLEST_DPLL_MPU      (CM_WKUP_BASE + 0x20)
#define CM_CLKSEL_DPLL_CORE    (CM_WKUP_BASE + 0x68)
#define CM_CLKMODE_DPLL_CORE   (CM_WKUP_BASE + 0x90)
#define CM_DIV_M4_DPLL_CORE    (CM_WKUP_BASE + 0x80)
#define CM_DIV_M5_DPLL_CORE    (CM_WKUP_BASE + 0x84)
#define CM_DIV_M6_DPLL_CORE    (CM_WKUP_BASE + 0xD8)
#define CM_WKUP_CLKSTCTRL   (CM_WKUP_BASE + 0x00)
#define CM_WKUP_CONTROL_CLKCTRL (CM_WKUP_BASE + 0x4)
#define CM_IDLEST_DPLL_PER     (CM_WKUP_BASE + 0x70)
#define CM_CLKMODE_DPLL_PER     (CM_WKUP_BASE + 0x8C)
#define CM_CLKSEL_DPLL_PERIPH     (CM_WKUP_BASE + 0x9C)
#define CM_DIV_M2_DPLL_PER   (CM_WKUP_BASE + 0xAC)

// Control Module Registers
#define CONTROL_STATUS      (CTRL_MODULE_BASE + 0x40)
#define CONTROL_INIT        (CTRL_MODULE_BASE + 0x44)

void configure_core_dpll(void) {
    // Put CORE DPLL in bypass
    REG32(CM_CLKMODE_DPLL_CORE) = 0x4;
    // Wait for bypass
    while ((REG32(CM_IDLEST_DPLL_CORE) & (1 << 8)) == 0);

    // Set multiply and divide values for 1GHz
    // M = 1000, N = 23
    REG32(CM_CLKSEL_DPLL_CORE) = (1000 << 8) | 23;

    // Configure dividers
    REG32(CM_DIV_M4_DPLL_CORE) = 10;  // 200MHz
    REG32(CM_DIV_M5_DPLL_CORE) = 8;   // 250MHz
    REG32(CM_DIV_M6_DPLL_CORE) = 4;   // 500MHz

    // Lock DPLL
    REG32(CM_CLKMODE_DPLL_CORE) = 0x7;

    // Wait for lock
    while ((REG32(CM_CLKMODE_DPLL_CORE) & (1 << 0)) == 0);
}

void configure_mpu_dpll(void) {
    // Put MPU DPLL in bypass
    REG32(CM_CLKMODE_DPLL_MPU) = 0x4;

    // Wait for bypass
    while ((REG32(CM_IDLEST_DPLL_MPU) & (1 << 8)) == 0);

    // Set multiply and divide values
    REG32(CM_CLKSEL_DPLL_MPU) = (500 << 8) | 23;  // 1GHz

    // Lock DPLL
    REG32(CM_CLKMODE_DPLL_MPU) = 0x7;

    // Wait for lock
    while ((REG32(CM_CLKMODE_DPLL_MPU) & (1 << 0)) == 0);
}

void enable_basic_clocks(void) {
    // Enable L3 and L4 interface clocks
    REG32(CM_PER_L3_CLKSTCTRL) = 0x2;    // Enable L3 clocks
    REG32(CM_PER_L3_INSTR_CLKCTRL) = 0x2; // explicit enable
    REG32(CM_PER_L3_CLKCTRL) = 0x2;       // explicit enable
    REG32(CM_PER_OCPWP_L3_CLKSTCTRL) = 0x2; // force reset

    // Enable WKUP domain clocks
    REG32(CM_WKUP_CLKSTCTRL) = 0x2;         // force wakeup
    REG32(CM_WKUP_CONTROL_CLKCTRL) = 0x2;   // force wakeup

    // Wait for clock module ready
    while ((REG32(CM_WKUP_CONTROL_CLKCTRL) & (0x3 << 16)) != 0);
    // printk("CM READY\n");
}


#define GET32(x) (*(volatile unsigned int *)(x))
#define PUT32(x,y) (*(volatile unsigned int *)(x))=(y)

void PER_PLL_Config(uint32_t clkin, uint32_t n, uint32_t m, uint32_t m2)
{
   uint32_t clkmode,clksel,div_m2;


   clkmode = GET32(CM_CLKMODE_DPLL_PER);
   clksel = GET32(CM_CLKSEL_DPLL_PERIPH);
   div_m2 = GET32(CM_DIV_M2_DPLL_PER);

   clkmode &= ~0x7;
   clkmode |= 0x4;
   PUT32(CM_CLKMODE_DPLL_PER,clkmode);
   while((GET32(CM_IDLEST_DPLL_PER) & 0x100) != 0x100);

   clksel &= 0x00F00000;
   clksel |= 0x04000000; // SD divider = 4 for both OPP100 and OPP50
   clksel |= (m<<8)|n;
   PUT32(CM_CLKSEL_DPLL_PERIPH,clksel);
   div_m2 = 0xFFFFFF80 | m2;
   PUT32(CM_DIV_M2_DPLL_PER,div_m2);
   clkmode &= 0xFFFFFFF8;
   clkmode |= 0x7;
   PUT32(CM_CLKMODE_DPLL_PER,clkmode);

   while((GET32(CM_IDLEST_DPLL_PER) & 0x1) != 0x1);
}


void init_all_plls(void) {
    // 1. Enable basic clocks first
    // printk("Enabling basic clocks...\n");
    enable_basic_clocks();
    printk("Basic clocks enabled\n");

    // 2. Configure Core DPLL first
    // printk("Configuring Core DPLL...\n");
    configure_core_dpll();
    printk("Core DPLL configured\n");

    // 3. Configure MPU DPLL
    // printk("Configuring MPU DPLL...\n");
    configure_mpu_dpll();
    printk("MPU DPLL configured\n");

    // 4. Now configure Peripheral DPLL
    printk("Configuring Peripheral DPLL...\n");
    PER_PLL_Config (24, 23, 960,  5);
}

ccm_t ccm_driver = {
    .init = init_all_plls,
};
