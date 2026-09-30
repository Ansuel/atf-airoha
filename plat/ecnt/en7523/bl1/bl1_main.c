/*
 * EN7523 open-source BL1 - SoC platform stage (C part)
 *
 * Implements the two platform routines the reset stub calls:
 *   bl1_plat_init()        - MCUCFG (AXI/L2C) config + NPU-SRAM self-test/clear
 *                            (blob @0x1CC)
 *   bl1_boot_next_stage()  - read SPI strap, load the next stage from the boot
 *                            flash into L2 SRAM and jump to it (blob @0x2F0/0x374)
 *
 * No console output on purpose: the vendor blob does not touch the UART.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

/* ---------------- MCUCFG (CPU subsystem config) ------------------- */
#define MCUCFG_BASE		0x1EFBE000UL
#define MCUCFG_AXI_CONFIG	(MCUCFG_BASE + 0x02C)	/* EN7523_AXI_CONFIG */
#define MCUCFG_L2C_CONFIG	(MCUCFG_BASE + 0x07F0)	/* EN7523_L2C_CONFIG */
#define MCUCFG_L2C_CFG_MP1	(MCUCFG_BASE + 0x0640)
#define MCUCFG_MP0_CFG_7C0	(MCUCFG_BASE + 0x07C0)

/* ---------------- SCU / SCREG ------------------------------------- */
#define SCU_BASE		0x1FB00000UL
#define SCU_STRAP_PKG		(SCU_BASE + 0x009C)	/* package/strap select */
#define SCU_BL1_HANDOFF		(SCU_BASE + 0x0958)

/* ---------------- memory regions (en7523_def.h) ------------------- */
#define NPU_SRAM_BASE		0x1E800000UL
#define NPU_SRAM_SIZE		0x60000UL		/* 384 KB */

#define L2_SRAM_BASE		0x08000000UL		/* ECNT_L2_SRAM_BASE */
#define L2_SRAM_SIZE		0x20000UL		/* 128 KB */

/* Flash layout for the next stage (blob literals) */
#define FIP_OFFSET		0x800UL
#define NEXT_STAGE_OFFSET	0xC00UL
#define NEXT_STAGE_MAGIC	0xAA640001UL

/* ---------------- SPI controller (0x1FA10000) --------------------- */
#define SPI_BASE		0x1FA10000UL
#define SPI_STRAP		(SPI_BASE + 0x0114)	/* SPI_CONTROLLER_REGS_STRAP */

/* register offsets decoded from the blob (see document/bl1_en7523_reverse.md) */
#define SPI_REG_CTRL		0x04
#define SPI_REG_MODE		0x14
#define SPI_REG_READY		0x18
#define SPI_REG_START		0x20
#define SPI_REG_DONE		0x24
#define SPI_REG_CMD		0x28
#define SPI_REG_BUSY		0x2C
#define SPI_REG_TRIG		0x30
#define SPI_REG_TX_RDY		0x34
#define SPI_REG_TX_DATA		0x38
#define SPI_REG_RX_RDY		0x3C
#define SPI_REG_RX_ACK		0x40
#define SPI_REG_RX_DATA		0x44

#define SPI_OP_CSH		0x00
#define SPI_OP_OUTS		0x08
#define SPI_OP_INS		0x0C
#define SPI_OP_IND		0x0E

static inline uint32_t rd32(uintptr_t a)
{
	return *(volatile uint32_t *)a;
}

static inline void wr32(uintptr_t a, uint32_t v)
{
	*(volatile uint32_t *)a = v;
}

/* implemented in bl1_entry.S */
void bl1_panic(void);

/* ------------------------------------------------------------------ */
/* NPU-SRAM self-test (0x55555555 / 0xAAAAAAAA) then clear to 0.       */
/* Faithful to blob @0x23C..0x280: any mismatch hangs the CPU.         */
/* ------------------------------------------------------------------ */
static void bl1_npu_sram_selftest_clear(void)
{
	volatile uint32_t *p = (volatile uint32_t *)NPU_SRAM_BASE;
	volatile uint32_t *end = (volatile uint32_t *)(NPU_SRAM_BASE + NPU_SRAM_SIZE);

	while (p < end) {
		*p = 0x55555555U;
		if (*p != 0x55555555U)
			bl1_panic();
		*p = 0xAAAAAAAAU;
		if (*p != 0xAAAAAAAAU)
			bl1_panic();
		*p = 0U;
		p++;
	}
}

