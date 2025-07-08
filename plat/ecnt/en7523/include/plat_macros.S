#include <aarch64/arm_macros.S>
#include <en7523_def.h>


	.macro plat_crash_print_regs
	/* Assume Base Cortex mmap */
	mov_imm	x17, BASE_GICC_BASE
	mov_imm	x16, BASE_GICD_BASE
	arm_print_gic_regs
	.endm
