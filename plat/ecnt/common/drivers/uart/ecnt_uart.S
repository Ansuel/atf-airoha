/*
 * Copyright (c) 2015-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <asm_macros.S>
#include <console_macros.S>
#include <ecnt_uart.h>

	.globl  console_ecnt_register
	.globl	console_ecnt_putc
	.globl	console_ecnt_getc
	.globl	console_ecnt_flush
	.globl	console_core_init
	.globl	console_core_putc
	.globl	console_core_getc
	.globl	console_core_flush

#ifdef AARCH64
func console_ecnt_register
	mov	x7, x30
	mov	x6, x1
	cbz	x6, register_fail
	str	x0, [x6, #CONSOLE_T_BASE]

	bl	console_core_init
	cbz	x0, register_fail

	mov	x0, x6
	mov	x30, x7
	finish_console_register ecnt putc=1, getc=1, flush=1

register_fail:
	ret	x7
endfunc console_ecnt_register

	/* -----------------------------------------------
	 * int console_ecnt_init(unsigned long base_addr,
	 * unsigned int uart_clk, unsigned int baud_rate)
	 * Function to initialize the console without a
	 * C Runtime to print debug information. This
	 * function will be accessed by console_init and
	 * crash reporting.
	 * In: r0 - console base address
	 *     r1 - Uart clock in Hz
	 *     r2 - Baud rate
	 * Out: return 1 on success else 0 on error
	 * Clobber list : r1, r2, r3
	 * -----------------------------------------------
	 */
