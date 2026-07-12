/***************************************************************************************
 *      Copyright(c) 2014 ECONET Incorporation All rights reserved.
 *
 *      This is unpublished proprietary source code of ECONET Networks Incorporation
 *
 *      The copyright notice above does not evidence any actual or intended
 *      publication of such source code.
 ***************************************************************************************
 */

#include <lib/mmio.h>
#include <common/debug.h>
#include <string.h>

#include <en7523_def.h>
#include "flashhal.h"
#include <ecnt_spi_nand_flash.h>
#include <ecnt_spi_controller.h>
#include "spi_nor_flash.h"
#include <assert.h>
#define NANDFLASH_HWTRAP	(mmio_read_32(EN7523_SFC_STRAP) & 0x2)
#ifdef TCSUPPORT_EMMC
#include <mtk-sd.h>
#include <drivers/mmc.h>

#define RG_I2C0_SDA_PU		(0x1FA20044)
#define RG_GPIO_L_PU		(0x1FA2004C)

/* Need to check index in GPIO table for each CPU */
#if defined(TCSUPPORT_CPU_AN7583)
#define EMMC_CMD			(1<<13)

#define EMMC_DATA0			(1<<5)
#define EMMC_DATA1			(1<<6)
#define EMMC_DATA2			(1<<7)
#define EMMC_DATA3			(1<<20)
#define EMMC_DATA4			(1<<21)
#define EMMC_DATA5			(1<<22)
#define EMMC_DATA6			(1<<23)
#define EMMC_DATA7			(1<<24)
#else /* TCSUPPORT_CPU_AN7581 */
#define EMMC_CMD			(1<<13)

#define EMMC_DATA0			(1<<17)
#define EMMC_DATA1			(1<<18)
#define EMMC_DATA2			(1<<19)
#define EMMC_DATA3			(1<<20)
#define EMMC_DATA4			(1<<21)
#define EMMC_DATA5			(1<<22)
#define EMMC_DATA6			(1<<23)
#define EMMC_DATA7			(1<<24)
#endif

__attribute__((aligned(MMC_BLOCK_SIZE))) static uint8_t emmc_blk_buf[MMC_BLOCK_SIZE];

extern uint8_t support_emmc_feature (void);
#endif

extern int nandflash_init(int rom_base);
extern int nandflash_read(unsigned long from, unsigned long len, u32 *retlen, unsigned char *buf, SPI_NAND_FLASH_RTN_T *status);
extern int nandflash_write(unsigned long to, unsigned long len, u32 *retlen, unsigned char *buf);
extern int nandflash_erase(unsigned long offset, unsigned long len);
extern uint32_t uartDisable;

static hw_trap_t *hwtrap;

#if 0
static void dram_dump(unsigned char *buf, int len)
{
	int i;
	
	printf("\ndump 0x%p: len:0x%x\n", buf, (unsigned int)len);
	for(i=1;i<=len;i++){
		if((i - 1) % 16 == 0) {
			printf("0x%p: ", &(buf[i-1]));
		}
		printf("%02x ", buf[i-1]);
		if(i % 8 == 0) {
			printf(" ");
		}
		if(i % 16 == 0) {
			printf("\n");
		}
	}
	printf("\n");
}
#elif TCSUPPORT_EMMC
static void dram_dump(unsigned char *buf, int len)
{
	return;
}
#else

#endif


FLASH_INIT_T flash_init(hw_trap_t *hw_trap)
{
	hwtrap = hw_trap;
#if defined (TCSUPPORT_CPU_EN7581) || defined(TCSUPPORT_CPU_AN7583) || defined (TCSUPPORT_CPU_AN7552)
	if (hwtrap->is_spi_nand_device_ecc ||
		hwtrap->is_spi_nand_ctrl_ecc ||
		hwtrap->is_parallel_nand) {
		if (nandflash_init(0) != 0) {
			return FLASH_INIT_FAIL;
		}
	}
#else
	if (IS_NANDFLASH) {
 		if (nandflash_init(0) != 0) {
			return FLASH_INIT_FAIL;
		}
 	}
#endif


#ifdef TCSUPPORT_EMMC
	else if (hwtrap->is_emmc) {
		/* 7581CT and 7581DT does not support emmc */
		if (support_emmc_feature() == 0){
			ERROR("Unsupport emmc. emmc init fail.\n");
			return FLASH_INIT_FAIL;
		}

		/* The eMMC data pin and cmd pin should enable internal PU before mmc init. */
		mmio_write_32(RG_I2C0_SDA_PU, (mmio_read_32(RG_I2C0_SDA_PU) | EMMC_CMD));
		mmio_write_32(RG_GPIO_L_PU, (mmio_read_32(RG_GPIO_L_PU) | (EMMC_DATA0) | (EMMC_DATA1) | (EMMC_DATA2) | (EMMC_DATA3) | (EMMC_DATA4) | (EMMC_DATA5) | (EMMC_DATA6) | (EMMC_DATA7)));

		if (mtk_mmc_init() != 0) {
			ERROR("emmc init fail.\n");
			return FLASH_INIT_FAIL;
		}
	} 
#endif
	else {
		spi_nor_init();
	}

	return FLASH_INIT_SUCCESS;
}

