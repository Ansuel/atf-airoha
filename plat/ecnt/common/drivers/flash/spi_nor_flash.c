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
#include <stdio.h>

#include "spi_nor_flash.h"
#include "spi/spi_controller.h"
#include <en7523_def.h>

#define _SPI_NOR_ENABLE_MANUAL_MODE				SPI_CONTROLLER_Enable_Manual_Mode
#define _SPI_NOR_WRITE_ONE_BYTE					SPI_CONTROLLER_Write_One_Byte
#define _SPI_NOR_WRITE_ONE_BYTE_WITH_CMD		SPI_CONTROLLER_Write_One_Byte_With_Cmd
#define _SPI_NOR_WRITE_NBYTE					SPI_CONTROLLER_Write_NByte
#define _SPI_NOR_READ_NBYTE						SPI_CONTROLLER_Read_NByte
#define _SPI_NOR_CHIP_SELECT_HIGH				SPI_CONTROLLER_Chip_Select_High
#define _SPI_NOR_CHIP_SELECT_LOW				SPI_CONTROLLER_Chip_Select_Low

#define _SPI_NOR_OP_RD_STATUS1					(0x05)	/* Read Status 1 */
#define _SPI_NOR_OP_WR_STATUS1					(0x01)	/* Write Status 1 */
#define _SPI_NOR_OP_WRITE_EN					(0x06)	/* Write Enable */
#define _SPI_NOR_OP_WRITE_DS					(0x04)	/* Write Disable */
#define _SPI_NOR_OP_READ_SINGLE					(0x03)	/* Read data of SPI NOR chip, single speed */
#define _SPI_NOR_OP_WRITE_SINGLE				(0x02)	/* Write data of SPI NOR chip, single speed */
#define _SPI_NOR_OP_ERASE						(0xD8)	/* Rease of SPI NOR chip */

#define _SPI_NOR_STATUS_WIP						(0x01)	/* Write-In-Progress */
#define _SPI_NOR_STATUS_WEL						(0x02)	/* Write-Enable-Latch */

#define _SPI_NOR_PAGE_SIZE						(0x100)

void spiflash_read_status(uint8_t *status)
{
	_SPI_NOR_ENABLE_MANUAL_MODE();

	_SPI_NOR_CHIP_SELECT_LOW();
	_SPI_NOR_WRITE_ONE_BYTE( _SPI_NOR_OP_RD_STATUS1 );
	_SPI_NOR_READ_NBYTE( status, 0x1, SPI_CONTROLLER_SPEED_SINGLE );
	_SPI_NOR_CHIP_SELECT_HIGH();
}

void spiflash_disable_write(void)
{
	uint8_t status = 0;

	_SPI_NOR_ENABLE_MANUAL_MODE();
	
	do
	{
		spiflash_read_status(&status);
	} while (status & _SPI_NOR_STATUS_WIP);

	_SPI_NOR_CHIP_SELECT_LOW();
	_SPI_NOR_WRITE_ONE_BYTE( _SPI_NOR_OP_WRITE_DS );
	_SPI_NOR_CHIP_SELECT_HIGH();

}

void spiflash_enable_write(void)
{
	uint8_t status = 0;

	_SPI_NOR_ENABLE_MANUAL_MODE();

	do
	{
		_SPI_NOR_CHIP_SELECT_LOW();
		_SPI_NOR_WRITE_ONE_BYTE( _SPI_NOR_OP_WRITE_EN );
		_SPI_NOR_CHIP_SELECT_HIGH();

		spiflash_read_status(&status);
	} while (!(status & _SPI_NOR_STATUS_WEL));
}

void spiflash_erase_oneblock(uint32_t addr, int32_t len)
{
	spiflash_enable_write();

	_SPI_NOR_CHIP_SELECT_LOW();

	_SPI_NOR_WRITE_ONE_BYTE( _SPI_NOR_OP_ERASE );

	if (mmio_read_32(EN7523_SFC_STRAP) & (0x1))
	{
		_SPI_NOR_WRITE_ONE_BYTE( ((addr >> 24 ) &(0xff)) );
	}

	_SPI_NOR_WRITE_ONE_BYTE( ((addr >> 16 ) &(0xff)) );
	_SPI_NOR_WRITE_ONE_BYTE( ((addr >> 8 ) &(0xff)) );
	_SPI_NOR_WRITE_ONE_BYTE( ((addr >> 0 ) &(0xff)) );

	_SPI_NOR_CHIP_SELECT_HIGH();

	spiflash_disable_write();
}

