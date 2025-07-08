 /***************************************************************************************
 *      Copyright(c) 2014 ECONET Incorporation All rights reserved.
 *
 *      This is unpublished proprietary source code of ECONET Incorporation
 *
 *      The copyright notice above does not evidence any actual or intended
 *      publication of such source code.
 ***************************************************************************************
 */

#ifndef __SPI_NOR_FLASH_H__
#define __SPI_NOR_FLASH_H__

#include <stdint.h>

void spi_nor_init(void);
void spi_nor_erase(uint32_t addr, int32_t len);
void spi_nor_write(uint8_t *p_data, uint32_t addr, int32_t len);
void spi_nor_read(uint8_t *p_data, uint32_t addr, int32_t len);
uint8_t spi_nor_read_byte(uint32_t addr);

#endif
/* End of [spi_nor_flash.h] package */