FLASH_READ_STATUS_T flash_read(uint32_t from, uint32_t len, uint8_t *p_buf)
{
	uint32_t retlen = 0;
	SPI_NAND_FLASH_RTN_T status = SPI_NAND_FLASH_RTN_NO_ERROR;
	int read_status = 0;
#if defined (TCSUPPORT_CPU_EN7581) || defined(TCSUPPORT_CPU_AN7583) || defined (TCSUPPORT_CPU_AN7552)
	if (hwtrap->is_spi_nand_device_ecc ||
		hwtrap->is_spi_nand_ctrl_ecc ||
		hwtrap->is_parallel_nand) {
		if(!uartDisable){
		NOTICE("2-5-1\n");
		}
		read_status = nandflash_read(from, len, &retlen, p_buf, &status);

		if(read_status != 0)
		{
			return FLASH_READ_STATUS_INCORRECT;
		}
	} 
#else
	if (IS_NANDFLASH) {
 		read_status = nandflash_read(from, len, &retlen, p_buf, &status);
		if(read_status != 0)
		{
			return FLASH_READ_STATUS_INCORRECT;
		}
 	}

#endif


#ifdef TCSUPPORT_EMMC
	else if(hwtrap->is_emmc) {
		uint32_t remain_len = 0;
		uint8_t *buf_tmp = p_buf;
		uint32_t nonalignment_from = 0, nonalignment_len = 0, offset = 0, 
			blknr_start = 0, blknr_end = 0, blkcntIdx = 0;
		
		/* blknr_start is mmc block size alignment
		 * if "from" address is none mmc block size alignment
		 * we need memory shift copy.
		 */
		remain_len = len;
		nonalignment_from = from;
		offset = nonalignment_from % MMC_BLOCK_SIZE;

		//printf("%s %d:from:0x%x, len:0x%x, offset:0x%x\n", __func__, __LINE__, from, len, offset);
		//printf("%s %d:nonalignment_from:0x%x\n", __func__, __LINE__, nonalignment_from);

		if(offset != 0) {
			blknr_start = nonalignment_from / MMC_BLOCK_SIZE;
			//printf("%s %d:blknr_start:0x%x\n", __func__, __LINE__, blknr_start);
			retlen = mmc_read_blocks(blknr_start, (uintptr_t)emmc_blk_buf, MMC_BLOCK_SIZE);
			if(retlen != MMC_BLOCK_SIZE) {
				ERROR("%s %d:emmc read fail.\n", __func__, __LINE__);
				return FLASH_READ_STATUS_INCORRECT;
			}
			
			nonalignment_len = (MMC_BLOCK_SIZE - offset);
			if(remain_len < nonalignment_len) {
				nonalignment_len = remain_len;
			}

			//printf("%s %d:nonalignment_len:0x%x\n", __func__, __LINE__, nonalignment_len);
			
			memcpy(buf_tmp, emmc_blk_buf + offset, nonalignment_len);
			remain_len -= nonalignment_len;
			nonalignment_from += nonalignment_len;
			buf_tmp += nonalignment_len;
		}

		//printf("%s %d:nonalignment_from:0x%x, remain_len:0x%x\n", __func__, __LINE__, nonalignment_from, remain_len);

		if(remain_len && (remain_len >= MMC_BLOCK_SIZE)) {
			blknr_start = nonalignment_from / MMC_BLOCK_SIZE;
			blknr_end = (nonalignment_from + remain_len) / MMC_BLOCK_SIZE;
			//printf("%s %d:blknr_start:0x%x, blknr_end:0x%x\n", __func__, __LINE__, blknr_start, blknr_end);
			for(blkcntIdx = blknr_start; blkcntIdx < blknr_end; blkcntIdx++) {
				//printf("%s %d:blkcntIdx:0x%x\n", __func__, __LINE__, blkcntIdx);
				retlen = mmc_read_blocks(blkcntIdx, (uintptr_t)emmc_blk_buf, MMC_BLOCK_SIZE);
				if(retlen != MMC_BLOCK_SIZE) {
					ERROR("%s %d:emmc read fail.\n", __func__, __LINE__);
					return FLASH_READ_STATUS_INCORRECT;
				}
				memcpy(buf_tmp, emmc_blk_buf, MMC_BLOCK_SIZE);
				remain_len -= MMC_BLOCK_SIZE;
				nonalignment_from += MMC_BLOCK_SIZE;
				buf_tmp += MMC_BLOCK_SIZE;
			}
		}

		//printf("%s %d:nonalignment_from:0x%x, remain_len:0x%x\n", __func__, __LINE__, nonalignment_from, remain_len);

		if(remain_len) {
			blknr_start = nonalignment_from / MMC_BLOCK_SIZE;
			//printf("%s %d:blknr_start:0x%x\n", __func__, __LINE__, blknr_start);
			retlen = mmc_read_blocks(blknr_start, (uintptr_t)emmc_blk_buf, MMC_BLOCK_SIZE);
			if(retlen != MMC_BLOCK_SIZE) {
				ERROR("%s %d:emmc read fail.\n", __func__, __LINE__);
				return FLASH_READ_STATUS_INCORRECT;
			}
			memcpy(buf_tmp, emmc_blk_buf, remain_len);
			buf_tmp += remain_len;
		}

		dram_dump(p_buf, len);
	} 
#endif
	else {
		NOTICE("2-5-2\n");
		spi_nor_read(p_buf, from, len);
	}

	return FLASH_READ_STATUS_CORRECT;
}