/* ------------------------------------------------------------------ */
/* MCUCFG setup (blob @0x1CC).                                         */
/*   - AXI_CONFIG bit4 cleared                                         */
/*   - L2C_CONFIG: [11:8]=0, bit0=1  -> 128K L2 + 128K SRAM            */
/*   - if SCU strap bit0: two extra MCUCFG writes                      */
/* ------------------------------------------------------------------ */
void bl1_plat_init(void)
{
	wr32(MCUCFG_AXI_CONFIG, rd32(MCUCFG_AXI_CONFIG) & ~(1U << 4));
	wr32(MCUCFG_L2C_CONFIG, (rd32(MCUCFG_L2C_CONFIG) & ~0xF00U) | 1U);

	if (rd32(SCU_STRAP_PKG) & 1U) {
		wr32(MCUCFG_L2C_CFG_MP1, (rd32(MCUCFG_L2C_CFG_MP1) & ~0x1FU) | 0x12U);
		wr32(MCUCFG_MP0_CFG_7C0, rd32(MCUCFG_MP0_CFG_7C0) | 0x600U);
	}

	bl1_npu_sram_selftest_clear();
}

/* ------------------------------------------------------------------ */
/* SPI controller primitives (blob @0x584 / 0x620 / 0x5E8).            */
/* ------------------------------------------------------------------ */
static void spi_cmd(uint32_t op, uint32_t mode)
{
	wr32(SPI_BASE + SPI_REG_CMD, ((op & 0x1FU) << 9) | (mode & 0x1FFU));
	while (rd32(SPI_BASE + SPI_REG_BUSY) != 0U)
		;
	wr32(SPI_BASE + SPI_REG_TRIG, 1U);
	while (rd32(SPI_BASE + SPI_REG_DONE) != 1U)
		;
}

static void spi_tx(uint8_t b)
{
	while (rd32(SPI_BASE + SPI_REG_TX_RDY) != 0U)
		;
	wr32(SPI_BASE + SPI_REG_TX_DATA, b);
}

static uint8_t spi_rx(void)
{
	uint8_t b;

	while (rd32(SPI_BASE + SPI_REG_RX_RDY) != 0U)
		;
	b = (uint8_t)rd32(SPI_BASE + SPI_REG_RX_DATA);
	wr32(SPI_BASE + SPI_REG_RX_ACK, 1U);
	return b;
}

/* ------------------------------------------------------------------ */
/* Read `len` bytes starting at flash byte address `addr` -> dst.      */
/* Uses the standard SPI-NOR READ (0x03) command; matches the blob's    */
/* NOR path (SPI strap bit1 == 0).                                     */
/* ------------------------------------------------------------------ */
static void spi_flash_read(uint32_t addr, uint8_t *dst, uint32_t len)
{
	uint32_t i;

	wr32(SPI_BASE + SPI_REG_CTRL, 0U);
	while (rd32(SPI_BASE + SPI_REG_READY) != 0U)
		;
	wr32(SPI_BASE + SPI_REG_MODE, 9U);
	wr32(SPI_BASE + SPI_REG_START, 1U);

	spi_cmd(SPI_OP_CSH, 1U);		/* CS high  */
	spi_cmd(SPI_OP_OUTS, 1U);		/* clock out */
	spi_tx(0x03U);				/* READ     */
	spi_cmd(SPI_OP_OUTS, 3U);		/* 3-byte address */
	spi_tx((addr >> 16) & 0xFFU);
	spi_tx((addr >> 8) & 0xFFU);
	spi_tx(addr & 0xFFU);

	for (i = 0U; i < len; i++) {
		spi_cmd(SPI_OP_INS, 1U);
		dst[i] = spi_rx();
	}

	spi_cmd(SPI_OP_CSH, 1U);
}

/* ------------------------------------------------------------------ */
/* Boot-media dispatch (blob @0x2F0).                                  */
/*   SPI strap bit1 == 0: load [0xC00 .. L2_END) -> L2_SRAM, jump       */
/*   SPI strap bit1 != 0: first pull a 4KB header to L2_SRAM+0x800 and  */
/*                        verify magic @L2_SRAM+0x1000, then load the   */
/*                        main image to L2_SRAM and jump                */
/* ------------------------------------------------------------------ */
void bl1_boot_next_stage(void)
{
	uint32_t strap = rd32(SPI_STRAP);
	uint32_t dst = L2_SRAM_BASE;

	if (strap & 2U) {
		spi_flash_read(0U, (uint8_t *)(L2_SRAM_BASE + FIP_OFFSET), 0x1000U);
		if (rd32(L2_SRAM_BASE + 0x1000) != NEXT_STAGE_MAGIC)
			bl1_panic();
	}

	spi_flash_read(NEXT_STAGE_OFFSET, (uint8_t *)dst,
		       L2_SRAM_SIZE - NEXT_STAGE_OFFSET);

	/* hand control to the loaded image (BL2 / preloader) */
	((void (*)(void))dst)();

	bl1_panic();
}
