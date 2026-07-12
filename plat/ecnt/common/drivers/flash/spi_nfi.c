/***************************************************************************************
 *      Copyright(c) 2016 ECONET Incorporation All rights reserved.
 *
 *      This is unpublished proprietary source code of ECONET Incorporation
 *
 *      The copyright notice above does not evidence any actual or intended
 *      publication of such source code.
 ***************************************************************************************
 */

/*======================================================================================
 * MODULE NAME: spi
 * FILE NAME: spi_nfi.c
 * DATE: 2016/03/18
 * VERSION: 1.00
 * PURPOSE: To Provide SPI NFI(DMA) Access Internace.
 * NOTES:
 *
 * AUTHOR : Chuck Kuo         REVIEWED by
 *
 * FUNCTIONS
 *
 * DEPENDENCIES
 *
 * * $History: $
 * MODIFICTION HISTORY:
 * Version 1.00 - Date 2016/03/18 By Chuck Kuo
 * ** This is the first versoin for creating to support the functions of
 *    current module.
 *
 *======================================================================================
 */


/* INCLUDE FILE DECLARATIONS --------------------------------------------------------- */
#include <asm/io.h>
#include <linux/types.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
#include <asm/tc3162/tc3162.h>
#include <linux/delay.h>
#define SPI_NFI_DEBUG
#else
#include <asm/tc3162.h>
#endif
#if !defined(SPRAM_IMG) && !defined(LZMA_IMG)
#define SPI_NFI_DEBUG
#endif

#include <ecnt_spi_nfi.h>

#if defined(CONFIG_ECNT_UBOOT) /* U-boot likes MIPS bootram, don't follow ARMV8 path */
#undef SPI_NFI_DEBUG
#undef TCSUPPORT_CPU_ARMV8

#endif

/* NAMING CONSTANT DECLARATIONS ------------------------------------------------------ */

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
#define delay1us udelay
#endif

/*******************************************************************************
 * NFI Register Definition 
 *******************************************************************************/
#ifdef TCSUPPORT_CPU_ARMV8
#undef _SPI_NFI_REGS_BASE
#define _SPI_NFI_REGS_BASE					0x00000000
#else
#if defined(CONFIG_ECNT_UBOOT) /* Only for EN7523 */
#undef _SPI_NFI_REGS_BASE
#define _SPI_NFI_REGS_BASE					0x1FA11000
#else
#undef _SPI_NFI_REGS_BASE
#define _SPI_NFI_REGS_BASE					0xBFA11000
#endif
#endif
#define _SPI_NFI_REGS_CNFG			(_SPI_NFI_REGS_BASE + 0x0000)
#define _SPI_NFI_REGS_PAGEFMT		(_SPI_NFI_REGS_BASE + 0x0004)
#define _SPI_NFI_REGS_CON			(_SPI_NFI_REGS_BASE + 0x0008)
#define _SPI_NFI_REGS_INTR_EN		(_SPI_NFI_REGS_BASE + 0x0010)
#define _SPI_NFI_REGS_INTR			(_SPI_NFI_REGS_BASE + 0x0014)
#define _SPI_NFI_REGS_CMD			(_SPI_NFI_REGS_BASE + 0x0020)
#define _SPI_NFI_REGS_STA			(_SPI_NFI_REGS_BASE + 0x0060)
#define _SPI_NFI_REGS_FIFOSTA		(_SPI_NFI_REGS_BASE + 0x0064)
#define _SPI_NFI_REGS_STRADDR		(_SPI_NFI_REGS_BASE + 0x0080)
#define _SPI_NFI_REGS_FDM0L			(_SPI_NFI_REGS_BASE + 0x00A0)
#define _SPI_NFI_REGS_FDM0M			(_SPI_NFI_REGS_BASE + 0x00A4)
#define _SPI_NFI_REGS_FDM7L			(_SPI_NFI_REGS_BASE + 0x00D8)
#define _SPI_NFI_REGS_FDM7M			(_SPI_NFI_REGS_BASE + 0x00DC)
#define _SPI_NFI_REGS_FIFODATA0		(_SPI_NFI_REGS_BASE + 0x0190)
#define _SPI_NFI_REGS_FIFODATA1		(_SPI_NFI_REGS_BASE + 0x0194)
#define _SPI_NFI_REGS_FIFODATA2		(_SPI_NFI_REGS_BASE + 0x0198)
#define _SPI_NFI_REGS_FIFODATA3		(_SPI_NFI_REGS_BASE + 0x019C)
#define _SPI_NFI_REGS_MASTERSTA		(_SPI_NFI_REGS_BASE + 0x0224)
#define _SPI_NFI_REGS_SECCUS_SIZE	(_SPI_NFI_REGS_BASE + 0x022C)
#define _SPI_NFI_REGS_RD_CTL2		(_SPI_NFI_REGS_BASE + 0x0510)
#define _SPI_NFI_REGS_RD_CTL3		(_SPI_NFI_REGS_BASE + 0x0514)
#define _SPI_NFI_REGS_PG_CTL1		(_SPI_NFI_REGS_BASE + 0x0524)
#define _SPI_NFI_REGS_PG_CTL2		(_SPI_NFI_REGS_BASE + 0x0528)
#define _SPI_NFI_REGS_NOR_PROG_ADDR	(_SPI_NFI_REGS_BASE + 0x052C)
#define _SPI_NFI_REGS_NOR_RD_ADDR	(_SPI_NFI_REGS_BASE + 0x0534)
#define _SPI_NFI_REGS_SNF_MISC_CTL	(_SPI_NFI_REGS_BASE + 0x0538)
#define _SPI_NFI_REGS_SNF_MISC_CTL2	(_SPI_NFI_REGS_BASE + 0x053C)
#define _SPI_NFI_REGS_SNF_STA_CTL1	(_SPI_NFI_REGS_BASE + 0x0550)
#define _SPI_NFI_REGS_SNF_STA_CTL2	(_SPI_NFI_REGS_BASE + 0x0554)
#define _SPI_NFI_REGS_SNF_NFI_CNFG		(_SPI_NFI_REGS_BASE + 0x055C)

#if defined(TCSUPPORT_PARALLEL_NAND)
#define _PARALLEL_NFI_REGS_ACCCON		(_SPI_NFI_REGS_BASE + 0x00C)
#define _PARALLEL_NFI_REGS_ADDRNOB		(_SPI_NFI_REGS_BASE + 0x030)
#define _PARALLEL_NFI_REGS_COLADDR		(_SPI_NFI_REGS_BASE + 0x034)
#define _PARALLEL_NFI_REGS_ROWADDR		(_SPI_NFI_REGS_BASE + 0x038)
#define _PARALLEL_NFI_REGS_CNRNB		(_SPI_NFI_REGS_BASE + 0x044)
#define _PARALLEL_NFI_REGS_DATAW		(_SPI_NFI_REGS_BASE + 0x050)
#define _PARALLEL_NFI_REGS_DATAR		(_SPI_NFI_REGS_BASE + 0x054)
#define _PARALLEL_NFI_REGS_PIO_DRDY		(_SPI_NFI_REGS_BASE + 0x058)
#define _PARALLEL_NFI_REGS_LOCKSTA		(_SPI_NFI_REGS_BASE + 0x068)
#define _PARALLEL_NFI_REGS_CSEL			(_SPI_NFI_REGS_BASE + 0x090)
#define _PARALLEL_NFI_REGS_LOCK			(_SPI_NFI_REGS_BASE + 0x100)
#define _PARALLEL_NFI_REGS_LOCK_CSEN	(_SPI_NFI_REGS_BASE + 0x104)
#define _PARALLEL_NFI_REGS_LOCKANOB		(_SPI_NFI_REGS_BASE + 0x104)
#define _PARALLEL_NFI_REGS_LOCK00ADD	(_SPI_NFI_REGS_BASE + 0x110)
#define _PARALLEL_NFI_REGS_LOCK00FMT	(_SPI_NFI_REGS_BASE + 0x114)
#define _PARALLEL_NFI_REGS_LOCK01ADD	(_SPI_NFI_REGS_BASE + 0x118)
#define _PARALLEL_NFI_REGS_LOCK01FMT	(_SPI_NFI_REGS_BASE + 0x11C)
#define _PARALLEL_NFI_REGS_LOCK02ADD	(_SPI_NFI_REGS_BASE + 0x120)
#define _PARALLEL_NFI_REGS_LOCK02FMT	(_SPI_NFI_REGS_BASE + 0x124)
#define _PARALLEL_NFI_REGS_LOCK03ADD	(_SPI_NFI_REGS_BASE + 0x128)
#define _PARALLEL_NFI_REGS_LOCK03FMT	(_SPI_NFI_REGS_BASE + 0x12C)
#define _PARALLEL_NFI_REGS_LOCK04ADD	(_SPI_NFI_REGS_BASE + 0x130)
#define _PARALLEL_NFI_REGS_LOCK04FMT	(_SPI_NFI_REGS_BASE + 0x134)
#define _PARALLEL_NFI_REGS_LOCK05ADD	(_SPI_NFI_REGS_BASE + 0x138)
#define _PARALLEL_NFI_REGS_LOCK05FMT	(_SPI_NFI_REGS_BASE + 0x13C)
#define _PARALLEL_NFI_REGS_LOCK06ADD	(_SPI_NFI_REGS_BASE + 0x140)
#define _PARALLEL_NFI_REGS_LOCK06FMT	(_SPI_NFI_REGS_BASE + 0x144)
#define _PARALLEL_NFI_REGS_LOCK07ADD	(_SPI_NFI_REGS_BASE + 0x148)
#define _PARALLEL_NFI_REGS_LOCK07FMT	(_SPI_NFI_REGS_BASE + 0x14C)
#define _PARALLEL_NFI_REGS_LOCK08ADD	(_SPI_NFI_REGS_BASE + 0x150)
#define _PARALLEL_NFI_REGS_LOCK08FMT	(_SPI_NFI_REGS_BASE + 0x154)
#define _PARALLEL_NFI_REGS_LOCK09ADD	(_SPI_NFI_REGS_BASE + 0x158)
#define _PARALLEL_NFI_REGS_LOCK09FMT	(_SPI_NFI_REGS_BASE + 0x15C)
#define _PARALLEL_NFI_REGS_LOCK10ADD	(_SPI_NFI_REGS_BASE + 0x160)
#define _PARALLEL_NFI_REGS_LOCK10FMT	(_SPI_NFI_REGS_BASE + 0x164)
#define _PARALLEL_NFI_REGS_LOCK11ADD	(_SPI_NFI_REGS_BASE + 0x168)
#define _PARALLEL_NFI_REGS_LOCK11FMT	(_SPI_NFI_REGS_BASE + 0x16C)
#define _PARALLEL_NFI_REGS_LOCK12ADD	(_SPI_NFI_REGS_BASE + 0x170)
#define _PARALLEL_NFI_REGS_LOCK12FMT	(_SPI_NFI_REGS_BASE + 0x174)
#define _PARALLEL_NFI_REGS_LOCK13ADD	(_SPI_NFI_REGS_BASE + 0x178)
#define _PARALLEL_NFI_REGS_LOCK13FMT	(_SPI_NFI_REGS_BASE + 0x17C)
#define _PARALLEL_NFI_REGS_LOCK14ADD	(_SPI_NFI_REGS_BASE + 0x180)
#define _PARALLEL_NFI_REGS_LOCK14FMT	(_SPI_NFI_REGS_BASE + 0x184)
#define _PARALLEL_NFI_REGS_LOCK15ADD	(_SPI_NFI_REGS_BASE + 0x188)
#define _PARALLEL_NFI_REGS_LOCK15FMT	(_SPI_NFI_REGS_BASE + 0x18C)
#define _PARALLEL_NFI_REGS_RD_EDGE_CS	(_SPI_NFI_REGS_BASE + 0x564)
#define _PARALLEL_NFI_REGS_RD_CNT		(_SPI_NFI_REGS_BASE + 0x568)
#define _PARALLEL_NFI_REGS_WR_CNT		(_SPI_NFI_REGS_BASE + 0x56C)
#define _PARALLEL_NFI_REGS_CNT_CLR		(_SPI_NFI_REGS_BASE + 0x578)
#endif


