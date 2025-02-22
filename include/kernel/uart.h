#ifndef DRIVERS_UART_H
#define DRIVERS_UART_H

#include <stdint.h>
#include <kernel/sched.h>

#define UART0_INCOMING_BUFFER_SIZE 256


typedef struct {
    // TODO support multiple UARTs, qemu only has one
    void (*init)(void);
    void (*putc)(char c);
    char (*getc)(void);
    void (*enable_interrupts)(void);
    void (*set_interrupt_handler)(void (*handler)(void));
    void (*disable_interrupts)(void);

    char incoming_buffer[UART0_INCOMING_BUFFER_SIZE];
    uint32_t incoming_buffer_head;
    uint32_t incoming_buffer_tail;

    process_t* wait_queue;
} uart_driver_t;

extern uart_driver_t uart_driver;

// TEMP
void uart_init_interrupts(void);

#endif
