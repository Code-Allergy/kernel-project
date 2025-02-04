.global swtch

swtch:
    @ Save registers before switching
    STMFD   SP!, {R0-R12, LR}

    @ Switch to System Mode
    MRS     R0, CPSR
    BIC     R0, R0, #0x1F
    ORR     R0, R0, #0x1F
    MSR     CPSR_c, R0

    @ Perform operations in System Mode...

    @ Restore User Mode
    MRS     R0, CPSR
    BIC     R0, R0, #0x1F
    ORR     R0, R0, #0x10
    MSR     CPSR_c, R0

    @ Restore registers
    LDMFD   SP!, {R0-R12, PC}^   @ Return to User Mode