/*******************************************************************************
 * NFI Register Field Definition 
 *******************************************************************************/

/* NFI_CNFG */
#define _SPI_NFI_REGS_CNFG_AHB						(0x0001)
#define _SPI_NFI_REGS_CNFG_READ_EN					(0x0002)
#define _SPI_NFI_REGS_CNFG_DMA_BURST_EN				(0x0004)
/* Flash -> SRAM */
#define _SPI_NFI_REGS_CNFG_DMA_WR_SWAP_EN			(0x0008)
/* SRAM -> Flash */
#define _SPI_NFI_REGS_CNFG_DMA_RD_SWAP_EN			(0x0010)
#define _SPI_NFI_REGS_CNFG_ECC_DATA_SOURCE_INV_EN	(0x0020)
#define _SPI_NFI_REGS_CNFG_HW_ECC_EN				(0x0100)
#define _SPI_NFI_REGS_CNFG_AUTO_FMT_EN				(0x0200)

#if defined(TCSUPPORT_PARALLEL_NAND)
#define _SPI_NFI_REGS_CONF_OP_IDEL			(0x00)
#define _SPI_NFI_REGS_CONF_OP_SINGLE_READ	(0x02)
#define _SPI_NFI_REGS_CONF_OP_ERASE			(0x04)
#endif
#define _SPI_NFI_REGS_CONF_OP_PRGM			(3)
#define _SPI_NFI_REGS_CONF_OP_READ			(6)
#define _SPI_NFI_REGS_CONF_OP_MASK			(0x7000)
#define _SPI_NFI_REGS_CONF_OP_SHIFT			(12)

/* Flash -> SRAM */
#define _SPI_NFI_REGS_CNFG_DMA_WR_SWAP_SHIFT	(0x0003)
/* SRAM -> Flash */
#define _SPI_NFI_REGS_CNFG_DMA_RD_SWAP_SHIFT	(0x0004)
#define _SPI_NFI_REGS_CNFG_DMA_WR_SWAP_MASK		(1 << _SPI_NFI_REGS_CNFG_DMA_WR_SWAP_SHIFT)
#define _SPI_NFI_REGS_CNFG_DMA_RD_SWAP_MASK		(1 << _SPI_NFI_REGS_CNFG_DMA_RD_SWAP_SHIFT)

#define _SPI_NFI_REGS_CNFG_ECC_DATA_SOURCE_INV_SHIFT	(0x0005)
#define _SPI_NFI_REGS_CNFG_ECC_DATA_SOURCE_INV_MASK		(1 << _SPI_NFI_REGS_CNFG_ECC_DATA_SOURCE_INV_SHIFT)

/* NFI_PAGEFMT */
#define _SPI_NFI_REGS_PAGEFMT_PAGE_512		(0x0000)
#define _SPI_NFI_REGS_PAGEFMT_PAGE_2K		(0x0001)
#define _SPI_NFI_REGS_PAGEFMT_PAGE_4K		(0x0002)
#define _SPI_NFI_REGS_PAGEFMT_PAGE_MASK		(0x0003)
#define _SPI_NFI_REGS_PAGEFMT_PAGE_SHIFT	(0x0000)

#define _SPI_NFI_REGS_PAGEFMT_SPARE_16			(0x0000)
#define _SPI_NFI_REGS_PAGEFMT_SPARE_26			(0x0001)
#define _SPI_NFI_REGS_PAGEFMT_SPARE_27			(0x0002)
#define _SPI_NFI_REGS_PAGEFMT_SPARE_28			(0x0003)
#define _SPI_NFI_REGS_PAGEFMT_SPARE_MASK		(0x0030)
#define _SPI_NFI_REGS_PAGEFMT_SPARE_SHIFT		(4)

#define _SPI_NFI_REGS_PAGEFMT_FDM_MASK			(0x0F00)
#define _SPI_NFI_REGS_PAGEFMT_FDM_SHIFT			(8)
#define _SPI_NFI_REGS_PAGEFMT_FDM_ECC_MASK  	(0xF000)
#define _SPI_NFI_REGS_PAGEFMT_FDM_ECC_SHIFT 	(12)

#define _SPI_NFI_REGS_PPAGEFMT_SPARE_16     	(0x0000)
#define _SPI_NFI_REGS_PPAGEFMT_SPARE_26     	(0x0001)
#define _SPI_NFI_REGS_PPAGEFMT_SPARE_27     	(0x0002)
#define _SPI_NFI_REGS_PPAGEFMT_SPARE_28     	(0x0003)
#define _SPI_NFI_REGS_PPAGEFMT_SPARE_MASK   	(0x0030)
#define _SPI_NFI_REGS_PPAGEFMT_SPARE_SHIFT  	(4)

/* NFI_CON */
#define _SPI_NFI_REGS_CON_SEC_MASK				(0xF000)
#define _SPI_NFI_REGS_CON_WR_TRIG				(0x0200)
#define _SPI_NFI_REGS_CON_RD_TRIG				(0x0100)
#define _SPI_NFI_REGS_CON_SEC_SHIFT				(12)
#define _SPI_NFI_REGS_CON_RESET_VALUE			(0x3)

/* NFI_INTR_EN */
#define _SPI_NFI_REGS_INTR_EN_AHB_DONE_EN		(0x0040)

/* NFI_REGS_INTR */
#define _SPI_NFI_REGS_INTR_AHB_DONE_CHECK		(0x0040)

/* NFI_SECCUS_SIZE */
#define _SPI_NFI_REGS_SECCUS_SIZE_EN			(0x00010000)
#define _SPI_NFI_REGS_SECCUS_SIZE_MASK			(0x00001FFF)
#define _SPI_NFI_REGS_SECCUS_SIZE_SHIFT			(0)

/* NFI_SNF_MISC_CTL */
#define _SPI_NFI_REGS_SNF_MISC_CTL_DATA_RW_MODE_SHIFT	(16)

/* NFI_SNF_MISC_CTL2 */
#define _SPI_NFI_REGS_SNF_MISC_CTL2_WR_MASK		(0x1FFF0000)
#define _SPI_NFI_REGS_SNF_MISC_CTL2_WR_SHIFT	(16)
#define _SPI_NFI_REGS_SNF_MISC_CTL2_RD_MASK		(0x00001FFF)
#define _SPI_NFI_REGS_SNF_MISC_CTL2_RD_SHIFT	(0)

/* NFI_REGS_CMD */
#define _SPI_NFI_REGS_CMD_READ_VALUE			(0x00)
#define _SPI_NFI_REGS_CMD_WRITE_VALUE			(0x80)


/* NFI_REGS_PG_CTL1 */
#define _SPI_NFI_REGS_PG_CTL1_SHIFT				(8)


/* SNF_STA_CTL1 */
#define _SPI_NFI_REGS_LOAD_TO_CACHE_DONE		(0x04000000)
#define _SPI_NFI_REGS_READ_FROM_CACHE_DONE		(0x02000000)

#if defined(TCSUPPORT_PARALLEL_NAND)
#define _PARALLEL_NFI_REGS_CNT_CLR_WR			(0x04)
#define _PARALLEL_NFI_REGS_CNT_CLR_RD			(0x08)
#define _PARALLEL_NFI_REGS_CNRNB_TIME			(0x0F)   /* units is 16T*/
#endif

/* FUNCTION DECLARATIONS ------------------------------------------------------ */
#if !defined(IMAGE_BL2)
extern void * memcpy(void * dest,const void *src,size_t count);
#endif
void SPI_NFI_TRIGGER(SPI_NFI_CONF_DMA_TRIGGER_T rw);

/* MACRO DECLARATIONS ---------------------------------------------------------------- */

#ifdef TCSUPPORT_CPU_ARMV8
struct ecnt_spi2nfi_ctrl {
	struct device *dev;
	void __iomem *base;	/* spi2nfi base address */
};

struct ecnt_spi2nfi_ctrl *ecnt_spi2nfi_ctrl = NULL;
static int isInit = 0;

#define SPI_SPI2NFI_WRL(reg, data)	(writel(data, ecnt_spi2nfi_ctrl->base + reg))
#define SPI_SPI2NFI_RDL(reg)		(readl(ecnt_spi2nfi_ctrl->base + reg))

#define READ_REGISTER_UINT32(reg)		(SPI_SPI2NFI_RDL(reg))
#define WRITE_REGISTER_UINT32(reg, val)	(SPI_SPI2NFI_WRL(reg, val))

#define INREG32(x)          READ_REGISTER_UINT32(x)
#define OUTREG32(x, y)      WRITE_REGISTER_UINT32(x, y)
#else
#define READ_REGISTER_UINT32(reg) \
    (*(volatile unsigned int  * const)(reg))

#define WRITE_REGISTER_UINT32(reg, val) \
    (*(volatile unsigned int  * const)(reg)) = (val)

#define INREG32(x)          READ_REGISTER_UINT32((unsigned int *)((void*)(x)))
#define OUTREG32(x, y)      WRITE_REGISTER_UINT32((unsigned int *)((void*)(x)), (unsigned int )(y))
#endif

