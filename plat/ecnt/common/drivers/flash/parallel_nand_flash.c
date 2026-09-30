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
#include <ecnt_spi_nfi.h>
#include <ecnt_spi_ecc.h>
#include "parallel_nand_flash.h"

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
#include <linux/spinlock.h>
#include <linux/mtd/nand.h>
#include <asm/tc3162/tc3162.h>
#else
#include <asm/tc3162.h>
#endif

#if !defined(SPRAM_IMG) && !defined(LZMA_IMG)
#include <stdarg.h>
#endif

/* NAMING CONSTANT DECLARATIONS ------------------------------------------------------ */



/* FUNCTION DECLARATIONS ------------------------------------------------------------- */


 
/* MACRO DECLARATIONS ---------------------------------------------------------------- */
#define CHIP0					(0)
#define CHIP1					(1)
#define MAX_CHIPS				(2)
#define ID_LEN					(5)
#define STATUS_LEN				(1)
#define COL_NOB					(2)
#define ROW_NOB					(ptr_dev_info_t->addr_cycle - 2)
#define STATUS_FAIL_BIT			(0x01)

#define PARALLEL_NAND_FLASH_DEBUG

#if !defined(SPRAM_IMG) && !defined(LZMA_IMG)
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
	#define _PARALLEL_NAND_PRINTF				printk 		/* Always print information */
#else
#if !defined(PARALLEL_NAND_FLASH_DEBUG)
#if defined(CONFIG_ECNT_UBOOT)
	#define _PARALLEL_NAND_PRINTF				printf 		/* Always print information */
#else
	#define _PARALLEL_NAND_PRINTF				prom_printf	/* Always print information */
#endif
#else
	#define _PARALLEL_NAND_PRINTF(args...)
#endif
#endif
	#define _PARALLEL_NAND_DEBUG_PRINTF			parallel_nand_debug_printf
#else
	#define _PARALLEL_NAND_PRINTF(args...)						
	#define _PARALLEL_NAND_DEBUG_PRINTF(args...)	
#endif


/* TYPE DECLARATIONS ----------------------------------------------------------------- */



/* STATIC VARIABLE DECLARATIONS ------------------------------------------------------ */
extern struct SPI_NAND_FLASH_INFO_T	_current_flash_info_t;
struct SPI_NAND_FLASH_INFO_T *ptr_dev_info_t = &(_current_flash_info_t);
u8 _PARALLEL_NAND_DEBUG_LEVEL = 0;
extern SPI_NAND_FLASH_RTN_T parallel_nand_scan_flash_table(struct SPI_NAND_FLASH_INFO_T *ptr_rtn_device_t);

/* LOCAL SUBPROGRAM BODIES------------------------------------------------------------ */
#if !defined(SPRAM_IMG) && !defined(LZMA_IMG)
void parallel_nand_debug_printf( char *fmt, ... )
{
#if !defined(CONFIG_ECNT_UBOOT)
	if(_PARALLEL_NAND_DEBUG_LEVEL > 0)
	{
		unsigned char 		str_buf[100];	
		va_list 			argptr;
		int 				cnt;		
	
		va_start(argptr, fmt);
		cnt = vsprintf(str_buf, fmt, argptr);
		va_end(argptr);
				
		_PARALLEL_NAND_PRINTF("%s", str_buf);
	}
#else
		_PARALLEL_NAND_PRINTF(fmt);
#endif
}
#endif

void parallel_nand_chip_select(u8 chip)
{
	PARALLEL_NFI_SET_CHIP_SELECT(chip);
	_PARALLEL_NAND_DEBUG_PRINTF( "parallel_nand_chip_select : chip=0x%x\n", chip);
}

