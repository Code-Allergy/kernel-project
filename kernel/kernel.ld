/* linker.ld */
ENTRY(kernel_main)  /* Entry point */

/* we should have separate sections */
MEMORY
{
    RAM (rwx) : ORIGIN = 0xC0000000, LENGTH = 0x4000000
}

SECTIONS {
    . = ORIGIN(RAM);

    /* The .text section holds executable code */
    .text : {
        . = ALIGN(4096);
        KEEP(*(.text.kernel_main))
        *(.text*)    /* Include all .text sections (code) */
    }

    .vectors : {
        KEEP(*(.vectors))  /* Force inclusion of vector section */
        . = ALIGN(32);     /* Mandatory for VBAR alignment */
        _vvectors = .;     /* Create symbol for VBAR initialization */
    } > RAM

    .padding_after_code : {  /* padding to 4KB */
        . = ALIGN(4096);
        . += 0;
    } > RAM

    kernel_code_end = ALIGN(4096);

    /* The .data section holds initialized data */
    .data : {
        *(.data)    /* Include all .data sections (initialized variables) */
    }

    /* stack debugging */
    .debug_area : {
        . = ALIGN(4096);
        /* Large unused section to prevent accidental overwrites */
        *(.debug_area)
        . = . + 0x4000;  /* Add a 16KB debug gap */
    } > RAM


    .stacks : {
        . = ALIGN(8);

        /* Reserve space for stack canaries */
        canary_value_start = .;
        . += 4;  /* 4 bytes for stack canary */

        abort_stack_bottom = .;
        . += 0x1000;
        abort_stack_top = .;

        undef_stack_bottom = .;
        . += 0x1000;
        undef_stack_top = .;

        irq_stack_bottom = .;
        . += 0x1000;
        irq_stack_top = .;

        fiq_stack_bottom = .;
        . += 0x1000;
        fiq_stack_top = .;

        svc_stack_bottom = .;
        . += 0x1000;
        svc_stack_top = .;

        sys_stack_bottom = .;
        . += 0x1000;
        sys_stack_top = .;

        canary_value_end = .;
        . += 4;
    } > RAM

    /* The .bss section holds uninitialized data */
    .bss : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    } > RAM

    .padding_after_bss : {  /* padding to 4KB */
        . = ALIGN(4096);
    } > RAM

    kernel_end = .;
}