#define SETREG32(x, y)      OUTREG32(x, INREG32(x)|(y))
#define CLRREG32(x, y)      OUTREG32(x, INREG32(x)&~(y))
#define MASKREG32(x, y, z)  OUTREG32(x, (INREG32(x)&~(y))|(z))

#define _SPI_NFI_REG8_READ(addr)						INREG32(addr)
#define _SPI_NFI_REG8_WRITE(addr, data)					OUTREG32(addr, data)
#define _SPI_NFI_REG8_SETBITS(addr, data)				SETREG32(addr, data)
#define _SPI_NFI_REG8_CLRBITS(addr, data)				CLRREG32(addr, data)
#define _SPI_NFI_REG8_SETMASKBITS(addr, mask, data)	MASKREG32(addr, mask, data)

#define _SPI_NFI_REG16_READ(addr)						INREG32(addr)
#define _SPI_NFI_REG16_WRITE(addr, data)				OUTREG32(addr, data)
#define _SPI_NFI_REG16_SETBITS(addr, data)				SETREG32(addr, data)
#define _SPI_NFI_REG16_CLRBITS(addr, data)				CLRREG32(addr, data)
#define _SPI_NFI_REG16_SETMASKBITS(addr, mask, data)	MASKREG32(addr, mask, data)

#define _SPI_NFI_REG32_READ(addr)						INREG32(addr)
#define _SPI_NFI_REG32_WRITE(addr, data)				OUTREG32(addr, data)
#define _SPI_NFI_REG32_SETBITS(addr, data)				SETREG32(addr, data)
#define _SPI_NFI_REG32_CLRBITS(addr, data)				CLRREG32(addr, data)
#define _SPI_NFI_REG32_SETMASKBITS(addr, mask, data)	MASKREG32(addr, mask, data)

#define _SPI_NFI_GET_CONF_PTR							&(_spi_nfi_conf_info_t)
#define _SPI_NFI_GET_FDM_PTR							&(_spi_nfi_fdm_value)
#define _SPI_NFI_SET_FDM_PTR							&(_spi_nfi_fdm_value)
#define _SPI_NFI_DATA_SIZE_WITH_ECC						(512)
#define _SPI_NFI_CHECK_DONE_MAX_TIMES					(1000000)
#if !defined(SPI_NFI_DEBUG)
	#define _SPI_NFI_PRINTF(args...)						
	#define _SPI_NFI_DEBUG_PRINTF(args...)					
#else
	#ifdef SPRAM_IMG
	#define _SPI_NFI_PRINTF(fmt, args...)	prom_puts(fmt)		/* Always print information */
	#else
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
		#define _SPI_NFI_PRINTF									printk
		#else
		#define _SPI_NFI_PRINTF					prom_printf			/* Always print information */
		#endif
	#endif
#define _SPI_NFI_DEBUG_PRINTF			spi_nfi_debug_printf
#endif
#define _SPI_NFI_MEMCPY					memcpy
#define _SPI_NFI_MEMSET					memset
#define _SPI_NFI_MAX_FDM_NUMBER			(64)
#define _SPI_NFI_MAX_FDM_PER_SEC		(8)
extern u32 GET_NP_SCU_EMMC(void);

/* TYPE DECLARATIONS ----------------------------------------------------------------- */

typedef union {
#ifndef TCSUPPORT_LITTLE_ENDIAN
	struct NFI_CNFG {
		u32 unused2		:17;
		u32 opmode		:3;
		u32 unused1		:2;
		u32 auto_fdm	:1;
		u32 hw_ecc		:1;
		u32 unused0		:1;
		u32 byte_mode	:1;
		u32 ecc_data_inv:1;
		u32 wr_sram_swap:1;
		u32 rd_sram_swap:1;
		u32 dma_bust	:1;
		u32 read_mode	:1;
		u32 dma_mode	:1;
	} nfi_cnfg;

	struct NFI_CON {
		u32 unused2		:16;
		u32 sec_num		:4;
		u32 unused1		:2;
		u32 wr_trig		:1;
		u32 rd_trig		:1;
		u32 nob			:3;
		u32 srd			:1;
		u32 unused0		:2;
		u32 nfi_reset	:1;
		u32 fifo_flush	:1;
	} nfi_con;

	struct NFI_TIMING_CONF {
		u32 poecs		:4;
		u32 precs		:6;
		u32 c2r			:6;
		u32 w2r			:4;
		u32 wh			:4;
		u32 wst			:4;
		u32 rlt			:4;
	} nfi_acccon;

	struct NFI_ADDR_NOB {
		u32 unused1		:25;
		u32 row_nob		:3;
		u32 unused0		:1;
		u32 col_nob		:3;
	} nfi_addrnob;

	struct NFI_CHK_NAND_RB {
		u32 unused1		:24;
		u32 timeout		:4;
		u32 unused0		:3;
		u32 chk_trig	:1;
	} nfi_cnrnb;
	
	struct NFI_LOCK_ADDR_NOB {
		u32 unused3				:17;
		u32 lock_prog_row_nob	:3;
		u32 unused2				:1;
		u32 lock_prog_col_nob	:3;
		u32 unused1				:1;
		u32 lock_erase_row_nob	:3;
		u32 unused0				:1;
		u32 lock_erase_col_nob	:3;
	} nfi_lockannob;
#else
	struct NFI_CNFG {
		u32 dma_mode	:1;
		u32 read_mode	:1;
		u32 dma_bust	:1;
		u32 rd_sram_swap:1;
		u32 wr_sram_swap:1;
		u32 ecc_data_inv:1;
		u32 byte_mode	:1;
		u32 unused0		:1;
		u32 hw_ecc		:1;
		u32 auto_fdm	:1;
		u32 unused1		:2;
		u32 opmode		:3;
		u32 unused2		:17;
	} nfi_cnfg;

	struct NFI_CON {
		u32 fifo_flush	:1;
		u32 nfi_reset	:1;
		u32 unused0		:2;
		u32 srd			:1;
		u32 nob			:3;
		u32 rd_trig		:1;
		u32 wr_trig		:1;
		u32 unused1		:2;
		u32 sec_num		:4;
		u32 unused2		:16;
	} nfi_con;

	struct NFI_TIMING_CONF {
		u32 rlt			:4;
		u32 wst			:4;
		u32 wh			:4;
		u32 w2r			:4;
		u32 c2r			:6;
		u32 precs		:6;
		u32 poecs		:4;
	} nfi_acccon;

	struct NFI_ADDR_NOB {
		u32 col_nob		:3;
		u32 unused0		:1;
		u32 row_nob		:3;
		u32 unused1		:25;
	} nfi_addrnob;

	struct NFI_CHK_NAND_RB {
		u32 chk_trig	:1;
		u32 unused0		:3;
		u32 timeout		:4;
		u32 unused1		:24;
	} nfi_cnrnb;
	
	struct NFI_LOCK_ADDR_NOB {
		u32 lock_erase_col_nob	:3;
		u32 unused0				:1 ;
		u32 lock_erase_row_nob	:3;
		u32 unused1				:1 ;
		u32 lock_prog_col_nob	:3;
		u32 unused2				:1 ;
		u32 lock_prog_row_nob	:3;
		u32 unused3				:17;
	} nfi_lockannob;
#endif

	u32 reg;
} nfi_reg;


/* STATIC VARIABLE DECLARATIONS ------------------------------------------------------ */
SPI_NFI_CONF_T	_spi_nfi_conf_info_t;
u8				_spi_nfi_fdm_value[_SPI_NFI_MAX_FDM_NUMBER];
u8				_SPI_NFI_DEBUG_FLAG = 0;	/* For control printf debug message or not */

/* LOCAL SUBPROGRAM BODIES------------------------------------------------------------ */

#if defined(SPI_NFI_DEBUG)
static void spi_nfi_debug_printf( char *fmt, ... )
{
	if( _SPI_NFI_DEBUG_FLAG == 1 )
	{
#ifdef SPRAM_IMG
		_SPI_NFI_PRINTF(fmt);
#else
		unsigned char 		str_buf[100];	
		va_list 			argptr;
		int 				cnt;		
	
		va_start(argptr, fmt);
		cnt = vsprintf(str_buf, fmt, argptr);
		va_end(argptr);
				
		_SPI_NFI_PRINTF("%s", str_buf);	
#endif
	}
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)

#if !defined(IMAGE_BL2)
#if defined(CONFIG_ECNT_UBOOT)
int get_SYS_HCLK(void)
{
	return SYS_HCLK;
}
#endif

static void delay1us(int us)
{
	volatile uint32 timer_now, timer_last;
	volatile uint32 tick_acc;
	uint32 one_tick_unit = 1 * get_SYS_HCLK() / 2;
	volatile uint32 tick_wait = us * one_tick_unit; 
	volatile uint32 timer1_ldv = VPint(CR_TIMER1_LDV);

	tick_acc = 0;
	timer_last = VPint(CR_TIMER1_VLR);
	do {
		timer_now = VPint(CR_TIMER1_VLR);
	  	if (timer_last >= timer_now) 
	  		tick_acc += timer_last - timer_now;
		else
			tick_acc += timer1_ldv - timer_now + timer_last;
		timer_last = timer_now;
	} while (tick_acc < tick_wait);
}
#else
#include <drivers/delay_timer.h>

static void delay1us(int us)
{
	udelay(us);
}

#endif
#endif

SPI_NFI_RTN_T spi_nfi_get_fdm_from_register(void)
{
	u32				idx, i, j, reg_addr, val;	
	u8				*fdm_value;
	SPI_NFI_CONF_T	*spi_nfi_conf_info_t;		
	u8				spi_nfi_mapping_fdm_value[_SPI_NFI_MAX_FDM_NUMBER];	
	
	fdm_value = (u8 *) _SPI_NFI_GET_FDM_PTR;
	spi_nfi_conf_info_t = _SPI_NFI_GET_CONF_PTR;

    _SPI_NFI_MEMSET(spi_nfi_mapping_fdm_value, 0xff, _SPI_NFI_MAX_FDM_NUMBER);
	_SPI_NFI_MEMSET(fdm_value, 0xff, _SPI_NFI_MAX_FDM_NUMBER);
	
	idx = 0;
	for( reg_addr = _SPI_NFI_REGS_FDM0L ; reg_addr <= _SPI_NFI_REGS_FDM7M ; reg_addr+=4 )
	{
		val = _SPI_NFI_REG32_READ(reg_addr);
		spi_nfi_mapping_fdm_value[idx++] = ( val & 0xFF) ;
		spi_nfi_mapping_fdm_value[idx++] = ((val >> 8) & 0xFF) ;
		spi_nfi_mapping_fdm_value[idx++] = ((val >> 16) & 0xFF) ;
		spi_nfi_mapping_fdm_value[idx++] = ((val >> 24) & 0xFF) ;

		_SPI_NFI_DEBUG_PRINTF("spi_nfi_get_fdm_from_register : reg(0x%x)=0x%x\n", reg_addr, _SPI_NFI_REG32_READ(reg_addr));
	}

	j=0;
	for(idx=0 ; idx< (spi_nfi_conf_info_t->sec_num) ; idx++)
	{
		for(i =0; i< (spi_nfi_conf_info_t->fdm_num); i++)
		{
			fdm_value[j] = spi_nfi_mapping_fdm_value[(idx*_SPI_NFI_MAX_FDM_PER_SEC)+i];
			j++;
		}
	}
	
	return (SPI_NFI_RTN_NO_ERROR);	
}


