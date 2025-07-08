/*
 * Copyright (c) 2015-2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <assert.h>
#include <stdio.h>

#include <platform_def.h>

#include <arch.h>
#include <arch_helpers.h>
#include <drivers/generic_delay_timer.h>
#include <drivers/console.h>
#include <lib/mmio.h>
#include <tools_share/firmware_image_package.h>
#include <common/desc_image_load.h>
#include <common/image_decompress.h>

#include <lib/xlat_tables/xlat_tables_v2.h>

#include <LzmaTools.h>

#include <plat/common/platform.h>
#include <plat_private.h>
#include <flashhal.h>
#include <xmodem.h>
#include <ecnt_scu.h>

/* Data structure which holds the extents of the trusted SRAM for BL1*/
static meminfo_t bl2_el3_tzram_layout;
static console_t console;
static hw_trap_t hw_trap;
static int debug_flag = 0;

unsigned int bl31_base_addr = BL31_BASE;
unsigned int rst_vector_base_addr = RVBADDRESS_CPU0;

extern int console_ecnt_register(uintptr_t baseaddr, console_t *console);
extern int XModemReceive(console_t *console, unsigned int bufLen , unsigned char *bufBase);
extern void get_bootimage_by_npu_iNIC(unsigned int imgDstAddr);
extern void disable_NPU_dbgMsg(void);
extern void phy_config_efuse_load(void);
extern void ef_read_parse(unsigned int start, unsigned int len, unsigned char *data);

#ifdef CPU_BUS_BL2_TEST
extern void tests_on_l2c_sram(void);
#endif

