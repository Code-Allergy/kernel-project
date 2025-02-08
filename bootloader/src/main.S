#include <memory_map.h>

.cpu	cortex-a8
.text

.global	main
main:
	push	{ lr }

	bl loader
	pop	{ pc }
