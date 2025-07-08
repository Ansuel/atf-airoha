/*
 * Copyright (c) 2014-2015, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef EN7523_DEF_H
#define EN7523_DEF_H

#if RESET_TO_BL31
#error "EN7523 is incompatible with RESET_TO_BL31!"
#endif

#define EN7523_PRIMARY_CPU	0x0

/* Register base address */
#define IO_PHYS				(0x10000000)
#define EN7523_MEM_BASE		UL(0x80000000)
#define EN7523_MEM_SIZE		UL(0x40000000)

/* Aggregate of all devices in the first GB */
#define ECNT_DEV_IO_BASE	(0x8000000)
#define ECNT_DEV_IO_SIZE	(0x17D00000)

#define ECNT_GIC_BASE		(0x9000000)
#define ECNT_GIC_SIZE		(0x440000)

#define ECNT_NPU_SRAM_BASE	(IO_PHYS + 0xE800000)
#ifdef TCSUPPORT_CPU_AN7552
#define ECNT_NPU_SRAM_SIZE	(0x40000)
#else
#define ECNT_NPU_SRAM_SIZE	(0x60000)
#endif

#define ECNT_NPU_SRAM2_BASE	(IO_PHYS + 0xE900000)
#define ECNT_NPU_SRAM2_SIZE	(0x4000)

#define ECNT_GDMP_SRAM_BASE	(IO_PHYS + 0xFA40000)
#define ECNT_GDMP_SRAM_SIZE	(0x8000)

#define ECNT_L2_SRAM_BASE	(0x8000000)
#define ECNT_L2_SRAM_SIZE	(0x20000)


#define ECNT_MCUCFG_BASE	(IO_PHYS + 0xEFBC000)
#define ECNT_MCUCFG_SIZE	(0x4000)

#define MCUCFG_BASE		(IO_PHYS + 0xEFBE000)
#define SPM_BASE		(IO_PHYS + 0xEFBC000)

#define ECNT_BL21_BASE			(ECNT_L2_SRAM_BASE)
#define ECNT_BL22_BASE			(0x08004000)
#define ECNT_BL23_BASE			(0x81000000)
#define ECNT_FLASH_TABLE_BASE	(0x81100000)

/* SRAMROM related registers */
#define EN7523_AXI_CONFIG	(MCUCFG_BASE + 0x2C)
#define EN7523_L2C_CONFIG	(MCUCFG_BASE + 0x7F0)
#define RVBADDRESS_CPU0		(MCUCFG_BASE + 0x38)

#define EN7523_SCREG_HIR	(IO_PHYS + 0xFB00064)
#define CR_NP_SCU_BOOT_TRAP_CONF_DEC	(IO_PHYS + 0xFB000B8)
#define EN7523_SCREG_WF0	(IO_PHYS + 0xFB00240)
#define EN7523_SCREG_WF1	(IO_PHYS + 0xFB00244)
#define EN7523_SCREG_WR0	(IO_PHYS + 0xFB00280)
#define EN7523_SCREG_WR1	(IO_PHYS + 0xFB00284)
#define EN7581_R2C_SEL		(IO_PHYS + 0xFB00074)
#define CR_NP_SCU_PDIDR		(IO_PHYS + 0xFB0005C)

/*******************************************************************************
 * UART related constants
 ******************************************************************************/
#define EN7523_UART0_BASE	(IO_PHYS + 0xFBF0000)
#define EN7523_UART2_BASE	(EN7523_UART0_BASE + 0x300)

#define EN7523_GPIO_CTRL	(IO_PHYS + 0xFBF0200)
#define EN7523_GPIO_DATA	(IO_PHYS + 0xFBF0204)
#define EN7523_GPIO_OE		(IO_PHYS + 0xFBF0214)
#define EN7581_GPIO_CTRL1	(IO_PHYS + 0xFBF0220)

#define EN7523_SCREG_WF1	(IO_PHYS + 0xFB00244)
#define EN7523_HWTRAP_CONF	(IO_PHYS + 0xFB000B4)
/*******************************************************************************
 * Other Modules
 ******************************************************************************/
#define EN7523_SFC_STRAP	(IO_PHYS + 0xFA10114)

#define EN7523_ECC_SEL		(IO_PHYS + 0xFA20254)
#define RG_TOP_REV_24		(IO_PHYS + 0xFA2036C)
#define EN7523_SEC_SSR		(IO_PHYS + 0xFBE2E00)
#define	BOOT_FROM_SPI		(U(1) << 0)
#define	BOOT_SEL_BY_HWTRAP	(U(1) << 1)

#define ECNT_TZPC_REG_BASE 	0x1fbe2d00
#define ECNT_BL2_BOOT_IDX	(ECNT_TZPC_REG_BASE + 0x10)
#define EN7523_MODULE_ATTR	(IO_PHYS + 0xFBE2E04)
#define MODULE_EFUSE_OFF	(U(1) << 1)
#define MODULE_NPU_BIT_OFF	(U(1) << 3)
#define MODULE_NPU_SRAM_OFF	(U(1) << 12)

#define EN7523_HWTRAP_CONF	(IO_PHYS + 0xFB000B4)
#define DRAMC_CONF			(IO_PHYS + 0xFB00074)


#ifdef TCSUPPORT_CPU_AN7552
#define BL2_OPTIMIZE_STATUS	(ECNT_NPU_SRAM_BASE + ECNT_L2_SRAM_SIZE)
#define BL2_OPTIMIZE_HEADER_BASE	(ECNT_NPU_SRAM_BASE + ECNT_L2_SRAM_SIZE + 0x3c00)
#define BL2_OPTIMIZE_BASE	ECNT_NPU_SRAM_BASE+0x20400