SPI_NFI_RTN_T spi_nfi_set_fdm_into_register(void)
{
	u32				idx, i,j, reg_addr, val;
	u8				*fdm_value;
	SPI_NFI_CONF_T	*spi_nfi_conf_info_t;	
	u8				spi_nfi_mapping_fdm_value[_SPI_NFI_MAX_FDM_NUMBER];
	
	fdm_value 			= (u8 *) _SPI_NFI_GET_FDM_PTR;
	spi_nfi_conf_info_t = _SPI_NFI_GET_CONF_PTR;

    _SPI_NFI_MEMSET(spi_nfi_mapping_fdm_value, 0xff, _SPI_NFI_MAX_FDM_NUMBER);

	j=0;
	for(idx=0 ; idx< (spi_nfi_conf_info_t->sec_num) ; idx++)
	{
		for(i =0; i< (spi_nfi_conf_info_t->fdm_num); i++)
		{
			spi_nfi_mapping_fdm_value[(idx*_SPI_NFI_MAX_FDM_PER_SEC)+i] = fdm_value[j];
			j++;
		}
	}

	
	idx = 0;
	for( reg_addr = _SPI_NFI_REGS_FDM0L ; reg_addr <= _SPI_NFI_REGS_FDM7M ; reg_addr+=4 )
	{
		val = 0;

		val |= (spi_nfi_mapping_fdm_value[idx++] & (0xFF));
		val |= ((spi_nfi_mapping_fdm_value[idx++] & (0xFF)) << 8);
		val |= ((spi_nfi_mapping_fdm_value[idx++] & (0xFF)) << 16);
		val |= ((spi_nfi_mapping_fdm_value[idx++] & (0xFF)) << 24);

		 _SPI_NFI_REG32_WRITE(reg_addr, val); 

		_SPI_NFI_DEBUG_PRINTF("spi_nfi_set_fdm_into_register : reg(0x%x)=0x%x\n", reg_addr, _SPI_NFI_REG32_READ(reg_addr));
	}
	
	return (SPI_NFI_RTN_NO_ERROR);
}



/* EXPORTED SUBPROGRAM BODIES -------------------------------------------------------- */

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
SPI_NFI_RTN_T SPI_NFI_Regs_Dump( void )
{
	u32		idx;
	
	for(idx = _SPI_NFI_REGS_BASE ; idx <= _SPI_NFI_REGS_SNF_STA_CTL2 ; idx +=4)
	{
		_SPI_NFI_PRINTF("reg(0x%x) = 0x%x\n", idx, _SPI_NFI_REG32_READ(idx) );
	}
	
	return (SPI_NFI_RTN_NO_ERROR);
}
#endif

SPI_NFI_RTN_T SPI_NFI_Read_SPI_NAND_FDM(u8 *ptr_rtn_oob, u32 oob_len)
{
	u8		*fdm_value;
#if !defined(IMAGE_BL2)
	u32		idx;
#endif
	
	spi_nfi_get_fdm_from_register();	
	fdm_value = (u8 *) _SPI_NFI_GET_FDM_PTR;
	
	_SPI_NFI_MEMCPY(ptr_rtn_oob, fdm_value, oob_len);	
	
	return (SPI_NFI_RTN_NO_ERROR);
}

SPI_NFI_RTN_T SPI_NFI_Write_SPI_NAND_FDM(u8 *ptr_oob, u32 oob_len)
{
	u8		*fdm_value;
#if !defined(IMAGE_BL2)
	u32		idx;
#endif
	
	fdm_value = (u8 *) _SPI_NFI_GET_FDM_PTR;

	if( oob_len	> _SPI_NFI_MAX_FDM_NUMBER ) 
	{
		_SPI_NFI_MEMCPY(fdm_value, ptr_oob, _SPI_NFI_MAX_FDM_NUMBER);	
	}
	else
	{
		_SPI_NFI_MEMCPY(fdm_value, ptr_oob, oob_len);
	}

	spi_nfi_set_fdm_into_register();
	
	return (SPI_NFI_RTN_NO_ERROR);
}

SPI_NFI_RTN_T SPI_NFI_Read_SPI_NAND_Page(SPI_NFI_MISC_SPEDD_CONTROL_T speed_mode, u32 read_cmd, u16 read_addr, unsigned long *prt_rtn_data)
{
	u32				check_cnt;
	SPI_NFI_CONF_T	*spi_nfi_conf_info_t;
	SPI_NFI_RTN_T	rtn_status = SPI_NFI_RTN_NO_ERROR;

	
	spi_nfi_conf_info_t = _SPI_NFI_GET_CONF_PTR;

	/* Set DMA destination address */	
	_SPI_NFI_REG32_WRITE( _SPI_NFI_REGS_STRADDR, prt_rtn_data);
	
	/* Set Read length */
	if( (spi_nfi_conf_info_t->cus_sec_size_en_t) == SPI_NFI_CONF_CUS_SEC_SIZE_Disable )
	{
		_SPI_NFI_REG32_SETMASKBITS(_SPI_NFI_REGS_SNF_MISC_CTL2, _SPI_NFI_REGS_SNF_MISC_CTL2_RD_MASK,	\
				((_SPI_NFI_DATA_SIZE_WITH_ECC + (spi_nfi_conf_info_t->spare_size_t)) * (spi_nfi_conf_info_t->sec_num))<< _SPI_NFI_REGS_SNF_MISC_CTL2_RD_SHIFT );
	}
	if( (spi_nfi_conf_info_t->cus_sec_size_en_t) == SPI_NFI_CONF_CUS_SEC_SIZE_Enable )
	{
		_SPI_NFI_REG32_SETMASKBITS(_SPI_NFI_REGS_SNF_MISC_CTL2, _SPI_NFI_REGS_SNF_MISC_CTL2_RD_MASK,	\
				((spi_nfi_conf_info_t->sec_size) * (spi_nfi_conf_info_t->sec_num))<< _SPI_NFI_REGS_SNF_MISC_CTL2_RD_SHIFT );
	}
	
	/* Set Read Command */
	_SPI_NFI_REG32_WRITE(_SPI_NFI_REGS_RD_CTL2, (read_cmd & 0xFF));

	/* Set Read mode */
	_SPI_NFI_REG32_WRITE(_SPI_NFI_REGS_SNF_MISC_CTL, (speed_mode << _SPI_NFI_REGS_SNF_MISC_CTL_DATA_RW_MODE_SHIFT));
	
	/* Set Read Address (Note : Controller will use following register, depend on the Hardware TRAP of SPI NAND/SPI NOR  )*/
	_SPI_NFI_REG32_WRITE( _SPI_NFI_REGS_RD_CTL3, read_addr);		/* Set Address into SPI NAND address register*/
	
	/* Set NFI Read */
	_SPI_NFI_REG16_SETMASKBITS(_SPI_NFI_REGS_CNFG, _SPI_NFI_REGS_CONF_OP_MASK, \
								(_SPI_NFI_REGS_CONF_OP_READ << _SPI_NFI_REGS_CONF_OP_SHIFT ));
	_SPI_NFI_REG16_SETBITS(_SPI_NFI_REGS_CNFG, _SPI_NFI_REGS_CNFG_READ_EN);
	_SPI_NFI_REG16_SETBITS(_SPI_NFI_REGS_CNFG, _SPI_NFI_REGS_CNFG_AHB);


	_SPI_NFI_REG16_WRITE( _SPI_NFI_REGS_CMD, _SPI_NFI_REGS_CMD_READ_VALUE);

	
	/* Trigger DMA read active*/
	SPI_NFI_TRIGGER(SPI_NFI_CON_DMA_TRIGGER_READ);
	
	/* Check read from cache  done or not */
	for( check_cnt = 0 ; check_cnt < _SPI_NFI_CHECK_DONE_MAX_TIMES ; check_cnt ++)
	{
		if( (_SPI_NFI_REG16_READ(_SPI_NFI_REGS_SNF_STA_CTL1)& (_SPI_NFI_REGS_READ_FROM_CACHE_DONE)) != 0 )
		{		
			/* Clear this bit is neccessary for NFI state machine */
			_SPI_NFI_REG32_SETBITS(_SPI_NFI_REGS_SNF_STA_CTL1, _SPI_NFI_REGS_READ_FROM_CACHE_DONE);
			break;
		}
	}
	if(check_cnt == _SPI_NFI_CHECK_DONE_MAX_TIMES)
	{
		_SPI_NFI_PRINTF("[Error] Read DMA : Check READ FROM CACHE Done Timeout ! \n");
		rtn_status = SPI_NFI_RTN_READ_FROM_CACHE_DONE_TIMEOUT;
	}

	/* Check DMA done or not */
	for( check_cnt = 0 ; check_cnt < _SPI_NFI_CHECK_DONE_MAX_TIMES ; check_cnt ++)
	{
		if( (_SPI_NFI_REG16_READ(_SPI_NFI_REGS_INTR)& (_SPI_NFI_REGS_INTR_AHB_DONE_CHECK)) != 0 )
		{
			break;
		}
	}
	if(check_cnt == _SPI_NFI_CHECK_DONE_MAX_TIMES)
	{		
		_SPI_NFI_PRINTF("[Error] Read DMA : Check AHB Done Timeout ! \n");
		rtn_status = SPI_NFI_RTN_CHECK_AHB_DONE_TIMEOUT;
	}

	/* Does DMA read need delay for data ready from controller to DRAM */
	delay1us(1);

	return (rtn_status);
}