void parallel_nand_chip_select_by_page(u32 *p_page_number)
{
	u8 chip = CHIP0;

	if (ptr_dev_info_t->feature & PARALLEL_NAND_FLASH_2CE)
	{
		if (*p_page_number >= ((ptr_dev_info_t->device_size / ptr_dev_info_t->page_size) / 2))
		{
			*p_page_number -= ((ptr_dev_info_t->device_size / ptr_dev_info_t->page_size) / 2);
			chip = CHIP1;
		}
	}

	parallel_nand_chip_select(chip);
}

void parallel_nand_setting_timing(void)
{
	PARALLEL_NFI_SET_TIMING(ptr_dev_info_t->timing_setting);
}

void parallel_nand_initial_hw(void)
{
	/* TO_DO */
	/* iomux */

	SPI_NFI_Reset();

	/* Enable NFI All Interrupt */
	PARALLEL_NFI_INIT();
}

SPI_NAND_FLASH_RTN_T parallel_nand_transfer_data(unsigned long *p_data, u8 dirt)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	SPI_NFI_RTN_T			status = SPI_NFI_RTN_NO_ERROR;

	status = PARALLEL_NFI_START_DMA(p_data, dirt);
	if (status != SPI_NFI_RTN_NO_ERROR)
	{
		rtn_status = SPI_NAND_FLASH_RTN_NFI_FAIL;
		_PARALLEL_NAND_PRINTF("%s Data Failed\n", ((dirt == SPI_NFI_WRITE_DATA) ? "Write" : "Read"));
	}

	return rtn_status;
}

SPI_NAND_FLASH_RTN_T parallel_nand_sigle_read(u8 len, u8 *p_data)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	SPI_NFI_RTN_T			status = SPI_NFI_RTN_NO_ERROR;

	status = PARALLEL_NFI_START_BYTE_READ(len, p_data);;
	if (status != SPI_NFI_RTN_NO_ERROR)
	{
		rtn_status = SPI_NAND_FLASH_RTN_NFI_FAIL;
		_PARALLEL_NAND_PRINTF("Single Read Failed: len=0x%x\n", len);
	}

	return rtn_status;
}

SPI_NAND_FLASH_RTN_T parallel_nand_send_address(u32 col_addr, u32 row_addr, u8 col_nob, u8 row_nob)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	SPI_NFI_RTN_T			status = SPI_NFI_RTN_NO_ERROR;

	status = PARALLEL_NFI_ISSUE_ADDR(col_addr, row_addr, col_nob, row_nob);
	if (status != SPI_NFI_RTN_NO_ERROR)
	{
		if (status == SPI_NFI_RTN_ACCESS_LOCK)
		{
			rtn_status = SPI_NAND_FLASH_RTN_CMD_ABORT;
		}
		else
		{
			rtn_status = SPI_NAND_FLASH_RTN_NFI_FAIL;
			_PARALLEL_NAND_PRINTF("Send Address Failed: col_addr=0x%x row_addr=0x%x col_nob=0x%x row_nob=0x%x\n",
					col_addr, row_addr, col_nob, row_nob);
		}
	}
	_PARALLEL_NAND_DEBUG_PRINTF( "Send Address: col_addr=0x%x row_addr=0x%x col_nob=0x%x row_nob=0x%x\n",
			col_addr, row_addr, col_nob, row_nob);
	return rtn_status;
}

SPI_NAND_FLASH_RTN_T parallel_nand_send_cmd(u8 cmd)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	SPI_NFI_RTN_T			status = SPI_NFI_RTN_NO_ERROR;

	switch (cmd)
	{
		case NAND_CMD_READID:
		case NAND_CMD_STATUS:
		case NAND_CMD_ERASE1:
			PARALLEL_NFI_RESET();
		case NAND_CMD_READ:
		case NAND_CMD_SEQIN:
			PARALLEL_NFI_ISSUE_CMD_1(cmd);
			break;
		case NAND_CMD_RESET:
		case NAND_CMD_ERASE2:
		case NAND_CMD_PAGEPROG:
		case NAND_CMD_READSTART:
			status = PARALLEL_NFI_ISSUE_CMD_2(cmd);
			break;
		default:
			_PARALLEL_NAND_PRINTF("Unkonw Command: 0x%x\n", cmd);
	}

	if (status != SPI_NFI_RTN_NO_ERROR)
	{
		rtn_status = SPI_NAND_FLASH_RTN_NFI_FAIL;
		_PARALLEL_NAND_PRINTF("Send Command Failed: cmd=0x%x\n", cmd);
	}
	_PARALLEL_NAND_DEBUG_PRINTF( "Send Command: cmd=0x%x\n", cmd);
	return rtn_status;
}