void spi_nor_erase(uint32_t addr, int32_t len)
{
	if (addr & (0x10000 - 1))
		return;
	if (len & (0x10000 - 1))
		return;

	while (len)
	{
		spiflash_erase_oneblock(addr, len);

		addr += 0x10000;
		len -= 0x10000;

		printf(".");
	}
	printf("\n");
}


void spi_nor_write_page(uint8_t *p_data, uint32_t addr, int32_t len)
{
	spiflash_enable_write();

	_SPI_NOR_CHIP_SELECT_LOW();

	_SPI_NOR_WRITE_ONE_BYTE( _SPI_NOR_OP_WRITE_SINGLE );

	if (mmio_read_32(EN7523_SFC_STRAP) & (0x1))
	{
		_SPI_NOR_WRITE_ONE_BYTE( ((addr >> 24 ) &(0xff)) );
	}

	_SPI_NOR_WRITE_ONE_BYTE( ((addr >> 16 ) &(0xff)) );
	_SPI_NOR_WRITE_ONE_BYTE( ((addr >> 8 ) &(0xff)) );
	_SPI_NOR_WRITE_ONE_BYTE( ((addr >> 0 ) &(0xff)) );

	_SPI_NOR_WRITE_NBYTE(p_data, len, SPI_CONTROLLER_SPEED_SINGLE);

	_SPI_NOR_CHIP_SELECT_HIGH();

	spiflash_disable_write();
}

void spi_nor_write(uint8_t *p_data, uint32_t addr, int32_t len)
{
	int32_t tmp_len = 0;

	while (len)
	{

		if (len > _SPI_NOR_PAGE_SIZE)
		{
			tmp_len = _SPI_NOR_PAGE_SIZE;
		}
		else
		{
			tmp_len = len;
		}

		spi_nor_write_page(p_data, addr, tmp_len);

		p_data += tmp_len;
		addr += tmp_len;
		len -= tmp_len;

		printf(".");
	}
	printf("\n");
}


void spi_nor_read(uint8_t *p_data, uint32_t addr, int32_t len)
{
	_SPI_NOR_ENABLE_MANUAL_MODE();

	_SPI_NOR_CHIP_SELECT_LOW();

	_SPI_NOR_WRITE_ONE_BYTE( _SPI_NOR_OP_READ_SINGLE );

	if (mmio_read_32(EN7523_SFC_STRAP) & (0x1))
	{
		_SPI_NOR_WRITE_ONE_BYTE( ((addr >> 24 ) &(0xff)) );
	}

	_SPI_NOR_WRITE_ONE_BYTE( ((addr >> 16 ) &(0xff)) );
	_SPI_NOR_WRITE_ONE_BYTE( ((addr >> 8 ) &(0xff)) );
	_SPI_NOR_WRITE_ONE_BYTE( ((addr >> 0 ) &(0xff)) );		

	_SPI_NOR_READ_NBYTE(p_data, len, SPI_CONTROLLER_SPEED_SINGLE);

	_SPI_NOR_CHIP_SELECT_HIGH();
}

uint8_t spi_nor_read_byte(uint32_t addr)
{
	uint8_t data;

	spi_nor_read(&data, addr, 1);

	return data;
}

void spi_nor_init(void)
{
	spi_set_clock_speed(25);

	spiflash_enable_write();
	_SPI_NOR_CHIP_SELECT_LOW();
	_SPI_NOR_WRITE_ONE_BYTE( _SPI_NOR_OP_WR_STATUS1 );
	_SPI_NOR_WRITE_ONE_BYTE( 0 );
	_SPI_NOR_CHIP_SELECT_HIGH();
	spiflash_disable_write();
}

/* End of [spi_nor_flash.c] package */
