/*
 * Copyright (c) 2014-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLATFORM_DEF_H
#define PLATFORM_DEF_H

#include <common/tbbr/tbbr_img_def.h>
#include <lib/utils_def.h>
#include <plat/common/common_def.h>

#include "en7523_def.h"

/*******************************************************************************
 * Platform binary types for linking
 ******************************************************************************/
//#define PLATFORM_LINKER_FORMAT		"elf64-littleaarch64"
//#define PLATFORM_LINKER_ARCH		aarch64

/*******************************************************************************
 * Generic platform constants
 ******************************************************************************/

/* Size of cacheable stacks */
#if defined(IMAGE_BL1)
#define PLATFORM_STACK_SIZE 0x440
#elif defined(IMAGE_BL2)
#define PLATFORM_STACK_SIZE 0x1000
#elif defined(IMAGE_BL31)
#define PLATFORM_STACK_SIZE 0x1000
#elif defined(IMAGE_BL32)
#define PLATFORM_STACK_SIZE 0x1000
#endif

#define FIRMWARE_WELCOME_STR		"Booting Trusted Firmware\n"

#define PLATFORM_MAX_AFFLVL		MPIDR_AFFLVL2
#define PLAT_MAX_PWR_LVL		U(2)
#define PLAT_MAX_RET_STATE		U(1)
#define PLAT_MAX_OFF_STATE		U(2)
#define PLATFORM_SYSTEM_COUNT		1
#define PLATFORM_CLUSTER_COUNT		1
#if defined(TCSUPPORT_CPU_EN7581) && !defined(TCSUPPORT_CPU_AN7583)
#define PLATFORM_CLUSTER0_CORE_COUNT	4
#else
#define PLATFORM_CLUSTER0_CORE_COUNT	2
#endif
#define PLATFORM_CORE_COUNT		(PLATFORM_CLUSTER0_CORE_COUNT)
#define PLATFORM_MAX_CPUS_PER_CLUSTER	(PLATFORM_CLUSTER0_CORE_COUNT)
#define PLATFORM_NUM_AFFS		(PLATFORM_SYSTEM_COUNT +	\
					 PLATFORM_CLUSTER_COUNT +	\
					 PLATFORM_CORE_COUNT)


/*
 * EN7523 TZRAM memory layout
 * 0x80000000 +-------------------+
 *            | reserved    (8KB) |
 * 0x80002000 +-------------------+
 *            | shared mem  (4KB) |
 * 0x80003000 +-------------------+
 *            |                   |
 *            |   BL3-1   (200KB) |
 *            |                   |
 * 0x80035000 +-------------------+
 *            | reserved   (44KB) |
 * 0x80040000 +-------------------+
 */

#define TZRAM_BASE_OFFSET	UL(0x2000)
#if defined(TCSUPPORT_CPU_AN7583)
#define TZRAM_TOTAL_SZIE	UL(0x50000)
#else
#define TZRAM_TOTAL_SZIE	UL(0x40000)
#endif
#if !defined(TCSUPPORT_TPL_SUPPORT) || defined(IMAGE_BL31)
#define TZRAM_BASE			(EN7523_MEM_BASE + TZRAM_BASE_OFFSET)
#define TZRAM_SIZE			(TZRAM_TOTAL_SZIE - TZRAM_BASE_OFFSET - TZRAM2_SIZE)
#else
#if defined(TCSUPPORT_TPL_ENC)
#define TZRAM_BASE			(0x8a900000)
#define TZRAM_SIZE			(0x23000)
#else
#define TZRAM_BASE			(0x1f000000 + 0x00000)
#define TZRAM_SIZE			(0x17000)
#endif
#endif


#if defined(TCSUPPORT_TPL_ENC)
#define FE_SRAM_END         (0x1f020000)
#define SSK_BASE            (FE_SRAM_END - 0x400) // reserve last 1K 
#endif

/* Reserved: 64KB */
#define TZRAM2_BASE			(TZRAM_BASE + TZRAM_SIZE)
#define TZRAM2_SIZE			UL(0xb000)
//#define TZRAM2_SIZE			UL(0x10000)

/*******************************************************************************
 * BL1 specific defines.
 ******************************************************************************/

