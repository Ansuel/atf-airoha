/*
 * EN7523 open-source BL1 (BootROM stage)
 *
 * Clean-room reimplementation of the vendor blob
 *   plat/ecnt/blobs/en7523/bl1.bin   (2016 bytes, AArch32/ARMv7-A)
 *
 * The BootROM loads this image into a <=2KB window and jumps to offset 0.
 * Layout mirrors the blob: entry @0x00, exception vectors @0x7C0, image
 * ends at <=0x800.  The image is linked at base 0x0 (platform_def.h
 * BL1_RO_BASE == 0x0) because the self-relocation below computes the
 * destination of the "second half" as  0x1f000000 + <link address>.
 *
 * Boot flow (identical to the blob):
 *   1. reset SCTLR, enter Monitor(mode 0x16)
 *   2. VBAR/MVBAR = _bl1_vectors
 *   3. Cortex-A53 core-specific init (MIDR dispatch table)
 *   4. SCTLR.I/A, SCR.SIF, NSACR(CP10/CP11 for NS), aux/attr, VFP probe
 *   5. MCUCFG (AXI/L2C) + NPU-SRAM self-test & clear   [C: bl1_plat_init]
 *   6. relocate whole image 0x0 -> 0x1f000000, continue there
 *   7. read SPI strap, load next stage into L2 SRAM, jump
 *                                                    [C: bl1_boot_next_stage]
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

	.syntax unified
	.arch	armv7-a
	.arm

/* ------------------------------------------------------------------ */
/* Boot stack: must NOT live in the NPU-SRAM region cleared by step 5.  */
/* Use NPU SRAM2 (en7523_def.h ECNT_NPU_SRAM2_BASE=0x1E900000, 16KB).   */
/* ------------------------------------------------------------------ */
#define BL1_BOOT_STACK		0x1E904000

	/* ============================ entry ============================ */
	.section .text.start, "ax", %progbits
	.global	_bl1_start
	.type	_bl1_start, %function
_bl1_start:
	/* 1. SCTLR -> known reset value: MMU/D-cache/I-cache off */
	ldr	r0, =0x00c50838
	mcr	p15, 0, r0, c1, c0, 0
	isb

	/* 2. run in Monitor mode (secure) */
	cps	#0x16
	isb

	/* 3. exception vectors (set before anything can fault) */
	ldr	r0, =_bl1_vectors
	mcr	p15, 0, r0, c12, c0, 0		/* VBAR  */
	mcr	p15, 0, r0, c12, c0, 1		/* MVBAR */
	isb

	/* boot stack (blob sets this later, just before relocation) */
	ldr	sp, =BL1_BOOT_STACK

	/* 4. Cortex-A53 core init via MIDR table */
	bl	bl1_cpu_early_init

	/* 5a. enable alignment check + I-cache */
	mrc	p15, 0, r0, c1, c0, 0
	orr	r0, r0, #0x1000
	orr	r0, r0, #0x2
	mcr	p15, 0, r0, c1, c0, 0
	isb

	/* 5b. SCR.SIF = 1, enable async abort */
	mov	r0, #0x200
	mcr	p15, 0, r0, c1, c1, 0
	cpsie	a
	isb

	/* 5c. NSACR: preserve [18:16], allow CP10/CP11 (VFP) in NS world */
	mrc	p15, 0, r0, c1, c1, 2
	and	r0, r0, #0x70000
	orr	r0, r0, #0xc00
	mcr	p15, 0, r0, c1, c1, 2
	isb

	/* 5d. auxiliary / attribute registers (transcribed verbatim) */
	mov	r0, #0xf00000
	mcr	p15, 0, r0, c1, c0, 2
	ldr	r0, =0x00808000
	mcr	p15, 0, r0, c1, c3, 1
	mov	r0, #0xe0
	mcr	p15, 0, r0, c9, c12, 0
	isb

	/* 5e. VFP/ThumbEE present? -> set CPSR.A */
	mrc	p15, 0, r0, c0, c1, 0		/* ID_PFR0 */
	and	r0, r0, #0x0f000000
	cmp	r0, #0x01000000
	bne	1f
	mrs	r0, cpsr
	orr	r0, r0, #0x200000
	msr	cpsr_fsxc, r0
1:
	/* 6. MCUCFG (AXI/L2C) + NPU SRAM self-test & clear */
	bl	bl1_plat_init

	/* 7. release 0x1f000000 window, relocate, continue there */
	b	bl1_relocate

	/* ==================== self-relocation ==================== */
	.type	bl1_relocate, %function
bl1_relocate:
	ldr	r2, =0x1fb00958			/* SCU hand-off register */
	mov	r0, #1
	str	r0, [r2]

	ldr	r0, =0x1f000000			/* dst */
	ldr	r1, =0x0			/* src (link base) */
	ldr	r3, =_bl1_image_end		/* size (8-byte aligned) */
	add	r2, r1, r3			/* end address */
	add	r3, r0, #0			/* dst cursor */
	ldr	r4, =_bl1_resume
	add	r4, r4, r0			/* relocated resume address */
.Lreloc_copy:
	ldm	r1!, {r5, r6}
	stmia	r3!, {r5, r6}
	cmp	r1, r2
	blo	.Lreloc_copy
	bx	r4

	/* ============ relocated continuation (was blob @0x2F0) ============ */
	.type	_bl1_resume, %function