#if !defined(LZMA_IMG) || defined(TCSUPPORT_BB_256KB) 
SPI_NFI_RTN_T SPI_NFI_Write_SPI_NAND_page(SPI_NFI_MISC_SPEDD_CONTROL_T speed_mode, u32 write_cmd, u16 write_addr, unsigned long *prt_data)
{
	
	volatile u32				check_cnt;
	SPI_NFI_CONF_T	*spi_nfi_conf_info_t;
	SPI_NFI_RTN_T	rtn_status = SPI_NFI_RTN_NO_ERROR;	

	_SPI_NFI_DEBUG_PRINTF("SPI_NFI_Write_SPI_NAND_page : enter, speed_mode=%d, write_cmd=0x%x, write_addr=0x%x, prt_data=0x%x\n", speed_mode, write_cmd, write_addr, ((u32) prt_data));


	spi_nfi_conf_info_t = _SPI_NFI_GET_CONF_PTR;

	/* Set DMA destination address */	
	_SPI_NFI_REG32_WRITE( _SPI_NFI_REGS_STRADDR, prt_data);

	_SPI_NFI_DEBUG_PRINTF("SPI_NFI_Write_SPI_NAND_page: _SPI_NFI_REGS_STRADDR=0x%x\n", _SPI_NFI_REG32_READ(_SPI_NFI_REGS_STRADDR));	
	_SPI_NFI_DEBUG_PRINTF("SPI_NFI_Write_SPI_NAND_page\n");
	
	/* Set Write length */
	if( (spi_nfi_conf_info_t->cus_sec_size_en_t) == SPI_NFI_CONF_CUS_SEC_SIZE_Disable )
	{
		_SPI_NFI_REG32_SETMASKBITS(_SPI_NFI_REGS_SNF_MISC_CTL2, _SPI_NFI_REGS_SNF_MISC_CTL2_WR_MASK,	\
				((_SPI_NFI_DATA_SIZE_WITH_ECC + (spi_nfi_conf_info_t->spare_size_t)) * (spi_nfi_conf_info_t->sec_num))<< _SPI_NFI_REGS_SNF_MISC_CTL2_WR_SHIFT );
	}
	if( (spi_nfi_conf_info_t->cus_sec_size_en_t) == SPI_NFI_CONF_CUS_SEC_SIZE_Enable )
	{
		_SPI_NFI_REG32_SETMASKBITS(_SPI_NFI_REGS_SNF_MISC_CTL2, _SPI_NFI_REGS_SNF_MISC_CTL2_WR_MASK,	\
				((spi_nfi_conf_info_t->sec_size) * (spi_nfi_conf_info_t->sec_num))<< _SPI_NFI_REGS_SNF_MISC_CTL2_WR_SHIFT );
	}	
	
	/* Set Write Command */
	_SPI_NFI_REG32_WRITE( _SPI_NFI_REGS_PG_CTL1, ((write_cmd & 0xFF) << _SPI_NFI_REGS_PG_CTL1_SHIFT));

	/* Set Write mode */
	_SPI_NFI_REG32_WRITE(_SPI_NFI_REGS_SNF_MISC_CTL, (speed_mode << _SPI_NFI_REGS_SNF_MISC_CTL_DATA_RW_MODE_SHIFT));

	/* Set Write Address (Note : Controller will use following register, depend on the Hardware TRAP of SPI NAND/SPI NOR  )*/
	_SPI_NFI_REG32_WRITE( _SPI_NFI_REGS_PG_CTL2, write_addr);		/* Set Address into SPI NAND address register*/

	/* Set NFI Write */
	_SPI_NFI_REG16_CLRBITS(_SPI_NFI_REGS_CNFG, _SPI_NFI_REGS_CNFG_READ_EN);
	_SPI_NFI_REG16_SETMASKBITS(_SPI_NFI_REGS_CNFG, _SPI_NFI_REGS_CONF_OP_MASK, \
								(_SPI_NFI_REGS_CONF_OP_PRGM << _SPI_NFI_REGS_CONF_OP_SHIFT ));

	_SPI_NFI_REG16_SETBITS(_SPI_NFI_REGS_CNFG, _SPI_NFI_REGS_CNFG_AHB);
	_SPI_NFI_REG16_WRITE( _SPI_NFI_REGS_CMD, _SPI_NFI_REGS_CMD_WRITE_VALUE);
	
	/* Trigger DMA write active*/
	SPI_NFI_TRIGGER(SPI_NFI_CON_DMA_TRIGGER_WRITE);

	/* Does DMA write need delay for data ready from controller to Flash */
	delay1us(1);
	
	/* Check DMA done or not */
	for( check_cnt = 0 ; check_cnt < _SPI_NFI_CHECK_DONE_MAX_TIMES ; check_cnt ++)
	{
		if( (_SPI_NFI_REG16_READ(_SPI_NFI_REGS_INTR)& (_SPI_NFI_REGS_INTR_AHB_DONE_CHECK)) != 0 )
		{
			break;
		}
	}
	if(check_cnt == _SPI_NFI_CHECK_DONE_MAX_TIMES)
	{		
		_SPI_NFI_PRINTF("[Error] Write DMA : Check AHB Done Timeout ! \n");
		rtn_status = SPI_NFI_RTN_CHECK_AHB_DONE_TIMEOUT;
	}

	/* Check load to cache  done or not */
	for( check_cnt = 0 ; check_cnt < _SPI_NFI_CHECK_DONE_MAX_TIMES ; check_cnt ++)
	{
		if( (_SPI_NFI_REG16_READ(_SPI_NFI_REGS_SNF_STA_CTL1)& (_SPI_NFI_REGS_LOAD_TO_CACHE_DONE)) != 0 )
		{		
			/* Clear this bit is neccessary for NFI state machine */
			_SPI_NFI_REG32_SETBITS(_SPI_NFI_REGS_SNF_STA_CTL1, _SPI_NFI_REGS_LOAD_TO_CACHE_DONE);
			break;
		}
	}
	if(check_cnt == _SPI_NFI_CHECK_DONE_MAX_TIMES)
	{		
		_SPI_NFI_PRINTF("[Error] Write DMA : Check LOAD TO CACHE Done Timeout ! \n");
		rtn_status = SPI_NFI_RTN_LOAD_TO_CACHE_DONE_TIMEOUT;
	}

	_SPI_NFI_DEBUG_PRINTF("SPI_NFI_Write_SPI_NAND_page : exit \n");
	
	return (rtn_status);
	
}
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
SPI_NFI_RTN_T SPI_NFI_Read_SPI_NOR(u8 opcode, u16 read_addr, u32 *prt_rtn_data)
{
	u32				check_cnt;
	SPI_NFI_CONF_T	*spi_nfi_conf_info_t;
	SPI_NFI_RTN_T	rtn_status = SPI_NFI_RTN_NO_ERROR;
	
	
	spi_nfi_conf_info_t = _SPI_NFI_GET_CONF_PTR;
	
	/* Set Read length */
	if( (spi_nfi_conf_info_t->cus_sec_size_en_t) == SPI_NFI_CONF_CUS_SEC_SIZE_Disable )
	{
		_SPI_NFI_REG32_SETMASKBITS(_SPI_NFI_REGS_SNF_MISC_CTL2, _SPI_NFI_REGS_SNF_MISC_CTL2_RD_MASK,	\
				((_SPI_NFI_DATA_SIZE_WITH_ECC + (spi_nfi_conf_info_t->spare_size_t)) * (spi_nfi_conf_info_t->sec_num))<< _SPI_NFI_REGS_CON_SEC_SHIFT );
	}
	if( (spi_nfi_conf_info_t->cus_sec_size_en_t) == SPI_NFI_CONF_CUS_SEC_SIZE_Enable )
	{
		_SPI_NFI_REG32_SETMASKBITS(_SPI_NFI_REGS_SNF_MISC_CTL2, _SPI_NFI_REGS_SNF_MISC_CTL2_RD_MASK,	\
				((spi_nfi_conf_info_t->sec_size) * (spi_nfi_conf_info_t->sec_num))<< _SPI_NFI_REGS_CON_SEC_SHIFT );
	}
	
	/* Set Read Command */
	_SPI_NFI_REG32_WRITE( _SPI_NFI_REGS_RD_CTL2, (u32) opcode);
	
	/* Set Read Address (Note : Controller will use following register, depend on the Hardware TRAP of SPI NAND/SPI NOR  )*/
	_SPI_NFI_REG32_WRITE( _SPI_NFI_REGS_NOR_RD_ADDR, read_addr);	/* Set Address into SPI NOR address register*/
	
	/* Reset NFI statemachile and flush fifo*/
	_SPI_NFI_REG16_SETBITS( _SPI_NFI_REGS_CON, _SPI_NFI_REGS_CON_RESET_VALUE);
	
	/* Set NFI Read */
	_SPI_NFI_REG16_WRITE( _SPI_NFI_REGS_CMD, _SPI_NFI_REGS_CMD_READ_VALUE);
	
	/* Set DMA destination address */	
	_SPI_NFI_REG32_WRITE( _SPI_NFI_REGS_STRADDR, prt_rtn_data);
	
	/* Trigger DMA read active*/
	_SPI_NFI_REG16_CLRBITS( _SPI_NFI_REGS_CON, _SPI_NFI_REGS_CON_RD_TRIG);
	 /* [Note : Is here need to have little time delay or not ? */
	_SPI_NFI_REG16_SETBITS( _SPI_NFI_REGS_CON, _SPI_NFI_REGS_CON_RD_TRIG);	
	
	/* Check DMA done or not */
	for( check_cnt = 0 ; check_cnt < _SPI_NFI_CHECK_DONE_MAX_TIMES ; check_cnt ++)
	{
		if( (_SPI_NFI_REG16_READ(_SPI_NFI_REGS_INTR)& (_SPI_NFI_REGS_INTR_AHB_DONE_CHECK)) != 0 )
		{
			break;
		}
	}
	if(check_cnt == _SPI_NFI_CHECK_DONE_MAX_TIMES)
	{		
		_SPI_NFI_PRINTF("[Error] Read DMA : Check AHB Done Timeout ! \n");
		rtn_status = SPI_NFI_RTN_CHECK_AHB_DONE_TIMEOUT;
	}		
	
	return (rtn_status);
}