int flash_erase(uint32_t addr, uint32_t size)
{
#ifdef TCSUPPORT_EMMC
	int lba = 0;
	uint32_t retlen = 0;
#endif

#if defined (TCSUPPORT_CPU_EN7581) || defined(TCSUPPORT_CPU_AN7583) || defined (TCSUPPORT_CPU_AN7552)
	if (hwtrap->is_spi_nand_device_ecc ||
		hwtrap->is_spi_nand_ctrl_ecc ||
		hwtrap->is_parallel_nand) {
		return nandflash_erase(addr, size);
	}
#else
	if (NANDFLASH_HWTRAP){
 		return nandflash_erase(addr, size);
 	}
#endif

#ifdef TCSUPPORT_EMMC
	else if(hwtrap->is_emmc) {
		assert(((addr & MMC_BLOCK_MASK) == 0U) &&
   		((size & MMC_BLOCK_MASK) == 0U));
		/* lba is mmc block size(512bytes) alignment
		 */
		lba = addr / MMC_BLOCK_SIZE;
		retlen = mmc_erase_blocks(lba, size);
		if(retlen != size) {
			ERROR("emmc erase fail: retlen:%u, size:%u.\n", retlen, size);
			return -1;
		}
	}
#endif
	else {
		spi_nor_erase(addr, size);
	}
	return 0;
}

int flash_write(uint32_t to, uint32_t len, uint8_t *p_buf)
{
	uint32_t retlen = 0;
#ifdef TCSUPPORT_EMMC
	int lba = 0;
#endif

#if defined (TCSUPPORT_CPU_EN7581) || defined(TCSUPPORT_CPU_AN7583) || defined (TCSUPPORT_CPU_AN7552)
	if (hwtrap->is_spi_nand_device_ecc ||
		hwtrap->is_spi_nand_ctrl_ecc ||
		hwtrap->is_parallel_nand) {
		return nandflash_write(to, len, &retlen, p_buf);
	}
#else
	if (NANDFLASH_HWTRAP){
		return nandflash_write(to, len, &retlen, p_buf);
	}
#endif

#ifdef TCSUPPORT_EMMC
	else if(hwtrap->is_emmc) {
		assert(((to & MMC_BLOCK_MASK) == 0U) &&
	       		((len & MMC_BLOCK_MASK) == 0U));
		
		/* lba is mmc block size alignment
		 */
		lba = to / MMC_BLOCK_SIZE;
		retlen = mmc_write_blocks(lba, (const uintptr_t)p_buf, len);
		if(retlen != len) {
			ERROR("emmc write fail: retlen:%u, size:%u.\n", retlen, len);
			return -1;
		}
	}
#endif
	else {
		spi_nor_write(p_buf, to, len);
	}
	return 0;
}


