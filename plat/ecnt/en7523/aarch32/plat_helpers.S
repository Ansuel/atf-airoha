/*
 * Copyright (c) 2013-2015, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <arch.h>
#include <asm_macros.S>
#include <en7523_def.h>

	.globl	plat_secondary_cold_boot_setup
	.globl	plat_is_my_cpu_primary
	.globl  plat_my_core_pos
	.globl	plat_crash_console_init
	.globl	plat_crash_console_putc
	.globl	plat_crash_console_flush
	.globl	platform_mem_init
	.globl	plat_invaild_clean_cache
#if defined(IMAGE_BL1)
	.globl	platform_flash_init
#else
	.globl	jumparch64
#endif

.section .rodata.magic_str
.align 4
magic_str:
	.ascii "ECNT"

	/* -----------------------------------------------------
	 * void plat_secondary_cold_boot_setup (void);
	 *
	 * This function performs any platform specific actions
	 * needed for a secondary cpu after a cold reset e.g
	 * mark the cpu's presence, mechanism to place it in a
	 * holding pen etc.
	 * -----------------------------------------------------
	 */
func plat_secondary_cold_boot_setup
	/* EN7523 Oak does not do cold boot for secondary CPU */
cb_panic:
	b	cb_panic
endfunc plat_secondary_cold_boot_setup

func plat_is_my_cpu_primary
	ldcopr	r0, MPIDR
	ldr	r1, =(MPIDR_CLUSTER_MASK | MPIDR_CPU_MASK)
	and	r0, r1
	cmp	r0, #EN7523_PRIMARY_CPU
	moveq	r0, #1
	movne	r0, #0
	bx		lr
endfunc plat_is_my_cpu_primary

	/* -----------------------------------------------------
	 * unsigned int plat_my_core_pos(void);
	 *
	 * result: CorePos = CoreId + (ClusterId << 2)
	 * -----------------------------------------------------
	 */
func plat_my_core_pos
	ldcopr	r0, MPIDR
	and     r1, r0, #MPIDR_CPU_MASK
	and     r0, r0, #MPIDR_CLUSTER_MASK
	add     r0, r1, r0, LSR #6
	bx		lr
endfunc plat_my_core_pos

	/* ---------------------------------------------
	 * int plat_crash_console_init(void)
	 * Function to initialize the crash console
	 * without a C Runtime to print crash report.
	 * Clobber list : r0 - r4
	 * ---------------------------------------------
	 */
func plat_crash_console_init
	mov_imm	r0, EN7523_UART0_BASE
	b	console_core_init
endfunc plat_crash_console_init

	/* ---------------------------------------------
	 * int plat_crash_console_putc(void)
	 * Function to print a character on the crash
	 * console without a C Runtime.
	 * Clobber list : r1, r2
	 * ---------------------------------------------
	 */
func plat_crash_console_putc
#if defined(IMAGE_BL1)
	ldr		r2, =0x1fb00244
	ldr		r2, [r2]
	ldr		r1, =magic_str
	ldr		r1, [r1]
	cmp		r2, r1
	beq		put_msg
	bx		lr
put_msg:
#endif
	mov_imm	r1, EN7523_UART0_BASE
	b	console_core_putc
endfunc plat_crash_console_putc

func plat_crash_console_flush
	b	console_core_flush
endfunc plat_crash_console_flush

func platform_mem_init
#if defined(IMAGE_BL1)
	/* ---------------------------------------------
	 * Enable L2_SRAM
	 * ---------------------------------------------
	 */
	ldcopr	r2, SCTLR
	bic		r3, r2, #SCTLR_I_BIT
	stcopr	r3, SCTLR
	ldr		r1, =0x1EFBE02C
	ldr		r3, [r1]
	bic		r3, r3, #16
	str		r3, [r1]
	ldr		r1, =0x1EFBE7F0
	ldr		r3, [r1]
	bic		r3, r3, #3840
	bic		r3, r3, #1
	#if defined(TCSUPPORT_CPU_EN7581) && !defined(TCSUPPORT_CPU_AN7583) /* for 256k_sram_256k_l2c config */
	orr		r3, r3, #256
	#endif
	orr		r3, r3, #1
	str		r3, [r1]
	stcopr	r2, SCTLR
	/* ---------------------------------------------
	 * Up to 500MHz
	 * ---------------------------------------------
	 */
	ldr		r0, =0x1fb0009c
	ldr		r1, =0x1efbe640
	ldr		r2, =0x1efbe7c0
	ldr		r3, [r0]
	and		r3, r3, #1
	cmp		r3, #1
	bne		skip
	ldr		r3, [r1]
	bic		r3, r3, #31
	orr		r3, r3, #18
	str		r3, [r1]
	ldr		r3, [r2]
	orr		r3, r3, #1536
	str		r3, [r2]
