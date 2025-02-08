#ifndef UART_H
#define UART_H

#include <stdint.h>
#define UART0_BASE 0x01C28000

typedef struct {
    volatile uint32_t RBR_THR_DLL; // 0x00: Receive/Transmit Buffer
    volatile uint32_t IER_DLH;     // 0x04: Interrupt Enable
    volatile uint32_t IIR_FCR;     // 0x08: Interrupt Identification
    volatile uint32_t LCR;         // 0x0C: Line Control
    volatile uint32_t MCR;         // 0x10: Modem Control
    volatile uint32_t LSR;         // 0x14: Line Status
    volatile uint32_t MSR;         // 0x18: Modem Status
    volatile uint32_t SCR;         // 0x1C: Scratch
} UART_Reg;

#define UART0 ((UART_Reg *)UART0_BASE)

// Interrupt Enable Register (IER) bits
#define UART_IER_RX_INT 0x01  // Receive Data Available Interrupt

#endif
