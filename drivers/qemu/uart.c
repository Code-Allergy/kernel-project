#include <stdint.h>

#include <kernel/uart.h>
#include <kernel/printk.h>
#include <kernel/intc.h>
#include <kernel/panic.h>
#include "uart.h"
#include "intc.h"

// UART0 Base Allwinner A10 (Cubieboard)

void uart_init(void) {
    // Disable interrupts (IER)
    UART0->IER_DLH = 0x00;

    // Enable Divisor Latch Access (LCR[7] = 1)
    UART0->LCR = 0x80;

    // Set Divisor for 115200 baud (assuming 24 MHz clock)
    // Divisor = (24,000,000) / (16 * 115200) ~= 13
    UART0->RBR_THR_DLL = 13;  // DLL (Divisor Latch Low)
    UART0->IER_DLH = 0x00;    // DLH (Divisor Latch High)

    // Configure 8N1 (8 data bits, no parity, 1 stop bit)
    UART0->LCR = 0x03;  // LCR[1:0] = 11 (8 bits), LCR[3] = 0 (no parity)

    // Enable FIFO (FCR[0] = 1)
    UART0->IIR_FCR = 0x01;
}

void uart_init_interrupts(void) {
    // Disable UART first
    UART0->IER_DLH = 0;

    // Configure line control (8N1)
    UART0->LCR = 0x03; // 8 bits, no parity, 1 stop bit

    // Enable FIFOs
    UART0->IIR_FCR = 0x01;

    // Enable receive interrupts
    UART0->IER_DLH = UART_IER_RX_INT;

    interrupt_controller.register_irq(1, uart_handler, NULL);
    interrupt_controller.enable_irq(1);
}

void uart_disable_interrupts(void) {
    panic("Disabling UART interrupts unimplemented\n");
    // Disable UART
    // UART0->IER_DLH = 0;
}

void uart_putc(char c) {
    // Wait until Transmit Holding Register is empty (LSR[5] = 1)
    while (!(UART0->LSR & (1 << 5)));
    UART0->RBR_THR_DLL = c;
}

char uart_getc(void) {
    // Wait until Data Ready (LSR[0] = 1)
    while (!(UART0->LSR & 1));
    return UART0->RBR_THR_DLL;
}

// simple handler for cubieboard
void uart_handler(int irq, void *data) {
    (void)data;
    char c = UART0->RBR_THR_DLL;  // Read the character (clears interrupt)
    uart_driver.incoming_buffer[uart_driver.incoming_buffer_head++ % UART0_INCOMING_BUFFER_SIZE] = c;
    INTC->IRQ_PEND[0] = (irq << 1);
    // uart_putc(c); // echo the character for now
}



uart_driver_t uart_driver = {
    .init = uart_init,
    .putc = uart_putc,
    .getc = uart_getc,
    .enable_interrupts = uart_init_interrupts,
    .disable_interrupts = uart_disable_interrupts,

    .incoming_buffer = {0},
    .incoming_buffer_head = 0,
    .incoming_buffer_tail = 0,
};
