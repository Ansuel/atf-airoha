/***************************************************************************************
 *      Copyright(c) 2014 ECONET Incorporation All rights reserved.
 *
 *      This is unpublished proprietary source code of ECONET Networks Incorporation
 *
 *      The copyright notice above does not evidence any actual or intended
 *      publication of such source code.
 ***************************************************************************************
 */


/*======================================================================================
 * MODULE NAME:  
 * FILE NAME: 
 * DATE: 
 * VERSION: 1.00
 * PURPOSE: 
 * NOTES:
 *
 * AUTHOR :          REVIEWED by
 *
 * FUNCTIONS
 *
 * DEPENDENCIES
 *
 * * $History: $
 * MODIFICTION HISTORY:
 *======================================================================================
 */

/* INCLUDE FILE DECLARATIONS --------------------------------------------------------- */



/* NAMING CONSTANT DECLARATIONS ------------------------------------------------------ */



/* FUNCTION DECLARATIONS ------------------------------------------------------------- */


 
/* MACRO DECLARATIONS ---------------------------------------------------------------- */



/* TYPE DECLARATIONS ----------------------------------------------------------------- */



/* STATIC VARIABLE DECLARATIONS ------------------------------------------------------ */



/* LOCAL SUBPROGRAM BODIES------------------------------------------------------------ */
#include "parallel_nand_flash.h"


