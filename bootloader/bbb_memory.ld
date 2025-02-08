ENTRY(_start)

MEMORY
{
    INTERNAL_RAM (rwx)  : ORIGIN = 0x402F0000, LENGTH = 0x10000     /* 16KB (first 1KB reserved) */
    L3_OCMCO_RAM (rwx)  : ORIGIN = 0x40300000, LENGTH = 0x10000     /* 64KB */
    EXTERNAL_RAM (rwx)  : ORIGIN = 0x80000000, LENGTH = 0x20000000  /* 512MB */
}

/* ROM bootloader just loads a single contiguous segment, so make the ELF
 * program headers do the same */
PHDRS {
	image PT_LOAD;
}

SECTIONS
{
    /* Reserve the first 1 KB of INTERNAL_RAM */
    .reserved ORIGIN(INTERNAL_RAM) (NOLOAD) : { . += 0x400; } > INTERNAL_RAM :NONE
    . = ORIGIN(INTERNAL_RAM) + 0x400; /* Skip 1 KB (1024 bytes) */

    /* .text section */
    .text : {
		KEEP( *( .text._start ) )
		*( .text .text.* )
	} :image
    . = ALIGN(4);  /* Ensure proper alignment for the next section */

    /* .rodata section */
    .rodata : {
		*( .rodata .rodata.* )
    } :image
    . = ALIGN(4);  /* Ensure proper alignment for the next section */

    /* .data section */
    .data : {
		*( .data .data.* )
	} :image

    /* Stack (4 KB) */
    . = ALIGN(8);
    . = . + 0x1000; /* 4 KB stack memory */
    _stack_start = .;

    /* Shared uninitialized RAM */
	.shared (NOLOAD) : { *( .shared .shared.* ) } >L3_OCMCO_RAM :NONE

	/* Private uninitialized RAM */
	.noinit (NOLOAD) : { *( .noinit .noinit.* ) } >INTERNAL_RAM :NONE

    /* Catch any unknown sections and yell loudly about them */

}