func console_core_init
	mov		w4, #0xF
	str		w4, [x0, #0x8]

	mov		w4, #0x0
	str		w4, [x0, #0x10]

	mov		w4, #0x0
	str		w4, [x0, #0x24]

	mov		w4, #0x1
	str		w4, [x0, #0x4]

	mov		w4, #0x80
	str		w4, [x0, #0xC]

	ldr		w4, =0xEA00FDE8
	str		w4, [x0, #0x2C]

	mov		w4, #0x1
	str		w4, [x0, #0x0]

	mov		w4, #0x0
	str		w4, [x0, #0x4]

	mov		w4, #0x3
	str		w4, [x0, #0xC]

	mov		w0, #1
	ret
endfunc console_core_init

	/* --------------------------------------------------------
	 * int console_ecnt_putc(int c, unsigned long base_addr)
	 * Function to output a character over the console. It
	 * returns the character printed on success or -1 on error.
	 * In : r0 - character to be printed
	 *      r1 - console base address
	 * Out : return -1 on error else return character.
	 * Clobber list : r2
	 * --------------------------------------------------------
	 */

func console_ecnt_putc
	ldr		x1, [x1, #CONSOLE_T_BASE]
	b		console_core_putc
endfunc console_ecnt_putc


func console_core_putc
	cbz		x1, put_err
check_tx:
	ldr		w2, [x1, #0x14]
	tbz		w2, #0x5, check_tx
	str		w0, [x1, #0x0]
	ret
put_err:
	mov		w0, #-2
	ret
endfunc console_core_putc

	/* ---------------------------------------------
	 * int console_ecnt_getc(unsigned long base_addr)
	 * Function to get a character from the console.
	 * It returns the character grabbed on success
	 * or -1 on error.
	 * In : r0 - console base address
	 * Clobber list : r0, r1
	 * ---------------------------------------------
	 */
func console_ecnt_getc
	ldr		x0, [x0, #CONSOLE_T_BASE]
	b		console_core_getc
endfunc console_ecnt_getc

func console_core_getc
	cbz		x0, get_err
	ldr		w1, [x0, #0x14]
	tbz		w1, #0x0, no_char
	ldr		w0, [x0, #0x0]
	ret
get_err:
	mov		w0, #-2
	ret
no_char:
	mov		w0, #-1
	ret
endfunc console_core_getc

	/* ---------------------------------------------
	 * int console_core_flush(uintptr_t base_addr)
	 * Function to force a write of all buffered
	 * data that hasn't been output.
	 * In : r0 - console base address
	 * Out : return -1 on error else bx	lrurn 0.
	 * Clobber list : r0, r1
	 * ---------------------------------------------
	 */
func console_ecnt_flush
	b		console_core_flush
endfunc console_ecnt_flush

func console_core_flush
	ret
endfunc console_core_flush
#else
func console_ecnt_register
	push	{r4, lr}
	mov		r4, r1
	cmp		r4, #0
	beq		register_fail
	str		r0, [r4, #CONSOLE_T_BASE]

	bl		console_core_init
	cmp		r0, #1
	bne		register_fail

	mov		r0, r1
	pop		{r4, lr}
	finish_console_register ecnt putc=1, getc=1, flush=1

register_fail:
	pop	{r4, pc}
endfunc console_ecnt_register

	/* -----------------------------------------------
	 * int console_ecnt_init(unsigned long base_addr,
	 * unsigned int uart_clk, unsigned int baud_rate)
	 * Function to initialize the console without a
	 * C Runtime to print debug information. This
	 * function will be accessed by console_init and
	 * crash reporting.
	 * In: r0 - console base address
	 *     r1 - Uart clock in Hz
	 *     r2 - Baud rate
	 * Out: return 1 on success else 0 on error
	 * Clobber list : r1, r2, r3
	 * -----------------------------------------------
	 */
func console_core_init
	mov		r4, #0xF
	str		r4, [r0, #0x8]

	mov		r4, #0x0
	str		r4, [r0, #0x10]

	mov		r4, #0x0
	str		r4, [r0, #0x24]

	mov		r4, #0x1
	str		r4, [r0, #0x4]

	mov		r4, #0x80
	str		r4, [r0, #0xC]

	ldr		r4, =0xEA00FDE8
	str		r4, [r0, #0x2C]

	mov		r4, #0x1
	str		r4, [r0, #0x0]

	mov		r4, #0x0
	str		r4, [r0, #0x4]

	mov		r4, #0x3
	str		r4, [r0, #0xC]

	mov		r0, #1
	bx		lr
endfunc console_core_init

	/* --------------------------------------------------------
	 * int console_ecnt_putc(int c, unsigned long base_addr)
	 * Function to output a character over the console. It
	 * returns the character printed on success or -1 on error.
	 * In : r0 - character to be printed
	 *      r1 - console base address
	 * Out : return -1 on error else return character.
	 * Clobber list : r2
	 * --------------------------------------------------------
	 */

func console_ecnt_putc
	ldr		r1, [r1, #CONSOLE_T_BASE]
	b		console_core_putc
endfunc console_ecnt_putc


func console_core_putc
	cmp		r1, #0
	beq		put_err
check_tx:
	ldr		r2, [r1, #0x14]
	tst		r2, #32
	beq		check_tx
	str		r0, [r1, #0x0]
	bx		lr
put_err:
	mov		r0, #-2
	bx		lr
endfunc console_core_putc

	/* ---------------------------------------------
	 * int console_ecnt_getc(unsigned long base_addr)
	 * Function to get a character from the console.
	 * It returns the character grabbed on success
	 * or -1 on error.
	 * In : r0 - console base address
	 * Clobber list : r0, r1
	 * ---------------------------------------------
	 */
func console_ecnt_getc
	ldr		r0, [r0, #CONSOLE_T_BASE]
	b		console_core_getc
endfunc console_ecnt_getc

func console_core_getc
	cmp		r0, #0
	beq		get_err
check_rx:
	ldr		r1, [r0, #0x14]
	tst		r1, #1
	beq		no_char
	ldr		r0, [r0, #0x0]
	bx		lr
get_err:
	mov		r0, #-2
	bx		lr
no_char:
	mov		r0, #-1
	bx		lr
endfunc console_core_getc

	/* ---------------------------------------------
	 * int console_core_flush(uintptr_t base_addr)
	 * Function to force a write of all buffered
	 * data that hasn't been output.
	 * In : r0 - console base address
	 * Out : return -1 on error else bx	lrurn 0.
	 * Clobber list : r0, r1
	 * ---------------------------------------------
	 */
func console_ecnt_flush
	b		console_core_flush
endfunc console_ecnt_flush

func console_core_flush
	bx		lr
endfunc console_core_flush
#endif
