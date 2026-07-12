/*
 * Copyright (c) 2013-2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform_def.h>

#include <arch_helpers.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <lib/utils.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <lib/mmio.h>

#include <plat_private.h>
#include <en7523_def.h>

extern uint32_t uartDisable;
/* Table of regions to map using the MMU.  */
const mmap_region_t plat_mmap[] = {
	MAP_REGION_FLAT(ECNT_DEV_IO_BASE, ECNT_DEV_IO_SIZE,
			MT_DEVICE | MT_RW | MT_SECURE),
	{ 0 }
};

/*******************************************************************************
 * Macro generating the code for the function setting up the pagetables as per
 * the platform memory map & initialize the mmu, for the given exception level
 ******************************************************************************/
#if defined(IMAGE_BL1) || defined(IMAGE_BL2)
#define DEFINE_CONFIGURE_MMU_EL(_el)						\
	void plat_configure_mmu_##_el(unsigned long total_base,	\
					  unsigned long total_size,				\
					  unsigned long ro_start,				\
					  unsigned long ro_limit,				\
					  unsigned long coh_start,				\
					  unsigned long coh_limit)				\
	{														\
		mmap_add_region(total_base, total_base,				\
				total_size,									\
				MT_DEVICE | MT_RW | MT_SECURE);				\
		mmap_add_region(ro_start, ro_start,					\
				ro_limit - ro_start,						\
				MT_MEMORY | MT_RO | MT_SECURE);				\
		mmap_add_region(coh_start, coh_start,				\
				coh_limit - coh_start,						\
				MT_DEVICE | MT_RW | MT_SECURE);				\
		mmap_add(plat_mmap);								\
		init_xlat_tables();									\
		enable_mmu_##_el(0);								\
	}
#else
#define DEFINE_CONFIGURE_MMU_EL(_el)						\
	void plat_configure_mmu_##_el(unsigned long total_base,	\
					  unsigned long total_size,				\
					  unsigned long ro_start,				\
					  unsigned long ro_limit,				\
					  unsigned long coh_start,				\
					  unsigned long coh_limit)				\
	{														\
		mmap_add_region(total_base, total_base,				\
				total_size,									\
				MT_MEMORY | MT_RW | MT_SECURE);				\
		mmap_add_region(ro_start, ro_start,					\
				ro_limit - ro_start,						\
				MT_MEMORY | MT_RO | MT_SECURE);				\
		mmap_add_region(coh_start, coh_start,				\
				coh_limit - coh_start,						\
				MT_DEVICE | MT_RW | MT_SECURE);				\
		mmap_add(plat_mmap);								\
		init_xlat_tables();									\
		enable_mmu_##_el(0);								\
	}
#endif

/* Define EL3 variants of the function initialising the MMU */
#ifdef AARCH32
DEFINE_CONFIGURE_MMU_EL(svc_mon)
#else
DEFINE_CONFIGURE_MMU_EL(el3)
//DEFINE_CONFIGURE_MMU_EL(svc_mon)
#endif


unsigned int plat_get_syscnt_freq2(void)
{
#if defined(TCSUPPORT_CPU_EN7581) || defined(TCSUPPORT_CPU_AN7583) || defined(TCSUPPORT_CPU_AN7552)
	/* 7581 CPU timer clk fixed in 25M*/
	return SYS_COUNTER_FREQ_IN_TICKS_25M;
#else
	unsigned int freq = SYS_COUNTER_FREQ_IN_TICKS_25M;

#ifndef IMAGE_BL21
	if (get_freq_by_efuse())
	{
		NOTICE("freq select by efuse\n");
		if (get_freq_sel())
		{
			NOTICE("freq is 20M\n");
			freq = SYS_COUNTER_FREQ_IN_TICKS_20M;
		}
	}
	else
	{
		NOTICE("freq select by hwtrap\n");
		if (!(mmio_read_32(EN7523_ECC_SEL) & (1 << 19)))
		{
			NOTICE("freq is 20M\n");
			freq = SYS_COUNTER_FREQ_IN_TICKS_20M;
		}
	}
#else
	freq = SYS_COUNTER_FREQ_IN_TICKS_20M;
#endif

	return freq;
#endif
}


unsigned int is_asic(void)
{
	if (get_asic_mode_by_efuse())
	{
		INFO("ASIC Mode by efuse\n");
		return 1;
	}
	else
	{
		INFO("ASIC Mode by register\n");
		return ((mmio_read_32(EN7523_SSTR_REG) & IS_ASIC) == IS_ASIC);
	}
}