SPI_NAND_FLASH_RTN_T parallel_nand_protocol_read(u32 col_addr, u32 row_addr, unsigned long *p_data)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;	

	rtn_status = parallel_nand_send_cmd(NAND_CMD_READ);
	if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR)
	{
		rtn_status = parallel_nand_send_address(col_addr, row_addr, COL_NOB, ROW_NOB);
		if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR)
		{
			rtn_status = parallel_nand_send_cmd(NAND_CMD_READSTART);
			if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR)
			{
				rtn_status = parallel_nand_transfer_data(p_data, SPI_NFI_READ_DATA);
			}
		}
	}

	_PARALLEL_NAND_DEBUG_PRINTF( "parallel_nand_protocol_read : col_addr=0x%x row_addr=0x%x\n", col_addr, row_addr);
	
	return rtn_status;
}

#if !defined(LZMA_IMG) || defined(TCSUPPORT_BB_256KB)
SPI_NAND_FLASH_RTN_T parallel_nand_protocol_program(u32 col_addr, u32 row_addr, unsigned long *p_data)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;	

	rtn_status = parallel_nand_send_cmd(NAND_CMD_SEQIN);
	if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR)
	{
		rtn_status = parallel_nand_send_address(col_addr, row_addr, COL_NOB, ROW_NOB);
		if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR)
		{
			rtn_status = parallel_nand_transfer_data(p_data, SPI_NFI_WRITE_DATA);
			if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR)
			{
				rtn_status = parallel_nand_send_cmd(NAND_CMD_PAGEPROG);
			}
		}
	}

 	_PARALLEL_NAND_DEBUG_PRINTF( "parallel_nand_protocol_program : col_addr=0x%x row_addr=0x%x\n", col_addr, row_addr);
	
	return rtn_status;
}

SPI_NAND_FLASH_RTN_T parallel_nand_protocol_get_status(u8 *p_status)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;	
	u8						buf[8] = {0};

	rtn_status = parallel_nand_send_cmd(NAND_CMD_STATUS);
	if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR)
	{
		rtn_status = parallel_nand_sigle_read(STATUS_LEN, buf);
		if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR)
		{
			memcpy(p_status, &(buf[0]), sizeof(u8));
		}
	}

	_PARALLEL_NAND_DEBUG_PRINTF( "parallel_nand_protocol_get_status : status=0x%x\n", *p_status);
	
	return rtn_status;
}

SPI_NAND_FLASH_RTN_T parallel_nand_protocol_erase(u32 row_addr)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;	

	rtn_status = parallel_nand_send_cmd(NAND_CMD_ERASE1);
	if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR)
	{
		rtn_status = parallel_nand_send_address(0, row_addr, 0, ROW_NOB);
		if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR)
		{
			rtn_status = parallel_nand_send_cmd(NAND_CMD_ERASE2);
		}
	}

	_PARALLEL_NAND_DEBUG_PRINTF( "parallel_nand_protocol_erase : row_addr=0x%x\n", row_addr);
	
	return rtn_status;
}
#endif

