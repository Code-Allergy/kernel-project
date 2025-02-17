// place in special section to allow linker script to place it at beginning
.section .text._start

// force ARM mode (required by ROM booloader)
.arm

// entrypoint
.global _start
_start:
	bl init