#if !defined(MT7510_EN7512_FPGA_STAGE)
#ifdef TCSUPPORT_BL2_OPTIMIZATION
#define BL_SRAM_BASE	(ECNT_NPU_SRAM_BASE) + (ECNT_L2_SRAM_SIZE/2)
#define BL_SRAM_SIZE	(ECNT_L2_SRAM_SIZE/2)
#else
#define BL_SRAM_BASE	(ECNT_NPU_SRAM_BASE + ECNT_L2_SRAM_SIZE)
#define BL_SRAM_SIZE	(ECNT_L2_SRAM_SIZE)
#endif
#else
#define BL_SRAM_BASE	0x1F002000
#define BL_SRAM_SIZE	0x1E000
#endif




#define MAX_IO_HANDLES			3
#define MAX_IO_DEVICES			3

#define BL1_RO_BASE		(0x0)
#define BL1_RO_SIZE		(0x1C000)
#define BL1_RO_LIMIT	(BL1_RO_BASE + BL1_RO_SIZE)
/*
 * Put BL1 RW at the top of the memory allocated for BL images in NS DRAM.
 */
#define BL1_RW_BASE			(BL_SRAM_BASE)
#define BL1_RW_LIMIT		(BL_SRAM_BASE + BL_SRAM_SIZE)

#define BL1_RW2_BASE		(ECNT_GDMP_SRAM_BASE)
#define BL1_RW2_SIZE		(ECNT_GDMP_SRAM_SIZE)

/*******************************************************************************
 * BL2 specific defines.
 ******************************************************************************/
#ifdef IMAGE_BL21
#define BL2_BASE			(ECNT_L2_SRAM_BASE)
#elif IMAGE_BL22
#define BL2_BASE				(ECNT_BL22_BASE)
#elif IMAGE_BL23
#define BL2_BASE				(ECNT_BL23_BASE)
#else
#define BL2_BASE			(ECNT_L2_SRAM_BASE)
#endif
#define BL2_SIZE			(ECNT_L2_SRAM_SIZE)
#define BL2_LIMIT			(BL2_BASE + BL2_SIZE)
#define BL2_RO_BASE			(BL2_BASE)
#define BL2_RO_SIZE			(BL2_SIZE)
#define BL2_RO_LIMIT		(BL2_LIMIT)
#define BL2_RW_BASE			(BL_SRAM_BASE)
#define BL2_RW_LIMIT		(BL_SRAM_BASE + BL_SRAM_SIZE)


#define BL2_RW2_BASE		(ECNT_GDMP_SRAM_BASE)
#define BL2_RW2_SIZE		(ECNT_GDMP_SRAM_SIZE)

/*******************************************************************************
 * BL31 specific defines.
 ******************************************************************************/
/*
 * Put BL3-1 at the top of the Trusted SRAM (just below the shared memory, if
 * present). BL31_BASE is calculated using the current BL3-1 debug size plus a
 * little space for growth.
 */
#define BL31_BASE			(TZRAM_BASE + UL(0x1000))
#define BL31_LIMIT			(TZRAM_BASE + TZRAM_SIZE)
//#define BL31_LIMIT			(TZRAM_BASE + TZRAM_SIZE + UL(0x1000))
#define TZRAM2_LIMIT		(TZRAM2_BASE + TZRAM2_SIZE)

#define	BL33_BASE			(EN7523_MEM_BASE + UL(0x1E00000))
#define BL33_LIMIT			(BL33_BASE + UL(0x100000))

#ifdef TCSUPPORT_OPTEE
//#define BL32_BASE           (BL31_LIMIT + UL(0x1000))
//#define BL32_LIMIT          (BL32_BASE + UL(0x6c400))// 433k
#define BL32_BASE			(EN7523_MEM_BASE + UL(0xa800000))
#define BL32_LIMIT			(BL32_BASE + UL(0x800000)) // be aware of TA section
#endif

/* FIP placed after ROM to append it to BL1 with very little padding. */

