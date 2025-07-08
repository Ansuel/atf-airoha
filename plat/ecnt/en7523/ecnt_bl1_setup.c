/*
 * Copyright (c) 2015-2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <assert.h>

#include <platform_def.h>

#include <arch.h>
#include <arch_helpers.h>
#include <drivers/generic_delay_timer.h>
#include <drivers/console.h>
#include <lib/mmio.h>
#include <tools_share/firmware_image_package.h>

#include <plat/common/platform.h>
#include <plat_private.h>
#include <flashhal.h>
#include <xmodem.h>

/* Data structure which holds the extents of the trusted SRAM for BL1*/
static meminfo_t bl1_tzram_layout;
static console_t console;
static hw_trap_t hw_trap;
static int debug_flag = 0;

extern int console_ecnt_register(uintptr_t baseaddr, console_t *console);
extern int XModemReceive(console_t *console, unsigned int bufLen , unsigned char *bufBase);
extern void	get_bootimage_by_npu_iNIC(unsigned int imgDstAddr);
extern void disable_NPU_dbgMsg(void);

void hw_trap_init(void)
{
	int mode = 0;

	if (debug_flag)
	{
		mode = mmio_read_32(EN7523_SCREG_WF0);

		switch (mode)
		{
			case DBG_INIC_MODE:
			{
				hw_trap.inc_mode = 1;
				hw_trap.fw_upgrade_mode = 0;
				hw_trap.skip_fw_upgrade = 0;
				NOTICE("DBG_INIC_MODE\n");
				break;
			}
			case DBG_FWU_MODE:
			{
				hw_trap.inc_mode = 0;
				hw_trap.fw_upgrade_mode = 1;
				hw_trap.skip_fw_upgrade = 0;
				NOTICE("DBG_FWU_MODE\n");
				break;
			}
			case DBG_FLASH_MODE:
			{
				hw_trap.inc_mode = 0;
				hw_trap.fw_upgrade_mode = 0;
				hw_trap.skip_fw_upgrade = 0;
				NOTICE("DBG_FLASH_MODE\n");
				break;
			}
			default:
				NOTICE("Unknow Debug mode\n");
		}
	}
	else
	{
		hw_trap.skip_fw_upgrade	= !(mmio_read_32(EN7523_SEC_SSR) & BOOT_SEL_BY_HWTRAP);
		hw_trap.fw_upgrade_mode	= !(mmio_read_32(EN7523_HWTRAP_CONF) & HWTRAP_FW_UPGRADE);
		hw_trap.inc_mode 		= ((mmio_read_32(EN7523_HWTRAP_CONF) & 0x7) == HWTRAP_INIC_MODE);
	}

	hw_trap.hw_trap_decode	= (mmio_read_32(EN7523_HWTRAP_DEC) & 0x2f);
}

void debug_init(void)
{
	if (((mmio_read_32(EN7523_HWTRAP_CONF) & 0x7) == 0)	||
		((mmio_read_32(EN7523_HWTRAP_CONF) & 0x7) == 1) ||
		((mmio_read_32(EN7523_HWTRAP_CONF) & 0x7) == 3))
	{
		if (mmio_read_32(EN7523_SCREG_WF1) == DEBUG_MAGIC)
		{
			debug_flag = 1;
			tf_log_set_max_level(LOG_LEVEL_NOTICE);
		}
	}
	else
	{
		tf_log_set_max_level(LOG_LEVEL_ERROR);
	}
}

void __dead2 plat_error_handler(int err)
{
	mmio_write_32(EN7523_SCREG_WF1, err);
	while (1)
		wfi();
}

meminfo_t *bl1_plat_sec_mem_layout(void)
{
	return &bl1_tzram_layout;
}

/*******************************************************************************
 * Perform any BL1 specific platform actions.
 ******************************************************************************/
