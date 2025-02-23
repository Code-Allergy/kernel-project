
#include <stdbool.h>
#include <kernel/printk.h>
#include <kernel/panic.h>

#include "i2c.h"

#define TWI_CTRL_M_STA_HIGH (1 << 5)
#define TWI_CTRL_BUS_EN_HIGH (1 << 6)
#define TWI_CTRL_INT_EN_HIGH (1 << 7)

#define TWI_CTRL_INT_FLAG_HIGH (1 << 3)


void twi_init(struct cubie_twi* twi, uint32_t base_addr, uint32_t clock_speed) {
    twi->base = (volatile uint32_t*)base_addr;
    twi->clock_speed = clock_speed;

    // Soft-reset TWI controller
    twi->base[TWI_SRST] = 0x1;
    while (twi->base[TWI_SRST] & 0x1); // Wait for reset

    // Set clock divisor (simplified; adjust based on AHB clock)
    // Example: For 24 MHz AHB clock and 100 kHz SCL:
    uint32_t div = 24000000 / (2 * twi->clock_speed);
    twi->base[TWI_CLK] = div & 0xFFFF;
}

void twi_start(struct cubie_twi* twi) {
    twi->base[TWI_CTRL] = TWI_CTRL_M_STA_HIGH | TWI_CTRL_BUS_EN_HIGH | TWI_CTRL_INT_EN_HIGH; // START bit & BUS_EN

    while (twi->base[TWI_CTRL] & (1 << 5)); // Wait for START to clear

    while (!(twi->base[TWI_CTRL] & (1 << 3))); // Wait for IRQ
}

void twi_stop(struct cubie_twi* twi) {
    twi->base[TWI_CTRL] = (1 << 1) | // STOP bit
                           (1 << 3); // Interrupt flag clear
}

int twi_write(struct cubie_twi* twi, uint8_t data) {
    twi->base[TWI_DATA] = data;
    twi->base[TWI_CTRL] = (1 << 3); // Clear IRQ
    while (!(twi->base[TWI_CTRL] & (1 << 3))); // Wait for ACK/NACK

    // Check status (simplified; see A10 manual for full codes)
    uint32_t stat = twi->base[TWI_STAT];
    if (stat != 0x28) return -1; // 0x28 = byte transmitted + ACK
    return 0;
}

int twi_read(struct cubie_twi* twi, bool send_ack) {
    // Start the read operation
    twi->base[TWI_CTRL] = send_ack ? (1 << 2) : 0; // Set ACK bit if needed
    twi->base[TWI_CTRL] |= (1 << 3); // Clear IRQ

    // Wait for data to be received
    while (!(twi->base[TWI_CTRL] & (1 << 3)));

    // Check status
    uint32_t stat = twi->base[TWI_STAT];
    if (send_ack && stat != 0x50) return -1;  // Data received + ACK sent
    if (!send_ack && stat != 0x58) return -1; // Data received + NACK sent

    // Return received data
    return twi->base[TWI_DATA];
}


#define EEPROM_ADDR 0x50
#define HEADER_SIZE 8  // Basic header size

struct eeprom_header {
    uint8_t magic;       // Should be 0xAA or similar
    uint8_t version;     // EEPROM format version
    uint8_t board_type;  // Cubieboard type/variant
    uint8_t board_rev;   // Board revision
    uint32_t size;       // Total EEPROM size in bytes
};

int read_eeprom_header(struct cubie_twi* twi, struct eeprom_header* header) {
    uint8_t buffer[HEADER_SIZE];

    printk("Status: %p\n", twi->base[TWI_STAT]);

    // Start condition
    twi_start(twi);
    printk("OK\n");

    // Write device address
    if (twi_write(twi, (EEPROM_ADDR << 1) | 0) < 0)
        return -1;

    // Write memory address (starting at 0)
    if (twi_write(twi, 0) < 0)
        return -1;

    // Repeated start for reading
    twi_start(twi);

    // Write device address for reading
    if (twi_write(twi, (EEPROM_ADDR << 1) | 1) < 0)
        return -1;

    // Read header bytes
    for (int i = 0; i < HEADER_SIZE - 1; i++) {
        int data = twi_read(twi, true);  // Send ACK for all but last byte
        if (data < 0) return -1;
        buffer[i] = data;
    }

    // Read last byte with NACK
    int data = twi_read(twi, false);
    if (data < 0) return -1;
    buffer[HEADER_SIZE - 1] = data;

    // Stop condition
    twi_stop(twi);

    // Parse header
    header->magic = buffer[0];
    header->version = buffer[1];
    header->board_type = buffer[2];
    header->board_rev = buffer[3];
    header->size = (buffer[4] << 24) | (buffer[5] << 16) |
                  (buffer[6] << 8) | buffer[7];

    return 0;
}


// doesn't work.
uint8_t rtc_i2c_read(struct cubie_twi* twi, uint8_t reg_addr) {
    (void)twi, (void)reg_addr;
    panic("unimplemented!\n");
    struct eeprom_header header;

    read_eeprom_header(twi, &header);
    printk("Header read OK!\n");


    // uint8_t data[128];
    // twi_start(twi);
    // uint8_t memaddr = 0;

    // // Send device address (0x50 << 1) | WRITE
    // twi_write(twi, (0x50 << 1) | 0);

    // // Send memory address
    // twi_write(twi, memaddr);

    // // Repeated start
    // twi_start(twi);

    // // Send device address for read
    // twi_write(twi, (0x50 << 1) | 1);

    // // Read data bytes
    // for(int i = 0; i < 128; i++) {
    //     data[i] = twi_read(twi, i < 128-1);  // Send ACK except for last byte
    // }

    // twi->slave_addr = 0xA2; // PCF8563 I2C address (7-bit: 0x51 << 1)

    // // Send START + Slave Address (Write)
    // twi_start(twi);
    // twi_write(twi, (twi->slave_addr & 0xFE)); // Write mode

    // // Send register address to read from
    // twi_write(twi, reg_addr);

    // // Repeated START + Slave Address (Read)
    // twi_start(twi);
    // twi_write(twi, (twi->slave_addr | 0x1)); // Read mode

    // // Read data (NACK final byte)
    // twi->base[TWI_CTRL] = (1 << 3); // Clear IRQ
    // while (!(twi->base[TWI_CTRL] & (1 << 3)));
    // uint8_t data = twi->base[TWI_DATA];

    // twi_stop(twi);
    // return data;
    return 0;
}