#define PLAT_ECNT_MULTI_BOOT_SIZE			(0x100000)
#define PLAT_ECNT_FIP_OFFSET				(0x800)
#define PLAT_ECNT_BL31_FIP_OFFSET	(0x20000)
#if defined(IMAGE_BL1)
#define PLAT_ECNT_FIP_BASE		(BL_SRAM_BASE + BL_SRAM_SIZE)
#define PLAT_ECNT_FIP_MAX_SIZE	(0x1F800)
#else
#define PLAT_ECNT_FIP_BASE		(EN7523_MEM_BASE + UL(0x1800000))
#define PLAT_ECNT_MV_DATA_SIZE	UL(0x800)
#if defined(TCSUPPORT_TPL_SUPPORT)
#define PLAT_ECNT_FIP_MAX_SIZE	UL(0x1ff800)
#else
#ifdef TCSUPPORT_TCBOOT_1MB_SIZE
#define PLAT_ECNT_FIP_MAX_SIZE	UL(0x100000)//original:7f800 
#else
#define PLAT_ECNT_FIP_MAX_SIZE	UL(0x7f800)
#endif
#endif

#if defined(IMAGE_BL21)
#define EN7523_IMAGE_BUF_OFFSET		ECNT_NPU_SRAM_BASE
#define EN7523_IMAGE_BUF_SIZE		(ECNT_L2_SRAM_SIZE/2)
#else
#ifdef TCSUPPORT_TCBOOT_1MB_SIZE
#define EN7523_IMAGE_BUF_OFFSET		(PLAT_ECNT_FIP_BASE + UL(0x100000))
#else
#define EN7523_IMAGE_BUF_OFFSET		(PLAT_ECNT_FIP_BASE + UL(0x80000))
#endif
#define EN7523_IMAGE_BUF_SIZE		UL(0x58000)
#endif
#endif

#ifdef TCSUPPORT_BL2_OPTIMIZATION
#ifdef IMAGE_BL21
#define VPint							*(volatile unsigned int *)
#endif
#if !defined(__ASSEMBLER__)
typedef struct bl2_optimize_header_t{
	unsigned int bl22_length;
	unsigned int bl23_length;
	unsigned int flash_table_length;
	unsigned int lzma_src;
	unsigned int lzma_des;
	unsigned int lzma_length;
	unsigned int lzma_cmd;
	unsigned int fw_ver;
	unsigned int reserved;
}Bl2_optimize_header_t;
#endif
#endif

/*******************************************************************************
 * Platform specific page table and MMU setup constants
 ******************************************************************************/
#define PLAT_PHY_ADDR_SPACE_SIZE	(1ULL << 32)
#define PLAT_VIRT_ADDR_SPACE_SIZE	(1ULL << 32)
#define MAX_XLAT_TABLES		7
#define MAX_MMAP_REGIONS	16

/*******************************************************************************
 * Declarations and constants to access the mailboxes safely. Each mailbox is
 * aligned on the biggest cache line size in the platform. This is known only
 * to the platform as it might have a combination of integrated and external
 * caches. Such alignment ensures that two maiboxes do not sit on the same cache
 * line at any cache level. They could belong to different cpus/clusters &
 * get written while being protected by different locks causing corruption of
 * a valid mailbox address.
 ******************************************************************************/
#define CACHE_WRITEBACK_SHIFT	6
#define CACHE_WRITEBACK_GRANULE	(U(1) << CACHE_WRITEBACK_SHIFT)

#define PLAT_ARM_GICD_BASE      BASE_GICD_BASE
#define PLAT_ARM_GICC_BASE      BASE_GICC_BASE
#define PLAT_ARM_GICR_BASE      BASE_GICR_BASE

#define PLAT_ARM_G1S_IRQ_PROPS(grp) \
	INTR_PROP_DESC(ECNT_IRQ_SEC_SGI_0, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(ECNT_IRQ_SEC_SGI_1, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(ECNT_IRQ_SEC_SGI_2, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(ECNT_IRQ_SEC_SGI_3, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(ECNT_IRQ_SEC_SGI_4, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(ECNT_IRQ_SEC_SGI_5, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(ECNT_IRQ_SEC_SGI_6, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(ECNT_IRQ_SEC_SGI_7, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_EDGE)

#define PLAT_ARM_G0_IRQ_PROPS(grp)

#endif /* PLATFORM_DEF_H */
