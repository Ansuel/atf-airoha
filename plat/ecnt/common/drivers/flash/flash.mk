#
# Copyright (c) 2017-2020, ARM Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

FLASH_SRCS	:=	$(addprefix plat/ecnt/common/drivers/flash/,	\
				flashhal.c										\
				spi_nor_flash.c									\
				bmt.c											)

BL2_CFLAGS	+= -march=armv8-a+crc
FLASH_SRCS	+= $(addprefix drivers/io/ubi/,		\
				io_ubi.c						\
				ubispl.c						\
				crc32.c)
FLASH_SRCS	+= $(addprefix plat/ecnt/en7523/,	\
				bl2_boot_nand_ubi.c)

FLASH_SRCS	+= $(addprefix plat/ecnt/common/drivers/flash/,	\
				spi_nand_flash.c				\
				spi_nand_flash_table.c			\
				spi_controller.c				\
				spi_ecc.c						\
				spi_nfi.c)

ifneq ($(TCSUPPORT_PARALLEL_NAND),)
ifeq ($(TCSUPPORT_ATF_RELEASE),)
FLASH_SRCS	+=	$(KERNEL_EXT_SPI_NAND_DIR)/parallel_nand_flash.c	\
				$(KERNEL_EXT_SPI_NAND_DIR)/parallel_nand_flash_table.c
else
FLASH_SRCS	+=	$(addprefix drivers/mtd/chips/,	\
				parallel_nand_flash.c			\
				parallel_nand_flash_table.c)
endif
endif

ifeq ($(TCSUPPORT_ATF_RELEASE),)
FLASH_REBUILD_SRCS += $(KERNEL_EXT_SPI_NAND_DIR)/spi_nand_flash_table.c
else
FLASH_REBUILD_SRCS +=	$(addprefix drivers/mtd/chips/,	\
						spi_nand_flash_table.c)
endif

#eMMC
ifneq ($(TCSUPPORT_EMMC),)
FLASH_SRCS	+=	$(addprefix drivers/mmc/,	\
				mmc.c)
FLASH_SRCS	+=	$(addprefix plat/ecnt/common/drivers/flash/,	\
				mtk-sd.c)
endif