SPI_NFI_RTN_T SPI_NFI_Write_SPI_NOR(u8 opcode, u16 write_addr, u32 *prt_data)
{
	
	u32				check_cnt;
	SPI_NFI_CONF_T	*spi_nfi_conf_info_t;
	SPI_NFI_RTN_T	rtn_status = SPI_NFI_RTN_NO_ERROR;	
	
	/* Set Write length */
	spi_nfi_conf_info_t = _SPI_NFI_GET_CONF_PTR; //COV, CID 140259
	if( (spi_nfi_conf_info_t->cus_sec_size_en_t) == SPI_NFI_CONF_CUS_SEC_SIZE_Disable )
	{
		_SPI_NFI_REG32_SETMASKBITS(_SPI_NFI_REGS_SNF_MISC_CTL2, _SPI_NFI_REGS_SNF_MISC_CTL2_RD_MASK,	\
				((_SPI_NFI_DATA_SIZE_WITH_ECC + (spi_nfi_conf_info_t->spare_size_t)) * (spi_nfi_conf_info_t->sec_num))<< _SPI_NFI_REGS_CON_SEC_SHIFT );
	}
	if( (spi_nfi_conf_info_t->cus_sec_size_en_t) == SPI_NFI_CONF_CUS_SEC_SIZE_Enable )
	{
		_SPI_NFI_REG32_SETMASKBITS(_SPI_NFI_REGS_SNF_MISC_CTL2, _SPI_NFI_REGS_SNF_MISC_CTL2_RD_MASK,	\
				((spi_nfi_conf_info_t->sec_size) * (spi_nfi_conf_info_t->sec_num))<< _SPI_NFI_REGS_CON_SEC_SHIFT );
	}	
	
	/* Set Write Command */
	_SPI_NFI_REG32_WRITE( _SPI_NFI_REGS_PG_CTL1, ((u32) opcode) << _SPI_NFI_REGS_PG_CTL1_SHIFT);
	
	/* Set Write Address (Note : Controller will use following register, depend on the Hardware TRAP of SPI NAND/SPI NOR  )*/
	_SPI_NFI_REG32_WRITE( _SPI_NFI_REGS_NOR_PROG_ADDR, write_addr);	/* Set Address into SPI NOR address register*/	

	/* Reset NFI statemachile and flush fifo*/
	_SPI_NFI_REG16_SETBITS( _SPI_NFI_REGS_CON, _SPI_NFI_REGS_CON_RESET_VALUE);
	
	/* Set NFI Write */
	_SPI_NFI_REG16_WRITE( _SPI_NFI_REGS_CMD, _SPI_NFI_REGS_CMD_WRITE_VALUE);
	
	/* Set DMA destination address */	
	_SPI_NFI_REG32_WRITE( _SPI_NFI_REGS_STRADDR, prt_data);
	
	/* Trigger DMA read active*/
	_SPI_NFI_REG16_CLRBITS( _SPI_NFI_REGS_CON, _SPI_NFI_REGS_CON_WR_TRIG);
	 /* [Note : Is here need to have little time delay or not ? */
	_SPI_NFI_REG16_SETBITS( _SPI_NFI_REGS_CON, _SPI_NFI_REGS_CON_WR_TRIG);
	
	/* Check DMA done or not */
	for( check_cnt = 0 ; check_cnt < _SPI_NFI_CHECK_DONE_MAX_TIMES ; check_cnt ++)
	{
		if( (_SPI_NFI_REG16_READ(_SPI_NFI_REGS_INTR)& (_SPI_NFI_REGS_INTR_AHB_DONE_CHECK)) != 0 )
		{
			break;
		}
	}
	if(check_cnt == _SPI_NFI_CHECK_DONE_MAX_TIMES)
	{		
		_SPI_NFI_PRINTF("[Error] Write DMA : Check AHB Done Timeout ! \n");
		rtn_status = SPI_NFI_RTN_CHECK_AHB_DONE_TIMEOUT;
	}	
	
	
	return (rtn_status);
	
}
#endif

SPI_NFI_RTN_T SPI_NFI_Get_Configure( SPI_NFI_CONF_T *ptr_rtn_nfi_conf_t )
{	
	SPI_NFI_CONF_T  *ptr_spi_nfi_conf_info_t;
	
	ptr_spi_nfi_conf_info_t = _SPI_NFI_GET_CONF_PTR;
	_SPI_NFI_MEMCPY(ptr_rtn_nfi_conf_t, ptr_spi_nfi_conf_info_t, sizeof(SPI_NFI_CONF_T));	
	
	return (SPI_NFI_RTN_NO_ERROR);
}


SPI_NFI_RTN_T SPI_NFI_Set_Configure( SPI_NFI_CONF_T *ptr_nfi_conf_t )
{
	SPI_NFI_CONF_T  *ptr_spi_nfi_conf_info_t;
			
	/* Store new setting */
	ptr_spi_nfi_conf_info_t = _SPI_NFI_GET_CONF_PTR;
	_SPI_NFI_MEMCPY(ptr_spi_nfi_conf_info_t, ptr_nfi_conf_t, sizeof(SPI_NFI_CONF_T));	

	
	_SPI_NFI_DEBUG_PRINTF("SPI_NFI_Set_Configure: hw_ecc_t= 0x%x\n", ptr_nfi_conf_t->hw_ecc_t );
	
	/* Set Auto FDM */
	if( (ptr_nfi_conf_t->auto_fdm_t) == SPI_NFI_CON_AUTO_FDM_Disable )
	{
		_SPI_NFI_REG16_CLRBITS(_SPI_NFI_REGS_CNFG, _SPI_NFI_REGS_CNFG_AUTO_FMT_EN);
	}
	if( (ptr_nfi_conf_t->auto_fdm_t) == SPI_NFI_CON_AUTO_FDM_Enable )
	{
		_SPI_NFI_REG16_SETBITS(_SPI_NFI_REGS_CNFG, _SPI_NFI_REGS_CNFG_AUTO_FMT_EN);
	}

	/* Set Hardware ECC */
	if( (ptr_nfi_conf_t->hw_ecc_t) == SPI_NFI_CON_HW_ECC_Disable )
	{
		_SPI_NFI_REG16_CLRBITS(_SPI_NFI_REGS_CNFG, _SPI_NFI_REGS_CNFG_HW_ECC_EN);
	}
	if( (ptr_nfi_conf_t->hw_ecc_t) == SPI_NFI_CON_HW_ECC_Enable )
	{
		_SPI_NFI_REG16_SETBITS(_SPI_NFI_REGS_CNFG, _SPI_NFI_REGS_CNFG_HW_ECC_EN);
	}
	
	/* Set DMA BURST */
	if( (ptr_nfi_conf_t->dma_burst_t) == SPI_NFI_CON_DMA_BURST_Disable )
	{
		_SPI_NFI_REG16_CLRBITS(_SPI_NFI_REGS_CNFG, _SPI_NFI_REGS_CNFG_DMA_BURST_EN);
	}
	if( (ptr_nfi_conf_t->dma_burst_t) == SPI_NFI_CON_DMA_BURST_Enable )
	{
		_SPI_NFI_REG16_SETBITS(_SPI_NFI_REGS_CNFG, _SPI_NFI_REGS_CNFG_DMA_BURST_EN);
	}
	
	/* Set FDM Number */
	_SPI_NFI_REG16_SETMASKBITS(_SPI_NFI_REGS_PAGEFMT, _SPI_NFI_REGS_PAGEFMT_FDM_MASK,	\
								(ptr_nfi_conf_t->fdm_num)<< _SPI_NFI_REGS_PAGEFMT_FDM_SHIFT );
								
	/* Set FDM ECC Number */
	_SPI_NFI_REG16_SETMASKBITS(_SPI_NFI_REGS_PAGEFMT, _SPI_NFI_REGS_PAGEFMT_FDM_ECC_MASK,	\
								(ptr_nfi_conf_t->fdm_ecc_num)<< _SPI_NFI_REGS_PAGEFMT_FDM_ECC_SHIFT );
								
	/* Set SPARE Size */
	if( (ptr_nfi_conf_t->spare_size_t) == SPI_NFI_CONF_SPARE_SIZE_16BYTE )
	{
		_SPI_NFI_REG16_SETMASKBITS(_SPI_NFI_REGS_PAGEFMT, _SPI_NFI_REGS_PAGEFMT_SPARE_MASK,	\
								_SPI_NFI_REGS_PAGEFMT_SPARE_16 << _SPI_NFI_REGS_PAGEFMT_SPARE_SHIFT );
	}
	if( (ptr_nfi_conf_t->spare_size_t) == SPI_NFI_CONF_SPARE_SIZE_26BYTE )
	{
		_SPI_NFI_REG16_SETMASKBITS(_SPI_NFI_REGS_PAGEFMT, _SPI_NFI_REGS_PAGEFMT_SPARE_MASK,	\
								_SPI_NFI_REGS_PAGEFMT_SPARE_26 << _SPI_NFI_REGS_PAGEFMT_SPARE_SHIFT );
	}
	if( (ptr_nfi_conf_t->spare_size_t) == SPI_NFI_CONF_SPARE_SIZE_27BYTE )
	{
		_SPI_NFI_REG16_SETMASKBITS(_SPI_NFI_REGS_PAGEFMT, _SPI_NFI_REGS_PAGEFMT_SPARE_MASK,	\
								_SPI_NFI_REGS_PAGEFMT_SPARE_27 << _SPI_NFI_REGS_PAGEFMT_SPARE_SHIFT );
	}
	if( (ptr_nfi_conf_t->spare_size_t) == SPI_NFI_CONF_SPARE_SIZE_28BYTE )
	{
		_SPI_NFI_REG16_SETMASKBITS(_SPI_NFI_REGS_PAGEFMT, _SPI_NFI_REGS_PAGEFMT_SPARE_MASK,	\
								_SPI_NFI_REGS_PAGEFMT_SPARE_28 << _SPI_NFI_REGS_PAGEFMT_SPARE_SHIFT );
	}
	
	/* Set PAGE Size */
	if( (ptr_nfi_conf_t->page_size_t) == SPI_NFI_CONF_PAGE_SIZE_512BYTE )
	{
		_SPI_NFI_REG16_SETMASKBITS(_SPI_NFI_REGS_PAGEFMT, _SPI_NFI_REGS_PAGEFMT_PAGE_MASK,	\
								   _SPI_NFI_REGS_PAGEFMT_PAGE_512 << _SPI_NFI_REGS_PAGEFMT_PAGE_SHIFT );
	}
	if( (ptr_nfi_conf_t->page_size_t) == SPI_NFI_CONF_PAGE_SIZE_2KBYTE )
	{
		_SPI_NFI_REG16_SETMASKBITS(_SPI_NFI_REGS_PAGEFMT, _SPI_NFI_REGS_PAGEFMT_PAGE_MASK,	\
								   _SPI_NFI_REGS_PAGEFMT_PAGE_2K << _SPI_NFI_REGS_PAGEFMT_PAGE_SHIFT );
	}
	if( (ptr_nfi_conf_t->page_size_t) == SPI_NFI_CONF_PAGE_SIZE_4KBYTE )
	{
		_SPI_NFI_REG16_SETMASKBITS(_SPI_NFI_REGS_PAGEFMT, _SPI_NFI_REGS_PAGEFMT_PAGE_MASK,	\
								   _SPI_NFI_REGS_PAGEFMT_PAGE_4K << _SPI_NFI_REGS_PAGEFMT_PAGE_SHIFT );
	}		
	
	/* Set sector number */
	_SPI_NFI_REG16_SETMASKBITS(_SPI_NFI_REGS_CON, _SPI_NFI_REGS_CON_SEC_MASK,	\
								(ptr_nfi_conf_t->sec_num)<< _SPI_NFI_REGS_CON_SEC_SHIFT );		
	
	/* Enable Customer setting sector size or not */
	if( (ptr_nfi_conf_t->cus_sec_size_en_t) == SPI_NFI_CONF_CUS_SEC_SIZE_Disable )
	{
		_SPI_NFI_REG32_CLRBITS(_SPI_NFI_REGS_SECCUS_SIZE, _SPI_NFI_REGS_SECCUS_SIZE_EN);
	}
	if( (ptr_nfi_conf_t->cus_sec_size_en_t) == SPI_NFI_CONF_CUS_SEC_SIZE_Enable )
	{
		_SPI_NFI_REG32_SETBITS(_SPI_NFI_REGS_SECCUS_SIZE, _SPI_NFI_REGS_SECCUS_SIZE_EN);
	}
	
	/* Set Customer sector size */
	_SPI_NFI_REG32_SETMASKBITS(_SPI_NFI_REGS_SECCUS_SIZE, _SPI_NFI_REGS_SECCUS_SIZE_MASK,	\
								(ptr_nfi_conf_t->sec_size)<< _SPI_NFI_REGS_SECCUS_SIZE_SHIFT );			

	return SPI_NFI_RTN_NO_ERROR;
}

