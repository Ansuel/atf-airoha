 /***************************************************************************************
 *      Copyright(c) 2014 ECONET Incorporation All rights reserved.
 *
 *      This is unpublished proprietary source code of ECONET Incorporation
 *
 *      The copyright notice above does not evidence any actual or intended
 *      publication of such source code.
 ***************************************************************************************
 */

/*======================================================================================
 * MODULE NAME: spi
 * FILE NAME: spi_nand_flash.h
 * DATE: 2014/11/21
 * VERSION: 1.00
 * PURPOSE: To Provide SPI NAND Access interface.
 * NOTES:
 *
 * AUTHOR : Chuck Kuo         REVIEWED by
 *
 * FUNCTIONS  
 *      SPI_NAND_Flash_Init             To provide interface for SPI NAND init. 
 *      SPI_NAND_Flash_Get_Flash_Info   To get system current flash info. 
 *      SPI_NAND_Flash_Write_Nbyte      To provide interface for Write N Bytes into SPI NAND Flash. 
 *      SPI_NAND_Flash_Read_NByte       To provide interface for Read N Bytes from SPI NAND Flash. 
 *      SPI_NAND_Flash_Erase            To provide interface for Erase SPI NAND Flash. 
 *      SPI_NAND_Flash_Read_Byte        To provide interface for read 1 Bytes from SPI NAND Flash. 
 *      SPI_NAND_Flash_Read_DWord       To provide interface for read Double Word from SPI NAND Flash. 
 *
 * DEPENDENCIES
 *
 * * $History: $
 * MODIFICTION HISTORY:
 * Version 1.00 - Date 2014/11/21 By Chuck Kuo
 * ** This is the first versoin for creating to support the functions of
 *    current module.
 *
 *======================================================================================
 */

#ifndef __SPI_NAND_FLASH_H__
#define __SPI_NAND_FLASH_H__

#define BOOTROM_INT

#include <stdint.h>
#include <stddef.h>

/* INCLUDE FILE DECLARATIONS --------------------------------------------------------- */

/* MACRO DECLARATIONS ---------------------------------------------------------------- */

/* TYPE DECLARATIONS ----------------------------------------------------------------- */
typedef enum{
	SPI_NAND_FLASH_READ_DUMMY_BYTE_PREPEND,
	SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND,
	
	SPI_NAND_FLASH_READ_DUMMY_BYTE_DEF_NO
	
} SPI_NAND_FLASH_READ_DUMMY_BYTE_T;

typedef enum{
	SPI_NAND_FLASH_RTN_NO_ERROR =0,
	SPI_NAND_FLASH_RTN_PROBE_ERROR,
	SPI_NAND_FLASH_RTN_ALIGNED_CHECK_FAIL,
	SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK,
	SPI_NAND_FLASH_RTN_ERASE_FAIL,
	SPI_NAND_FLASH_RTN_PROGRAM_FAIL,
	SPI_NAND_FLASH_RTN_NFI_FAIL,
	SPI_NAND_FLASH_RTN_ECC_DECODE_FAIL,
	SPI_NAND_FLASH_RTN_TIMEOUT,

	SPI_NAND_FLASH_RTN_DEF_NO
} SPI_NAND_FLASH_RTN_T;

typedef enum{
	SPI_NAND_FLASH_READ_SPEED_MODE_SINGLE =0,
	SPI_NAND_FLASH_READ_SPEED_MODE_DUAL,
	SPI_NAND_FLASH_READ_SPEED_MODE_QUAD,
	
	SPI_NAND_FLASH_READ_SPEED_MODE_DEF_NO	
} SPI_NAND_FLASH_READ_SPEED_MODE_T;


typedef enum{
	SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE =0,
	SPI_NAND_FLASH_WRITE_SPEED_MODE_QUAD,
	
	SPI_NAND_FLASH_WRITE_SPEED_MODE_DEF_NO	
} SPI_NAND_FLASH_WRITE_SPEED_MODE_T;


typedef enum{
	SPI_NAND_FLASH_DEBUG_LEVEL_0 =0,
	SPI_NAND_FLASH_DEBUG_LEVEL_1,
	SPI_NAND_FLASH_DEBUG_LEVEL_2,
	
	SPI_NAND_FLASH_DEBUG_LEVEL_DEF_NO	
} SPI_NAND_FLASH_DEBUG_LEVEL_T;



struct SPI_NAND_FLASH_INFO_T {
	uint32_t							page_size;
	SPI_NAND_FLASH_READ_DUMMY_BYTE_T	dummy_mode;
	SPI_NAND_FLASH_READ_SPEED_MODE_T	read_mode;
};

/* EXPORTED SUBPROGRAM SPECIFICATION ------------------------------------------------- */
int nandflash_init(int rom_base);
int nandflash_read(unsigned long from, unsigned long len, uint32_t *retlen, unsigned char *buf, SPI_NAND_FLASH_RTN_T *status);

#endif /* ifndef __SPI_NAND_FLASH_H__ */
/* End of [spi_nand_flash.h] package */

