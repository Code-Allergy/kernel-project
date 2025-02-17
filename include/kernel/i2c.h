#ifndef KERNEL_I2C_H
#define KERNEL_I2C_H
#include <stdint.h>
#include <stddef.h>
#include <kernel/panic.h>

typedef struct {
    int (*init)(void);

    // Function to write data to an I2C device
    int (*write)(uint8_t device_addr, const uint8_t *data, size_t length);

    // Function to read data from an I2C device
    int (*read)(uint8_t device_addr, uint8_t *data, size_t length);

    // Function to deinitialize the I2C interface
    int (*deinit)(void);

    uint32_t slave_addr;
} i2c_driver_t;

#define I2C_DRIVER_INIT() \
    { \
        .init = unimplemented_driver, \
        .write = unimplemented_driver, \
        .read = unimplemented_driver, \
        .deinit = unimplemented_driver, \
        .slave_addr = 0, \
    }

extern i2c_driver_t i2c_driver;

#endif