#else
#define BL2_OPTIMIZE_STATUS	(ECNT_NPU_SRAM_BASE + (ECNT_L2_SRAM_SIZE<<1))
#define BL2_OPTIMIZE_HEADER_BASE	(ECNT_NPU_SRAM_BASE + (ECNT_L2_SRAM_SIZE<<1) + 0x3c00)
#define BL2_OPTIMIZE_BASE	ECNT_NPU_SRAM_BASE+0x40400
#endif

#if defined(TCSUPPORT_CPU_EN7581) || defined(TCSUPPORT_CPU_AN7552)
#define HWTRAP_FW_UPGRADE	(U(1) << 4)

#if defined(TCSUPPORT_CPU_EN7581)
#define HWTRAP_INIC_MODE	(0xa)

#define HWTRAP_INIC_MDIO_MODE	(0x2)
#define HWTRAP_EMMC_MODE		(0x6)
#define HWTRAP_EMMC_MODE2		(0xe)
#endif

#define HWTRAP_FLASH_NAND_MODE	(0x3)
#define HWTRAP_FLASH_NAND_MODE2	(0xb)

#define HWTRAP_FLASH_3BNOR_MODE		(0x1)
#define HWTRAP_FLASH_3BNOR_MODE2	(0x9)
#define HWTRAP_FLASH_4BNOR_MODE		(0x0)
#define HWTRAP_FLASH_4BNOR_MODE2	(0x8)

#define HWTRAP_ROM_3BNOR_MODE		(0x5)
#define HWTRAP_ROM_4BNOR_MODE		(0x4)
#define HWTRAP_ROM_NAND_MODE		(0x7)
#define HWTRAP_ROM_NAND_MODE2   	(0xf)

#define HWTRAP_ROM_NAND_CONTROLLER_ECC_MODE (0xc)

#define HWTRAP_PARRALLEL_NAND_MODE	(0xd)

#define HWTRAP_MASK				(0xf)
#define HWTRAP_DEC_MASK			(0x7f)

#else
#define HWTRAP_FW_UPGRADE	(U(1) << 3)
#define HWTRAP_INIC_MODE	(0x2)
#define HWTRAP_MASK				(0x7)
#define HWTRAP_DEC_MASK			(0x2f)

#endif

#define EN7523_EJTAG_ENABLE		(IO_PHYS + 0xFBE2E18)
#define BOOT_FROM_BOOTROM_EJTAG	(U(1) << 0)
#define BOOT_FROM_SPI_EJTAG		(U(1) << 1)

#define EN7523_HWTRAP_DEC	(IO_PHYS + 0xFB000B8)

#define EN7523_SSTR_REG		(IO_PHYS + 0xFB0009C)
#define	IS_ASIC				(U(1) << 0)

/*******************************************************************************
 * System counter frequency related constants
 ******************************************************************************/
#define SYS_COUNTER_FREQ_IN_TICKS_25M	25000000
#define SYS_COUNTER_FREQ_IN_TICKS_20M	20000000
#define SYS_COUNTER_FREQ_IN_TICKS_50M	50000000

#define DEBUG_MAGIC		0x544E4345		/* ECNT */
#define DBG_INIC_MODE	0x752301
#define DBG_FWU_MODE	0x752302
#define DBG_FLASH_MODE	0x752303
#define DBG_INIC_MDIO_MODE 0x758101
#define DBG_7583_BOOT_MAGIC    0x03

/*******************************************************************************
 * GIC
 ******************************************************************************/
/* Base ECNT_platform compatible GIC memory map */
#define BASE_GICD_BASE		0x9000000
#define BASE_GICR_BASE		0x9080000
#define BASE_GICC_BASE		0x9400000
#define BASE_GICH_BASE		0x9410000
#define BASE_GICV_BASE		0x9420000

#define ECNT_IRQ_SEC_SGI_0	8
#define ECNT_IRQ_SEC_SGI_1	9
#define ECNT_IRQ_SEC_SGI_2	10
#define ECNT_IRQ_SEC_SGI_3	11
#define ECNT_IRQ_SEC_SGI_4	12
#define ECNT_IRQ_SEC_SGI_5	13
#define ECNT_IRQ_SEC_SGI_6	14
#define ECNT_IRQ_SEC_SGI_7	15

/*******************************************************************************
 * R2C mode
 ******************************************************************************/
/* add #if for avoid compile error when assembly file trying include enum*/
#if !defined(__ASSEMBLER__)

typedef enum {
	R2C_OLD = 0,
	R2C_NEW128,
	R2C_NEW256_NO_WRAP,
	R2C_NEW256,
} R2C_MODE_T;
#endif

#define R2C_NEW_EN (1<<15)
#define R2C_256_EN (1<<17)
#define R2C_WRAP_EN (1<<16)


/*
 *  Macros for local power states in MTK platforms encoded by State-ID field
 *  within the power-state parameter.
 */
/* Local power state for power domains in Run state. */
#define MTK_LOCAL_STATE_RUN     0
/* Local power state for retention. Valid only for CPU power domains */
#define MTK_LOCAL_STATE_RET     1
/* Local power state for OFF/power-down. Valid for CPU and cluster power
 * domains
 */
#define MTK_LOCAL_STATE_OFF     2

#endif /* EN7523_DEF_H */