SPI_NAND_FLASH_RTN_T parallel_nand_protocol_read_id(struct SPI_NAND_FLASH_INFO_T *ptr_rtn_flash_id)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	u8						buf[8] = {0};
	u32						ext_id = 0;

	rtn_status = parallel_nand_send_cmd(NAND_CMD_READID);
	
	rtn_status = parallel_nand_send_address(0, 0, 1, 0);
	if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR)
	{
		rtn_status = parallel_nand_sigle_read(ID_LEN, buf);
		if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR)
		{
			memcpy((void *) &(ptr_rtn_flash_id->mfr_id), (void *) &(buf[0]), sizeof(u8));
			memcpy((void *) &(ptr_rtn_flash_id->dev_id), (void *) &(buf[1]), sizeof(u8));

			ext_id = buf[2] | (buf[3] << 8) | (buf[4] << 16);
			memcpy((void *) &(ptr_rtn_flash_id->ext_id), (void *) &ext_id, sizeof(u32));
		}
	}

	_PARALLEL_NAND_DEBUG_PRINTF( "parallel_nand_protocol_read_id : mfr_id=0x%x, dev_id=0x%x, ext_id=0x%x\n",
			ptr_rtn_flash_id->mfr_id, ptr_rtn_flash_id->dev_id, ptr_rtn_flash_id->ext_id);
	
	return (rtn_status);
}

SPI_NAND_FLASH_RTN_T parallel_nand_protocol_reset (void)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	rtn_status = parallel_nand_send_cmd(NAND_CMD_RESET);

	_PARALLEL_NAND_DEBUG_PRINTF( "parallel_nand_protocol_reset\n");
	
	return (rtn_status);
}