skip:
#endif
	/* ---------------------------------------------
	 * Test NPU_SRAM
	 * ---------------------------------------------
	 */
#if !defined(MT7510_EN7512_FPGA_STAGE) && !defined(TCSUPPORT_BL2_OPTIMIZATION)
	ldr		r0, =0x1e800000
	ldr		r1, =0x1e860000
	ldr		r2, =0x55555555
	ldr		r3,	=0xAAAAAAAA
	mov		r5, #0
loop:
	str		r2, [r0]
	ldr		r4, [r0]
	cmp		r2, r4
	bne		fail
	str		r3, [r0]
	ldr		r4, [r0]
	cmp		r3, r4
	bne		fail
	str		r5, [r0]
	add		r0, r0, #4
	cmp		r1, r0
	bne		loop
#else
	ldr		r0, =0x1FB00958
	mov		r1, #1
	str		r1, [r0]
#endif

	bx		lr
fail:
	orr		r0, r0, #1
	b		plat_error_handler
endfunc platform_mem_init

#if defined(IMAGE_BL1)
func platform_flash_init

/****************************************************************/
	/* Enable fe_Sram */
	ldr		r7, =0x1FB00958
	mov		r1, #1
	str		r1, [r7]
/****************************************************************/
	adr		lr, sram_continue
	ldr		r1, =__RO_START__
	ldr		r0,	=0x1F000000
	sub		r0, r0, r1
	add		lr, lr, r0
	ldr		r0, =0x1F000000			/* r0 <- CONFIG_SYS_TEXT_BASE */
	ldr		r1, =__RO_START__		/* r1 <- SRC &__image_copy_start */
	ldr		r2, =__BL1_ROM_END__	/* r2 <- SRC &__image_copy_end */
	ldr		r3, =copy_sram

copy_sram:
	ldmia	r1!, {r10-r11}			/* copy from source address [r1] */
	stmia	r0!, {r10-r11}			/* copy to   target address [r0] */
	cmp		r1, r2					/* until source end address [r2] */
	bxlo	r3

	bx        lr
/****************************************************************/

sram_continue:
	ldr		r7, =0x1FA10114			/* HW TRAP */
	ldr		r4, [r7]
	and		r5, r4, #1				/* Store Information=> 0: 3Byte Mode , 1: 4Byte Mode */
	and		r6, r4, #2				/* Store Information=> 0: SPI NOR , 1: SPI NAND */

	cmp 	r6, #0
	beq		skip_check

	mov		r0, #12					/*	PAGE _SIZE 4K */

	ldr		r1, =0x0				/* flash_start */
	ldr		r2,	=0xFFF				/* flash_end */
	ldr		r3,	=0x8000800			/* sram address */
	adr		r4,	check				/* jump function */

	b		move_data_begin

check:
	ldr		r1, =0x8001000
	ldr		r2, [r1]
	ldr		r1, =0xAA640001
	cmp		r2, r1
	beq		page_4k

	mov		r0, #11					/*	PAGE _SIZE 2K */

skip_check:
page_4k:
	ldr		r1, =0xC00				/* flash_start */
	ldr		r2,	=0x1FFFF			/* flash_end */
	ldr		r3,	=0x8000000			/* sram address */
	ldr		r4,	=0x8000000			/* jump function */

	b		move_data_begin

endfunc platform_flash_init

func move_data_begin

#if 0
	/* set Tn to 1	*/
	ldr		r10, =0x1FA20068
	mov		r11, #9
	str		r11, [r10]
#endif
	/* Disable read idle enable */
	ldr		r10, =0x1FA10004
	mov		r11, #0
	str		r11, [r10]

fsm_idle:
	/*  wait until auto read status is IDLE - start */
	ldr		r10, =0x1FA10018
	ldr		r11, [r10]
	cmp		r11, #0
	bne		fsm_idle

	/* manual begin */
	ldr		r10, =0x1FA10014
	mov		r11, #9
	str		r11, [r10]

	ldr		r10, =0x1FA10020
	mov		r11, #1
	str		r11, [r10]

