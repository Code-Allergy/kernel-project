/* uldivmod.S */
.global __aeabi_uldivmod
__aeabi_uldivmod:
    /* Input: 
        r0-r1 = numerator 
        r2-r3 = denominator 
       Output:
        r0-r1 = quotient
        r2-r3 = remainder
    */
    stmfd sp!, {r4-r11, lr}
    
    mov r4, r0        /* num_lo */
    mov r5, r1        /* num_hi */
    mov r6, r2        /* den_lo */
    mov r7, r3        /* den_hi */
    
    mov r8, #0        /* rem_lo */
    mov r9, #0        /* rem_hi */
    mov r10, #0       /* quo_lo */
    mov r11, #0       /* quo_hi */
    mov r12, #64      /* bit counter */
    
1:  /* Main loop */
    /* Shift numerator left into remainder */
    lsls r4, r4, #1
    adc r5, r5, r5
    adc r8, r8, r8
    adc r9, r9, r9
    
    /* Compare remainder >= denominator */
    cmp r9, r7
    blo 2f
    bhi 3f
    cmp r8, r6
    blo 2f
    
3:  /* Subtract denominator from remainder */
    subs r8, r8, r6
    sbc r9, r9, r7
    
    /* Set quotient bit */
    orr r10, r10, #1
    
2:  /* Shift quotient */
    subs r12, r12, #1
    beq 4f
    lsl r10, r10, #1
    lsl r11, r11, #1
    orr r10, r10, r11, lsr #31
    orr r11, r11, r10, lsr #31
    b 1b
    
4:  /* Prepare return values */
    mov r0, r10       /* quo_lo */
    mov r1, r11       /* quo_hi */
    mov r2, r8        /* rem_lo */
    mov r3, r9        /* rem_hi */
    
    ldmfd sp!, {r4-r11, pc}