void SPI_NFI_Reset( void )
{
	/* Reset NFI statemachile and flush fifo*/
	_SPI_NFI_REG16_WRITE( _SPI_NFI_REGS_CON, _SPI_NFI_REGS_CON_RESET_VALUE);	
}

SPI_NFI_RTN_T SPI_NFI_Init( void )
{
	/* Enable AHB Done Interrupt Function */
	_SPI_NFI_REG16_SETBITS( _SPI_NFI_REGS_INTR_EN, _SPI_NFI_REGS_INTR_EN_AHB_DONE_EN);

	return SPI_NFI_RTN_NO_ERROR;
}

u32 SPI_NFI_Get_CNFG(void)
{
	return _SPI_NFI_REG32_READ(_SPI_NFI_REGS_CNFG);
}

void SPI_NFI_DEBUG_ENABLE( void )
{	
	_SPI_NFI_DEBUG_FLAG = 1;	
}

void SPI_NFI_DEBUG_DISABLE( void )
{	
	_SPI_NFI_DEBUG_FLAG = 0;	
}

/* Trigger DMA read active*/
void SPI_NFI_TRIGGER(SPI_NFI_CONF_DMA_TRIGGER_T rw)
{
	if(rw == SPI_NFI_CON_DMA_TRIGGER_READ) {
		_SPI_NFI_REG16_CLRBITS( _SPI_NFI_REGS_CON, _SPI_NFI_REGS_CON_RD_TRIG);
		_SPI_NFI_REG16_SETBITS( _SPI_NFI_REGS_CON, _SPI_NFI_REGS_CON_RD_TRIG);
	} else {
		_SPI_NFI_REG16_CLRBITS( _SPI_NFI_REGS_CON, _SPI_NFI_REGS_CON_WR_TRIG);
		_SPI_NFI_REG16_SETBITS( _SPI_NFI_REGS_CON, _SPI_NFI_REGS_CON_WR_TRIG);
	}
}

/* Set DMA(flash -> SRAM) byte swap*/
void SPI_NFI_DMA_WR_BYTE_SWAP(SPI_NFI_CONF_DMA_WR_BYTE_SWAP_T enable)
{
	_SPI_NFI_REG16_SETMASKBITS(_SPI_NFI_REGS_CNFG, _SPI_NFI_REGS_CNFG_DMA_WR_SWAP_MASK, enable << _SPI_NFI_REGS_CNFG_DMA_WR_SWAP_SHIFT);
}

/* Set ECC decode invert*/
void SPI_NFI_ECC_DATA_SOURCE_INV(SPI_NFI_CONF_ECC_DATA_SOURCE_INV_T enable)
{
	_SPI_NFI_REG16_SETMASKBITS(_SPI_NFI_REGS_CNFG, _SPI_NFI_REGS_CNFG_ECC_DATA_SOURCE_INV_MASK, enable << _SPI_NFI_REGS_CNFG_ECC_DATA_SOURCE_INV_SHIFT);
}

SPI_NFI_TYPE_T nfi_type(void)
{
	u32 val;

	if(GET_NP_SCU_EMMC()) {
		return SPI_NFI_EMMC;
	} else {
		val = _SPI_NFI_REG32_READ(_SPI_NFI_REGS_SNF_NFI_CNFG);
		if((val & 0x4) == 0) {
			return SPI_NFI_PARALLEL;
		} else {
			return SPI_NFI_SPI;
		}
	}
}
EXPORT_SYMBOL(nfi_type);

#if defined(TCSUPPORT_PARALLEL_NAND)
SPI_NFI_RTN_T SPI_NFI_START_DMA(SPI_NFI_CONF_DMA_TRIGGER_T rw)
{
	u32				check_cnt = 0;
	SPI_NFI_RTN_T	rtn_status = SPI_NFI_RTN_NO_ERROR;

	/* Trigger DMA active*/
	SPI_NFI_TRIGGER(rw);

	/* Check DMA done or not */
	for(check_cnt = 0; check_cnt < _SPI_NFI_CHECK_DONE_MAX_TIMES; check_cnt++)
	{
		if((_SPI_NFI_REG16_READ(_SPI_NFI_REGS_INTR) & (_SPI_NFI_REGS_INTR_AHB_DONE_CHECK)) != 0)
		{
			break;
		}
	}

	if(check_cnt == _SPI_NFI_CHECK_DONE_MAX_TIMES)
	{		
		_SPI_NFI_PRINTF("[Error] %s DMA : Check AHB Done Timeout ! \n",
				(rw == SPI_NFI_CON_DMA_TRIGGER_WRITE) ? "WRITE" : "READ");
		rtn_status = SPI_NFI_RTN_CHECK_AHB_DONE_TIMEOUT;
	}
	else
	{
		delay1us(1);
	}

	return rtn_status;
}

void PARALLEL_NFI_CLR_CNFG(void)
{
	nfi_reg	reg;

	reg.reg = 0;

	reg.nfi_cnfg.byte_mode = 1;
	reg.nfi_cnfg.read_mode = 1;
	reg.nfi_cnfg.dma_mode = 1;
	_SPI_NFI_REG32_SETMASKBITS(_SPI_NFI_REGS_CNFG, _SPI_NFI_REGS_CONF_OP_MASK, (_SPI_NFI_REGS_CONF_OP_IDEL << _SPI_NFI_REGS_CONF_OP_SHIFT ));
	_SPI_NFI_REG32_CLRBITS(_SPI_NFI_REGS_CNFG, reg.reg);
}


SPI_NFI_RTN_T PARALLEL_NFI_CHECK_INT_STATUS(u16 intr_check)
{
	SPI_NFI_RTN_T status = SPI_NFI_RTN_NO_ERROR;
	u32 timeout = _SPI_NFI_CHECK_DONE_MAX_TIMES;
	nfi_reg	reg;

	reg.reg = _SPI_NFI_REG32_READ(_SPI_NFI_REGS_INTR_EN);
	if (((reg.reg & intr_check) == intr_check))
	{
		reg.reg = 0;
		reg.nfi_cnrnb.timeout = _PARALLEL_NFI_REGS_CNRNB_TIME;
		reg.nfi_cnrnb.chk_trig = 1;

		_SPI_NFI_REG32_WRITE(_PARALLEL_NFI_REGS_CNRNB, reg.reg);

		do
		{
			reg.reg = _SPI_NFI_REG32_READ(_SPI_NFI_REGS_INTR);
			timeout--;
		} while ((timeout && ((reg.reg & intr_check) != intr_check)));

		if ((reg.reg & NFI_INTR_ACCESS_LOCK))
		{
			_SPI_NFI_PRINTF("Access Lock Area: intr_status=0x%x\n", reg.reg);
		}

		if ((reg.reg & NFI_INTR_CB2R_TIMEOUT))
		{
			_SPI_NFI_PRINTF("Check B2R Timeout: intr_status=0x%x\n", reg.reg);
		}

		if (timeout == 0)
		{
			_SPI_NFI_PRINTF("Unexpected Interrupt Loss: intr_status=0x%x\n", reg.reg);
			status = SPI_NFI_RTN_WAIT_TIMEOUT;
		}
	}
	else
	{
		_SPI_NFI_PRINTF("No Interrupt and delay 1ms: inter_en=0x%x intr_check=0x%x\n", reg.reg, intr_check);
		delay1us(1000);
		_SPI_NFI_PRINTF("intr_status=0x%x\n", _SPI_NFI_REG32_READ(_SPI_NFI_REGS_INTR));
	}
	return status;
}

SPI_NFI_RTN_T PARALLEL_NFI_START_DMA(unsigned long *p_data, u8 dirt)
{
	SPI_NFI_RTN_T status = SPI_NFI_RTN_NO_ERROR;
	u32 timeout = _SPI_NFI_CHECK_DONE_MAX_TIMES;
	u32 len = 0, cnt = 0;
	SPI_NFI_CONF_DMA_TRIGGER_T trig = SPI_NFI_CON_DMA_TRIGGER_READ;
	u32 cnt_reg = _PARALLEL_NFI_REGS_RD_CNT;
	u32	cnt_clr = _PARALLEL_NFI_REGS_CNT_CLR_RD;
	SPI_NFI_CONF_T *spi_nfi_conf_info_t = _SPI_NFI_GET_CONF_PTR;

	if (dirt == SPI_NFI_WRITE_DATA)
	{
		trig = SPI_NFI_CON_DMA_TRIGGER_WRITE;
		cnt_reg =_PARALLEL_NFI_REGS_WR_CNT;
		cnt_clr = _PARALLEL_NFI_REGS_CNT_CLR_WR;
	}

	if( (spi_nfi_conf_info_t->cus_sec_size_en_t) == SPI_NFI_CONF_CUS_SEC_SIZE_Disable )
	{
		len = ((_SPI_NFI_DATA_SIZE_WITH_ECC + (spi_nfi_conf_info_t->spare_size_t)) * (spi_nfi_conf_info_t->sec_num));
	}
	if( (spi_nfi_conf_info_t->cus_sec_size_en_t) == SPI_NFI_CONF_CUS_SEC_SIZE_Enable )
	{
		len = ((spi_nfi_conf_info_t->sec_size) * (spi_nfi_conf_info_t->sec_num));
	}	

	_SPI_NFI_REG32_WRITE(_SPI_NFI_REGS_STRADDR, ((u32) p_data));

	_SPI_NFI_REG32_WRITE(_PARALLEL_NFI_REGS_CNT_CLR, cnt_clr);

	status = SPI_NFI_START_DMA(trig);

	if (status == SPI_NFI_RTN_NO_ERROR)
	{
		do
		{
			cnt = _SPI_NFI_REG32_READ(cnt_reg);
			timeout--;
		} while ((timeout && (cnt != len)));
	}

	if (timeout == 0)
	{
		_SPI_NFI_PRINTF("Transmission Loss: len=0x%x, cnt=0x%x\n", len, cnt);
		status = SPI_NFI_RTN_WAIT_TIMEOUT;
	}

	PARALLEL_NFI_CLR_CNFG();

	return status;
}

