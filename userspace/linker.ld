ENTRY(_start)

SECTIONS {
    . = 0x10000; /* Start of the program (adjust as needed) */

    .text : {
        *(.text)  /* Collect all .text sections (code) */
    }

    .data : {
        *(.data)  /* Collect all .data sections (initialized data) */
    }

    .bss : {
        *(.bss)    /* Include all bss sections (uninitialized data) */
    }

    . = ALIGN(4); /* Align the sections on 4-byte boundaries */
}