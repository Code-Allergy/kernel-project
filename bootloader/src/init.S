//-------------- Allocate stack ----------------------------------------------//

.pushsection .noinit.stack, "aw", %nobits
    .balign 8           // Align the stack on an 8-byte boundary
stack:
    .space 512          // Reserve 512 bytes for the stack
    .size stack, 512    // Define the size of the stack
    .global stack        // Make stack a global symbol
    .set stack_top, .    // Define stack_top as the top of the stack
.popsection

.global init
init:
    .fpu neon
	// minimalistic cortex-a8 initialization
	subs	r4, lr, #4	// vector base, except bit 0 indicates thumb

	cpsid	fi, #0b11111	// thread mode, mask all async exceptions

	ldr	sp, =stack_top

	mcr	p15, 0, r4, c12, c0, 0	// set vector base (bit 0 is ignored)

	mov	r0, #(3 << 11)		// enable branch prediction and i-cache
	bfi	r0, r4, #30, #1		// configure arm/thumb-mode exceptions
	mcr	p15, 0, r0, c1, c0, 0	// system control register

	/* System Control Register Setup */
    mrc     p15, 0, r0, c1, c0, 0 // Read SCTLR
    orr     r0, r0, #(1 << 12)    // I-cache enable
    orr     r0, r0, #(1 << 11)    // Branch prediction enable
    bic     r0, r0, #(1 << 30)    // ARM exception handling (TE=0)
    mcr     p15, 0, r0, c1, c0, 0 // Write SCTLR

   	/* altcr reg */
	mrc     p15, 0, r0, c1, c0, 1    // Read ACTLR
    orr     r0, r0, #(1 << 2)        // Enable L2 prefetch
    orr     r0, r0, #(1 << 6)        // Enable SMP (does nothing on BBB/cubie)
    mcr     p15, 0, r0, c1, c0, 1    // Write ACTLR

    /* Enable NEON/VFP */
    mrc   p15, 0, r0, c1, c0, 2  // Read CPACR
    orr   r0, r0, #(0xF << 20)   // Full access to CP10/11
    mcr   p15, 0, r0, c1, c0, 2  // Write CPACR
    isb
    mov   r0, #0x40000000         // Enable VFP
    vmsr  FPEXC, r0

	//cpsie	a			 enable async aborts
	b loader
