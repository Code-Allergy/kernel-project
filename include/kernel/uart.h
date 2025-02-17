#ifndef DRIVERS_UART_H
#define DRIVERS_UART_H

typedef struct {
    // TODO support multiple UARTs, qemu only has one
    void (*init)(void);
    void (*putc)(char c);
    char (*getc)(void);
    void (*enable_interrupts)(void);
    void (*disable_interrupts)(void);
} uart_driver_t;

extern const uart_driver_t uart_driver;

// TEMP
void uart_init_interrupts(void);

#endif
