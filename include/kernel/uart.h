#ifndef DRIVERS_UART_H
#define DRIVERS_UART_H

#include <stdint.h>

#define UART0_INCOMING_BUFFER_SIZE 256


typedef struct {
    // TODO support multiple UARTs, qemu only has one
    void (*init)(void);
    void (*putc)(char c);
    char (*getc)(void);
    void (*enable_interrupts)(void);
    void (*disable_interrupts)(void);

    char incoming_buffer[UART0_INCOMING_BUFFER_SIZE];
    volatile uint32_t incoming_buffer_head;
    volatile uint32_t incoming_buffer_tail;
} uart_driver_t;

extern uart_driver_t uart_driver;

// TEMP
void uart_init_interrupts(void);

#endif
