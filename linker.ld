/* linker.ld */
ENTRY(kernel_main)  /* Entry point */

SECTIONS {
    /* Set the memory regions: we are using 0x80000000 as the start address */
    . = 0x40120000; /* Start at 0x80000000 */

    /* The .text section holds executable code */
    .text : {
        KEEP(*(.text.kernel_main))

        *(.text*)    /* Include all .text sections (code) */
    }

    /* The .data section holds initialized data */
    .data : {
        *(.data)    /* Include all .data sections (initialized variables) */
    }

    /* The .bss section holds uninitialized data */
    .bss : {
        *(.bss)     /* Include all .bss sections (uninitialized variables) */
    }

    /* The .stack section for the stack */
    .stack : {
        . = ALIGN(8);
        *(.stack)
    }
}