void hw_trap_init(void)
{
	int mode = 0;
	uint32_t hwtrap_cfg = (mmio_read_32(EN7523_HWTRAP_CONF) & HWTRAP_MASK);

	memset(&hw_trap, 0, sizeof(hw_trap));

	if (debug_flag)
	{
		mode = mmio_read_32(EN7523_SCREG_WF0);

		switch (mode)
		{
#if (!defined(TCSUPPORT_CPU_AN7552) && !defined(TCSUPPORT_CPU_AN7583))
			case DBG_INIC_MODE:
			{
				hw_trap.inc_mode = 1;
#if defined(TCSUPPORT_CPU_EN7581) || defined(TCSUPPORT_CPU_AN7583)
				hw_trap.inc_mdio_mode = 0;
#endif
				hw_trap.fw_upgrade_mode = 0;
				hw_trap.skip_fw_upgrade = 0;
				NOTICE("DBG_INIC_MODE\n");
				break;
			}
#if defined(TCSUPPORT_CPU_EN7581) || defined(TCSUPPORT_CPU_AN7583)
			case DBG_INIC_MDIO_MODE:
			{
				hw_trap.inc_mode = 0;
				hw_trap.inc_mdio_mode = 1;
				hw_trap.fw_upgrade_mode = 0;
				hw_trap.skip_fw_upgrade = 0;
				NOTICE("DBG_INIC_MDIO_MODE\n");
				break;
			}
#endif
#endif
			case DBG_FWU_MODE:
			{
				hw_trap.inc_mode = 0;
#if defined(TCSUPPORT_CPU_EN7581) || defined(TCSUPPORT_CPU_AN7583)
				hw_trap.inc_mdio_mode = 0;
#endif
				hw_trap.fw_upgrade_mode = 1;
				hw_trap.skip_fw_upgrade = 0;
				NOTICE("DBG_FWU_MODE\n");
				break;
			}
			case DBG_FLASH_MODE:
			{
				hw_trap.inc_mode = 0;
#if defined(TCSUPPORT_CPU_EN7581) || defined(TCSUPPORT_CPU_AN7583)
				hw_trap.inc_mdio_mode = 0;
#endif
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
#if (!defined(TCSUPPORT_CPU_AN7552) && !defined(TCSUPPORT_CPU_AN7583))
		hw_trap.inc_mode 		= (hwtrap_cfg == HWTRAP_INIC_MODE);
#if defined(TCSUPPORT_CPU_EN7581) || defined(TCSUPPORT_CPU_AN7583)
		hw_trap.inc_mdio_mode	= (hwtrap_cfg == HWTRAP_INIC_MDIO_MODE);
#endif
#endif
	}

	SET_IS_SPI_CONTROLLER_ECC(0);

#if defined(TCSUPPORT_CPU_EN7581) || defined(TCSUPPORT_CPU_AN7583) || defined(TCSUPPORT_CPU_AN7552)
#if defined(TCSUPPORT_CPU_EN7581) || defined(TCSUPPORT_CPU_AN7583)
	if((hwtrap_cfg == HWTRAP_EMMC_MODE) ||
	   (hwtrap_cfg == HWTRAP_EMMC_MODE2)) {
		hw_trap.is_emmc = 1;
	}
#endif
	if((hwtrap_cfg == HWTRAP_ROM_3BNOR_MODE) ||
	   (hwtrap_cfg == HWTRAP_ROM_4BNOR_MODE) ||
	   (hwtrap_cfg == HWTRAP_FLASH_3BNOR_MODE) ||
	   (hwtrap_cfg == HWTRAP_FLASH_3BNOR_MODE2) ||
	   (hwtrap_cfg == HWTRAP_FLASH_4BNOR_MODE) ||
	   (hwtrap_cfg == HWTRAP_FLASH_4BNOR_MODE2)) {
		hw_trap.is_spi_nor = 1;
	}
	if((hwtrap_cfg == HWTRAP_FLASH_NAND_MODE) ||
	   (hwtrap_cfg == HWTRAP_FLASH_NAND_MODE2) ||
	   (hwtrap_cfg == HWTRAP_ROM_NAND_MODE) ||
	   (hwtrap_cfg == HWTRAP_ROM_NAND_MODE2)) {
		hw_trap.is_spi_nand_device_ecc = 1;
	}
	if((hwtrap_cfg == HWTRAP_PARRALLEL_NAND_MODE)) {
		hw_trap.is_parallel_nand = 1;
	}
	if(hwtrap_cfg == HWTRAP_ROM_NAND_CONTROLLER_ECC_MODE){
		hw_trap.is_spi_nand_ctrl_ecc = 1;
		SET_IS_SPI_CONTROLLER_ECC(1);
	}
#endif

	hw_trap.hw_trap_decode	= (mmio_read_32(EN7523_HWTRAP_DEC) & HWTRAP_DEC_MASK);
}

void debug_init(void)
{
	tf_log_set_max_level(LOG_LEVEL_NOTICE);
}

void __dead2 plat_error_handler(int err)
{
	mmio_write_32(EN7523_SCREG_WF0, err);
	while (1)
		wfi();
}

meminfo_t *bl2_plat_sec_mem_layout(void)
{
	return &bl2_el3_tzram_layout;
}

/*******************************************************************************
 * Perform any BL2 specific platform actions.
 ******************************************************************************/
void bl2_el3_early_platform_setup(u_register_t arg1, u_register_t arg2,
			u_register_t arg3, u_register_t arg4)
{
	/* Initialize the console to provide early debug support */
	console_ecnt_register(EN7523_UART0_BASE, &console);
#ifdef IMAGE_BL21

	unsigned int i = 0;
	unsigned int bl2_opt = VPint(BL2_OPTIMIZE_STATUS);

	/* need to copy whole bl2 to NPU SRAM*/
	if(bl2_opt != 0x12345678)
	{
		for (i = 0 ; i < ECNT_L2_SRAM_SIZE;i+=4)
		{
			VPint(BL2_OPTIMIZE_BASE+i) = VPint(ECNT_L2_SRAM_BASE+i);
		}
	}
	dsb();
	VPint(BL2_OPTIMIZE_STATUS) = 0x12345678;
#endif

	debug_init();

#ifndef IMAGE_BL21
	efuse_init();
	phy_config_efuse_load();
#endif

	/*
	 * Allow BL2 to see the whole Trusted RAM.
	 */
	bl2_el3_tzram_layout.total_base = BL_SRAM_BASE;
	bl2_el3_tzram_layout.total_size = BL_SRAM_SIZE;
}

/******************************************************************************
 * Perform the very early platform specific architecture setup.  This only
 * does basic initialization. Later architectural setup (bl1_arch_setup())
 * does not do anything platform specific.
 *****************************************************************************/
#ifdef TCSUPPORT_BL2_OPTIMIZATION
void bl2_el3_plat_arch_setup_optimize(void)
{

#if defined(IMAGE_BL21) || defined(IMAGE_BL23)
	write_cntfrq_el0(plat_get_syscnt_freq2());
	generic_delay_timer_init();
#endif

#ifdef IMAGE_BL22
	unsigned long long dram_size = 0;
	write_cntfrq_el0(plat_get_syscnt_freq2());
	generic_delay_timer_init();
	ecnt_system_init(&dram_size);

#ifdef CPU_BUS_BL2_TEST
	tests_on_l2c_sram();
#endif

	mmap_add_region(EN7523_MEM_BASE, EN7523_MEM_BASE, dram_size, MT_DEVICE | MT_RW | MT_SECURE);
	plat_configure_mmu_svc_mon(bl2_el3_tzram_layout.total_base,
				   bl2_el3_tzram_layout.total_size,
				   BL_CODE_BASE,
				   BL2_CODE_END,
				   BL_COHERENT_RAM_BASE,
				   BL_COHERENT_RAM_END);
#endif
}
#endif

void bl2_el3_plat_arch_setup(void)
{
#ifdef TCSUPPORT_BL2_OPTIMIZATION
	bl2_el3_plat_arch_setup_optimize();
#else
	unsigned long long dram_size = 0;

	write_cntfrq_el0(plat_get_syscnt_freq2());
	generic_delay_timer_init();
	ecnt_system_init(&dram_size);

#ifdef CPU_BUS_BL2_TEST
    tests_on_l2c_sram();
#endif

	mmap_add_region(EN7523_MEM_BASE, EN7523_MEM_BASE, dram_size, MT_DEVICE | MT_RW | MT_SECURE);
    	plat_configure_mmu_svc_mon(bl2_el3_tzram_layout.total_base,
                		   bl2_el3_tzram_layout.total_size,
                		   BL_CODE_BASE,
                		   BL2_CODE_END,
                		   BL_COHERENT_RAM_BASE,
                		   BL_COHERENT_RAM_END);

#endif

}

void bl2_el3_plat_prepare_exit(void)
{
	jumparch64(BL33_BASE, 0, 0, TZRAM_BASE);
	panic();
}

#ifdef TCSUPPORT_BL2_OPTIMIZATION
void bl2_plat_preload_setup_optimize(void)
{

#ifdef IMAGE_BL21
		struct image_info info = {0};
		Bl2_optimize_header_t *f_header = NULL;
		f_header = (Bl2_optimize_header_t*)((unsigned char*)BL2_OPTIMIZE_HEADER_BASE);

		/* parsing decompress info*/
		info.image_base = f_header->lzma_des;
		info.image_max_size = 0x19000;
		info.image_size = f_header->lzma_length;

#ifdef TCSUPPORT_CPU_AN7552
		if ((f_header->lzma_src & 0xffff0000) == 0x1e840000)
			f_header->lzma_src -= ECNT_L2_SRAM_SIZE;
#endif

		/* Setup the lzma attr*/
		image_decompress_init(f_header->lzma_src, EN7523_IMAGE_BUF_SIZE, lzmaBuffToBuffDecompress);
		image_decompress_work_buf_init(EN7523_IMAGE_BUF_OFFSET, EN7523_IMAGE_BUF_SIZE);
		image_decompress_prepare(&info);
		image_decompress(&info);

		__attribute__((noreturn)) void (*bl2)(void);
		bl2 = (void *)info.image_base;
		if (f_header->lzma_cmd == 1)
		{
			/* decompress flash_table*/
			info.image_base = ECNT_FLASH_TABLE_BASE;
			info.image_size = f_header->flash_table_length;
			image_decompress_init(f_header->lzma_src + f_header->bl23_length, EN7523_IMAGE_BUF_SIZE, lzmaBuffToBuffDecompress);
			image_decompress_work_buf_init(EN7523_IMAGE_BUF_OFFSET, EN7523_IMAGE_BUF_SIZE);
			image_decompress_prepare(&info);
			image_decompress(&info);
			VPint(BL2_OPTIMIZE_STATUS) = 0;
		}

		/* jump to nextimage*/
		inv_dcache_range(BL1_RW2_BASE, BL1_RW2_SIZE);
		disable_mmu_icache_secure();

		(*bl2)();

#elif IMAGE_BL22
		Bl2_optimize_header_t *f_header = NULL;

		f_header = (Bl2_optimize_header_t*)((unsigned char*)BL2_OPTIMIZE_HEADER_BASE);
		f_header->lzma_des = ECNT_BL23_BASE;
		f_header->lzma_src = BL2_OPTIMIZE_HEADER_BASE + f_header->bl22_length + sizeof(Bl2_optimize_header_t);
		f_header->lzma_length = f_header->bl23_length;
		f_header->lzma_cmd = 1;

		__attribute__((noreturn)) void (*bl2)(void);
		bl2 = (void *)ECNT_BL21_BASE;
		/* jump to bl21*/
		inv_dcache_range(BL1_RW2_BASE, BL1_RW2_SIZE);
		disable_mmu_icache_secure();

		(*bl2)();
#endif


}
#endif

void bl2_plat_preload_setup(void)
{
#if !defined(IMAGE_BL21) && !defined(IMAGE_BL22)
	FLASH_READ_STATUS_T flash_read_status = FLASH_READ_STATUS_CORRECT;
	int fwu_img_len = PLAT_ECNT_FIP_MAX_SIZE + PLAT_ECNT_MV_DATA_SIZE;
	int fip_offset = PLAT_ECNT_FIP_OFFSET;
#endif

#ifdef TCSUPPORT_BL2_OPTIMIZATION
	bl2_plat_preload_setup_optimize();
#endif

#if !defined(IMAGE_BL21) && !defined(IMAGE_BL22)

	image_decompress_init(EN7523_IMAGE_BUF_OFFSET, EN7523_IMAGE_BUF_SIZE, lzmaBuffToBuffDecompress);

	bl2_platform_setup();

	if (plat_get_dual_boot())
	{
		fip_offset += PLAT_ECNT_MULTI_BOOT_SIZE;
		fwu_img_len += PLAT_ECNT_MULTI_BOOT_SIZE;
	}

#ifdef INC_MODE
	if (hw_trap.inc_mode)
	{

		get_bootimage_by_npu_iNIC((unsigned int) PLAT_ECNT_FIP_BASE);
	}
	else
#endif
	{
		if (hw_trap.fw_upgrade_mode && !(hw_trap.skip_fw_upgrade) && !(plat_get_hw_bypass()))
		{
			int len = 0;

			flash_read_status = flash_read(PLAT_ECNT_FIP_OFFSET, PLAT_ECNT_FIP_MAX_SIZE, (uint8_t *) PLAT_ECNT_FIP_BASE);
			if ((flash_read_status == FLASH_READ_STATUS_INCORRECT) || 
				(flash_read_status == FLASH_READ_STATUS_CORRECT && plat_check_bypass() != BYPASS_FWUPGRADE))
			{
				printf("Press x to update firmware\n");
				while (len == 0)
				{
					if (console.getc(&console) == 'x')
					{
						len = XModemReceive(&console, fwu_img_len,
									(uint8_t *) (PLAT_ECNT_FIP_BASE - PLAT_ECNT_MV_DATA_SIZE));
					}
				}

#if defined(TCSUPPORT_TPL_SUPPORT)
				if (len >= 0x80000)
				{
					if(hw_trap.is_emmc){
						flash_erase(0, len);
						flash_write(0, len, (uint8_t *) (PLAT_ECNT_FIP_BASE - PLAT_ECNT_MV_DATA_SIZE));						
					}
					if (hw_trap.is_spi_nand_device_ecc ||
						hw_trap.is_spi_nand_ctrl_ecc ||
						hw_trap.is_parallel_nand) {
						if(len == 0x80000)
						{
							flash_erase(0, len);
							flash_write(0, len, (uint8_t *) (PLAT_ECNT_FIP_BASE - PLAT_ECNT_MV_DATA_SIZE));							
						}
						else
						{
							/*no need gpte padding*/
							flash_erase(0, len-0x4000);
							flash_write(0, 0x80000, (uint8_t *) (PLAT_ECNT_FIP_BASE - PLAT_ECNT_MV_DATA_SIZE));
							flash_write(0x80000, len-0x84000, (uint8_t *) (PLAT_ECNT_FIP_BASE - PLAT_ECNT_MV_DATA_SIZE + 0x84000));
						}
					}
				}
#else
				if (len == fwu_img_len)
				{
					flash_erase(0, len);
					flash_write(0, len, (uint8_t *) (PLAT_ECNT_FIP_BASE - PLAT_ECNT_MV_DATA_SIZE));
					if (plat_get_dual_boot())
					{
						/*put the image to FIP BASE for booting */
						flash_read(fip_offset, PLAT_ECNT_FIP_MAX_SIZE, (uint8_t *) PLAT_ECNT_FIP_BASE);
					}
				}
#endif
			} else if(flash_read_status == FLASH_READ_STATUS_CORRECT && plat_check_bypass() == BYPASS_FWUPGRADE){
				NOTICE("BYPASS\n");
			}
		}
		else
		{
			if (flash_read(fip_offset, PLAT_ECNT_FIP_MAX_SIZE, (uint8_t *) PLAT_ECNT_FIP_BASE) == FLASH_READ_STATUS_CORRECT)
			{
			}
			else
			{
				panic();
			}
		}
	}

	while (plat_check_header((uint8_t *) PLAT_ECNT_FIP_BASE) == 0)
	{
#ifdef INC_MODE
		if (get_into_inic())
		{
			get_bootimage_by_npu_iNIC((unsigned int) PLAT_ECNT_FIP_BASE);
		}
		else
#endif
		{
			panic();
		}
	}
#endif

#ifdef IMAGE_BL23
	/* Read BL31+U-Boot from offset for eMMC. For NAND FIP is read by UBI module */
	if (hw_trap.is_emmc &&
	    (!hw_trap.fw_upgrade_mode || hw_trap.skip_fw_upgrade || plat_get_hw_bypass())) {
		if (flash_read(PLAT_ECNT_BL31_FIP_OFFSET, PLAT_ECNT_FIP_MAX_SIZE, (uint8_t *) PLAT_ECNT_FIP_BASE) != FLASH_READ_STATUS_CORRECT)
			panic();
	}
#endif
}

void bl2_platform_setup(void)
{
#if !defined(IMAGE_BL21) && !defined(IMAGE_BL22)
	hw_trap_init();
	flash_init(&hw_trap);
	plat_ecnt_io_setup(&hw_trap);
#endif
}
#if !defined(IMAGE_BL21) && !defined(IMAGE_BL22)

int bl2_plat_handle_post_image_load(unsigned int image_id)
{
	bl_mem_params_node_t *bl_mem_params = get_bl_mem_params_node(image_id);

	if (!(bl_mem_params->image_info.h.attr & IMAGE_ATTRIB_SKIP_LOADING))
	{
		if (image_decompress(&(bl_mem_params->image_info)))
		{
			panic();
		}
	}

	#if defined(TCSUPPORT_TPL_ENC)
	unsigned char* pSSK = get_ssk();
	unsigned char i;
	
	unsigned char zero_key[32]={0};
	if(strcmp(zero_key,pSSK)==0){
		ERROR(" \033[31;1m You cannot boot without fusing secure key.\033[0m\n");
		panic();
	}
	
	for(i=0;i<32;i++){
		//NOTICE("[deliever ssk][0x%x] = 0x%x\n",pSSK+i,*(pSSK+i));
		mmio_write_8(SSK_BASE+i, *(pSSK+i));
	}		
	#endif
	
	

	return 0;
}

int bl2_plat_handle_pre_image_load(unsigned int image_id)
{
	bl_mem_params_node_t *bl_mem_params = get_bl_mem_params_node(image_id);

	image_decompress_prepare(&(bl_mem_params->image_info));

	return 0;
}

#endif

