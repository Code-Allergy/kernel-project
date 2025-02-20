ENTRY(_start)

MEMORY
{
    ROM (rx)  : ORIGIN = 0x40010000, LENGTH = 0x100000  /* 1MB */
    RAM (rwx) : ORIGIN = 0x40110000, LENGTH = 0x4000000 /* 64MB */
}

SECTIONS
{
    . = ORIGIN(ROM);
    .text : { *(.text) }

    . = ORIGIN(RAM);
    .data : { *(.data) }
    .bss : { *(.bss COMMON) }

    .stack (NOLOAD) : {
        . = ALIGN(16);
        _stack_bottom = .;
        . += 0x1000;
        _stack_top = .;
    }

    .boot_tables 0x5F610000 (NOLOAD) : {
        _boot_tables_start = .;
        *(.boot_tables*)
        _boot_tables_end = .;
    }
}