SPI_NAND_FLASH_RTN_T parallel_nand_probe(struct SPI_NAND_FLASH_INFO_T *ptr_rtn_device_t)
{
	SPI_NAND_FLASH_RTN_T			rtn_status = SPI_NAND_FLASH_RTN_PROBE_ERROR;
	u8								chip = 0;
	SPI_ECC_ENCODE_ABILITY_T		ecc_abiliry = SPI_ECC_ENCODE_ABILITY_4BITS;
	struct SPI_NAND_FLASH_INFO_T	parallel_nand_dev_info[MAX_CHIPS];

	_PARALLEL_NAND_DEBUG_PRINTF( "parallel_nand_probe: start \n");

	memset(parallel_nand_dev_info, 0, (sizeof(struct SPI_NAND_FLASH_INFO_T) * MAX_CHIPS));

	parallel_nand_initial_hw();

	for (chip = 0; chip < MAX_CHIPS; chip++)
	{
		parallel_nand_chip_select(chip);
		parallel_nand_protocol_reset();
		parallel_nand_protocol_read_id(&parallel_nand_dev_info[chip]);

		if ((parallel_nand_dev_info[chip].mfr_id != 0x0) &&
			(parallel_nand_dev_info[chip].dev_id != 0x0) &&
			(parallel_nand_dev_info[chip].ext_id != 0x0))
		{
			_PARALLEL_NAND_DEBUG_PRINTF("parallel_nand_probe: mfr_id=0x%x, dev_id=0x%x, ext_id=0x%x\n",
					parallel_nand_dev_info[chip].mfr_id, parallel_nand_dev_info[chip].dev_id, parallel_nand_dev_info[chip].ext_id);

			break;
		}
	}
	if ( chip < MAX_CHIPS )
	{
		memcpy(ptr_rtn_device_t, &parallel_nand_dev_info[chip], sizeof(struct SPI_NAND_FLASH_INFO_T));
	}

	_PARALLEL_NAND_DEBUG_PRINTF("parallel_nand_probe: mfr_id=0x%x, dev_id=0x%x, ext_id=0x%x\n",
			ptr_rtn_device_t->mfr_id, ptr_rtn_device_t->dev_id, ptr_rtn_device_t->ext_id);

#ifdef BOOTROM_EXT
	prom_puts("===mfr_id:0x");
	prom_print_hex(ptr_rtn_device_t->mfr_id, 8);
	prom_puts(", dev_id:0x");
	prom_print_hex(ptr_rtn_device_t->dev_id, 8);
	prom_puts(", ext_id:0x");
	prom_print_hex(ptr_rtn_device_t->ext_id, 8);
	prom_puts("===\n");
#endif

	rtn_status = parallel_nand_scan_flash_table(ptr_rtn_device_t);
	if(rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR)
	{
		_PARALLEL_NAND_DEBUG_PRINTF( "parallel_nand_probe: device size    =0x%x \n", ptr_rtn_device_t->device_size);
		_PARALLEL_NAND_DEBUG_PRINTF( "parallel_nand_probe: page size      =0x%x \n", ptr_rtn_device_t->page_size);
		_PARALLEL_NAND_DEBUG_PRINTF( "parallel_nand_probe: erase size     =0x%x \n", ptr_rtn_device_t->erase_size);
		_PARALLEL_NAND_DEBUG_PRINTF( "parallel_nand_probe: oob_size       =0x%x \n", ptr_rtn_device_t->oob_size);
		_PARALLEL_NAND_DEBUG_PRINTF( "parallel_nand_probe: feature        =0x%x \n", ptr_rtn_device_t->feature);
		_PARALLEL_NAND_DEBUG_PRINTF( "parallel_nand_probe: timing_setting =0x%x \n", ptr_rtn_device_t->timing_setting);
		_PARALLEL_NAND_DEBUG_PRINTF( "parallel_nand_probe: min_ecc_req    =0x%x \n", ptr_rtn_device_t->min_ecc_req);
		_PARALLEL_NAND_DEBUG_PRINTF( "parallel_nand_probe: addr_cycle     =0x%x \n", ptr_rtn_device_t->addr_cycle);
		_PARALLEL_NAND_DEBUG_PRINTF( "parallel_nand_probe: soc_ecc_ability     =0x%x \n", ptr_rtn_device_t->soc_ecc_ability);
	}
	else
	{
		_PARALLEL_NAND_PRINTF("parallel_nand_scan_flash_table failed\n");
		while(1);
	}
#if 0 //defined(BOOTROM_EXT)
	{
		if (ptr_rtn_device_t->min_ecc_req > SPI_ECC_ENCODE_ABILITY_4BITS)
		{

			if (ptr_rtn_device_t->min_ecc_req <= SPI_ECC_ENCODE_ABILITY_8BITS)
			{
				ecc_abiliry = SPI_ECC_ENCODE_ABILITY_8BITS;
				spare_size_t = SPI_NFI_CONF_SPARE_SIZE_26BYTE;
			}
			else if(ptr_rtn_device_t->min_ecc_req <= SPI_ECC_ENCODE_ABILITY_10BITS)
			{
				ecc_abiliry = SPI_ECC_ENCODE_ABILITY_10BITS;
				spare_size_t = SPI_NFI_CONF_SPARE_SIZE_26BYTE;
			}
			else
			{
				ecc_abiliry = SPI_ECC_ENCODE_ABILITY_12BITS;
				spare_size_t = SPI_NFI_CONF_SPARE_SIZE_28BYTE;
				if(ptr_rtn_device_t->min_ecc_req > SPI_ECC_ENCODE_ABILITY_12BITS)
					_PARALLEL_NAND_PRINTF("min_ecc_req not support: req=0x%x\n", ptr_rtn_device_t->min_ecc_req);
			}
		}

		spi_nand_info = ((ecc_abiliry << 16) | spare_size_t);
		VPint(SPI_NAND_CONTROLLER_ECC_INFO_SRAM_ADDR_INFO) = spi_nand_info;
	}
#else
	{
		ecc_abiliry = ptr_rtn_device_t->soc_ecc_ability;

		if (ecc_abiliry < ptr_rtn_device_t->min_ecc_req)
		{
			_PARALLEL_NAND_PRINTF("min_ecc_req check failed: req=0x%x now=0x%x\n", ptr_rtn_device_t->min_ecc_req, ecc_abiliry);
			while(1);
		}
		if (!(isFPGA))
		{
			/* 7581 should use _PARALLEL_NFI_REGS_ACCCON HW default value */
#if !defined(TCSUPPORT_CPU_EN7581)
			parallel_nand_setting_timing();
#endif
		}

	}
#endif
	_PARALLEL_NAND_DEBUG_PRINTF( "parallel_nand_probe: end \n");
	
	return (rtn_status);
}

