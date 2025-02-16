
#include <utils.h>
#include "gpio.h"
#include "ccm.h"


void GPIO_init(void) {
    // only configures GPIO1 for now
    //1. Enable the clock for GPIO1
    REG32_write(CM_PER_BASE, CM_PER_GPIO1_CLKCTRL, 0x2);
    //2. Wait for the clock to be enabled
    while ((REG32_read(CM_PER_BASE, CM_PER_GPIO1_CLKCTRL) & (0x3 << 16)) != 0x0);
    //3. Configure the GPIO1 module clock gating to disabled (module not gated)
    REG32_write(GPIO1_BASE, GPIO_CTRL_OFF, 0x0);

    //4. Configure the GPIO1 pins 21-24 as outputs (LEDs)
    REG32_write_masked(GPIO1_BASE, GPIO_OE_OFF, 0xF << 21, 0x0);
}


void GPIO_set(unsigned int gpio_base, unsigned int pins) {
    REG32_write_masked(gpio_base, GPIO_SETDATAOUT_OFF, pins, pins);
}

void GPIO_clear(unsigned int gpio_base, unsigned int pins) {
    REG32_write_masked(gpio_base, GPIO_CLEARDATAOUT_OFF, pins, pins);
}
