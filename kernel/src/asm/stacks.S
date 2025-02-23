
.global setup_stacks
setup_stacks:
    /* switch to secure mode */
    /* TODO */

    /* all -4 because we need to align them to 8 bytes */

    /* Switch to FIQ mode and set stack pointer */
    msr CPSR_c, #0x11     /* FIQ mode */
    ldr sp, =fiq_stack_top - 4

    /* Switch to IRQ mode and set stack pointer */
    msr CPSR_c, #0x12     /* IRQ mode */
    ldr sp, =irq_stack_top - 4

    /* Switch to Abort mode and set stack pointer */
    msr CPSR_c, #0x17     /* Abort mode */
    ldr sp, =abort_stack_top - 4

    /* Switch to Undefined Instruction mode */
    msr CPSR_c, #0x1B     /* Undefined mode */
    ldr sp, =undef_stack_top - 4

    /* Switch to Supervisor mode */
    msr CPSR_c, #0x13     /* SVC mode */
    ldr sp, =svc_stack_top - 4

    /* Switch back to System mode (or User mode if needed) */
    /* Save return address */
    /*mov r0, lr           Store lr in r0 temporarily */
    /* Switch back to System mode */
    msr CPSR_c, #0x1F   /* System mode */
    ldr sp, =sys_stack_top - 4 /* Load the new stack pointer */
    /* Restore return address */
    /*mov lr, r0           Restore lr */

    bx lr
