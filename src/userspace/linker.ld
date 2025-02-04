ENTRY(_start)

SECTIONS {
    . = 0x80000000; /* Start of the program (adjust as needed) */

    .text : {
        *(.text)  /* Collect all .text sections (code) */
    }

    .data : {
        *(.data)  /* Collect all .data sections (initialized data) */
    }

    .bss : {
        *(.bss)   /* Collect all .bss sections (uninitialized data) */
    }



    .heap : {
        . = . + 0x1000;  /* 4KB heap */
    }

    /* Stack and heap setup (adjust size as needed) */
    .stack : {
        . = . + 0x400;
    }

    /* Ensure .end section at the end of the program */
    .end : {
        _end = .;
    }
}