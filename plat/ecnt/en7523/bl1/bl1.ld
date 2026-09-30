/*
 * EN7523 open-source BL1 linker script.
 *
 * Flat image, linked at base 0x0 (platform_def.h: BL1_RO_BASE == 0).
 * Must fit the BootROM load window of 2 KB (vendor blob is 0x7E0 bytes).
 * Layout mirrors the vendor blob: entry @0x00, vectors @0x7C0.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_bl1_start)

SECTIONS
{
	. = 0x0;

	.text : {
		KEEP(*(.text.start))
		*(.text .text.*)
	}

	.rodata : {
		*(.rodata .rodata.*)
	}

	/* exception vectors, exactly where the vendor blob keeps them */
	. = 0x7C0;
	.vectors : {
		KEEP(*(.vectors))
	}

	.data : {
		*(.data .data.*)
		*(.got .got.*)
	}

	/* end of the bytes that get relocated to 0x1f000000 */
	. = ALIGN(8);
	_bl1_image_end = .;

	.bss (NOLOAD) : {
		_bss_start = .;
		*(.bss .bss.*)
		*(COMMON)
		. = ALIGN(8);
		_bss_end = .;
	}

	/DISCARD/ : {
		*(.comment)
		*(.note .note.*)
		*(.ARM.attributes)
		*(.debug*)
	}
}

ASSERT(_bl1_image_end <= 0x800, "BL1 image exceeds the 2KB BootROM window")