const struct SPI_NAND_FLASH_INFO_T parallel_nand_flash_tables[] = {
	{
		mfr_id:						_SPI_NAND_MANUFACTURER_ID_MXIC,
		dev_id:						0xF1,
		ptr_name:					(const u8 *)"MX30LF1G18AC",
		device_size:				_SPI_NAND_CHIP_SIZE_1GBIT,
		page_size:					_SPI_NAND_PAGE_SIZE_2KBYTE,
		erase_size:					_SPI_NAND_BLOCK_SIZE_128KBYTE,
		oob_size:					_SPI_NAND_OOB_SIZE_64BYTE,
		feature:					SPI_NAND_FLASH_FEATURE_NONE,
		ext_id:						0x029580,
		timing_setting:				0x44333,
		min_ecc_req:				4,
		addr_cycle:					4,
		soc_ecc_ability:			4,
		oob_free_layout : 			&ooblayout_spi_controller_ecc,
	},

	{
		mfr_id:						_SPI_NAND_MANUFACTURER_ID_MXIC,
		dev_id:						0xDA,
		ptr_name:					(const u8 *)"MX30LF2G18AC",
		device_size:				_SPI_NAND_CHIP_SIZE_2GBIT,
		page_size:					_SPI_NAND_PAGE_SIZE_2KBYTE,
		erase_size:					_SPI_NAND_BLOCK_SIZE_128KBYTE,
		oob_size:					_SPI_NAND_OOB_SIZE_64BYTE,
		feature:					SPI_NAND_FLASH_FEATURE_NONE,
		ext_id:						0x069590,
		timing_setting:				0x44333,
		min_ecc_req:				4,
		addr_cycle:					5,
		soc_ecc_ability:			4,
		oob_free_layout : 			&ooblayout_spi_controller_ecc,
	},

	{
		mfr_id:						_SPI_NAND_MANUFACTURER_ID_MXIC,
		dev_id:						0xF1,
		ptr_name:					(const u8 *)"MX30LF1G08AA",
		device_size:				_SPI_NAND_CHIP_SIZE_1GBIT,
		page_size:					_SPI_NAND_PAGE_SIZE_2KBYTE,
		erase_size:					_SPI_NAND_BLOCK_SIZE_128KBYTE,
		oob_size:					_SPI_NAND_OOB_SIZE_64BYTE,
		feature:					SPI_NAND_FLASH_FEATURE_NONE,
		ext_id:						0xC21D80,
		timing_setting:				PARALLEL_NAND_FLASH_TIMING,
		min_ecc_req:				1,
		addr_cycle:					4,
		soc_ecc_ability:			4,
		oob_free_layout : 			&ooblayout_spi_controller_ecc,
	},

	{
		mfr_id:						_SPI_NAND_MANUFACTURER_ID_ESMT,
		dev_id:						0xDA,
		ptr_name:					(const u8 *)"F59L2G81A",
		device_size:				_SPI_NAND_CHIP_SIZE_2GBIT,
		page_size:					_SPI_NAND_PAGE_SIZE_2KBYTE,
		erase_size:					_SPI_NAND_BLOCK_SIZE_128KBYTE,
		oob_size:					_SPI_NAND_OOB_SIZE_64BYTE,
		feature:					SPI_NAND_FLASH_FEATURE_NONE,
		ext_id:						0x449590,
		timing_setting:				0x44333,
		min_ecc_req:				4,
		addr_cycle:					5,
		soc_ecc_ability:			4,
		oob_free_layout : 			&ooblayout_spi_controller_ecc,
	},

	{
		mfr_id:						_SPI_NAND_MANUFACTURER_ID_ESMT,
		dev_id:						0xDC,
		ptr_name:					(const u8 *)"F59L4G81A",
		device_size:				_SPI_NAND_CHIP_SIZE_4GBIT,
		page_size:					_SPI_NAND_PAGE_SIZE_2KBYTE,
		erase_size:					_SPI_NAND_BLOCK_SIZE_128KBYTE,
		oob_size:					_SPI_NAND_OOB_SIZE_64BYTE,
		feature:					SPI_NAND_FLASH_FEATURE_NONE,
		ext_id:						0x549590,
		timing_setting:				0x44333,
		min_ecc_req:				4,
		addr_cycle:					5,
		soc_ecc_ability:			4,
		oob_free_layout : 			&ooblayout_spi_controller_ecc,
	},

	{
		mfr_id:						_SPI_NAND_MANUFACTURER_ID_WINBOND,
		dev_id:						0xDC,
		ptr_name:					(const u8 *)"W29N08GVSIAD",
		device_size:				_SPI_NAND_CHIP_SIZE_8GBIT,
		page_size:					_SPI_NAND_PAGE_SIZE_2KBYTE,
		erase_size:					_SPI_NAND_BLOCK_SIZE_128KBYTE,
		oob_size:					_SPI_NAND_OOB_SIZE_64BYTE,
		feature:					PARALLEL_NAND_FLASH_2CE,
		ext_id:						0x549590,
		timing_setting:				0x44333,
		min_ecc_req:				1,
		addr_cycle:					5,
		soc_ecc_ability:			4,
		oob_free_layout : 			&ooblayout_spi_controller_ecc,
	},

	{
		mfr_id:						_SPI_NAND_MANUFACTURER_ID_WINBOND,
		dev_id:						0xD3,
		ptr_name:					(const u8 *)"W29N08GVSIAA",
		device_size:				_SPI_NAND_CHIP_SIZE_8GBIT,
		page_size:					_SPI_NAND_PAGE_SIZE_2KBYTE,
		erase_size:					_SPI_NAND_BLOCK_SIZE_128KBYTE,
		oob_size:					_SPI_NAND_OOB_SIZE_64BYTE,
		feature:					SPI_NAND_FLASH_FEATURE_NONE,
		ext_id:						0x589591,
		timing_setting:				0x44333,
		min_ecc_req:				1,
		addr_cycle:					5,
		soc_ecc_ability:			4,
		oob_free_layout : 			&ooblayout_spi_controller_ecc,
	},

	{
		mfr_id:						_SPI_NAND_MANUFACTURER_ID_MICRON,
		dev_id:						0xF1,
		ptr_name:					(const u8 *)"MT29F01G08ABAEA",
		device_size:				_SPI_NAND_CHIP_SIZE_1GBIT,
		page_size:					_SPI_NAND_PAGE_SIZE_2KBYTE,
		erase_size:					_SPI_NAND_BLOCK_SIZE_128KBYTE,
		oob_size:					_SPI_NAND_OOB_SIZE_64BYTE,
		feature:					SPI_NAND_FLASH_FEATURE_NONE,
		ext_id:						0x049580,
		timing_setting:				0x44333,
		min_ecc_req:				4,
		addr_cycle:					5,
		soc_ecc_ability:			4,
		oob_free_layout : 			&ooblayout_spi_controller_ecc,
	},

	{
		mfr_id:						_SPI_NAND_MANUFACTURER_ID_MICRON,
		dev_id:						0xDA,
		ptr_name:					(const u8 *)"MT29F02G08ABAGA",
		device_size:				_SPI_NAND_CHIP_SIZE_2GBIT,
		page_size:					_SPI_NAND_PAGE_SIZE_2KBYTE,
		erase_size:					_SPI_NAND_BLOCK_SIZE_128KBYTE,
		oob_size:					_SPI_NAND_OOB_SIZE_128BYTE,
		feature:					SPI_NAND_FLASH_FEATURE_NONE,
		ext_id:						0x069590,
		timing_setting:				0x44333,
		min_ecc_req:				8,
		addr_cycle:					5,
		soc_ecc_ability:			12,
		oob_free_layout : 			&ooblayout_spi_controller_ecc,
	},

	{
		mfr_id:						_SPI_NAND_MANUFACTURER_ID_MICRON,
		dev_id:						0xD3,
		ptr_name:					(const u8 *)"MT29F08G08ABACA",
		device_size:				_SPI_NAND_CHIP_SIZE_8GBIT,
		page_size:					_SPI_NAND_PAGE_SIZE_4KBYTE,
		erase_size:					_SPI_NAND_BLOCK_SIZE_256KBYTE,
		oob_size:					0xE0,
		feature:					SPI_NAND_FLASH_FEATURE_NONE,
		ext_id:						0x64A690,
		timing_setting:				0x44333,
		min_ecc_req:				8,
		addr_cycle:					5,
		soc_ecc_ability:			12,
		oob_free_layout : 			&ooblayout_spi_controller_ecc,
	},

	{
		mfr_id:						_SPI_NAND_MANUFACTURER_ID_TOSHIBA,
		dev_id:						0xD3,
		ptr_name:					(const u8 *)"TC58NVG4S0HTA20",
		device_size:				_SPI_NAND_CHIP_SIZE_16GBIT,
		page_size:					_SPI_NAND_PAGE_SIZE_4KBYTE,
		erase_size:					_SPI_NAND_BLOCK_SIZE_256KBYTE,
		oob_size:					_SPI_NAND_OOB_SIZE_256BYTE,
		feature:					PARALLEL_NAND_FLASH_2CE,
		ext_id:						0x762691,
		timing_setting:				0x44333,
		min_ecc_req:				8,
		addr_cycle:					5,
		soc_ecc_ability:			12,
		oob_free_layout : 			&ooblayout_spi_controller_ecc,
	},
	{
		mfr_id: 					_SPI_NAND_MANUFACTURER_ID_TOSHIBA,
		dev_id: 					0xDA,
		ptr_name:					(const u8 *)"TC58NVG1S3HTAI0",
		device_size:				_SPI_NAND_CHIP_SIZE_2GBIT,
		page_size:					_SPI_NAND_PAGE_SIZE_2KBYTE,
		erase_size: 				_SPI_NAND_BLOCK_SIZE_128KBYTE,
		oob_size:					_SPI_NAND_OOB_SIZE_128BYTE,
		feature:					SPI_NAND_FLASH_FEATURE_NONE,
		ext_id: 					0x761590,
		timing_setting: 			0x44333,
		min_ecc_req:				8,
		addr_cycle: 				5,
		soc_ecc_ability:			8,
		oob_free_layout :			&ooblayout_spi_controller_ecc,
	},
#if 0
	{
		mfr_id:						_SPI_NAND_MANUFACTURER_ID_TOSHIBA,
		dev_id:						0xD1,
		ptr_name:					(const u8 *)"TC58NVG1S3ETA00",
		device_size:				_SPI_NAND_CHIP_SIZE_1GBIT,
		page_size:					_SPI_NAND_PAGE_SIZE_2KBYTE,
		erase_size:					_SPI_NAND_BLOCK_SIZE_128KBYTE,
		oob_size:					_SPI_NAND_OOB_SIZE_64BYTE,
		feature:					SPI_NAND_FLASH_FEATURE_NONE,
		ext_id:						0x761590,
		timing_setting:				0x44333,
		min_ecc_req:				1,
		addr_cycle:					4,
		soc_ecc_ability:			4,
		oob_free_layout : 			&ooblayout_spi_controller_ecc,
	},

	{
		mfr_id:						_SPI_NAND_MANUFACTURER_ID_SAMSUNG,
		dev_id:						0xDC,
		ptr_name:					(const u8 *)"K9F4G08U0D",
		device_size:				_SPI_NAND_CHIP_SIZE_4GBIT,
		page_size:					_SPI_NAND_PAGE_SIZE_2KBYTE,
		erase_size:					_SPI_NAND_BLOCK_SIZE_128KBYTE,
		oob_size:					_SPI_NAND_OOB_SIZE_64BYTE,
		feature:					SPI_NAND_FLASH_FEATURE_NONE,
		ext_id:						0x549510,
		timing_setting:				0x44333,
		min_ecc_req:				1,
		addr_cycle:					5,
		soc_ecc_ability:			4,
		oob_free_layout : 			&ooblayout_spi_controller_ecc,
	},

	{
		mfr_id:						_SPI_NAND_MANUFACTURER_ID_MICRON,
		dev_id:						0x48,
		ptr_name:					(const u8 *)"MT29F16G08ABACA",
		device_size:				_SPI_NAND_CHIP_SIZE_16GBIT,
		page_size:					_SPI_NAND_PAGE_SIZE_4KBYTE,
		erase_size:					_SPI_NAND_BLOCK_SIZE_512KBYTE,
		oob_size:					0xE0,
		feature:					SPI_NAND_FLASH_FEATURE_NONE,
		ext_id:						0xA92600,
		timing_setting:				0x44333,
		min_ecc_req:				8,
		addr_cycle:					5,
		soc_ecc_ability:			12,
		oob_free_layout : 			&ooblayout_spi_controller_ecc,
	},
#endif
};

SPI_NAND_FLASH_RTN_T parallel_nand_scan_flash_table(struct SPI_NAND_FLASH_INFO_T *ptr_rtn_device_t)
{
	u32 i = 0;
	SPI_NAND_FLASH_RTN_T rtn_status = SPI_NAND_FLASH_RTN_PROBE_ERROR;
	
	for (i = 0; i < (sizeof(parallel_nand_flash_tables) / sizeof(parallel_nand_flash_tables[0])) ; i++) 
	{
		if (((ptr_rtn_device_t->mfr_id) == parallel_nand_flash_tables[i].mfr_id) &&
			((ptr_rtn_device_t->dev_id) == parallel_nand_flash_tables[i].dev_id) &&
			((ptr_rtn_device_t->ext_id) == parallel_nand_flash_tables[i].ext_id)) 
		{
			memcpy(ptr_rtn_device_t, &parallel_nand_flash_tables[i], sizeof(struct SPI_NAND_FLASH_INFO_T));
			
			rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
			break;
		}
	}

	return rtn_status;
}

/* End of [spi_nand_flash_table.c] package */
