.global setup_stacks
setup_stacks:
    // no hardware support for FIQs, we don't setup a stack for it

    // Switch to IRQ mode
    mrs r0, cpsr
    bic r0, r0, #0x1F       // Clear mode bits
    orr r0, r0, #0x12       // Set IRQ mode (0x12)
    msr cpsr_c, r0
    ldr sp, =irq_stack_top - 4

    // Switch to Abort mode
    mrs r0, cpsr
    bic r0, r0, #0x1F
    orr r0, r0, #0x17       // Abort mode (0x17)
    msr cpsr_c, r0
    ldr sp, =abort_stack_top - 4

    // Switch to Undefined mode
    mrs r0, cpsr
    bic r0, r0, #0x1F
    orr r0, r0, #0x1B       // Undefined mode (0x1B)
    msr cpsr_c, r0
    ldr sp, =undef_stack_top - 4

    // Switch to SVC mode
    mrs r0, cpsr
    bic r0, r0, #0x1F
    orr r0, r0, #0x13       // SVC mode (0x13)
    msr cpsr_c, r0
    ldr sp, =svc_stack_top - 4

    // Switch to System mode
    mrs r0, cpsr
    bic r0, r0, #0x1F
    orr r0, r0, #0x1F       // System mode (0x1F)
    msr cpsr_c, r0
    ldr sp, =sys_stack_top - 4

    bx lr