_bl1_resume:
	bl	bl1_boot_next_stage		/* C, noreturn */
	b	bl1_panic

	/* ==================== panic / hang ==================== */
	.global	bl1_panic
	.type	bl1_panic, %function
bl1_panic:
	wfi
	b	bl1_panic
	.size	bl1_panic, . - bl1_panic

/* ------------------------------------------------------------------ */
/* Exception handler: any exception is fatal (blob @0x6EC).            */
/* ------------------------------------------------------------------ */
	.type	bl1_exc_handler, %function
bl1_exc_handler:
	mrs	r0, cpsr
	and	r0, r0, #0x1f
	b	bl1_panic

/* ------------------------------------------------------------------ */
/* Cortex-A53 core-specific init, dispatched on MIDR (@0x648/0x19C).   */
/* ------------------------------------------------------------------ */
	.type	bl1_cpu_early_init, %function
bl1_cpu_early_init:
	mov	r8, lr
	bl	bl1_midr_match			/* r0 = table entry | 0 */
	cmp	r0, #0
	beq	1f
	ldr	r1, [r0, #4]			/* init fn pointer */
	cmp	r1, #0
	mov	lr, r8
	bxne	r1				/* run it (returns to lr) */
	bx	lr

	.type	bl1_midr_match, %function
bl1_midr_match:
	ldr	r4, =_bl1_cpu_table
	ldr	r5, =_bl1_cpu_table_end
	mov	r0, #0
	mrc	p15, 0, r2, c0, c0, 0		/* MIDR */
	ldr	r3, =0xff00fff0
	and	r2, r2, r3
0:	cmp	r4, r5
	bhs	1f
	ldr	r1, [r4], #8
	and	r1, r1, r3
	cmp	r1, r2
	bne	0b
	sub	r0, r4, #8
1:	bx	lr

	/* Cortex-A53 (part 0xD03) init (@0x19C) */
	.type	bl1_a53_init, %function
bl1_a53_init:
	mov	r5, lr
	bl	bl1_midr_part			/* r0 = part number */
	mov	r4, r0
	mov	r0, r4
	bl	bl1_impl_reg_set_bit24
	mov	r0, r4
	bl	bl1_impl_reg_touch
	mrrc	p15, 1, r0, r1, c15
	orr	r0, r0, #0x40
	mcrr	p15, 1, r0, r1, c15
	isb
	bx	r5

	/* r0 = ((MIDR >> 16) & 0xFF) | (MIDR & 0xF) (@0x6AC) */
	.type	bl1_midr_part, %function
bl1_midr_part:
	mrc	p15, 0, r1, c0, c0, 0
	ubfx	r0, r1, #16, #8
	bfi	r0, r1, #0, #4
	bx	lr

	/* if (part <= 3) impl_reg(c15,opc1=0) |= BIT(24)  (@0x148) */
	.type	bl1_impl_reg_set_bit24, %function
bl1_impl_reg_set_bit24:
	mov	r2, lr
	mov	r1, #3
	bl	bl1_cmp_le
	mov	lr, r2
	cmp	r0, #0
	beq	1f
	mrrc	p15, 0, r0, r1, c15
	orr	r0, r0, #0x1000000
	mcrr	p15, 0, r0, r1, c15
1:	bx	lr

	/* if (part >= 3) impl_reg(c15,opc1=0) read-modify-write  (@0x174) */
	.type	bl1_impl_reg_touch, %function
bl1_impl_reg_touch:
	mov	r2, lr
	mov	r1, #3
	bl	bl1_cmp_ge
	mov	lr, r2
	cmp	r0, #0
	beq	1f
	mrrc	p15, 0, r0, r1, c15
	mcrr	p15, 0, r0, r1, c15
1:	bx	lr

	/* r0 = (r0 <= r1) ? 1 : 0  (@0x6BC) */
	.type	bl1_cmp_le, %function
bl1_cmp_le:
	cmp	r0, r1
	movls	r0, #1
	movhi	r0, #0
	bx	lr

	/* r0 = (r0 >= r1) ? 1 : 0  (@0x6CC) */
	.type	bl1_cmp_ge, %function
bl1_cmp_ge:
	cmp	r0, r1
	movge	r0, #1
	movlt	r0, #0
	bx	lr

/* ------------------------------------------------------------------ */
/* MIDR dispatch table (blob @0x7A0): (midr_masked, init_fn | 0)       */
/* ------------------------------------------------------------------ */
	.section .rodata._bl1_cpu_table, "a", %progbits
	.align	3
	.global	_bl1_cpu_table
_bl1_cpu_table:
	.word	0x410fd030			/* Cortex-A53 */
	.word	bl1_a53_init
_bl1_cpu_table_end:

/* ------------------------------------------------------------------ */
/* Exception vectors (blob @0x7C0). Linker script places .vectors @0x7C0 */
/* ------------------------------------------------------------------ */
	.section .vectors, "ax", %progbits
	.align	5
	.global	_bl1_vectors
_bl1_vectors:
	b	_bl1_start			/* reset        */
	b	bl1_exc_handler			/* undefined    */
	b	bl1_exc_handler			/* SVC          */
	b	bl1_exc_handler			/* prefetch abt */
	b	bl1_exc_handler			/* data abort   */
	b	bl1_exc_handler			/* reserved     */
	b	bl1_exc_handler			/* IRQ          */
	b	bl1_exc_handler			/* FIQ          */
