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
	// minimalistic cortex-a8 initialization
	subs	r4, lr, #4	// vector base, except bit 0 indicates thumb

	cpsid	fia, #0b11111	// thread mode, mask all async exceptions

	ldr	sp, =stack_top

	mcr	p15, 0, r4, c12, c0, 0	// set vector base (bit 0 is ignored)

	mov	r0, #(3 << 11)		// enable branch prediction and i-cache
	bfi	r0, r4, #30, #1		// configure arm/thumb-mode exceptions
	mcr	p15, 0, r0, c1, c0, 0	// system control register

	cpsie	a			// enable async aborts

main:
	push	{ lr }

	bl loader
	pop	{ pc }