SPI_NAND_FLASH_RTN_T parallel_nand_dma_read(u32 read_addr, u32 page_number, unsigned long *p_data)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_PROBE_ERROR;

	parallel_nand_chip_select_by_page(&page_number);

	rtn_status = parallel_nand_protocol_read(read_addr, page_number, p_data);

	return rtn_status;
}

#if !defined(LZMA_IMG) || defined(TCSUPPORT_BB_256KB)
SPI_NAND_FLASH_RTN_T parallel_nand_dma_write(u32 write_addr, u32 page_number, unsigned long *p_data, u32 oob_len, u8 *ptr_oob)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	u8						status = 0;
	SPI_NFI_CONF_T			spi_nfi_conf_t;
	SPI_ECC_ENCODE_CONF_T	spi_ecc_encode_conf_t;

	SPI_NFI_Reset();

	SPI_NFI_Get_Configure(&spi_nfi_conf_t);
	SPI_NFI_Set_Configure(&spi_nfi_conf_t);

	SPI_ECC_Encode_Get_Configure(&spi_ecc_encode_conf_t);
	SPI_ECC_Encode_Set_Configure(&spi_ecc_encode_conf_t);
	
	if(spi_nfi_conf_t.hw_ecc_t == SPI_NFI_CON_HW_ECC_Enable) {
		SPI_ECC_Encode_Get_Configure(&spi_ecc_encode_conf_t);
		
		if(spi_ecc_encode_conf_t.encode_en == SPI_ECC_ENCODE_ENABLE) {
			spi_ecc_encode_conf_t.encode_block_size = (spi_nfi_conf_t.fdm_ecc_num + 512) ;
			SPI_ECC_Encode_Set_Configure(&spi_ecc_encode_conf_t);

			SPI_ECC_Encode_Disable();
			SPI_ECC_Decode_Disable();
			SPI_ECC_Encode_Enable();
		}
	}

	if(spi_nfi_conf_t.auto_fdm_t == SPI_NFI_CON_AUTO_FDM_Enable)
	{
		SPI_NFI_Write_SPI_NAND_FDM(ptr_oob, oob_len);
	}

	parallel_nand_chip_select_by_page(&page_number);

	rtn_status = parallel_nand_protocol_program(write_addr, page_number, p_data);

	if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR)
	{
		rtn_status = parallel_nand_protocol_get_status(&status);

		if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR)
		{
			if (status & STATUS_FAIL_BIT)
			{
				rtn_status = SPI_NAND_FLASH_RTN_PROGRAM_FAIL;
			}
		}
	}

	if (rtn_status == SPI_NAND_FLASH_RTN_CMD_ABORT)
	{
		rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	}

	return rtn_status;
}

SPI_NAND_FLASH_RTN_T parallel_nand_erase(u32 page_number)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	u8						status = 0;

	parallel_nand_chip_select_by_page(&page_number);

	rtn_status = parallel_nand_protocol_erase(page_number);

	if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR)
	{
		rtn_status = parallel_nand_protocol_get_status(&status);

		if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR)
		{
			if (status & STATUS_FAIL_BIT)
			{
				rtn_status = SPI_NAND_FLASH_RTN_ERASE_FAIL;
			}
		}
	}

	if (rtn_status == SPI_NAND_FLASH_RTN_CMD_ABORT)
	{
		rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	}

	return rtn_status;
}
#endif

/* End of [parallel_nand_flash.c] package */