void bl1_early_platform_setup(void)
{
	/* Initialize the console to provide early debug support */
	console_ecnt_register(EN7523_UART0_BASE, &console);
	debug_init();
	efuse_init();
	if ((debug_flag == 0) && is_asic())
	{
		console_switch_state(CONSOLE_FLAG_RUNTIME);
		tf_log_set_max_level(LOG_LEVEL_NONE);
		disable_NPU_dbgMsg();
	}

	NOTICE("bl1_setup\n");

	/* Allow BL1 to see the whole Trusted RAM */
	bl1_tzram_layout.total_base = BL_SRAM_BASE;
	bl1_tzram_layout.total_size = BL_SRAM_SIZE;

	NOTICE("BL1: 0x%x - 0x%x [size = %zu]\n", BL_SRAM_BASE,
		(BL_SRAM_BASE + BL_SRAM_SIZE),
		BL_SRAM_SIZE);
	NOTICE("BL1: 0x%lx - 0x%lx [size = %lu]\n", BL_CODE_BASE,
		BL_CODE_END,
		(BL_CODE_END - BL_CODE_BASE));
}

/******************************************************************************
 * Perform the very early platform specific architecture setup.  This only
 * does basic initialization. Later architectural setup (bl1_arch_setup())
 * does not do anything platform specific.
 *****************************************************************************/

void bl1_plat_arch_setup(void)
{
    plat_configure_mmu_svc_mon(bl1_tzram_layout.total_base,
                   bl1_tzram_layout.total_size,
                   BL_CODE_BASE,
                   BL1_CODE_END,
                   BL_COHERENT_RAM_BASE,
                   BL_COHERENT_RAM_END);
}

void bl1_platform_setup(void)
{
	write_cntfrq_el0(plat_get_syscnt_freq2());
	generic_delay_timer_init();
	hw_trap_init();
	flash_init();
	plat_ecnt_io_setup();
}

int bl1_plat_handle_post_image_load(unsigned int image_id)
{
	__attribute__((noreturn)) void (*bl2)(void);

	bl2 = (void *)BL2_BASE;
	inv_dcache_range(BL1_RW2_BASE, BL1_RW2_SIZE);
	disable_mmu_icache_secure();
	(*bl2)();

	return 0;
}

int bl1_plat_handle_pre_image_load(unsigned int image_id)
{
	int len = 0;

	if (hw_trap.inc_mode)
	{
		NOTICE("3-1\n");
		get_bootimage_by_npu_iNIC((unsigned int) PLAT_ECNT_FIP_BASE);
	}
	else if (hw_trap.fw_upgrade_mode && !(hw_trap.skip_fw_upgrade))
	{
		NOTICE("3-2->3-3\n");
		if (flash_read(PLAT_ECNT_FIP_OFFSET, PLAT_ECNT_FIP_MAX_SIZE, (uint8_t *) PLAT_ECNT_FIP_BASE) != FLASH_READ_STATUS_CORRECT)
		{
			panic();
		}

		if (plat_check_bypass() != BYPASS_FWUPGRADE)
		{
			NOTICE("3-6\n");
			while (len == 0)
			{
				if (console.getc(&console) == 'x')
				{
					len = XModemReceive(&console, PLAT_ECNT_FIP_MAX_SIZE, (uint8_t *) PLAT_ECNT_FIP_BASE);
				}
			}
		}
		else
		{
			NOTICE("3-5\n");
		}
	}
	else
	{
		NOTICE("3-2->3-4\n");
		if (flash_read(PLAT_ECNT_FIP_OFFSET, PLAT_ECNT_FIP_MAX_SIZE, (uint8_t *) PLAT_ECNT_FIP_BASE) == FLASH_READ_STATUS_CORRECT)
		{
			NOTICE("3-4-1\n");
		}
		else
		{
			NOTICE("3-4-2\n");
			panic();
		}
	}

	while (plat_check_header((uint8_t *) PLAT_ECNT_FIP_BASE) == 0)
	{
		NOTICE("3-8\n");
		if (get_into_inic())
		{
			NOTICE("3-1-1\n");
			get_bootimage_by_npu_iNIC((unsigned int) PLAT_ECNT_FIP_BASE);
		}
		else
		{
			panic();
		}
	}

	NOTICE("3-7\n");

	return 0;
}

struct image_desc *bl1_plat_get_image_desc(unsigned int image_id)
{
	static image_desc_t bl2_img_desc = BL2_IMAGE_DESC;

	return &bl2_img_desc;
}
