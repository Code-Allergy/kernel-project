.section .vectors, "ax"
.global _vectors
.align 5
_vectors:
    ldr pc, reset_addr    /* 0x00 Reset */
    ldr pc, undef_addr    /* 0x04 Undefined Instruction */
    ldr pc, swi_addr      /* 0x08 SWI */
    ldr pc, prefetch_addr /* 0x0C Prefetch Abort */
    ldr pc, data_addr     /* 0x10 Data Abort */
    nop                   /* 0x14 Reserved */
    ldr pc, irq_addr      /* 0x18 IRQ */
    ldr pc, fiq_addr      /* 0x1C FIQ */

/* Address table */
reset_addr:    .word reset_handler
undef_addr:    .word undef_handler
swi_addr:      .word swi_handler
prefetch_addr: .word prefetch_abort_handler
data_addr:     .word data_abort_handler
irq_addr:      .word irq_handler
fiq_addr:      .word fiq_handler

/* Default handlers */
.section .text
reset_handler:
undef_handler:
swi_handler:
prefetch_abort_handler:
data_abort_handler:
fiq_handler:
    b .  /* Infinite loop for unhandled exceptions */


irq_handler:
    sub lr, lr, #4        /* Adjust LR for IRQ return */
    mrs r0, spsr          /* Save SPSR */
    push {r0, r1-r12, lr} /* Save context and SPSR */
    
    bl handle_irq_c       /* Call C handler */
    
    pop {r0, r1-r12, lr}  /* Restore context and SPSR */
    msr spsr_cxsf, r0     /* Restore SPSR */
    subs pc, lr, #0       /* Return from exception */