SPI_NFI_RTN_T PARALLEL_NFI_START_BYTE_READ(u8 len, u8 *p_data)
{
	SPI_NFI_RTN_T status = SPI_NFI_RTN_NO_ERROR;
	u32 timeout = _SPI_NFI_CHECK_DONE_MAX_TIMES;
	nfi_reg	reg;

	if ((len == 0) || (len > 7) || (p_data == NULL))
	{
		return SPI_NFI_RTN_INVAILD_PARAM;
	}

	reg.reg = 0;

	reg.nfi_con.srd = 1;
	reg.nfi_con.nob = len;
	_SPI_NFI_REG32_SETBITS(_SPI_NFI_REGS_CON, reg.reg);

	while ((timeout && len))
	{
		reg.reg = _SPI_NFI_REG32_READ(_PARALLEL_NFI_REGS_PIO_DRDY);
		if ((reg.reg & NFI_PIO_DI_RDY))
		{
			*(p_data) = ((u8) (_SPI_NFI_REG32_READ(_PARALLEL_NFI_REGS_DATAR) & 0xFF));
			len--;
			p_data++;
			timeout = _SPI_NFI_CHECK_DONE_MAX_TIMES;
		}
		else
		{
			timeout--;
		}
	}

	if (timeout == 0)
	{
		_SPI_NFI_PRINTF("NFI PIO isn't ready: pio=0x%x\n", reg.reg);
		status = SPI_NFI_RTN_WAIT_TIMEOUT;
	}

	PARALLEL_NFI_CLR_CNFG();

	return status;
}

SPI_NFI_RTN_T PARALLEL_NFI_ISSUE_CMD_2(u8 cmd2)
{
	u16 intr_check = 0;

	_SPI_NFI_REG32_WRITE(_SPI_NFI_REGS_CMD, cmd2);

	switch (cmd2)
	{
		case NAND_CMD_RESET:
			intr_check = NFI_INTR_RESET_DONE;
			break;
		case NAND_CMD_ERASE2:
			intr_check = NFI_INTR_ERASE_DONE;
			break;
		case NAND_CMD_PAGEPROG:
			intr_check = NFI_INTR_WRITE_DONE;
			break;
		case NAND_CMD_READSTART:
			intr_check = NFI_INTR_BUSY_RETURN;
			break;
		default:
			intr_check = NFI_INTR_BUSY_RETURN;
	}

	return PARALLEL_NFI_CHECK_INT_STATUS(intr_check);
}

SPI_NFI_RTN_T PARALLEL_NFI_ISSUE_ADDR(u32 col_addr, u32 row_addr, u8 col_nob, u8 row_nob)
{
	SPI_NFI_RTN_T status = SPI_NFI_RTN_NO_ERROR;
	u32 timeout = _SPI_NFI_CHECK_DONE_MAX_TIMES;
	nfi_reg	reg;

	reg.reg = col_addr;
	_SPI_NFI_REG32_WRITE(_PARALLEL_NFI_REGS_COLADDR, reg.reg);

	reg.reg = row_addr;
	_SPI_NFI_REG32_WRITE(_PARALLEL_NFI_REGS_ROWADDR, reg.reg);

	reg.nfi_addrnob.col_nob = col_nob;
	reg.nfi_addrnob.row_nob = row_nob;
	_SPI_NFI_REG32_WRITE(_PARALLEL_NFI_REGS_ADDRNOB, reg.reg);

	do
	{
		reg.reg = _SPI_NFI_REG32_READ(_SPI_NFI_REGS_STA);
		timeout--;
	} while ((timeout && (reg.reg & (NFI_STA_CMD_MODE | NFI_STA_ADDR_MODE| NFI_STA_DATAR_MODE| NFI_STA_DATAW_MODE))));

	if (timeout == 0)
	{
		_SPI_NFI_PRINTF("NFI core is busy: sta=0x%x\n", reg.reg);
		status = SPI_NFI_RTN_WAIT_TIMEOUT;
	}

	if (_SPI_NFI_REG32_READ(_PARALLEL_NFI_REGS_LOCK))
	{
		reg.reg = _SPI_NFI_REG32_READ(_SPI_NFI_REGS_INTR);
		if ((reg.reg & NFI_INTR_ACCESS_LOCK))
		{
			_SPI_NFI_PRINTF("Access Lock Area: intr_status=0x%x\n", reg.reg);

			if (!(reg.reg & NFI_INTR_RESET_DONE))
				PARALLEL_NFI_CHECK_INT_STATUS(NFI_INTR_RESET_DONE);

			_SPI_NFI_DEBUG_PRINTF("Access Lock Area: sta=0x%x\n", _SPI_NFI_REG32_READ(_SPI_NFI_REGS_STA));
			_SPI_NFI_DEBUG_PRINTF("Access Lock Area: lock_sta=0x%x\n", _SPI_NFI_REG32_READ(_PARALLEL_NFI_REGS_LOCKSTA));

			return SPI_NFI_RTN_ACCESS_LOCK;
		}
	}

	return status;
}

void PARALLEL_NFI_ISSUE_CMD_1(u8 cmd1)
{
	nfi_reg	reg;
	u8 opmode = _SPI_NFI_REGS_CONF_OP_IDEL;

	reg.reg = 0;
	switch (cmd1)
	{
		case NAND_CMD_READID:
		case NAND_CMD_STATUS:
			opmode = _SPI_NFI_REGS_CONF_OP_SINGLE_READ;
			reg.nfi_cnfg.byte_mode = 1;
			reg.nfi_cnfg.read_mode = 1;
			reg.nfi_cnfg.dma_mode = 0;
			break;
		case NAND_CMD_ERASE1:
			opmode = _SPI_NFI_REGS_CONF_OP_ERASE;
			reg.nfi_cnfg.byte_mode = 0;
			reg.nfi_cnfg.read_mode = 0;
			reg.nfi_cnfg.dma_mode = 0;
			break;
		case NAND_CMD_READ:
			opmode = _SPI_NFI_REGS_CONF_OP_READ;
			reg.nfi_cnfg.byte_mode = 0;
			reg.nfi_cnfg.read_mode = 1;
			reg.nfi_cnfg.dma_mode = 1;
			break;
		case NAND_CMD_SEQIN:
			opmode = _SPI_NFI_REGS_CONF_OP_PRGM;
			reg.nfi_cnfg.byte_mode = 0;
			reg.nfi_cnfg.read_mode = 0;
			reg.nfi_cnfg.dma_mode = 1;
			break;
		default:
			opmode = _SPI_NFI_REGS_CONF_OP_IDEL;
			reg.nfi_cnfg.byte_mode = 0;
			reg.nfi_cnfg.read_mode = 0;
			reg.nfi_cnfg.dma_mode = 0;
	}
	_SPI_NFI_REG32_SETMASKBITS(_SPI_NFI_REGS_CNFG, _SPI_NFI_REGS_CONF_OP_MASK, (opmode << _SPI_NFI_REGS_CONF_OP_SHIFT ));
	_SPI_NFI_REG32_SETBITS(_SPI_NFI_REGS_CNFG, reg.reg);

	_SPI_NFI_REG32_WRITE(_SPI_NFI_REGS_CMD, cmd1);
}

void PARALLEL_NFI_RESET(void)
{
	nfi_reg	reg;

	SPI_NFI_Reset();

	reg.reg = 0;
	reg.nfi_cnfg.hw_ecc = 1;
	reg.nfi_cnfg.auto_fdm = 1;
	_SPI_NFI_REG32_CLRBITS(_SPI_NFI_REGS_CNFG, reg.reg);
}

void PARALLEL_NFI_SET_TIMING(u32 timing)
{
	_SPI_NFI_REG32_WRITE(_PARALLEL_NFI_REGS_ACCCON, timing);
}

void PARALLEL_NFI_SET_CHIP_SELECT(u8 chip)
{
	_SPI_NFI_REG32_WRITE(_PARALLEL_NFI_REGS_CSEL, chip);
}

void PARALLEL_NFI_INIT(void)
{
	PARALLEL_NFI_CLR_CNFG();

	/* Enable All Interrupt Function */
	_SPI_NFI_REG32_WRITE( _SPI_NFI_REGS_INTR_EN, NFI_INTR_ALL);
}
#endif

/*******************For ARM*******************/
#ifdef TCSUPPORT_CPU_ARMV8
static int init_ecnt_spi2nfi(struct platform_device *pdev)
{
	struct resource *res = NULL;
	int ret = 0;
	int irq;
	
	ecnt_spi2nfi_ctrl = devm_kzalloc(&pdev->dev, sizeof(struct ecnt_spi2nfi_ctrl), GFP_KERNEL);
	if (!ecnt_spi2nfi_ctrl)
		return -ENOMEM;
	platform_set_drvdata(pdev, ecnt_spi2nfi_ctrl);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ecnt_spi2nfi_ctrl->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(ecnt_spi2nfi_ctrl->base))
		return PTR_ERR(ecnt_spi2nfi_ctrl->base);

	ecnt_spi2nfi_ctrl->dev = &pdev->dev;

	return ret;
}

int init_spi_nfi2spi_base(struct platform_device *pdev)
{
	struct device_node *np = NULL;
	struct platform_device *pdev_spi2nfi = NULL;
	int ret = 0;
	
	if(isInit == 0) {
		np = of_parse_phandle(pdev->dev.of_node, "spi2nfi", 0);
		if (np) {
			pdev_spi2nfi = of_find_device_by_node(np);
			ret = init_ecnt_spi2nfi(pdev_spi2nfi);
			if(ret == 0) {
				isInit = 1;
			}
			of_node_put(np);
		} else {
			return -1;
		}
	}

	return ret;
}
#endif