START_LOAD:
	/* 0: SPI NOR , 1: SPI NAND */
	cmp 	r6, #0
	beq		READ_DATA

LOAD_PAGE:
	/* If S1 > S2, goto LOAD_DONE */
	cmp		r1, r2
	bgt		LOAD_DONE

	/* 1. CSL : Send Chip Select Low command */
	mov		r8, #1
	mov		r9, #1
	bl		send_opfifo_write_cmd

	/* 2. Send 13h OP code */
	mov		r8, #8
	mov		r9, #1
	bl		send_opfifo_write_cmd

	mov		r8, #0x13
	bl		send_datafifo_write_cmd

PAGE_NUMBER:
	/* 3. Caculate the Page number */
	mov		r7, r1, lsr r0

	/* 4. Send _SPI_NAND_OP_PAGE_READ 13H */
	mov		r8, #8
	mov		r9, #3
	bl		send_opfifo_write_cmd

	lsr		r8, r7, #16
	and 	r8, r8, #0xFF
	bl		send_datafifo_write_cmd

	lsr		r8, r7, #8
	and 	r8, r8, #0xFF
	bl		send_datafifo_write_cmd

	and 	r8, r7, #0xFF
	bl		send_datafifo_write_cmd

	/* 5. CSH : Send Chip Select High command */
	mov		r8, #0
	mov		r9, #1
	bl		send_opfifo_write_cmd

GET_FEATURE:
	/* 1. CSL : Send Chip Select Low command */
	mov		r8, #1
	mov		r9, #1
	bl		send_opfifo_write_cmd

	/* 2. Send _SPI_NAND_OP_GET_FEATURE 0Fh */
	mov		r8, #8
	mov		r9, #1
	bl		send_opfifo_write_cmd

	mov		r8, #0xF
	bl		send_datafifo_write_cmd

	/* 3. Send _SPI_NAND_ADDR_STATUS C0h */
	mov		r8, #8
	mov		r9, #1
	bl		send_opfifo_write_cmd

	mov		r8, #0xC0
	bl		send_datafifo_write_cmd

	/* 4. Read Status */
	mov		r8, #12
	mov		r9, #1
	bl		send_opfifo_write_cmd
	bl		send_datafifo_read_cmd

	and		r7, r8, #1

	cmp		r7, #0
	bne		GET_FEATURE

	/* 5. CSH : Send Chip Select High command */
	mov		r8, #0
	mov		r9, #1
	bl		send_opfifo_write_cmd

READ_DATA:
	/* If S1 > S2, goto LOAD_DONE */
	cmp		r1, r2
	bgt		LOAD_DONE

	mov		r7, r1

	/* 0: SPI NOR , 1: SPI NAND */
	cmp 	r6, #0
	beq		ADDR_OPCODE

	mov		r10, #1
	mov		r10, r10, lsl r0
	sub		r10, r10, #1

	and		r7, r7, r10

ADDR_OPCODE:
	/* 1. CSL : Send Chip Select Low command */
	mov		r8, #1
	mov		r9, #1
	bl		send_opfifo_write_cmd

	/* 2. Send _SPI_NAND_OP_READ_FROM_CACHE_SINGLE 03h */
	mov		r8, #8
	mov		r9, #1
	bl		send_opfifo_write_cmd

	mov		r8, #3
	bl		send_datafifo_write_cmd

SEND_ADDR:
	/* 3. Send Read Address */
	cmp 	r5, #0
	beq		THREE_BYTE_ADDR
	cmp 	r6, #1
	beq		THREE_BYTE_ADDR

	mov		r8, #8
	mov		r9, #1
	bl		send_opfifo_write_cmd

	lsr		r8, r7, #24
	and 	r8, r8, #0xFF
	bl		send_datafifo_write_cmd

THREE_BYTE_ADDR:
	mov		r8, #8
	mov		r9, #3
	bl		send_opfifo_write_cmd

	/* 0: SPI NOR , 1: SPI NAND */
	cmp 	r6, #0
	bne		DUMMY_APPEND

	lsr		r8, r7, #16
	and 	r8, r8, #0xFF
	bl		send_datafifo_write_cmd

