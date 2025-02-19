#ifndef A10_ALLWINNER_I2C_H
#define A10_ALLWINNER_I2C_H

#include <stdint.h>
#include <kernel/mm.h>

#define TWI0_BASE   (0x01C2AC00 + IO_KERNEL_OFFSET)

#define TWI_ADDR    0x00  // Slave address
#define TWI_XADDR   0x04  // Extended slave address
#define TWI_DATA    0x08  // Data buffer
#define TWI_CTRL    0x0C  // Control register
#define TWI_STAT    0x10  // Status register
#define TWI_CLK     0x14  // Clock control
#define TWI_SRST    0x18  // Software reset
#define TWI_EFR     0x1C  // Enhance feature register
#define TWI_LCR     0x20  // Line control


struct cubie_twi {
    volatile uint32_t* base;    // MMIO base (e.g., 0x01C2AC00 for TWI0)
    uint32_t clock_speed;       // Desired SCL clock (e.g., 100 kHz)
    uint8_t slave_addr;         // Slave device address
    uint32_t timeout;           // Timeout for transfers
};

#define TWI0 ((struct cubie_twi*)TWI0_BASE)

uint8_t rtc_i2c_read(struct cubie_twi* twi, uint8_t reg_addr);

#endif // A10_ALLWINNER_I2C_H