DUMMY_APPEND:
	lsr		r8, r7, #8
	and 	r8, r8, #0xFF
	bl		send_datafifo_write_cmd

	and 	r8, r7, #0xFF
	bl		send_datafifo_write_cmd

	/* 0: SPI NOR , 1: SPI NAND */
	cmp 	r6, #0
	beq		read_one_byte

	mov		r8, #0
	bl		send_datafifo_write_cmd

read_one_byte:
	/* If S1 > S2, READ_PAGE_DONE */
	cmp		r1, r2
	bgt		READ_PAGE_DONE

	mov		r8, #12
	mov		r9, #1
	bl		send_opfifo_write_cmd
	bl		send_datafifo_read_cmd

	strb	r8, [r3]

	add		r1, r1, #1
	add		r3, r3, #1
	add		r7, r7, #1

	mov		r10, #1
	mov		r10, r10, lsl r0
	sub		r10, r10, #1

	and		r8, r7, r10
	cmp		r8, #0
	bne		read_one_byte

READ_PAGE_DONE:
	/* 5. CSH : Send Chip Select High command */
	mov		r8, #0
	mov		r9, #1
	bl		send_opfifo_write_cmd
	b		START_LOAD

LOAD_DONE:
	bx		r4

endfunc move_data_begin

func send_opfifo_write_cmd

	/* (((op_cmd) & OP_CMD_MASK) << OP_SHIFT) | ((op_len) & OP_LEN_MASK) */
	/* write op_cmd to register OPFIFO_WDATA */
	ldr		r10, =0x1FA10028
	ldr		r11, =0x1F
	and 	r11, r8, r11
	lsl		r11, r11, #0x9
	ldr		r12, =0x1FF
	and 	r12, r9, r12
	orr		r11, r11, r12
	str		r11, [r10]

opfifo_full_check:
	/* wait until opfifo is not full */
	ldr		r10, =0x1FA1002C
	ldr		r11, [r10]
	cmp 	r11, #0
	bne		opfifo_full_check

	/* enable write from register OPFIFO_WDATA to opfifo */
	ldr		r10, =0x1FA10030
	mov		r11, #1
	str		r11, [r10]

opfifo_empty_check:
	/* wait until opfifo is empty */
	ldr		r10, =0x1FA10024
	ldr		r11, [r10]
	cmp 	r11, #1
	bne		opfifo_empty_check

	bx		r14

endfunc send_opfifo_write_cmd


func send_datafifo_read_cmd

dfifo_nonempty_check:
	/* wait until dfifo is not empty */
	ldr		r10, =0x1FA1003C
	ldr		r11, [r10]
	cmp 	r11, #0
	bne		dfifo_nonempty_check

	/* read 1 byte from dfifo to register DFIFO_RDATA */
	ldr		r10, =0x1FA10044
	ldr	 	r8, [r10]
	and 	r8, r8, #0xFF

	/* enable register DFIFO_RD to read next byte */
	ldr		r10, =0x1FA10040
	mov		r11, #1
	str		r11, [r10]

	bx		r14

endfunc send_datafifo_read_cmd


func send_datafifo_write_cmd

dfifo_full_check:
	/* wait until dfifo is not full */
	ldr		r10, =0x1FA10034
	ldr		r11, [r10]
	cmp 	r11, #0
	bne		dfifo_full_check

	/* write data  to register DFIFO_WDATA */
	ldr		r10, =0x1FA10038
	and 	r11, r8, #0xFF
	str		r11, [r10]

	bx		r14

endfunc send_datafifo_write_cmd

#else

.extern bl31_base_addr
.extern rst_vector_base_addr

.section .text.arch64
func jumparch64
    mov r4, r1   /* r4 argument */
    mov r5, r2   /* r5 argument */
    mov r6, r0   /* keep LK jump addr */
    mov r7, r3   /* r3 = TEE boot entry, relocate to r7 */

    /* setup the reset vector base address after warm reset to Aarch64 */
    ldr r0, =bl31_base_addr
    ldr r0,[r0]

    ldr r1, =rst_vector_base_addr
    ldr r1,[r1]
    str r0,[r1]

    /* setup the excution state after warm reset: 1:Aarch64, 0:Aarch32 */
    mov r2, #0x3
    mcr p15, 0, r2, c12, c0, 2

	isb
	dsb

	mov r0, #0
	mov r1, #0
	mov r2, #0
	mov r3, #0

wfi_loop:
    /* enter WFI to request a warm reset */
    wfi
    b wfi_loop
endfunc jumparch64

#endif
