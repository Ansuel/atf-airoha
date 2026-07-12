// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2024 AIROHA Inc
*/

/*======================================================================================
 * MODULE NAME: spi
 * FILE NAME: spi_nand_flash.c
 * DATE: 2014/11/21
 * VERSION: 1.00
 * PURPOSE: To Provide SPI NAND Access interface.
 * NOTES:
 *
 * AUTHOR :          REVIEWED by
 *
 * FUNCTIONS
 *
 *      SPI_NAND_Flash_Init             To provide interface for SPI NAND init. 
 *      SPI_NAND_Flash_Write_Nbyte      To provide interface for Write N Bytes into SPI NAND Flash. 
 *      SPI_NAND_Flash_Read_Byte        To provide interface for read 1 Bytes from SPI NAND Flash. 
 *      SPI_NAND_Flash_Read_DWord       To provide interface for read Double Word from SPI NAND Flash.  
 *      SPI_NAND_Flash_Read_NByte       To provide interface for Read N Bytes from SPI NAND Flash. 
 *      SPI_NAND_Flash_Erase            To provide interface for Erase SPI NAND Flash. 
 * 
 * DEPENDENCIES
 *
 * * $History: $
 * MODIFICTION HISTORY:
 * Version 1.00 - Date 2014/11/21 
 * ** This is the first versoin for creating to support the functions of
 *    current module.
 *
 *======================================================================================
 */

/* INCLUDE FILE DECLARATIONS --------------------------------------------------------- */
#include <linux/version.h>

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/gen_probe.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/dma-mapping.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/time.h>
#include <linux/miscdevice.h>
#include <linux/scatterlist.h>
#include <linux/kthread.h>
#include <linux/math64.h>
#include <linux/random.h>
#include <asm/tc3162/tc3162.h>
#include <asm/string.h>
#include <asm/cacheflush.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
#include <linux/uaccess.h>
#endif
#include <asm/uaccess.h>
#include <asm/tc3162/tc3162.h>

#if (defined(TCSUPPORT_CPU_EN7516) || defined(TCSUPPORT_CPU_EN7527) || defined(TCSUPPORT_CPU_EN7580)) && defined(TCSUPPORT_AUTOBENCH)	
#include <ecnt_hook/ecnt_hook.h>
#include <ecnt_hook/ecnt_hook_spi_nand.h>
#include "flash_test.h"
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
#include <uapi/mtd/mtd-abi.h>
#endif
#include <linux/ecnt_utility.h>


#if defined (TCSUPPORT_GPON_DUAL_IMAGE) || defined (TCSUPPORT_EPON_DUAL_IMAGE)
#include "flash_layout/tc_partition.h"
#endif

#define SPI_NAND_FLASH_DEBUG

#else
#include <flashhal.h>
#include <asm/tc3162.h>
#include <asm/system.h>
#endif //#if defined(TCSUPPORT_2_6_36_KERNEL) || defined(TCSUPPORT_3_18_21_KERNEL)

#include <asm/io.h>
#include <linux/types.h>

#if defined(SPI_NAND_FLASH_DEBUG)
#include <stdarg.h>
#endif
#if defined(TCSUPPORT_DSL_PHYMODE)
#include <dsl_phy_global/fttdp_inic.h>
#endif

#include <ecnt_spi_nand_flash.h>
#include <ecnt_spi_controller.h>
#include <ecnt_spi_nfi.h>
#include <ecnt_spi_ecc.h>
#include <ecnt_nand_flash_otp.h>
#include <ecnt_pkgid.h>

#if defined(CONFIG_ECNT_UBOOT)
#if !defined(IMAGE_BL2)
#include <asm/string.h>
#else
#define flush_dcache_range(x, y) flush_dcache_range(x, (y - x))
#endif
#endif

// #if	defined(TCSUPPORT_NAND_BMT)
#if (!defined(LZMA_IMG) && !defined(BOOTROM_EXT)) || defined(TCSUPPORT_BB_256KB)
#if defined(TCSUPPORT_OPENWRT)
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
#include "../econet/bmt.h"
#else
#include "bmt.h"
#endif
#else
#include "bmt.h"
#endif
#endif
// #endif


/* NAMING CONSTANT DECLARATIONS ------------------------------------------------------ */

/* SPI NAND Command Set */
#define _SPI_NAND_OP_GET_FEATURE					0x0F	/* Get Feature */
#define _SPI_NAND_OP_SET_FEATURE					0x1F	/* Set Feature */
#define _SPI_NAND_OP_PAGE_READ						0x13	/* Load page data into cache of SPI NAND chip */
#define _SPI_NAND_OP_READ_FROM_CACHE_SINGLE			0x03	/* Read data from cache of SPI NAND chip, single speed*/
#define _SPI_NAND_OP_READ_FROM_CACHE_DUAL			0x3B	/* Read data from cache of SPI NAND chip, dual speed*/
#define _SPI_NAND_OP_READ_FROM_CACHE_QUAD			0x6B	/* Read data from cache of SPI NAND chip, quad speed*/
#define _SPI_NAND_OP_WRITE_ENABLE					0x06	/* Enable write data to  SPI NAND chip */
#define _SPI_NAND_OP_WRITE_DISABLE					0x04	/* Reseting the Write Enable Latch (WEL) */
#define _SPI_NAND_OP_PROGRAM_LOAD_SINGLE			0x02	/* Write data into cache of SPI NAND chip with cache reset, single speed */
#define _SPI_NAND_OP_PROGRAM_LOAD_QUAD				0x32	/* Write data into cache of SPI NAND chip with cache reset, quad speed */
#define _SPI_NAND_OP_PROGRAM_LOAD_RAMDOM_SINGLE		0x84	/* Write data into cache of SPI NAND chip, single speed */
#define _SPI_NAND_OP_PROGRAM_LOAD_RAMDON_QUAD		0x34	/* Write data into cache of SPI NAND chip, quad speed */

#define _SPI_NAND_OP_PROGRAM_EXECUTE		0x10	/* Write data from cache into SPI NAND chip */
#define _SPI_NAND_OP_READ_ID				0x9F	/* Read Manufacture ID and Device ID */
#define _SPI_NAND_OP_BLOCK_ERASE			0xD8	/* Erase Block */
#define _SPI_NAND_OP_RESET					0xFF	/* Reset */
#define _SPI_NAND_OP_DIE_SELECT				0xC2	/* Die Select */

/* SPI NAND register address of command set */
#define _SPI_NAND_ADDR_MANUFACTURE_ID		0x00	/* Address of Manufacture ID */
#define _SPI_NAND_ADDR_DEVICE_ID			0x01	/* Address of Device ID */

/* SPI NAND value of register address of command set */
#define _SPI_NAND_VAL_DISABLE_PROTECTION	0x0		/* Value for disable write protection */
#define _SPI_NAND_VAL_ENABLE_PROTECTION		0x38	/* Value for enable write protection */
#define _SPI_NAND_VAL_OIP					0x1		/* OIP = Operaton In Progress */
#define _SPI_NAND_VAL_WEL					0x2		/* WEL = Write enable bit */
#define _SPI_NAND_VAL_ERASE_FAIL			0x4		/* E_FAIL = Erase Fail */
#define _SPI_NAND_VAL_PROGRAM_FAIL			0x8		/* P_FAIL = Program Fail */

/* Others Define */
#define _SPI_NAND_LEN_ONE_BYTE				(1)
#define _SPI_NAND_LEN_TWO_BYTE				(2)
#define _SPI_NAND_LEN_THREE_BYTE			(3)
#define _SPI_NAND_BLOCK_ROW_ADDRESS_OFFSET	(6)

#define _SPI_NFI_CHECK_ECC_DONE_MAX_TIMES	(1000000)
#define CACHE_LINE_SIZE						(128)

#define INVALID_BLOCK_INDEX				(0xFFFFFFFF)

#ifdef BLOCK_SIZE
#undef BLOCK_SIZE
#endif

#ifdef PAGE_SIZE
#undef PAGE_SIZE
#endif

#define BLOCK_SIZE		(_current_flash_info_t.erase_size)
#define PAGE_SIZE		(_current_flash_info_t.page_size)
#define PAGE_CNT_PER_BLOCK	(BLOCK_SIZE / PAGE_SIZE)


#define PAGE_SIZE_MAGIC_FLASH_ADDR			(0XFFB0)
#define PAGE_SIZE_MAGIC						(0x50414745)

#if	defined(TCSUPPORT_NAND_BMT)
#if (!defined(LZMA_IMG)) || defined(TCSUPPORT_BB_256KB)
#define ERASE_STATISTICS
#endif
#endif

#ifdef ERASE_STATISTICS
#define MAX_ERASE_CNT_SIZE				(4)
#define INVALID_ERASE_CNT				(0xFFFFFFFF)
#else
#define MAX_ERASE_CNT_SIZE				(0)
#endif

/* oob layout: 
 *	4bytes for bad block indicator
 *	nbytes for linux 
 *	4bytes for erase statistics
 */
#define KERNEL_BAD_BLOCK_INDICATOR_OFFSET	(0)
#define KERNEL_BAD_BLOCK_INDICATOR_SIZE		(2)
#define ARHT_BAD_BLOCK_INDICATOR_OFFSET		(OOB_INDEX_OFFSET)
#define ARHT_BAD_BLOCK_INDICATOR_SIZE		(OOB_INDEX_SIZE)

#if defined(TCSUPPORT_CPU_ARMV8)
#if (KERNEL_BAD_BLOCK_INDICATOR_OFFSET + KERNEL_BAD_BLOCK_INDICATOR_SIZE != ARHT_BAD_BLOCK_INDICATOR_OFFSET)
#error ARHT_BAD_BLOCK_INDICATOR_OFFSET error
#endif
#endif

#define MAX_LINUX_USE_OOB_SIZE			(_current_flash_info_t.oob_free_layout->oobsize)
#define MAX_USE_OOB_SIZE				(_current_flash_info_t.oob_size)

#if defined(IMAGE_BL2)
/* BL2 has DRAM usage limit so cannot create array with number 4096, but still need to know erase count and write protect.	*/
/* Use original setting to protect flash and save erase count. 																*/
#define MAX_BLOCK (2048)
#else
#define MAX_BLOCK (4096)
#endif

#ifdef ERASE_STATISTICS
static unsigned int 					erase_cnt_offset = 0;
#define MAX_ERASE_CNT_START_OFFSET		(erase_cnt_offset)
u32 erase_statistic_cnt[MAX_BLOCK];
u8	isSupportEraseCntStatistic = 0;
u8								erase_statictic_page_data[_SPI_NAND_PAGE_SIZE];	
u8								erase_statictic_oob_data[_SPI_NAND_OOB_SIZE];
#endif

#define MIN(a,b)			((a) < (b) ? (a) : (b))

#if defined(TCSUPPORT_CPU_ARMV8) || defined(CONFIG_ECNT_UBOOT) /* U-boot likes MIPS bootram, but no K0_TO_K1 & K1_TO_PHY */
//#define K0_TO_K1
#define K1_TO_PHY
#define dma_cache_inv(start,size)
#else
//#define K0_TO_K1(x)			(((uint32)x) | 0xa0000000)
#define K1_TO_PHY(x)		(((uint32)x) & 0x1fffffff)
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
/* kernel */
static DEFINE_SPINLOCK(spinandLock);

#define _SPI_NAND_SEMAPHORE_LOCK()			spin_lock_irqsave(&spinandLock, spinand_spinlock_flags)	/* Disable interrupt */
#define _SPI_NAND_SEMAPHORE_UNLOCK()		spin_unlock_irqrestore(&spinandLock, spinand_spinlock_flags)	/* Enable interrupt  */

struct spi_chip_info {
	struct spi_flash_info *flash;
	void (*destroy)(struct spi_chip_info *chip_info);

	u32 (*read)(struct map_info *map, u32 from, u32 to, u32 size);
	u32 (*read_manual)(struct mtd_info *mtd, unsigned long from, unsigned char *buf, unsigned long len);
	u32 (*write)(struct mtd_info *mtd, u32 from, u32 to, u32 size);
	u32 (*erase)(struct mtd_info *mtd, u32 addr);
};

extern unsigned char (*ranand_read_byte)(unsigned long long, SPI_NAND_FLASH_RTN_T *status);
extern unsigned long (*ranand_read_dword)(unsigned long long, SPI_NAND_FLASH_RTN_T *status);
extern void dump_bmt_info(int msg_lv, bmt_struct *bmt);
extern void dump_bbt_info(int msg_lv, init_bbt_struct *bbt);
extern int check_default_bad(u32 block_idx);
extern int check_runtime_bad(u32 block_idx);
extern int init_spi_ctrl_base(struct platform_device *pdev);
extern int init_spi_nfi2spi_base(struct platform_device *pdev);
extern int init_spi_ecc_base(struct platform_device *pdev);
extern u32 GET_NP_SCU_EMMC(void);

struct mtd_info 	*spi_nand_mtd;

struct _SPI_NAND_FLASH_RW_TEST_T {
	u32 times;
	u32 block_idx;
};

static struct _SPI_NAND_FLASH_RW_TEST_T rw_test_param;
static int _SPI_NAND_WRITE_FAIL_TEST_FLAG = 0;
static int _SPI_NAND_ERASE_FAIL_TEST_FLAG = 0;
static int _SPI_NAND_READ_EXCEEDED_THRESHOLD_TEST_FLAG = 0;

#define ERASE_WRITE_CNT_LOG
#define MULTI_WRITE_DBG

#ifdef ERASE_WRITE_CNT_LOG
typedef enum{
	SPI_NAND_FLASH_ERASE_WRITE_LOG_DISABLE = 0,
	SPI_NAND_FLASH_ERASE_WRITE_LOG_ENABLE,
} ERASE_WRITE_LOG_T;

ERASE_WRITE_LOG_T erase_write_flag;
unsigned long b_erase_cnt[MAX_BLOCK];
unsigned long b_write_total_cnt[MAX_BLOCK];
unsigned long b_write_cnt_per_erase[MAX_BLOCK];
#endif

#ifdef MULTI_WRITE_DBG
typedef enum{
	SPI_NAND_FLASH_MULTI_WRITE_DISABLE = 0,
	SPI_NAND_FLASH_MULTI_WRITE_ENABLE,
} MULTI_WRITE_LOG_T;

typedef struct {
	unsigned char	isProgramed		:	1;
	unsigned char	illegal_program	:	7;
} SPI_NAND_PAGE_INFO_T;

MULTI_WRITE_LOG_T multi_write_dbg_flag;
SPI_NAND_PAGE_INFO_T page_info[MAX_BLOCK][128];
#endif

#ifdef TCSUPPORT_CPU_ARMV8
struct ecnt_nand {
	struct device *dev;
};

struct ecnt_nand *ecnt_nand = NULL;
dma_addr_t rdma_addr;
dma_addr_t wdma_addr;
#else
char *rdma_addr;
char *wdma_addr;
#endif

#else
/* bootloader */
u8 *rdma_addr;
u8 *wdma_addr;

#if !defined(CONFIG_ECNT_UBOOT)
extern void * memcpy4(void * dest, const void *src, size_t count);
#define memcpy	memcpy4
#endif

#define _SPI_NAND_SEMAPHORE_LOCK()
#define _SPI_NAND_SEMAPHORE_UNLOCK()

unsigned int print_dot = 0;
int test_write_fail_flag = 0;
int en_oob_write = 0;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
extern unsigned char *get_block_buf(void);
#endif
#endif

/* FUNCTION DECLARATIONS ------------------------------------------------------ */
#if (!defined(TCSUPPORT_CPU_ARMV8)) || defined(TCSUPPORT_CPU_ARMV8_64) /* compile error for ARM32 due to "memset" becoming a Macro in arch/arm/include/asm/string.h */
#if !defined(IMAGE_BL2)
extern void * memset(void * s, int c, size_t count);
#endif
#endif
static void spi_nand_direct_select_die(u32 die);
SPI_NAND_FLASH_RTN_T spi_nand_erase_block(u32 block_index);
static SPI_NAND_FLASH_RTN_T spi_nand_write_page(u32 page_number, 
												u32 data_offset,
											  	const u8  *ptr_data, 
											  	u32 data_len, 
											  	u32 oob_offset,
												u8  *ptr_oob , 
												u32 oob_len,
											    SPI_NAND_FLASH_WRITE_SPEED_MODE_T speed_mode);

struct spi_nand_flash_ooblayout ooblayout_feature7 = {
	.oobsize = 3, 
	.oobfree = {{8, 3}, {0, 0}}
};

/* MACRO DECLARATIONS ---------------------------------------------------------------- */
#define _SPI_NAND_BLOCK_ALIGNED_CHECK( __addr__,__block_size__) ((__addr__) & ( __block_size__ - 1))
#define _SPI_NAND_GET_DEVICE_INFO_PTR		&(_current_flash_info_t)

/* Porting Replacement */
#if !defined(SPI_NAND_FLASH_DEBUG)
	#define _SPI_NAND_PRINTF(args...)
	#define _SPI_NAND_DEBUG_PRINTF(args...)
	#define _SPI_NAND_DEBUG_PRINTF_HEX(args...)
	#define _SPI_NAND_DEBUG_PRINTF_ARRAY(args...)
#else
	#ifdef SPRAM_IMG
		#define _SPI_NAND_PRINTF(fmt, args...)		prom_puts(fmt)		/* Always print information */
	#else
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
		#define _SPI_NAND_PRINTF					printk
		#else
		#define _SPI_NAND_PRINTF					prom_printf			/* Always print information */
		#endif
	#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
		#define _SPI_NAND_DEBUG_PRINTF_HEX			spi_nand_flash_debug_printf_hex
	#endif
	#define _SPI_NAND_DEBUG_PRINTF				spi_nand_flash_debug_printf
	#define _SPI_NAND_DEBUG_PRINTF_ARRAY		spi_nand_flash_debug_printf_array
#endif
#if defined(TCSUPPORT_PARALLEL_NAND)
extern SPI_NAND_FLASH_RTN_T parallel_nand_dma_read(u32 read_addr, u32 page_number, unsigned long *p_data);
extern SPI_NAND_FLASH_RTN_T parallel_nand_dma_write(u32 write_addr, u32 page_number, unsigned long *p_data, u32 oob_len, u8 *ptr_oob);
extern SPI_NAND_FLASH_RTN_T parallel_nand_erase(u32 page_number);
extern SPI_NAND_FLASH_RTN_T parallel_nand_probe(struct SPI_NAND_FLASH_INFO_T *ptr_rtn_device_t);
extern SPI_CONTROLLER_RTN_T spi_nand_enable_manual_mode(void);
extern SPI_NAND_FLASH_RTN_T parallel_nand_protocol_get_status(u8 *p_status);
extern SPI_NAND_FLASH_RTN_T parallel_nand_protocol_reset (void);

#define PARALLEL_NAND_READY (0x60)
#define _SPI_NAND_ENABLE_MANUAL_MODE		spi_nand_enable_manual_mode
#else
#define _SPI_NAND_ENABLE_MANUAL_MODE		SPI_CONTROLLER_Enable_Manual_Mode
#endif
#define _SPI_NAND_WRITE_ONE_BYTE			SPI_CONTROLLER_Write_One_Byte
#define _SPI_NAND_WRITE_ONE_BYTE_WITH_CMD	SPI_CONTROLLER_Write_One_Byte_With_Cmd
#define _SPI_NAND_WRITE_NBYTE				SPI_CONTROLLER_Write_NByte
#define _SPI_NAND_READ_NBYTE				SPI_CONTROLLER_Read_NByte
#define _SPI_NAND_READ_CHIP_SELECT_HIGH		SPI_CONTROLLER_Chip_Select_High
#define _SPI_NAND_READ_CHIP_SELECT_LOW		SPI_CONTROLLER_Chip_Select_Low

#if defined(CONFIG_ECNT_UBOOT) && !defined(CONFIG_TPL_BUILD)
#undef _SPI_NAND_PRINTF
#define _SPI_NAND_PRINTF				prom_printf
#endif

#if !defined(BOOTROM_EXT) || defined(TCSUPPORT_BB_256KB)
struct ra_nand_chip ra;
struct nand_info flashInfo;
flashdev_info devinfo;
#endif

u8 *dma_read_page = NULL;
#if	!defined(LZMA_IMG) || defined(TCSUPPORT_BB_256KB) 
u8 *dma_write_page = NULL;
#endif

#if defined(BOOTROM_EXT) || defined(IMAGE_BL2)
u8 	*tmp_dma_read_page = (u8 *)0x80000000; //9KB
u8 	*tmp_dma_write_page = (u8 *)0x80000000; //9KB
u8	_current_cache_page[_SPI_NAND_CACHE_SIZE];
u8	_current_cache_page_data[_SPI_NAND_PAGE_SIZE];
u8	_current_cache_page_oob[_SPI_NAND_OOB_SIZE];
u8	_current_cache_page_oob_mapping[_SPI_NAND_OOB_SIZE];
#else
u8 	tmp_dma_read_page[_SPI_NAND_CACHE_SIZE + CACHE_LINE_SIZE];
#if	!defined(LZMA_IMG) || defined(TCSUPPORT_BB_256KB)
u8	tmp_dma_write_page[_SPI_NAND_CACHE_SIZE + CACHE_LINE_SIZE];
#endif
u8	_current_cache_page[_SPI_NAND_CACHE_SIZE];
u8	_current_cache_page_data[_SPI_NAND_PAGE_SIZE];
u8	_current_cache_page_oob[_SPI_NAND_OOB_SIZE];
u8	_current_cache_page_oob_mapping[_SPI_NAND_OOB_SIZE];
#endif

/* TYPE DECLARATIONS ----------------------------------------------------------------- */

/* STATIC VARIABLE DECLARATIONS ------------------------------------------------------ */
#if	defined(TCSUPPORT_NAND_BMT) && (!defined(LZMA_IMG) || defined(TCSUPPORT_BB_256KB))

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
extern int nand_logic_size;
#endif

#if !defined(BOOTROM_EXT) || defined(TCSUPPORT_BB_256KB)
static int bmt_pool_size = 0;
static bmt_struct *g_bmt = NULL;
static init_bbt_struct *g_bbt = NULL;
#if !defined(IMAGE_BL2)
extern int nand_flash_avalable_size;
#endif
u32 maximum_bmt_block_count=0;

#define BMT_BAD_BLOCK_INDEX_OFFSET (1)
#define POOL_GOOD_BLOCK_PERCENT 8/100
#define MAX_BMT_SIZE_PERCENTAGE 1/10

/*#if defined(TCSUPPORT_CT_PON) || defined(TCSUPPORT_OPENWRT)*/
#define MAX_BMT_SIZE_PERCENTAGE_CT 1/8
/*#endif*/

#endif
#endif


#if defined(SPI_NAND_FLASH_DEBUG)
SPI_NAND_FLASH_DEBUG_LEVEL_T  _SPI_NAND_DEBUG_LEVEL = SPI_NAND_FLASH_DEBUG_LEVEL_0;
#endif

u32	_current_page_num	= UNKNOW_PAGE;
NAND_FLASH_OTP_ENABLE_T otp_status = NAND_FLASH_OTP_DISABLE;
u32 reservearea_size = 0;
unsigned char _ondie_ecc_flag=1;    /* Ondie ECC : [ToDo :  Init this flag base on diffrent chip ?] */
SPI_DMA_MODE_T _spi_dma_mode=SPI_DMA_MODE_DISABLE;
unsigned char _plane_select_bit=0;
unsigned char _die_id = 0;

struct SPI_NAND_FLASH_INFO_T	_current_flash_info_t;			/* Store the current flash information */

unsigned char _parallel_nand_mode = 0;
SPI_NAND_FLASH_RTN_T (*nand_probe)( struct SPI_NAND_FLASH_INFO_T *ptr_rtn_device_t ) = NULL;
SPI_NAND_FLASH_RTN_T (*arht_nand_reset)(void) = NULL;

int bad_block_rate = 0; //unit: 0.0001

/* LOCAL SUBPROGRAM BODIES------------------------------------------------------------ */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30) && !defined(CONFIG_UBOOT_ARHT)
static int generic_ffs(int x)
{
	int r = 1;

	if (!x)
		return 0;
	if (!(x & 0xffff)) {
		x >>= 16;
		r += 16;
	}
	if (!(x & 0xff)) {
		x >>= 8;
		r += 8;
	}
	if (!(x & 0xf)) {
		x >>= 4;
		r += 4;
	}
	if (!(x & 3)) {
		x >>= 2;
		r += 2;
	}
	if (!(x & 1)) {
		x >>= 1;
		r += 1;
	}
	return r;
}
#endif

#if defined(SPI_NAND_FLASH_DEBUG)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
SPI_NAND_FLASH_DEBUG_LEVEL_T get_debug_level(void)
{
	return _SPI_NAND_DEBUG_LEVEL;
}

void set_debug_level(SPI_NAND_FLASH_DEBUG_LEVEL_T DEBUG_LEVEL)
{
	_SPI_NAND_DEBUG_LEVEL = DEBUG_LEVEL;
}
#endif

static void spi_nand_flash_debug_printf( SPI_NAND_FLASH_DEBUG_LEVEL_T DEBUG_LEVEL, char *fmt, ... )
{
	if( _SPI_NAND_DEBUG_LEVEL >= DEBUG_LEVEL )
	{
#ifdef SPRAM_IMG
		_SPI_NAND_PRINTF(fmt);
#else
		unsigned char 		str_buf[100];	
		va_list 			argptr;
		int 				cnt;		
	
		va_start(argptr, fmt);
		cnt = vsprintf(str_buf, fmt, argptr);
		va_end(argptr);
				
		_SPI_NAND_PRINTF("%s", str_buf);
#endif
	}
}

static void spi_nand_flash_debug_printf_array(SPI_NAND_FLASH_DEBUG_LEVEL_T DEBUG_LEVEL, const char *buf, u32 len)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
	u32	idx_for_debug;
	
	if(_SPI_NAND_DEBUG_LEVEL >= DEBUG_LEVEL ) {
		for(idx_for_debug=0; idx_for_debug< len; idx_for_debug++) {				
			if((idx_for_debug % 8) == 0) {
				if((idx_for_debug % 16) == 0) {
					_SPI_NAND_PRINTF("\n%04x:", (idx_for_debug));
				} else {
					_SPI_NAND_PRINTF(": ");
				}
			}
			_SPI_NAND_PRINTF("%02x ", (unsigned char)buf[idx_for_debug]);
		}			
		_SPI_NAND_PRINTF("\n");		
	}
#else
	if( _SPI_NAND_DEBUG_LEVEL >= DEBUG_LEVEL ) {
		show_array(buf, len, "SPI NAND Flash array");	
	}
#endif
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
__attribute__((unused)) static void spi_nand_flash_debug_printf_hex(SPI_NAND_FLASH_DEBUG_LEVEL_T DEBUG_LEVEL, unsigned long val, int len)
{
	if( _SPI_NAND_DEBUG_LEVEL >= DEBUG_LEVEL )
	{
#ifdef SPRAM_IMG
		prom_print_hex(val, len);
#else
		_SPI_NAND_PRINTF("%lx", val);
#endif
	}
}
#endif
#endif

void spi_nand_protocol_set_otp(NAND_FLASH_OTP_ENABLE_T state)
{
	/* reset for forcing read page */
	_current_page_num = UNKNOW_PAGE;
	/* set OTP status */
	otp_status = state;
}

static SPI_NAND_FLASH_RTN_T spi_nand_protocol_chk_status(u8 mask, u8 value)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	u32						check_cnt;
	u8						status;
	
	check_cnt = 0;
	do {
		rtn_status = spi_nand_protocol_get_feature(_SPI_NAND_ADDR_STATUS, &status);
		if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
			_SPI_NAND_PRINTF("[Error] get status fail!\n");
			return rtn_status;
		}
		if(check_cnt == _SPI_NFI_CHECK_ECC_DONE_MAX_TIMES) {		
			_SPI_NAND_PRINTF("[Error] wait for busy free timeout, status=0x%x!\n", status);
			return SPI_NAND_FLASH_RTN_TIMEOUT;
		}
		check_cnt++;
	} while((status & mask) != value) ;

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_protocol_chk_status\n");
	
	return (SPI_NAND_FLASH_RTN_NO_ERROR);
}

static SPI_NAND_FLASH_RTN_T spi_nand_protocol_reset( void )
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	rtn_status = spi_nand_protocol_chk_status(_SPI_NAND_VAL_OIP, 0);
	if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
		_SPI_NAND_PRINTF("[Error] reset Timeout.\n");
		return (rtn_status);
	}
	
	/* 1. Chip Select low */
	_SPI_NAND_READ_CHIP_SELECT_LOW();

	/* 2. Send FFh opcode (Reset) */
	_SPI_NAND_WRITE_ONE_BYTE( _SPI_NAND_OP_SET_FEATURE );

	/* 5. Chip Select High */
	_SPI_NAND_READ_CHIP_SELECT_HIGH();	

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_protocol_reset\n");
	
	return (rtn_status);	
}

/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T spi_nand_protocol_set_feature( u8 addr, u8 protection )
 * PURPOSE : To implement the SPI nand protocol for set status register.
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : addr - register address
 *			 data - The variable of this register.
 *   OUTPUT: None
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2017/5/26.
 *
 *------------------------------------------------------------------------------------
 */
SPI_NAND_FLASH_RTN_T spi_nand_protocol_set_feature( u8 addr, u8 data )
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	rtn_status = spi_nand_protocol_chk_status(_SPI_NAND_VAL_OIP, 0);
	if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
		_SPI_NAND_PRINTF("[Error] set feature:0x%x Timeout.\n", addr);
		return (rtn_status);
	}
	
	/* 1. Chip Select low */
	_SPI_NAND_READ_CHIP_SELECT_LOW();

	/* 2. Send 0Fh opcode (Set Feature) */
	_SPI_NAND_WRITE_ONE_BYTE( _SPI_NAND_OP_SET_FEATURE );
	
	/* 3. Offset addr */
	_SPI_NAND_WRITE_ONE_BYTE( addr );
	
	/* 4. Write new setting */
	_SPI_NAND_WRITE_ONE_BYTE( data );

	/* 5. Chip Select High */
	_SPI_NAND_READ_CHIP_SELECT_HIGH();	

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_protocol_set_feature %x: val=0x%x\n", addr, data);
	
	return (rtn_status);
}

/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T spi_nand_protocol_get_feature( u8 addr, u8 *ptr_rtn_data )
 * PURPOSE : To implement the SPI nand protocol for get status register.
 * AUTHOR  :
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : addr - register address
 *   OUTPUT: ptr_rtn_protection  - A pointer to the ptr_rtn_protection variable.
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2017/5/26.
 *
 *------------------------------------------------------------------------------------
 */
SPI_NAND_FLASH_RTN_T spi_nand_protocol_get_feature( u8 addr, u8 *ptr_rtn_data )
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	if (_parallel_nand_mode)
	{
		return rtn_status;
	}
	
	/* 1. Chip Select low */
	_SPI_NAND_READ_CHIP_SELECT_LOW();

	/* 2. Send 0Fh opcode (Get Feature) */
	_SPI_NAND_WRITE_ONE_BYTE( _SPI_NAND_OP_GET_FEATURE );
	
	/* 3. Offset addr */
	_SPI_NAND_WRITE_ONE_BYTE_WITH_CMD(_SPI_CONTROLLER_VAL_OP_OS2ID, addr );
	
	/* 4. Read 1 byte data */
	_SPI_NAND_READ_NBYTE( ptr_rtn_data, _SPI_NAND_LEN_ONE_BYTE, SPI_CONTROLLER_SPEED_SINGLE);

	/* 5. Chip Select High */
	_SPI_NAND_READ_CHIP_SELECT_HIGH();	

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_protocol_get_feature %x: val=0x%x\n", addr, *ptr_rtn_data);
	
	return (rtn_status);
}

SPI_NAND_FLASH_RTN_T spi_nand_protocol_show_status(void)
{
	u8 feature;
	u8 i;
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	SPI_NAND_FLASH_RTN_T			rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;

	for(i = 0; i < ptr_dev_info_t->die_num; i++) {
		spi_nand_direct_select_die(i);
		_SPI_NAND_PRINTF("Die %d:\n", i);
		rtn_status = spi_nand_protocol_get_feature(_SPI_NAND_ADDR_PROTECTION, &feature);
		if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
			_SPI_NAND_PRINTF("spi_nand_protocol_show_status register:0x%02x fail.\n", _SPI_NAND_ADDR_PROTECTION);
			return (rtn_status);
		}
		_SPI_NAND_PRINTF("  0x%02x:0x%x\n", _SPI_NAND_ADDR_PROTECTION, feature);
		
		rtn_status = spi_nand_protocol_get_feature(_SPI_NAND_ADDR_FEATURE, &feature);
		if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
			_SPI_NAND_PRINTF("spi_nand_protocol_show_status register:0x%02x fail.\n", _SPI_NAND_ADDR_FEATURE);
			return (rtn_status);
		}
		_SPI_NAND_PRINTF("  0x%02x:0x%x\n", _SPI_NAND_ADDR_FEATURE, feature);
		
		rtn_status = spi_nand_protocol_get_feature(_SPI_NAND_ADDR_STATUS, &feature);
		if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
			_SPI_NAND_PRINTF("spi_nand_protocol_show_status register:0x%02x fail.\n", _SPI_NAND_ADDR_STATUS);
			return (rtn_status);
		}
		_SPI_NAND_PRINTF("  0x%02x:0x%x\n", _SPI_NAND_ADDR_STATUS, feature);
	}

	return (rtn_status);
}


#if !defined(BOOTROM_EXT)
/*------------------------------------------------------------------------------------
 * FUNCTION: SPI_NAND_FLASH_RTN_T SPI_NAND_Flash_Get_Flash_Info( struct SPI_NAND_FLASH_INFO_T    *ptr_rtn_into_t )
 * PURPOSE : To get system current flash info.
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : None
 *   OUTPUT: ptr_rtn_into_t  - A pointer to the structure of the ptr_rtn_into_t variable.
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2015/01/14  - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
SPI_NAND_FLASH_RTN_T SPI_NAND_Flash_Get_Flash_Info(struct SPI_NAND_FLASH_INFO_T *ptr_rtn_into_t)
{
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	SPI_NAND_FLASH_RTN_T			rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;
	
	memcpy( ptr_rtn_into_t, ptr_dev_info_t, sizeof(struct SPI_NAND_FLASH_INFO_T));
	
	return (rtn_status);	
}

SPI_NAND_FLASH_RTN_T SPI_NAND_Flash_Set_Flash_Info( struct SPI_NAND_FLASH_INFO_T *ptr_rtn_into_t)
{
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	SPI_NAND_FLASH_RTN_T			rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;
	
	memcpy(ptr_dev_info_t, ptr_rtn_into_t, sizeof(struct SPI_NAND_FLASH_INFO_T) );
	
	return (rtn_status);	
}
#endif

/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T spi_nand_protocol_read_id( struct SPI_NAND_FLASH_INFO_T *ptr_rtn_flash_id )
 * PURPOSE : To implement the SPI nand protocol for read id.
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : None
 *   OUTPUT: None
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/12  - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
static SPI_NAND_FLASH_RTN_T spi_nand_protocol_read_id ( struct SPI_NAND_FLASH_INFO_T *ptr_rtn_flash_id )
{
	u8 i = 0;
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;	

	rtn_status = spi_nand_protocol_chk_status(_SPI_NAND_VAL_OIP, 0);
	if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
		_SPI_NAND_PRINTF("[Error] read id Timeout.\n");
		return (rtn_status);
	}
	
	/* 1. Chip Select Low */
	_SPI_NAND_READ_CHIP_SELECT_LOW();
	
	/* 2. Write op_cmd 0x9F (Read ID) */
	_SPI_NAND_WRITE_ONE_BYTE( _SPI_NAND_OP_READ_ID );	
	
	/* 3. Write Address Byte (0x00) */
	_SPI_NAND_WRITE_ONE_BYTE( _SPI_NAND_ADDR_MANUFACTURE_ID );	
	
	/* 4. Read data (Manufacture ID and Device ID) */	
	_SPI_NAND_READ_NBYTE( (u8 *)&(ptr_rtn_flash_id->mfr_id), _SPI_NAND_LEN_ONE_BYTE, SPI_CONTROLLER_SPEED_SINGLE);	
	_SPI_NAND_READ_NBYTE( (u8 *)&(ptr_rtn_flash_id->dev_id), _SPI_NAND_LEN_ONE_BYTE, SPI_CONTROLLER_SPEED_SINGLE);

	for (i=0 ; i<MAX_SPI_NAND_FLASH_EXTEND_DEV_ID_LEN ; i++)
	{
		_SPI_NAND_READ_NBYTE( (u8 *)&(ptr_rtn_flash_id->extend_dev_id.extend_id[i]), _SPI_NAND_LEN_ONE_BYTE, SPI_CONTROLLER_SPEED_SINGLE);
	}

	/* 5. Chip Select High */
	_SPI_NAND_READ_CHIP_SELECT_HIGH();

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_protocol_read_id : mfr_id=0x%x, dev_id=0x%x\n", ptr_rtn_flash_id->mfr_id, ptr_rtn_flash_id->dev_id);
	
	return (rtn_status);
}

/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T spi_nand_protocol_read_id_2( struct SPI_NAND_FLASH_INFO_T *ptr_rtn_flash_id )
 * PURPOSE : To implement the SPI nand protocol for read id.
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : None
 *   OUTPUT: None
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/12  - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
static SPI_NAND_FLASH_RTN_T spi_nand_protocol_read_id_2 ( struct SPI_NAND_FLASH_INFO_T *ptr_rtn_flash_id )
{
	u8 i = 0;
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;	

	rtn_status = spi_nand_protocol_chk_status(_SPI_NAND_VAL_OIP, 0);
	if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
		_SPI_NAND_PRINTF("[Error] read id 2 Timeout.\n");
		return (rtn_status);
	}
	
	/* 1. Chip Select Low */
	_SPI_NAND_READ_CHIP_SELECT_LOW();
	
	/* 2. Write op_cmd 0x9F (Read ID) */
	_SPI_NAND_WRITE_ONE_BYTE( _SPI_NAND_OP_READ_ID );	
	
	/* 3. Read data (Manufacture ID and Device ID) */	
	_SPI_NAND_READ_NBYTE( (u8 *)&(ptr_rtn_flash_id->mfr_id), _SPI_NAND_LEN_ONE_BYTE, SPI_CONTROLLER_SPEED_SINGLE);	
	_SPI_NAND_READ_NBYTE( (u8 *)&(ptr_rtn_flash_id->dev_id), _SPI_NAND_LEN_ONE_BYTE, SPI_CONTROLLER_SPEED_SINGLE);

	for (i=0 ; i<MAX_SPI_NAND_FLASH_EXTEND_DEV_ID_LEN ; i++)
	{
		_SPI_NAND_READ_NBYTE( (u8 *)&(ptr_rtn_flash_id->extend_dev_id.extend_id[i]), _SPI_NAND_LEN_ONE_BYTE, SPI_CONTROLLER_SPEED_SINGLE);
	}

	/* 4. Chip Select High */
	_SPI_NAND_READ_CHIP_SELECT_HIGH();

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_protocol_read_id_2 : mfr_id=0x%x, dev_id=0x%x\n", ptr_rtn_flash_id->mfr_id, ptr_rtn_flash_id->dev_id);
	
	return (rtn_status);
}

static SPI_NAND_FLASH_RTN_T spi_nand_protocol_die_select_1( u8 die_id)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	rtn_status = spi_nand_protocol_chk_status(_SPI_NAND_VAL_OIP, 0);
	if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
		_SPI_NAND_PRINTF("[Error] die select 1 Timeout.\n");
		return (rtn_status);
	}
	
	/* 1. Chip Select low */
	_SPI_NAND_READ_CHIP_SELECT_LOW();

	/* 2. Send C2h opcode (Die Select) */
	_SPI_NAND_WRITE_ONE_BYTE( _SPI_NAND_OP_DIE_SELECT );

	/* 3. Send Die ID */
	_SPI_NAND_WRITE_ONE_BYTE( die_id );

	/* 5. Chip Select High */
	_SPI_NAND_READ_CHIP_SELECT_HIGH();

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_protocol_die_select_1\n");

	return (rtn_status);
}

static SPI_NAND_FLASH_RTN_T spi_nand_protocol_die_select_2( u8 die_id)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	u8 feature;

	rtn_status = spi_nand_protocol_get_feature(_SPI_NAND_ADDR_FEATURE_4, &feature);
	if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
		_SPI_NAND_PRINTF("spi_nand_protocol_die_select_2 get die select fail.\n");
		return (rtn_status);
	}
	
	if(die_id == 0) {
		feature &= ~(0x40);
	} else {
		feature |= 0x40;
	}
	rtn_status = spi_nand_protocol_set_feature(_SPI_NAND_ADDR_FEATURE_4, feature);

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_protocol_die_select_2\n");

	return (rtn_status);
}

void spi_nand_select_die (u32 page_number)
{
	struct SPI_NAND_FLASH_INFO_T *ptr_dev_info_t;
	u8 die_id = 0;

	ptr_dev_info_t = _SPI_NAND_GET_DEVICE_INFO_PTR;

	if(ptr_dev_info_t->feature & SPI_NAND_FLASH_DIE_SELECT_1_HAVE) {
		if(otp_status == NAND_FLASH_OTP_DISABLE) {
			/* single die = 1024blocks * 64pages */
			die_id = ((page_number >> 16) & 0xff);
		} 
#ifdef TCSUPPORT_NAND_FLASH_OTP
		else {
			if(ptr_dev_info_t->otp_page_num == -1) {
				_SPI_NAND_PRINTF("[error] do not support OTP function.\n");
				return;
			} else 	{
				die_id = page_number / ptr_dev_info_t->otp_page_num;
				if(die_id >= ptr_dev_info_t->die_num) {
					_SPI_NAND_PRINTF("[error] OTP die select page:0x%x more than 24.\n", page_number);
					return;
				}
			}
		}
#endif

		if (_die_id != die_id) {
			_die_id = die_id;
			spi_nand_protocol_die_select_1(die_id);

			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spi_nand_protocol_die_select_1: die_id=0x%x\n", die_id);
		}
	} else if(ptr_dev_info_t->feature & SPI_NAND_FLASH_DIE_SELECT_2_HAVE) {
		/* single die = 2plans * 1024blocks * 64pages */
		die_id = ((page_number >> 17) & 0xff);

		if (_die_id != die_id) {
			_die_id = die_id;
			spi_nand_protocol_die_select_2(die_id);

			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spi_nand_protocol_die_select_2: die_id=0x%x\n", die_id);
		}
	}
}

static void spi_nand_direct_select_die(u32 die)
{
	struct SPI_NAND_FLASH_INFO_T *ptr_dev_info_t;

	ptr_dev_info_t = _SPI_NAND_GET_DEVICE_INFO_PTR;

	if(ptr_dev_info_t->feature & SPI_NAND_FLASH_DIE_SELECT_1_HAVE) {
		if (_die_id != die) {
			_die_id = die;
			spi_nand_protocol_die_select_1(die);

			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spi_nand_protocol_die_select_1: die_id=0x%x\n", die);
		}
	} else if(ptr_dev_info_t->feature & SPI_NAND_FLASH_DIE_SELECT_2_HAVE) {
		if (_die_id != die) {
			_die_id = die;
			spi_nand_protocol_die_select_2(die);

			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spi_nand_protocol_die_select_2: die_id=0x%x\n", die);
		}
	}
}

/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T spi_nand_block_aligned_check( u32   addr,
 *                                                                     u32   len  )
 * PURPOSE : To check block align.
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : addr - The addr variable of this function.
 *           len  - The len variable of this function.
 *   OUTPUT: None
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/15  - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
static SPI_NAND_FLASH_RTN_T spi_nand_block_aligned_check( u32 addr, 
														  u32 len )
{
	struct SPI_NAND_FLASH_INFO_T		*ptr_dev_info_t;
	SPI_NAND_FLASH_RTN_T				rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;	
	
	
	ptr_dev_info_t = _SPI_NAND_GET_DEVICE_INFO_PTR ;
	
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "SPI_NAND_BLOCK_ALIGNED_CHECK_check: addr=0x%x, len=0x%x, block size =0x%x \n", addr, len, (ptr_dev_info_t->erase_size));	

	if (_SPI_NAND_BLOCK_ALIGNED_CHECK(len, (ptr_dev_info_t->erase_size))) 
	{
		len = ( (len/ptr_dev_info_t->erase_size) + 1) * (ptr_dev_info_t->erase_size);
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "SPI_NAND_BLOCK_ALIGNED_CHECK_check: erase block aligned first check OK, addr:%x len:%x, block size:%x\n", addr, len, (ptr_dev_info_t->erase_size));
	}

	if (_SPI_NAND_BLOCK_ALIGNED_CHECK(addr, (ptr_dev_info_t->erase_size)) || _SPI_NAND_BLOCK_ALIGNED_CHECK(len, (ptr_dev_info_t->erase_size)) ) 
	{
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "SPI_NAND_BLOCK_ALIGNED_CHECK_check: erase block not aligned, addr:0x%x len:0x%x, blocksize:0x%x\n", addr, len, (ptr_dev_info_t->erase_size));		
		rtn_status = SPI_NAND_FLASH_RTN_ALIGNED_CHECK_FAIL;
	}	
	
	return (rtn_status);
}

/*------------------------------------------------------------------------------------
 * FUNCTION: void SPI_NAND_Flash_Clear_Read_Cache_Data( void )
 * PURPOSE : To clear the cache data for read. 
 *           (The next time to read data will get data from flash chip certainly.)
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : None
 *   OUTPUT: None
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2015/01/21  - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
void SPI_NAND_Flash_Clear_Read_Cache_Data( void )
{
	_current_page_num	= UNKNOW_PAGE;
}

void SPI_NAND_Flash_Set_DmaMode( SPI_DMA_MODE_T input )
{
	_spi_dma_mode = input;
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "SPI_NAND_Flash_Set_DmaMode : dma_mode =%d\n", _spi_dma_mode);
}

void SPI_NAND_Flash_Get_DmaMode( SPI_DMA_MODE_T *val )
{
	*val = _spi_dma_mode;
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "SPI_NAND_Flash_Get_DmaMode : dma_mode =%d\n", _spi_dma_mode);
}

SPI_NAND_FLASH_RTN_T SPI_NAND_Flash_Enable_OnDie_ECC( void )
{
	unsigned char feature;
	struct SPI_NAND_FLASH_INFO_T *ptr_dev_info_t;
	u8 die_num;
	u32 die_page_num;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
	unsigned long spinand_spinlock_flags;
#endif

	if (_parallel_nand_mode)
	{
		_ondie_ecc_flag = 0;
		return (SPI_NAND_FLASH_RTN_NO_ERROR);
	}

	_SPI_NAND_SEMAPHORE_LOCK();
		
	ptr_dev_info_t = _SPI_NAND_GET_DEVICE_INFO_PTR;

	if(((ptr_dev_info_t->feature) & SPI_NAND_FLASH_DIE_SELECT_1_HAVE) ||
	   ((ptr_dev_info_t->feature) & SPI_NAND_FLASH_DIE_SELECT_2_HAVE)) {
		for(die_num = 0; die_num < ptr_dev_info_t->die_num; die_num++) {
			die_page_num = die_num * (ptr_dev_info_t->device_size / ptr_dev_info_t->die_num) / ptr_dev_info_t->page_size;
			spi_nand_select_die(die_page_num);

			spi_nand_protocol_get_feature(ptr_dev_info_t->ecc_en.ecc_en_addr, &feature);
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "before setting : SPI_NAND_Flash_Enable_OnDie_ECC, status reg=0x%x\n", feature);	
			
			if((feature & ptr_dev_info_t->ecc_en.ecc_en_mask) != ptr_dev_info_t->ecc_en.ecc_en_value) {
				feature = (feature & ~(ptr_dev_info_t->ecc_en.ecc_en_mask));
				feature |= (ptr_dev_info_t->ecc_en.ecc_en_value & ptr_dev_info_t->ecc_en.ecc_en_mask);

				spi_nand_protocol_set_feature(ptr_dev_info_t->ecc_en.ecc_en_addr, feature);
			
				/* Value check*/
				spi_nand_protocol_get_feature(ptr_dev_info_t->ecc_en.ecc_en_addr, &feature);
				if(feature & ptr_dev_info_t->ecc_en.ecc_en_mask) {
					_SPI_NAND_SEMAPHORE_UNLOCK();
					return (SPI_NAND_FLASH_RTN_NO_ERROR);
				} else {
					_SPI_NAND_SEMAPHORE_UNLOCK();
					return (SPI_NAND_FLASH_RTN_ENABLE_ECC_FAIL);
				}
			}
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "after setting : SPI_NAND_Flash_Enable_OnDie_ECC, status reg=0x%x\n", feature);	
		}
	} else {
		spi_nand_protocol_get_feature(ptr_dev_info_t->ecc_en.ecc_en_addr, &feature);
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "before setting : SPI_NAND_Flash_Enable_OnDie_ECC, status reg=0x%x\n", feature);	
		
		if((feature & ptr_dev_info_t->ecc_en.ecc_en_mask) != ptr_dev_info_t->ecc_en.ecc_en_value) {
			feature = (feature & ~(ptr_dev_info_t->ecc_en.ecc_en_mask));
			feature |= (ptr_dev_info_t->ecc_en.ecc_en_value & ptr_dev_info_t->ecc_en.ecc_en_mask);
			
			spi_nand_protocol_set_feature(ptr_dev_info_t->ecc_en.ecc_en_addr, feature);

			/* Value check*/
			spi_nand_protocol_get_feature(ptr_dev_info_t->ecc_en.ecc_en_addr, &feature);
			if(feature & ptr_dev_info_t->ecc_en.ecc_en_mask) {
				_SPI_NAND_SEMAPHORE_UNLOCK();
				return (SPI_NAND_FLASH_RTN_NO_ERROR);
			} else {
				_SPI_NAND_SEMAPHORE_UNLOCK();
				return (SPI_NAND_FLASH_RTN_ENABLE_ECC_FAIL);
			}
		}
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "after setting : SPI_NAND_Flash_Enable_OnDie_ECC, status reg=0x%x\n", feature);	
	}

	_ondie_ecc_flag = 1;

	_SPI_NAND_SEMAPHORE_UNLOCK();

	return (SPI_NAND_FLASH_RTN_NO_ERROR);
}

SPI_NAND_FLASH_RTN_T SPI_NAND_Flash_Disable_OnDie_ECC( void )
{
	unsigned char feature;
	struct SPI_NAND_FLASH_INFO_T *ptr_dev_info_t;
	u8 die_num;
	u32 die_page_num;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
	unsigned long spinand_spinlock_flags;
#endif

	if (_parallel_nand_mode)
	{
		_ondie_ecc_flag = 0;
		return (SPI_NAND_FLASH_RTN_NO_ERROR);
	}

	_SPI_NAND_SEMAPHORE_LOCK();

	ptr_dev_info_t = _SPI_NAND_GET_DEVICE_INFO_PTR;

	if(((ptr_dev_info_t->feature) & SPI_NAND_FLASH_DIE_SELECT_1_HAVE) ||
	   ((ptr_dev_info_t->feature) & SPI_NAND_FLASH_DIE_SELECT_2_HAVE)) {
		for(die_num = 0; die_num < ptr_dev_info_t->die_num; die_num++) {
			die_page_num = die_num * (ptr_dev_info_t->device_size / ptr_dev_info_t->die_num) / ptr_dev_info_t->page_size;
			spi_nand_select_die(die_page_num);

			spi_nand_protocol_get_feature(ptr_dev_info_t->ecc_en.ecc_en_addr, &feature);

			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "before setting : SPI_NAND_Flash_Disable_OnDie_ECC, status reg=0x%x\n", feature);	

			if((feature & ptr_dev_info_t->ecc_en.ecc_en_mask) == ptr_dev_info_t->ecc_en.ecc_en_value) {
				feature = (feature & ~(ptr_dev_info_t->ecc_en.ecc_en_mask));
				feature |= ((~(ptr_dev_info_t->ecc_en.ecc_en_value)) & ptr_dev_info_t->ecc_en.ecc_en_mask);
			
				spi_nand_protocol_set_feature(ptr_dev_info_t->ecc_en.ecc_en_addr, feature);

				/* Value check*/
				spi_nand_protocol_get_feature(ptr_dev_info_t->ecc_en.ecc_en_addr, &feature);
				if(feature & ptr_dev_info_t->ecc_en.ecc_en_mask) {
					_SPI_NAND_SEMAPHORE_UNLOCK();
					return (SPI_NAND_FLASH_RTN_DISABLE_ECC_FAIL);
				} else {
					_SPI_NAND_SEMAPHORE_UNLOCK();
					return (SPI_NAND_FLASH_RTN_NO_ERROR);
				}
			}
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "after setting : SPI_NAND_Flash_Disable_OnDie_ECC, status reg=0x%x\n", feature);
		}
	} else {
		spi_nand_protocol_get_feature(ptr_dev_info_t->ecc_en.ecc_en_addr, &feature);

		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "before setting : SPI_NAND_Flash_Disable_OnDie_ECC, status reg=0x%x\n", feature);	

		if((feature & ptr_dev_info_t->ecc_en.ecc_en_mask) == ptr_dev_info_t->ecc_en.ecc_en_value) {
			feature = (feature & ~(ptr_dev_info_t->ecc_en.ecc_en_mask));
			feature |= ((~(ptr_dev_info_t->ecc_en.ecc_en_value)) & ptr_dev_info_t->ecc_en.ecc_en_mask);
			
			spi_nand_protocol_set_feature(ptr_dev_info_t->ecc_en.ecc_en_addr, feature);

			/* Value check*/
			spi_nand_protocol_get_feature(ptr_dev_info_t->ecc_en.ecc_en_addr, &feature);
			if(feature & ptr_dev_info_t->ecc_en.ecc_en_mask) {
				_SPI_NAND_SEMAPHORE_UNLOCK();
				return (SPI_NAND_FLASH_RTN_DISABLE_ECC_FAIL);
			} else {
				_SPI_NAND_SEMAPHORE_UNLOCK();
				return (SPI_NAND_FLASH_RTN_NO_ERROR);
			}
		}
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "after setting : SPI_NAND_Flash_Disable_OnDie_ECC, status reg=0x%x\n", feature);
	}

	_ondie_ecc_flag = 0;

	_SPI_NAND_SEMAPHORE_UNLOCK();

	return (SPI_NAND_FLASH_RTN_NO_ERROR);
}

void enable_quad(void)
{
	unsigned char					feature;
	unsigned char					die;
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	
	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1,"SPI NAND Chip Init : Enable Quad Mode\n"); 

	for(die = 0; die < ptr_dev_info_t->die_num; die++) {
		spi_nand_direct_select_die(die);

		/* Enable Qual mode */
		if(ptr_dev_info_t->quad_en.quad_en_mask != 0x00) {
			spi_nand_protocol_get_feature(_SPI_NAND_ADDR_FEATURE, &feature);
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "Before enable qual mode setup, the status register2 =0x%x\n", feature);

			feature = (feature & ~(ptr_dev_info_t->quad_en.quad_en_mask));
			feature |= ptr_dev_info_t->quad_en.quad_en_value;
			spi_nand_protocol_set_feature(_SPI_NAND_ADDR_FEATURE, feature);
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "enable qual mode setup, the feature = 0x%x\n", feature);

			spi_nand_protocol_get_feature(_SPI_NAND_ADDR_FEATURE, &feature);
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "After enable qual mode setup, the status register2 =0x%x\n", feature);
		} else {
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "No need enable qual mode setup.\n");
		}
	}

	/* modify share pin for quad mode */
	set_spi_quad_mode_shared_pin();
	
	_SPI_NAND_PRINTF("enable SPI Quad mode\n");
}


/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T spi_nand_protocol_page_read( u32    page_number )
 * PURPOSE : To implement the SPI nand protocol for page read.
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : page_number - The page_number variable of this function.
 *   OUTPUT: None
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/17  - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
static SPI_NAND_FLASH_RTN_T spi_nand_protocol_page_read ( u32 page_number )
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	rtn_status = spi_nand_protocol_chk_status(_SPI_NAND_VAL_OIP, 0);
	if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
		_SPI_NAND_PRINTF("[Error] page read Timeout.\n");
		return (rtn_status);
	}
	
	/* 1. Chip Select low */
	_SPI_NAND_READ_CHIP_SELECT_LOW();
	
	/* 2. Send 13h opcode */
	_SPI_NAND_WRITE_ONE_BYTE( _SPI_NAND_OP_PAGE_READ );
	
	/* 3. Send page number */
	_SPI_NAND_WRITE_ONE_BYTE( ((page_number >> 16 ) & 0xff) );
	_SPI_NAND_WRITE_ONE_BYTE( ((page_number >> 8  ) & 0xff) );
	_SPI_NAND_WRITE_ONE_BYTE( ((page_number)        & 0xff) );
	
	/* 4. Chip Select High */
	_SPI_NAND_READ_CHIP_SELECT_HIGH();	
	
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_load_page_into_cache: value=0x%x\n", page_number);	
	
	return (rtn_status);
	
}

/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T spi_nand_protocol_read_from_cache( u32  data_offset,
 *                                                                          u32  len,
 *                                                                          u8   *ptr_rtn_buf,
 *																			u32	 read_mode,
 *																			SPI_NAND_FLASH_READ_DUMMY_BYTE_T dummy_mode)
 * PURPOSE : To implement the SPI nand protocol for read from cache with single speed.
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : data_offset  - The data_offset variable of this function.
 *           len          - The len variable of this function.
 *   OUTPUT: ptr_rtn_buf  - A pointer to the ptr_rtn_buf variable.
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 *
 *------------------------------------------------------------------------------------
 */
static SPI_NAND_FLASH_RTN_T spi_nand_protocol_read_from_cache( u32 data_offset, 
																u32 len, 
																u8 *ptr_rtn_buf, 
																u32 read_mode,
																SPI_NAND_FLASH_READ_DUMMY_BYTE_T dummy_mode )
{			

	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	
	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;	

	rtn_status = spi_nand_protocol_chk_status(_SPI_NAND_VAL_OIP, 0);
	if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
		_SPI_NAND_PRINTF("[Error] read from cache Timeout.\n");
		return (rtn_status);
	}
	
	/* 1. Chip Select low */
	_SPI_NAND_READ_CHIP_SELECT_LOW();
	
	/* 2. Send opcode */
	switch (read_mode)
	{
		/* 03h */
		case SPI_NAND_FLASH_READ_SPEED_MODE_SINGLE:
			_SPI_NAND_WRITE_ONE_BYTE( _SPI_NAND_OP_READ_FROM_CACHE_SINGLE );
			break;

		/* 3Bh */
		case SPI_NAND_FLASH_READ_SPEED_MODE_DUAL:
			_SPI_NAND_WRITE_ONE_BYTE( _SPI_NAND_OP_READ_FROM_CACHE_DUAL );
			break;										

		/* 6Bh */
		case SPI_NAND_FLASH_READ_SPEED_MODE_QUAD:
			_SPI_NAND_WRITE_ONE_BYTE( _SPI_NAND_OP_READ_FROM_CACHE_QUAD );
			break;

		default:
			break;
	}
	
	/* 3. Send data_offset addr */
	if( dummy_mode == SPI_NAND_FLASH_READ_DUMMY_BYTE_PREPEND )
	{
		_SPI_NAND_WRITE_ONE_BYTE(0xff);		/* dummy byte */
	}
	
	if( ((ptr_dev_info_t->feature) & SPI_NAND_FLASH_PLANE_SELECT_HAVE) )
	{
		if( _plane_select_bit == 0)
		{
			_SPI_NAND_WRITE_ONE_BYTE( ((data_offset >> 8 ) &(0xef)) );
		}
		if( _plane_select_bit == 1)
		{
			_SPI_NAND_WRITE_ONE_BYTE( ((data_offset >> 8 ) | (0x10)) );
		}				
	}	
	else
	{
		_SPI_NAND_WRITE_ONE_BYTE( ((data_offset >> 8 ) &(0xff)) );
	}

	if((dummy_mode == SPI_NAND_FLASH_READ_DUMMY_BYTE_PREPEND) && 
	  (read_mode == SPI_NAND_FLASH_READ_SPEED_MODE_SINGLE)) {
		_SPI_NAND_WRITE_ONE_BYTE_WITH_CMD(_SPI_CONTROLLER_VAL_OP_OS2IS, ((data_offset      ) &(0xff)) );	
	} else {
		_SPI_NAND_WRITE_ONE_BYTE( ((data_offset      ) &(0xff)) );	
	}
	
	if( dummy_mode == SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND )
	{
		/* dummy byte */
		switch(read_mode)
		{
			case SPI_NAND_FLASH_READ_SPEED_MODE_SINGLE:
				_SPI_NAND_WRITE_ONE_BYTE_WITH_CMD(_SPI_CONTROLLER_VAL_OP_OS2IS, 0xff);	
				break;

			case SPI_NAND_FLASH_READ_SPEED_MODE_DUAL:
				_SPI_NAND_WRITE_ONE_BYTE_WITH_CMD(_SPI_CONTROLLER_VAL_OP_OS2ID, 0xff);	
				break;										

			case SPI_NAND_FLASH_READ_SPEED_MODE_QUAD:
				_SPI_NAND_WRITE_ONE_BYTE_WITH_CMD(_SPI_CONTROLLER_VAL_OP_OS2IQ, 0xff);	
				break;

			default:
				break;
		}
	}

	if(dummy_mode == SPI_NAND_FLASH_READ_DUMMY_BYTE_PREPEND)
	{
		/* dummy byte */
		switch(read_mode)
		{
			case SPI_NAND_FLASH_READ_SPEED_MODE_DUAL:
				_SPI_NAND_WRITE_ONE_BYTE_WITH_CMD(_SPI_CONTROLLER_VAL_OP_OS2ID, 0xff);	
				break;										

			case SPI_NAND_FLASH_READ_SPEED_MODE_QUAD:
				_SPI_NAND_WRITE_ONE_BYTE_WITH_CMD(_SPI_CONTROLLER_VAL_OP_OS2IQ, 0xff);	
				break;

			default:
				break;
		}
	}
	
	/* 4. Read n byte (len) data */
	switch (read_mode)
	{
		case SPI_NAND_FLASH_READ_SPEED_MODE_SINGLE:
			_SPI_NAND_READ_NBYTE( ptr_rtn_buf, len, SPI_CONTROLLER_SPEED_SINGLE);
			break;

		case SPI_NAND_FLASH_READ_SPEED_MODE_DUAL:
			_SPI_NAND_READ_NBYTE( ptr_rtn_buf, len, SPI_CONTROLLER_SPEED_DUAL);
			break;										

		case SPI_NAND_FLASH_READ_SPEED_MODE_QUAD:
			_SPI_NAND_READ_NBYTE( ptr_rtn_buf, len, SPI_CONTROLLER_SPEED_QUAD);
			break;

		default:
			break;
	}
		
	/* 5. Chip Select High */
	_SPI_NAND_READ_CHIP_SELECT_HIGH();
	
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_protocol_read_from_cache : data_offset=0x%x, buf=%p\n", data_offset, ptr_rtn_buf);	
	
	return (rtn_status);
}

static SPI_NAND_FLASH_RTN_T ecc_fail_check( u32 page_number )
{
	u8								status;
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	SPI_NAND_FLASH_RTN_T			rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	
	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;

	spi_nand_protocol_get_feature(_SPI_NAND_ADDR_STATUS, &status);

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "[spinand_ecc_fail_check]: status=0x%x\n", status);

	if((ptr_dev_info_t->feature & SPI_NAND_FLASH_NO_ECC_STATUS_HAVE) == 0) {
		status = (status & ptr_dev_info_t->ecc_fail_check_info.ecc_check_mask);
		if(status == ptr_dev_info_t->ecc_fail_check_info.ecc_uncorrected_value) {
			rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
			_SPI_NAND_PRINTF("[spinand_ecc_fail_check] : ECC cannot recover detected !, page=0x%x\n", page_number);
		}
#if defined(TCSUPPORT_NAND_BMT) && !defined(IMAGE_BL2) && ((!defined(LZMA_IMG) && !defined(BOOTROM_EXT)) || defined(TCSUPPORT_BB_256KB))
		else if (ptr_dev_info_t->feature & SPI_NAND_FLASH_READ_ECC_ERROR_BIT_CHECK)
		{
			u8 i;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
			if (_SPI_NAND_READ_EXCEEDED_THRESHOLD_TEST_FLAG)
			{
				_SPI_NAND_READ_EXCEEDED_THRESHOLD_TEST_FLAG = 0;
				rtn_status = SPI_NAND_FLASH_RTN_ECC_EXCEEDED_THRESHOLD;
				_SPI_NAND_PRINTF("[spinand_ecc_fail_check] : Force ECC exceeded threshold !, page=0x%x\n", page_number);
			}
			else
#endif
			if (status == ptr_dev_info_t->read_ecc_ceck.ecc_threhsold_value[0])
			{
				rtn_status = SPI_NAND_FLASH_RTN_ECC_EXCEEDED_THRESHOLD;
				_SPI_NAND_PRINTF("[spinand_ecc_fail_check] : ECC exceeded threshold !, page=0x%x\n", page_number);
			}
			else if (ptr_dev_info_t->read_ecc_ceck.type == _SPI_NAND_CHECK_ECC_THROSHOLD_BY_REG)
			{
				/* Had been check ecc value[0] */
				for (i=1 ; i<SPI_NAND_FLASH_THRESHOLD_VALUE_MAX ; i++)
				{
					if ((ptr_dev_info_t->read_ecc_ceck.ecc_threhsold_value[i] != 0)
						&& (status == ptr_dev_info_t->read_ecc_ceck.ecc_threhsold_value[i]))
					{
						rtn_status = SPI_NAND_FLASH_RTN_ECC_EXCEEDED_THRESHOLD;
						_SPI_NAND_PRINTF("[spinand_ecc_fail_check] : ECC exceeded threshold !, page=0x%x\n", page_number);
					}
				}
			}
			else if (ptr_dev_info_t->read_ecc_ceck.type == _SPI_NAND_CHECK_ECC_THROSHOLD_BY_2REG)
			{
				if (status == ptr_dev_info_t->read_ecc_ceck.ecc_threhsold_value[1])
				{
					spi_nand_protocol_get_feature(ptr_dev_info_t->read_ecc_ceck.ext_data[READ_ECC_CHECK_ADDR_INDEX], &status);

					/* First index is extend flash address */
					for (i=1 ; i<SPI_NAND_FLASH_EXT_DATA_MAX ; i++)
					{
						if ((ptr_dev_info_t->read_ecc_ceck.ext_data[i] != 0)
							&& (status & ptr_dev_info_t->read_ecc_ceck.ext_data[i]))
						{
							rtn_status = SPI_NAND_FLASH_RTN_ECC_EXCEEDED_THRESHOLD;
							_SPI_NAND_PRINTF("[spinand_ecc_fail_check] : ECC exceeded threshold !, page=0x%x\n", page_number);
						}
					}
				}
			}
		}
#endif
	}

	return (rtn_status);
}

/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T spi_nand_load_page_into_cache( long  page_number )
 * PURPOSE : To load page into SPI NAND chip.
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : page_number - The page_number variable of this function.
 *   OUTPUT: None
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/16  - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
static SPI_NAND_FLASH_RTN_T spi_nand_load_page_into_cache( u32 page_number)
{
	u8						status;
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	if (_parallel_nand_mode)
	{
		return rtn_status;
	}
	
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_load_page_into_cache: page number =0x%x\n", page_number);
		
	if(_current_page_num == page_number) {
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_load_page_into_cache: page number == _current_page_num\n");
	} else {
		spi_nand_select_die(page_number);

		spi_nand_protocol_page_read(page_number);

		/*  Checking status for load page/erase/program complete */
		do{
			spi_nand_protocol_get_feature(_SPI_NAND_ADDR_STATUS, &status);
		} while(status & _SPI_NAND_VAL_OIP) ;

		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_load_page_into_cache : status =0x%x\n", status);

		if(!isSpiNandAndCtrlECC)
		{
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_load_page_into_cache: check flash ecc status.\n");
			rtn_status = ecc_fail_check(page_number);
		}
	}
	
	return (rtn_status);
}

static SPI_NAND_FLASH_RTN_T spi_nand_read_page (u32 page_number, SPI_NAND_FLASH_READ_SPEED_MODE_T speed_mode)
{
	u32								i;
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	SPI_NAND_FLASH_RTN_T			rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	SPI_NFI_RTN_T					nfi_status;
	u16 							read_addr;
	SPI_NFI_MISC_SPEDD_CONTROL_T	dma_speed_mode;
	u32								read_cmd;

	SPI_NFI_CONF_T			spi_nfi_conf_t;
	SPI_ECC_DECODE_CONF_T	spi_ecc_decode_conf_t;
	u32 					check_cnt;
  
	SPI_ECC_DECODE_STATUS_T decode_status_t;
	SPI_CONTROLLER_CONF_T	spi_conf_t;
	u32 					offset1, offset2, offset3, dma_sec_size;
	/* Disable print on BL2 for decreasing bl2.bin size. Should set it be unused to avoid unused-but-set-variable warning */
	u32 					ecc_decode_done_bit __attribute__((unused));

	static unsigned long ecc_threshold_exceeded_block __attribute__((unused)) = INVALID_BLOCK_INDEX;
	unsigned long current_block __attribute__((unused)) =INVALID_BLOCK_INDEX;

#if defined(TCSUPPORT_NAND_BMT) && !defined(IMAGE_BL2) && ((!defined(LZMA_IMG) && !defined(BOOTROM_EXT)) || defined(TCSUPPORT_BB_256KB))
			u8	data[_SPI_NAND_PAGE_SIZE], oob_mapping[_SPI_NAND_OOB_SIZE];
#endif	
	if(_current_page_num != page_number) {
		ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;

		/* read from read_addr index in the page */
		read_addr = 0;

		/* Switch to manual mode*/
		_SPI_NAND_ENABLE_MANUAL_MODE();

		/* 1. Load Page into cache of NAND Flash Chip */
		rtn_status = spi_nand_load_page_into_cache(page_number);
		if(rtn_status == SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK) {
			_SPI_NAND_PRINTF("spi_nand_read_page: Bad Block, ECC cannot recovery detecte, page=0x%x\n", page_number);
		}

		/* 2. Read whole data from cache of NAND Flash Chip */
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_read_page: curren_page_num=0x%x, page_number=0x%x\n", _current_page_num, page_number);

		/* No matter what status, we must read the cache data to dram */
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spi_nand_read_page: before read, _current_cache_page:\n");
		if(_spi_dma_mode == SPI_DMA_MODE_DISABLE) {
			_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &_current_cache_page[0], ptr_dev_info_t->page_size + ptr_dev_info_t->oob_size);
		} else {
			_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &dma_read_page[0], ptr_dev_info_t->page_size + ptr_dev_info_t->oob_size);
		}
		
		if(ptr_dev_info_t->feature & SPI_NAND_FLASH_PLANE_SELECT_HAVE) {
			_plane_select_bit = ((page_number >> 6)& (0x1));

			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1,"spi_nand_read_page: plane select = 0x%x\n",  _plane_select_bit);			
		}
		
		if(_spi_dma_mode == SPI_DMA_MODE_ENABLE) {	
			SPI_CONTROLLER_Get_Configure(&spi_conf_t);

			spi_conf_t.dummy_byte_num = 0;

			switch (speed_mode)
			{
				case SPI_NAND_FLASH_READ_SPEED_MODE_SINGLE:
					dma_speed_mode = SPI_NFI_MISC_CONTROL_X1;
					read_cmd = _SPI_NAND_OP_READ_FROM_CACHE_SINGLE;
					break;

				case SPI_NAND_FLASH_READ_SPEED_MODE_DUAL:
					dma_speed_mode = SPI_NFI_MISC_CONTROL_X2;
					read_cmd = _SPI_NAND_OP_READ_FROM_CACHE_DUAL;
					if(ptr_dev_info_t->dummy_mode == SPI_NAND_FLASH_READ_DUMMY_BYTE_PREPEND) {
						spi_conf_t.dummy_byte_num = 1;
					}
					break;										
					
				case SPI_NAND_FLASH_READ_SPEED_MODE_QUAD:
					dma_speed_mode = SPI_NFI_MISC_CONTROL_X4;
					read_cmd = _SPI_NAND_OP_READ_FROM_CACHE_QUAD;
					if(ptr_dev_info_t->dummy_mode == SPI_NAND_FLASH_READ_DUMMY_BYTE_PREPEND) {
						spi_conf_t.dummy_byte_num = 1;
					}
					break;

				default:
					_SPI_NAND_PRINTF("[Error] Read DMA : read speed setting error:%d!\n", speed_mode);
					dma_speed_mode = SPI_NFI_MISC_CONTROL_X1;
					read_cmd = _SPI_NAND_OP_READ_FROM_CACHE_SINGLE;
					break;
			}
			
			spi_conf_t.mode = SPI_CONTROLLER_MODE_DMA;
			SPI_CONTROLLER_Set_Configure(&spi_conf_t);

			/* Reset NFI statemachie is neccessary */
			SPI_NFI_Reset();
			
			SPI_NFI_Get_Configure(&spi_nfi_conf_t);
			SPI_NFI_Set_Configure(&spi_nfi_conf_t);

			SPI_ECC_Decode_Get_Configure(&spi_ecc_decode_conf_t);
			SPI_ECC_Decode_Set_Configure(&spi_ecc_decode_conf_t);
			
			if(spi_nfi_conf_t.hw_ecc_t == SPI_NFI_CON_HW_ECC_Enable) {
				SPI_ECC_Decode_Get_Configure(&spi_ecc_decode_conf_t);
				
				if(spi_ecc_decode_conf_t.decode_en == SPI_ECC_DECODE_ENABLE) {
					spi_ecc_decode_conf_t.decode_block_size = ((spi_nfi_conf_t.fdm_ecc_num + 512) * 8) + (spi_ecc_decode_conf_t.decode_ecc_abiliry * 13);

					SPI_ECC_Decode_Set_Configure(&spi_ecc_decode_conf_t);
					
					_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_read_page: decode block size=0x%x, ecc_num=0x%x, ecc_ab=0x%x\n", get_spi_ecc_deccnfg(), (spi_nfi_conf_t.fdm_ecc_num), (spi_ecc_decode_conf_t.decode_ecc_abiliry));
					_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_read_page : SPI_ECC_Decode_Enable \n");

					SPI_ECC_Decode_Disable();
					SPI_ECC_Encode_Disable();					
					SPI_ECC_Decode_Enable();
				}			
			}						

			/* Set plane select */
			if(ptr_dev_info_t->feature & SPI_NAND_FLASH_PLANE_SELECT_HAVE) {
				if(_plane_select_bit == 0) {
					read_addr &= ~(0x1000);
				} else if(_plane_select_bit == 1) {
					read_addr |= (0x1000);
				}				
			}

			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_read_page: dma_speed_mode:%d, read_cmd:%x\n", dma_speed_mode, read_cmd);

			/* Invalid dma_read_page */
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
#ifdef TCSUPPORT_CPU_ARMV8
			/* kernel part */
			/* release the rdma_addr to the device again */
			dma_sync_single_for_device(ecnt_nand->dev, rdma_addr, _SPI_NAND_CACHE_SIZE, DMA_FROM_DEVICE);
#else
			dma_cache_inv((unsigned long)dma_read_page, _SPI_NAND_CACHE_SIZE);
			rdma_addr = K1_TO_PHY(&dma_read_page[0]);
#endif
#else
			/* bootloader part */
			flush_dcache_range((unsigned long)dma_read_page, (unsigned long)(dma_read_page + _SPI_NAND_CACHE_SIZE));
			rdma_addr = K1_TO_PHY(&dma_read_page[0]);
#endif
			mb();

#if defined(TCSUPPORT_PARALLEL_NAND)
			if (_parallel_nand_mode)
			{
				rtn_status = parallel_nand_dma_read(read_addr, page_number, (unsigned long *)rdma_addr);
			}
			else
#endif
			{
				nfi_status = SPI_NFI_Read_SPI_NAND_Page(dma_speed_mode, read_cmd, read_addr, (unsigned long *)rdma_addr);
				if(nfi_status != SPI_NFI_RTN_NO_ERROR) {
					rtn_status = SPI_NAND_FLASH_RTN_NFI_FAIL;
				}
			}

#if ((defined(TCSUPPORT_CPU_ARMV8)) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)))
			/* get hold of the rdma_addr temporarily to do read */
			dma_sync_single_for_cpu(ecnt_nand->dev, rdma_addr, _SPI_NAND_CACHE_SIZE, DMA_FROM_DEVICE);
#endif

			if(spi_nfi_conf_t.hw_ecc_t == SPI_NFI_CON_HW_ECC_Enable) {
				if(spi_ecc_decode_conf_t.decode_en == SPI_ECC_DECODE_ENABLE) {
					/* Check Decode done or not */
					for(check_cnt = 0 ; check_cnt < _SPI_NFI_CHECK_ECC_DONE_MAX_TIMES; check_cnt++) {
	 					ecc_decode_done_bit = SPI_ECC_Decode_Check_Done(&decode_status_t, spi_nfi_conf_t.sec_num);
						
						if(decode_status_t == SPI_ECC_DECODE_STATUS_DONE) {
							break;
						}
					}
					if(check_cnt == _SPI_NFI_CHECK_ECC_DONE_MAX_TIMES) {		
						_SPI_NAND_PRINTF("[Error] Read ECC : Check Decode Done Timeout ! ecc_decode_done_bit: 0x%x\n", ecc_decode_done_bit);

						SPI_NAND_Flash_Clear_Read_Cache_Data();

						/*  return  somthing ? */
						rtn_status = SPI_NAND_FLASH_RTN_ECC_DECODE_FAIL;
					}

					if(SPI_ECC_DECODE_Check_Correction_Status() == SPI_ECC_RTN_CORRECTION_ERROR) {
						_SPI_NAND_PRINTF("[Error] Read ECC : ECC Fail! page:0x%x\n", page_number);
						SPI_NAND_Flash_Clear_Read_Cache_Data();

						/* Switch to manual mode*/
						_SPI_NAND_ENABLE_MANUAL_MODE();
						
						/*  return somthing ?*/
						rtn_status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
					}
				}
			}

			/* Switch to manual mode*/
			_SPI_NAND_ENABLE_MANUAL_MODE();
		} else {
			spi_nand_protocol_read_from_cache(read_addr, ((ptr_dev_info_t->page_size)+(ptr_dev_info_t->oob_size)), &_current_cache_page[0], speed_mode, ptr_dev_info_t->dummy_mode );
		}

		/* Divide read page into data segment and oob segment  */
		if((_spi_dma_mode == SPI_DMA_MODE_DISABLE) || 
		  ((_spi_dma_mode == SPI_DMA_MODE_ENABLE) && (!isSpiNandAndCtrlECC) && (spi_nfi_conf_t.auto_fdm_t == SPI_NFI_CON_AUTO_FDM_Disable) && (spi_nfi_conf_t.hw_ecc_t == SPI_NFI_CON_HW_ECC_Disable)))
		{
			
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spi_nand_read_page: after read, _current_cache_page:\n");

			if(_spi_dma_mode == SPI_DMA_MODE_DISABLE) {
				_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &_current_cache_page[0], ptr_dev_info_t->page_size + ptr_dev_info_t->oob_size);
				memcpy(&_current_cache_page_data[0], &_current_cache_page[0], ptr_dev_info_t->page_size);
				memcpy(&_current_cache_page_oob[0],  &_current_cache_page[ptr_dev_info_t->page_size], ptr_dev_info_t->oob_size);
			} else {
				_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &dma_read_page[0], ptr_dev_info_t->page_size + ptr_dev_info_t->oob_size);
				memcpy(&_current_cache_page_data[0], &dma_read_page[0], ptr_dev_info_t->page_size);
				memcpy(&_current_cache_page_oob[0],  &dma_read_page[ptr_dev_info_t->page_size], ptr_dev_info_t->oob_size);
			}
			
			/*  When chip->ecc.mode = NAND_ECC_NONE,  linux will automatic get user data from oob_free_layout*/
			memcpy( &_current_cache_page_oob_mapping[0],  &_current_cache_page_oob[0], (ptr_dev_info_t->oob_size) );
		}
		else
		{	
			
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spi_nand_read_page: after read, _current_cache_page:\n");
			_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &dma_read_page[0], ptr_dev_info_t->page_size + ptr_dev_info_t->oob_size);

			if(spi_nfi_conf_t.auto_fdm_t == SPI_NFI_CON_AUTO_FDM_Disable)  
			{
				offset1 = 0;
				offset2 = 0;
				offset3 = 0;
				dma_sec_size = ((spi_nfi_conf_t.page_size_t)/ (spi_nfi_conf_t.sec_num));
				for(i = 0; i < spi_nfi_conf_t.sec_num; i++) {
					memcpy( &_current_cache_page_data[offset1], &dma_read_page[offset2], dma_sec_size);
					memcpy( &_current_cache_page_oob[offset3], &dma_read_page[offset2+dma_sec_size], (spi_nfi_conf_t.spare_size_t) );
					offset1 += dma_sec_size;
					offset2 += (dma_sec_size+ (spi_nfi_conf_t.spare_size_t));
					offset3 += (spi_nfi_conf_t.spare_size_t);					
				}

				offset1 = 0;
				offset2 = 0;
				for(i = 0; i < spi_nfi_conf_t.sec_num; i++) {
					memcpy( &_current_cache_page_oob_mapping[offset1], &_current_cache_page_oob[offset2], spi_nfi_conf_t.fdm_num);
					offset1 += spi_nfi_conf_t.fdm_num;
					offset2 += spi_nfi_conf_t.spare_size_t;
				}
			}
			else /* Auto FDM enable : Data and oob alternate ,  Data inside DRAM , oob inside NFI register */  
			{
				memcpy( &_current_cache_page_data[0], &dma_read_page[0], (ptr_dev_info_t->page_size) );
				SPI_NFI_Read_SPI_NAND_FDM(&_current_cache_page_oob[0], (ptr_dev_info_t->oob_size));
				memcpy( &_current_cache_page_oob_mapping[0], &_current_cache_page_oob[0], ptr_dev_info_t->oob_size);
			}
		}
		
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spi_nand_read_page: _current_cache_page:\n");
		if(_spi_dma_mode == SPI_DMA_MODE_DISABLE) {
			_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &_current_cache_page[0], ptr_dev_info_t->page_size + ptr_dev_info_t->oob_size);
		} else {
			_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &dma_read_page[0], ptr_dev_info_t->page_size + ptr_dev_info_t->oob_size);
		}
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spi_nand_read_page: _current_cache_page_data:\n");
		_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &_current_cache_page_data[0], ptr_dev_info_t->page_size);
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spi_nand_read_page: _current_cache_page_oob:\n");
		_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &_current_cache_page_oob[0], ptr_dev_info_t->oob_size);		
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spi_nand_read_page: _current_cache_page_oob_mapping:\n");
		_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &_current_cache_page_oob_mapping[0], (ptr_dev_info_t->oob_free_layout)->oobsize);

#if defined(TCSUPPORT_NAND_BMT) && !defined(IMAGE_BL2) && ((!defined(LZMA_IMG) && !defined(BOOTROM_EXT)) || defined(TCSUPPORT_BB_256KB))
		if (rtn_status == SPI_NAND_FLASH_RTN_ECC_EXCEEDED_THRESHOLD)
		{
			current_block=page_number/PAGE_CNT_PER_BLOCK;
			if((current_block==ecc_threshold_exceeded_block)){
				_SPI_NAND_PRINTF("Block %lu ECC exceeded threshold!! Skipping read operation on this block\n",current_block);
				return -1;
			}
			else{
				ecc_threshold_exceeded_block=current_block;
			}

			

			memset(data, 0xFF, _SPI_NAND_PAGE_SIZE);
			memset(oob_mapping, 0xFF, _SPI_NAND_OOB_SIZE);

			memcpy( &data[0], &_current_cache_page_data[0], (ptr_dev_info_t->page_size) );
			memcpy( &oob_mapping[0], &_current_cache_page_oob_mapping[0], (ptr_dev_info_t->oob_free_layout)->oobsize );

			_SPI_NAND_PRINTF("read fail at page: %d\n", page_number);
			if(update_bmt((page_number * ptr_dev_info_t->page_size), UPDATE_READ_CHECK_FAIL, &data[0], &oob_mapping[0]))
			{
				_SPI_NAND_PRINTF("Update BMT success\n");
				rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
			}
			else
			{
				_SPI_NAND_PRINTF("Update BMT fail\n");
			}

			/* When write page, _current_cache_page_data will use in write data and it be modified. Need to restore original data. */
			memcpy( &_current_cache_page_data[0], &data[0], (ptr_dev_info_t->page_size) );
			memcpy( &_current_cache_page_oob_mapping[0], &oob_mapping[0], (ptr_dev_info_t->oob_free_layout)->oobsize );

			SPI_NAND_Flash_Clear_Read_Cache_Data();
			ecc_threshold_exceeded_block=INVALID_BLOCK_INDEX;
		}
		else
#endif
		{
			_current_page_num = page_number;
		}
	}	

	return rtn_status;
}

#if !defined(LZMA_IMG) && !defined(BOOTROM_EXT)
static SPI_NAND_FLASH_RTN_T spi_nand_boundary_check(
												#if defined(TCSUPPORT_2_6_36_KERNEL) || defined(TCSUPPORT_3_18_21_KERNEL)
													u64 addr,
												#else
													u32 addr,
												#endif
													u32 len)
{
	SPI_NAND_FLASH_RTN_T			rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t = _SPI_NAND_GET_DEVICE_INFO_PTR;
#if defined(TCSUPPORT_2_6_36_KERNEL) || defined(TCSUPPORT_3_18_21_KERNEL)
	u64								end_addr = 0;
#else
	u32								end_addr = 0;
#endif

	end_addr = (addr + len);

	if (end_addr> ptr_dev_info_t->device_size)
	{
		rtn_status = SPI_NAND_FLASH_RTN_ALIGNED_CHECK_FAIL;

#if defined(TCSUPPORT_2_6_36_KERNEL) || defined(TCSUPPORT_3_18_21_KERNEL)
		_SPI_NAND_PRINTF("[Error] Over Device Size : end_addr=0x%llx, device_size=0x%x!\n", end_addr, ptr_dev_info_t->device_size);
		dump_stack();
#else
		_SPI_NAND_PRINTF("[Error] Over Device Size : end_addr=0x%x, device_size=0x%x!\n", end_addr, ptr_dev_info_t->device_size);
#endif
	}

	return rtn_status;
		
}
#endif

#if !defined(BOOTROM_EXT)
/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T spi_nand_read_internal( u32     addr,
 *                                                               u32     len,
 *                                                               u8      *ptr_rtn_buf )
 * PURPOSE : To read flash internally.
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : addr         - The addr variable of this function.
 *           len          - The len variable of this function.
 *   OUTPUT: ptr_rtn_buf  - A pointer to the ptr_rtn_buf variable.
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/19  - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
static SPI_NAND_FLASH_RTN_T spi_nand_read_internal(
												#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
													u64 addr,
												#else
													u32 addr,
												#endif
													u32 len, 
													u8 *ptr_rtn_buf, 
													SPI_NAND_FLASH_READ_SPEED_MODE_T speed_mode,
													SPI_NAND_FLASH_RTN_T *status)
{
	u32			 					page_number, data_offset;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
	/* for exceed 64bits address */
	u64								read_addr, physical_read_addr;
#else
	u32								read_addr, physical_read_addr;
#endif
	u32			 					remain_len, logical_block, physical_block;
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	SPI_NAND_FLASH_RTN_T			rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	SPI_ECC_RTN_T					ecc_status = SPI_ECC_RTN_NO_ERROR;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
	unsigned long					spinand_spinlock_flags;
#endif
	SPI_NFI_CONF_T					spi_nfi_conf_t;
	SPI_ECC_DECODE_CONF_T			spi_ecc_decode_conf_t;
	
#if	defined(TCSUPPORT_NAND_BMT) && ((!defined(LZMA_IMG) && !defined(BOOTROM_EXT)) || defined(TCSUPPORT_BB_256KB))
    unsigned short phy_block_bbt;
	unsigned long  addr_offset_in_block;
	/* for exceed 64bits address */
	u64	physical_block_tmp, erase_size_tmp;
#endif
	
	if((0xbc000000 <= addr) && (addr <= 0xbfffffff)) {		/* Reserver address area for system */
		if( (addr & 0xbfc00000) == 0xbfc00000) {
			addr &= 0x003fffff;
		} else {
			addr &= 0x03ffffff;
		}
	}

	SPI_NFI_Get_Configure(&spi_nfi_conf_t);
	SPI_ECC_Decode_Get_Configure(&spi_ecc_decode_conf_t);
		
	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;			
	read_addr  		= addr;
	remain_len 		= len;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "\nspi_nand_read_internal : addr=0x%llx, len=0x%x\n", addr, len );
#else
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "\nspi_nand_read_internal : addr=0x%lx, len=0x%x\n", addr, len );
#endif

#if !defined(LZMA_IMG) && !defined(BOOTROM_EXT)
	rtn_status = spi_nand_boundary_check(read_addr, remain_len);
	if (rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR)
		return rtn_status;
#endif

	_SPI_NAND_SEMAPHORE_LOCK();

	*status = SPI_NAND_FLASH_RTN_NO_ERROR;

	while(remain_len > 0) {
		physical_read_addr = read_addr;

#if	defined(TCSUPPORT_NAND_BMT) && ((!defined(LZMA_IMG) && !defined(BOOTROM_EXT)) || defined(TCSUPPORT_BB_256KB))
		if(otp_status == NAND_FLASH_OTP_DISABLE) {
            #if defined(TCSUPPORT_CPU_ARMV8) && !defined(TCSUPPORT_CPU_ARMV8_64)
            logical_block = div_u64_rem(read_addr, ptr_dev_info_t->erase_size, &addr_offset_in_block);
            #else
            addr_offset_in_block = (read_addr % ptr_dev_info_t->erase_size);
			logical_block = (read_addr / (ptr_dev_info_t->erase_size));
            #endif
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1,"read_addr_high=0x%x, read_addr_low=0x%x, erase size =0x%x, logical_block =0x%x\n", (u32)((read_addr >> 32) & 0xffffffff), (u32)(read_addr & 0xffffffff), (ptr_dev_info_t->erase_size), logical_block);
#else
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1,"read_addr=0x%x, erase size =0x%x, logical_block =0x%x\n", read_addr, (ptr_dev_info_t->erase_size), logical_block);
#endif
			physical_block = get_mapping_block_index(logical_block, &phy_block_bbt);
			/* for coverity(CID:186222), avoid overflow */
			physical_block_tmp = physical_block;
			erase_size_tmp = ptr_dev_info_t->erase_size;
			physical_read_addr = (physical_block_tmp * erase_size_tmp) + addr_offset_in_block;
			if(physical_block != logical_block) {			
				_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "Bad Block Mapping, from %d block to %d block\n", logical_block, physical_block);
			}
		}
#endif					
		/* Caculate page number */
        #if defined(TCSUPPORT_CPU_ARMV8) && !defined(TCSUPPORT_CPU_ARMV8_64)
        page_number = div_u64_rem(physical_read_addr, ptr_dev_info_t->page_size, &data_offset);
        #else
        data_offset = (physical_read_addr % (ptr_dev_info_t->page_size));
		page_number = (physical_read_addr / (ptr_dev_info_t->page_size));
        #endif
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_read_internal: read_addr_high=0x%x, read_addr_low=0x%x, page_number=0x%x, data_offset=0x%x\n", (u32)((physical_read_addr >> 32) & 0xffffffff), (u32)(physical_read_addr & 0xffffffff), page_number, data_offset);
#else
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_read_internal: read_addr=0x%x, page_number=0x%x, data_offset=0x%x\n", physical_read_addr, page_number, data_offset);
#endif

		if(len == 1 && _current_page_num == page_number) {
			ptr_rtn_buf[0] = _current_cache_page_data[data_offset];
			_SPI_NAND_SEMAPHORE_UNLOCK();
			return rtn_status;
		}

		rtn_status = spi_nand_read_page(page_number, speed_mode);
		if(_spi_dma_mode == SPI_DMA_MODE_ENABLE) {
			if((spi_nfi_conf_t.hw_ecc_t == SPI_NFI_CON_HW_ECC_Enable) &&
			   (spi_ecc_decode_conf_t.decode_en == SPI_ECC_DECODE_ENABLE)) {
				ecc_status = SPI_ECC_DECODE_Check_Correction_Status();
				if(ecc_status == SPI_ECC_RTN_NO_ERROR ) {
					*status = SPI_NAND_FLASH_RTN_NO_ERROR;
				} else {
					*status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
					_SPI_NAND_SEMAPHORE_UNLOCK();
					return (rtn_status);
				}
			} else {
				if(rtn_status == SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK) {
					*status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
					_SPI_NAND_SEMAPHORE_UNLOCK();
					return (rtn_status);
				}
			}
		} else {
			if(rtn_status == SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK) {
				*status = SPI_NAND_FLASH_RTN_DETECTED_BAD_BLOCK;
				_SPI_NAND_SEMAPHORE_UNLOCK();
				return (rtn_status);
			}
		}

		/* 3. Retrieve the request data */
		if((data_offset + remain_len) < ptr_dev_info_t->page_size) {
			memcpy( &ptr_rtn_buf[len - remain_len], &_current_cache_page_data[data_offset], remain_len);
			remain_len =0;
		} else {
			memcpy( &ptr_rtn_buf[len - remain_len], &_current_cache_page_data[data_offset], ptr_dev_info_t->page_size - data_offset);
			remain_len -= (ptr_dev_info_t->page_size - data_offset);
			read_addr += (ptr_dev_info_t->page_size - data_offset);
		}
	}
	
	_SPI_NAND_SEMAPHORE_UNLOCK();

	return (rtn_status);
}

/*------------------------------------------------------------------------------------
 * FUNCTION: int SPI_NAND_Flash_Read_NByte( long     addr,
 *                                          long     len,
 *                                          long     *retlen,
 *                                          char     *buf    )
 * PURPOSE : To provide interface for Read N Bytes from SPI NAND Flash.
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : addr    - The addr variable of this function.
 *           len     - The len variable of this function.
 *           retlen  - The retlen variable of this function.
 *           buf     - The buf variable of this function.
 *   OUTPUT: None
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/12  - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
u32 SPI_NAND_Flash_Read_NByte(	
							#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
								u64 							addr,
							#else
								u32 							addr,
							#endif
								u32								 len, 
								u32								 *retlen, 
								u8								 *buf, 
								SPI_NAND_FLASH_READ_SPEED_MODE_T speed_mode,
								SPI_NAND_FLASH_RTN_T *status)
{
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "SPI_NAND_Flash_Read_NByte\n");
	
	return spi_nand_read_internal(addr, len, buf, speed_mode, status);	
}

/*------------------------------------------------------------------------------------
 * FUNCTION: char SPI_NAND_Flash_Read_Byte( long     addr )
 * PURPOSE : To provide interface for read 1 Bytes from SPI NAND Flash.
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : addr - The addr variable of this function.
 *   OUTPUT: None
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/12  - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
unsigned char SPI_NAND_Flash_Read_Byte(
									#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
										u64 addr,
									#else
										u32 addr,
									#endif
										SPI_NAND_FLASH_RTN_T *status)
{
	unsigned char 					ch;
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
	int 							ret = 0;
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
	struct mtd_oob_ops ops;
	#else
	size_t 							retlen;
    #endif
#endif

	ptr_dev_info_t  = _SPI_NAND_GET_DEVICE_INFO_PTR;

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "SPI_NAND_Flash_Read_Byte\n");

#ifdef TCSUPPORT_BB_256KB
	/* The mi.conf has new offset(0x3FE00) in 256KB tcboot.bin,
	 * The mi.conf offset is 0xFF00 in 128KB tcboot.bin.
	 */
	if(addr >= 0xFF00 && addr <= 0xFFFF) {
		addr += 0x2FF00;
	}
#endif

#if defined(TCSUPPORT_DSL_PHYMODE)
	memcpy(&ch, (addr), sizeof(ch));
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	spi_nand_read_internal(addr, 1, &ch, ptr_dev_info_t->read_mode, status);
#elif defined(TCSUPPORT_2_6_36_KERNEL)
	ret = spi_nand_mtd->read(spi_nand_mtd, (loff_t)addr, 1, &retlen, &ch);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0)
    ret = spi_nand_mtd->_read(spi_nand_mtd, (loff_t)addr, 1, &retlen, &ch);
#else
    memset(&ops, 0, sizeof(ops));
    ops.len = 1;
    ops.datbuf = &ch;
    ops.mode = MTD_OPS_PLACE_OOB;
	ret = spi_nand_mtd->_read_oob(spi_nand_mtd, (loff_t)addr, &ops);
#endif

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "SPI_NAND_Flash_Read_Byte : buf=0x%x\n", ch);
	
	return ch;
}

/*------------------------------------------------------------------------------------
 * FUNCTION: long SPI_NAND_Flash_Read_DWord( long    addr )
 * PURPOSE : To provide interface for read Double Word from SPI NAND Flash.
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : addr - The addr variable of this function.
 *   OUTPUT: None
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/12  - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
unsigned long SPI_NAND_Flash_Read_DWord(
									#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
										u64 addr,
									#else
										u32 addr,
									#endif
										SPI_NAND_FLASH_RTN_T *status)
{
	u32		 ret_val=0;
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
	int								ret = 0;
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
	struct mtd_oob_ops ops;
	#else
	size_t							retlen;
    #endif
#endif

	ptr_dev_info_t  = _SPI_NAND_GET_DEVICE_INFO_PTR;
	
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "SPI_NAND_Flash_Read_DWord\n");

#if defined(TCSUPPORT_DSL_PHYMODE)
	memcpy(&ret_val, (addr), sizeof(ret_val));
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	spi_nand_read_internal(addr, 4, (u8 *) &ret_val, ptr_dev_info_t->read_mode, status);
#elif defined(TCSUPPORT_2_6_36_KERNEL)
	ret = spi_nand_mtd->read(spi_nand_mtd, (loff_t)addr, 4, &retlen, &ret_val);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0)
	ret = spi_nand_mtd->_read(spi_nand_mtd, (loff_t)addr, 4, &retlen, &ret_val);
#else
    memset(&ops, 0, sizeof(ops));
    ops.len = 4;
    ops.datbuf = (uint8_t *)&ret_val;
    ops.mode = MTD_OPS_PLACE_OOB;
	ret = spi_nand_mtd->_read_oob(spi_nand_mtd, (loff_t)addr, &ops);
#endif

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "SPI_NAND_Flash_Read_DWord : ret_val=0x%x\n", ret_val);
	
	return ret_val;
}

int nandflash_read(unsigned long from, unsigned long len, u32 *retlen, unsigned char *buf, SPI_NAND_FLASH_RTN_T *status)
{
	struct SPI_NAND_FLASH_INFO_T	 *ptr_dev_info_t;
	 
	ptr_dev_info_t  = _SPI_NAND_GET_DEVICE_INFO_PTR;

	if( SPI_NAND_Flash_Read_NByte(from, len, retlen, buf, ptr_dev_info_t->read_mode, status) == SPI_NAND_FLASH_RTN_NO_ERROR) {
		return 0;
	} else {
		return -1;
	}	
}
#endif



#if !defined(LZMA_IMG) || defined(TCSUPPORT_BB_256KB)
/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T spi_nand_protocol_write_enable( void )
 * PURPOSE : To implement the SPI nand protocol for write enable.
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : None
 *   OUTPUT: None
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/17  - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
SPI_NAND_FLASH_RTN_T spi_nand_protocol_write_enable( void )
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	SPI_NAND_FLASH_RTN_T	feature_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	u8						feature;

	rtn_status = spi_nand_protocol_chk_status(_SPI_NAND_VAL_OIP, 0);
	if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
		_SPI_NAND_PRINTF("[Error] write enable Timeout.\n");
		return (rtn_status);
	}

	if (_parallel_nand_mode)
	{
		return rtn_status;
	}

	/* 1. Chip Select Low */
	_SPI_NAND_READ_CHIP_SELECT_LOW();
	
	/* 2. Write op_cmd 0x06 (Write Enable (WREN)*/
	_SPI_NAND_WRITE_ONE_BYTE( _SPI_NAND_OP_WRITE_ENABLE );
	
	/* 3. Chip Select High */
	_SPI_NAND_READ_CHIP_SELECT_HIGH();

	/*	Checking status for write enable */
	rtn_status = spi_nand_protocol_chk_status(_SPI_NAND_VAL_WEL, _SPI_NAND_VAL_WEL);
	if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
		feature_status = spi_nand_protocol_get_feature(_SPI_NAND_ADDR_STATUS, &feature);
		if(feature_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
			_SPI_NAND_PRINTF("[Error] get status fail!\n");
			return feature_status;
		}
		_SPI_NAND_PRINTF("[Error] wait for WEL bit = 1 Timeout, status=0x%x!\n", feature);
		return (rtn_status);
	}
	
	return (rtn_status);
}

/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T spi_nand_protocol_write_disable( void )
 * PURPOSE : To implement the SPI nand protocol for write disable.
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : None
 *   OUTPUT: None
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/17  - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
SPI_NAND_FLASH_RTN_T spi_nand_protocol_write_disable( void )
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	SPI_NAND_FLASH_RTN_T	feature_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	u8						feature;

	rtn_status = spi_nand_protocol_chk_status(_SPI_NAND_VAL_OIP, 0);
	if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
		_SPI_NAND_PRINTF("[Error] write disable Timeout.\n");
		return (rtn_status);
	}

	if (_parallel_nand_mode)
	{
		return rtn_status;
	}

	/* 1. Chip Select Low */
	_SPI_NAND_READ_CHIP_SELECT_LOW();
	
	/* 2. Write op_cmd 0x04 (Write Disable (WRDI)*/
	_SPI_NAND_WRITE_ONE_BYTE( _SPI_NAND_OP_WRITE_DISABLE );
	
	/* 3. Chip Select High */
	_SPI_NAND_READ_CHIP_SELECT_HIGH();

	/*	Checking status for write disable */
	rtn_status = spi_nand_protocol_chk_status(_SPI_NAND_VAL_WEL, 0);
	if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
		feature_status = spi_nand_protocol_get_feature(_SPI_NAND_ADDR_STATUS, &feature);
		if(feature_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
			_SPI_NAND_PRINTF("[Error] get status fail!\n");
			return feature_status;
		}
		_SPI_NAND_PRINTF("[Error] wait for WEL bit = 0 Timeout, status=0x%x!\n", feature);
		return (rtn_status);
	}
	
	return (rtn_status);
}

/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T spi_nand_protocol_program_load( u32     addr,
 *                                                                       u8      *ptr_data,
 *                                                                       u32     len,
 *																		 u32 write_mode)
 * PURPOSE : To implement the SPI nand protocol for program load, with single speed.
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : addr      - The addr variable of this function.
 *           ptr_data  - A pointer to the ptr_data variable.
 *           len       - The len variable of this function.
 *   OUTPUT: None
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 *
 *------------------------------------------------------------------------------------
 */
static SPI_NAND_FLASH_RTN_T spi_nand_protocol_program_load ( u32 addr, 
																		  u8 *ptr_data, 
																		  u32 len,
																		  u32 write_mode)
{

	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	
	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;
	
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_protocol_program_load: addr=0x%x, len=0x%x\n", addr, len );

	rtn_status = spi_nand_protocol_chk_status(_SPI_NAND_VAL_OIP, 0);
	if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
		_SPI_NAND_PRINTF("[Error] program load Timeout.\n");
		return (rtn_status);
	}
	
	/* 1. Chip Select low */
	_SPI_NAND_READ_CHIP_SELECT_LOW();
	
	/* 2. Send opcode */
	switch (write_mode)
	{
		/* 02h */
		case SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE:
			_SPI_NAND_WRITE_ONE_BYTE( _SPI_NAND_OP_PROGRAM_LOAD_SINGLE );
			break;

		/* 32h */
		case SPI_NAND_FLASH_WRITE_SPEED_MODE_QUAD:
			_SPI_NAND_WRITE_ONE_BYTE( _SPI_NAND_OP_PROGRAM_LOAD_QUAD );
			break;										

		default:
			break;
	}
	
	/* 3. Send address offset */
	if( ((ptr_dev_info_t->feature) & SPI_NAND_FLASH_PLANE_SELECT_HAVE) )
	{
		if( _plane_select_bit == 0)
		{
			_SPI_NAND_WRITE_ONE_BYTE( ((addr >> 8 ) & (0xef)) );
		}
		if( _plane_select_bit == 1)
		{
			_SPI_NAND_WRITE_ONE_BYTE( ((addr >> 8 ) | (0x10)) );
		}				
	}	
	else
	{
		_SPI_NAND_WRITE_ONE_BYTE( ((addr >> 8 ) & (0xff)) );
	}

	_SPI_NAND_WRITE_ONE_BYTE( ((addr)        & (0xff)) );
	
	/* 4. Send data */
	switch (write_mode)
	{
		case SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE:
			_SPI_NAND_WRITE_NBYTE( ptr_data, len, SPI_CONTROLLER_SPEED_SINGLE);
			break;

		case SPI_NAND_FLASH_WRITE_SPEED_MODE_QUAD:
			_SPI_NAND_WRITE_NBYTE( ptr_data, len, SPI_CONTROLLER_SPEED_QUAD);
			break;										

		default:
			break;
	}
	
	/* 5. Chip Select High */
	_SPI_NAND_READ_CHIP_SELECT_HIGH();	
	
	return (rtn_status);
}

#if !defined(IMAGE_BL2)
/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T spi_nand_protocol_program_load_random( u32     addr,
 *                                                                       u8      *ptr_data,
 *                                                                       u32     len,
 *																		 u32 write_mode)
 * PURPOSE : To implement the SPI nand protocol for program load, with single speed.
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : addr      - The addr variable of this function.
 *           ptr_data  - A pointer to the ptr_data variable.
 *           len       - The len variable of this function.
 *   OUTPUT: None
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/17  - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
__attribute__((unused)) static SPI_NAND_FLASH_RTN_T spi_nand_protocol_program_load_random ( u32 addr, 
																  u8 *ptr_data, 
																  u32 len,
																  u32 write_mode)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_protocol_program_load_random: addr=0x%x, len=0x%x\n", addr, len );

	rtn_status = spi_nand_protocol_chk_status(_SPI_NAND_VAL_OIP, 0);
	if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
		_SPI_NAND_PRINTF("[Error] program load random Timeout.\n");
		return (rtn_status);
	}
	
	/* 1. Chip Select low */
	_SPI_NAND_READ_CHIP_SELECT_LOW();
	
	/* 2. Send opcode */
	switch (write_mode)
	{
		/* 84 */
		case SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE:
			_SPI_NAND_WRITE_ONE_BYTE( _SPI_NAND_OP_PROGRAM_LOAD_RAMDOM_SINGLE );
			break;

		/* 34h */
		case SPI_NAND_FLASH_WRITE_SPEED_MODE_QUAD:
			_SPI_NAND_WRITE_ONE_BYTE( _SPI_NAND_OP_PROGRAM_LOAD_RAMDON_QUAD );
			break;										

		default:
			break;
	}
	
	/* 3. Send address offset */
	_SPI_NAND_WRITE_ONE_BYTE( ((addr >> 8  ) & 0xff) );
	_SPI_NAND_WRITE_ONE_BYTE( ((addr)        & 0xff) );
	
	/* 4. Send data */
	switch (write_mode)
	{
		case SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE:
			_SPI_NAND_WRITE_NBYTE( ptr_data, len, SPI_CONTROLLER_SPEED_SINGLE);
			break;

		case SPI_NAND_FLASH_WRITE_SPEED_MODE_QUAD:
			_SPI_NAND_WRITE_NBYTE( ptr_data, len, SPI_CONTROLLER_SPEED_QUAD);
			break;										

		default:
			break;
	}
	
	/* 5. Chip Select High */
	_SPI_NAND_READ_CHIP_SELECT_HIGH();	
	
	return (rtn_status);
}
#endif

/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T spi_nand_protocol_program_execute( u32  addr )
 * PURPOSE : To implement the SPI nand protocol for program execute.
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : addr - The addr variable of this function.
 *   OUTPUT: None
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/17  - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
SPI_NAND_FLASH_RTN_T spi_nand_protocol_program_execute ( u32 addr )
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	u8						status;

	
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_protocol_program_execute: addr=0x%x\n", addr);	

	rtn_status = spi_nand_protocol_chk_status(_SPI_NAND_VAL_OIP, 0);
	if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
		_SPI_NAND_PRINTF("[Error] program execute Timeout.\n");
		return (rtn_status);
	}
	
	/* 1. Chip Select low */
	_SPI_NAND_READ_CHIP_SELECT_LOW();
	
	/* 2. Send 10h opcode */
	_SPI_NAND_WRITE_ONE_BYTE(_SPI_NAND_OP_PROGRAM_EXECUTE);
	
	/* 3. Send address offset */
	_SPI_NAND_WRITE_ONE_BYTE( ((addr >> 16) & 0xff) );
	_SPI_NAND_WRITE_ONE_BYTE( ((addr >> 8 ) & 0xff) );
	_SPI_NAND_WRITE_ONE_BYTE( ((addr)       & 0xff) );
	
	/* 4. Chip Select High */
	_SPI_NAND_READ_CHIP_SELECT_HIGH();	

	rtn_status = spi_nand_protocol_chk_status(_SPI_NAND_VAL_OIP, 0);
	if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
		_SPI_NAND_PRINTF("[Error] program execute Timeout.\n");
		return (rtn_status);
	}

	/*	Check Program Fail Bit */
	rtn_status = spi_nand_protocol_get_feature(_SPI_NAND_ADDR_STATUS, &status);
	if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
		_SPI_NAND_PRINTF("[Error] get status fail!\n");
		return rtn_status;
	}

	if(status & _SPI_NAND_VAL_PROGRAM_FAIL) {
		_SPI_NAND_PRINTF("spi_nand_write_page : Program Fail at addr=0x%x, status=0x%x\n", addr, status);
		rtn_status = SPI_NAND_FLASH_RTN_PROGRAM_FAIL;
	}
	
	return (rtn_status);
}

static SPI_NAND_FLASH_RTN_T spi_nand_dma_program_load(u32 addr, u32 oob_len, SPI_NAND_FLASH_WRITE_SPEED_MODE_T speed_mode)
{
	SPI_CONTROLLER_CONF_T	spi_conf_t; 	
	SPI_NFI_CONF_T			spi_nfi_conf_t;
	SPI_ECC_ENCODE_CONF_T	spi_ecc_encode_conf_t;
	SPI_NAND_FLASH_RTN_T	rtn_status;
	u16 					write_addr;
	u32						write_cmd;
	SPI_NFI_MISC_SPEDD_CONTROL_T	dma_speed_mode;
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;

	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;	

	/* Set plane select */
	write_addr = addr;
	if(ptr_dev_info_t->feature & SPI_NAND_FLASH_PLANE_SELECT_HAVE) {
		if(_plane_select_bit == 0) {
			write_addr &= ~(0x1000);
		} else if(_plane_select_bit == 1) {
			write_addr |= (0x1000);
		}				
	}

	switch (speed_mode)
	{
		case SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE:
			dma_speed_mode = SPI_NFI_MISC_CONTROL_X1;
			write_cmd = _SPI_NAND_OP_PROGRAM_LOAD_SINGLE;
			break;
						
		case SPI_NAND_FLASH_WRITE_SPEED_MODE_QUAD:
			dma_speed_mode = SPI_NFI_MISC_CONTROL_X4;
			write_cmd = _SPI_NAND_OP_PROGRAM_LOAD_QUAD;
			break;

		default:
			_SPI_NAND_PRINTF("[Error] Write DMA : write speed setting error:%d!\n", speed_mode);
			dma_speed_mode = SPI_NFI_MISC_CONTROL_X1;
			write_cmd = _SPI_NAND_OP_PROGRAM_LOAD_SINGLE;
			break;
	}

	SPI_CONTROLLER_Get_Configure(&spi_conf_t);
	spi_conf_t.dummy_byte_num = 0 ;
	spi_conf_t.mode = SPI_CONTROLLER_MODE_DMA;
	SPI_CONTROLLER_Set_Configure(&spi_conf_t);

	/* Reset NFI statemachie is neccessary */
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

	if(spi_nfi_conf_t.auto_fdm_t == SPI_NFI_CON_AUTO_FDM_Enable) {	/* Data and oob alternate */
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_dma_program_load: set fdm\n");
		SPI_NFI_Write_SPI_NAND_FDM(&_current_cache_page_oob_mapping[0], oob_len);
	}

	rtn_status = (SPI_NAND_FLASH_RTN_T)SPI_NFI_Write_SPI_NAND_page(dma_speed_mode, write_cmd, write_addr, (unsigned long *)wdma_addr);

	/* Switch to manual mode*/
	_SPI_NAND_ENABLE_MANUAL_MODE();	

	return rtn_status;
}

#if !defined(IMAGE_BL2) && !defined(BOOTROM_EXT) && !defined(LZMA_IMG)
UBIFS_BLANK_PAGE_ECC_T check_blank_page(u32 page_number)
{
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	u32 block __attribute__((unused)), page_per_block, i = 0;
	u8 sec_idx;
	u8 ecc_parity_0[8] = {0};
	u8 ctlerECC_blank_ecc_4[] = {0x26, 0x20, 0x98, 0x1b, 0x87, 0x6e, 0xfc, 0xff};
	SPI_NAND_FLASH_DEBUG_LEVEL_T ubiDbgLv __attribute__((unused)) = SPI_NAND_FLASH_DEBUG_LEVEL_2;
	SPI_NFI_CONF_T spi_nfi_conf_t;
	int cmpVal;

	
	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;

	page_per_block = (ptr_dev_info_t->erase_size / ptr_dev_info_t->page_size);
	block = page_number / page_per_block;

	if ((!isSpiNandAndCtrlECC) && (ptr_dev_info_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_SKYHIGH)) {
		unsigned char curr_page_data = 0xFF, *curr_ptr;

		/* Get current page data. It should be a blank page. */
		for (i=0, curr_ptr = &(_current_cache_page_data[0]) ; i<ptr_dev_info_t->page_size ; i++, curr_ptr++) {
			curr_page_data &= *curr_ptr;
		}

		/* When it is not a blank page. This page had been write blank data. Should erase it to correct ECC parity bits. */
		if (curr_page_data != 0xFF) {
			_SPI_NAND_DEBUG_PRINTF(ubiDbgLv, "page 0x%x had been write blank data\n", page_number);

			return SPI_NAND_FLASH_UBIFS_BLANK_PAGE_ECC_MISMATCH; /* blank sector*/
		}
	} else if ((!isSpiNandAndCtrlECC) && (ptr_dev_info_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_WINBOND)) {
		if((ptr_dev_info_t->dev_id == _SPI_NAND_DEVICE_ID_W25N01GV) ||
		   (ptr_dev_info_t->dev_id == _SPI_NAND_DEVICE_ID_W25M02GV)) {
		   /* for winbond, if data has been written blank,
		    * the ECC parity is all 0.
		    */
			memset(ecc_parity_0, 0x0, sizeof(ecc_parity_0));

			for(i = 0; i < 4; i++) {
				cmpVal= memcmp(ecc_parity_0, &_current_cache_page_oob[i * 16 + 8], sizeof(ecc_parity_0));
				if(cmpVal == 0) {
					_SPI_NAND_DEBUG_PRINTF(ubiDbgLv, "page 0x%x detected ECC parity 0 at block:0x%x page offset:0x%x.\n", page_number, block, page_number % page_per_block);
					if(_spi_dma_mode == SPI_DMA_MODE_ENABLE) {
						_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &dma_read_page[0], ((ptr_dev_info_t->page_size)+(ptr_dev_info_t->oob_size)));
					} else {
						_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &_current_cache_page[0], ((ptr_dev_info_t->page_size)+(ptr_dev_info_t->oob_size)));
					}
					return SPI_NAND_FLASH_UBIFS_BLANK_PAGE_ECC_MISMATCH; /* blank sector*/
				}
			}
		}
		
		return SPI_NAND_FLASH_UBIFS_BLANK_PAGE_ECC_MATCH; /* Good Page*/
	} else if(isEN7526c && isSpiNandAndCtrlECC) {
		SPI_NFI_Get_Configure(&spi_nfi_conf_t);
		
		for (sec_idx = 0; sec_idx < spi_nfi_conf_t.sec_num; sec_idx++) {
			cmpVal = memcmp(ctlerECC_blank_ecc_4, &_current_cache_page_oob[sec_idx * spi_nfi_conf_t.spare_size_t + spi_nfi_conf_t.fdm_num], sizeof(ctlerECC_blank_ecc_4));
			if(cmpVal == 0) {
				_SPI_NAND_DEBUG_PRINTF(ubiDbgLv, "page 0x%x detected ECC parity 0 at block:0x%x page offset:0x%x.\n", page_number, block, page_number % page_per_block);
				_SPI_NAND_DEBUG_PRINTF(ubiDbgLv, "oob:\n");
				_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &_current_cache_page_oob[0], ((ptr_dev_info_t->oob_size)));
				_SPI_NAND_DEBUG_PRINTF(ubiDbgLv, "page:\n");
				_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &dma_read_page[0], ((ptr_dev_info_t->page_size)+(ptr_dev_info_t->oob_size)));
				return SPI_NAND_FLASH_UBIFS_BLANK_PAGE_ECC_MISMATCH; /* blank sector*/
			}
		}
	}

	/* for coverity(CID:186026), fix missing return*/
	return SPI_NAND_FLASH_UBIFS_BLANK_PAGE_ECC_MATCH; /* Good Page*/
}

SPI_NAND_FLASH_RTN_T store_block(u32 block, u8 *block_buf)
{
	struct SPI_NAND_FLASH_INFO_T *ptr_dev_info_t;
	int i;
	u32 page_per_block;
	u32 start_page;
	SPI_NAND_FLASH_RTN_T rtn_status;
	SPI_NAND_FLASH_DEBUG_LEVEL_T ubiDbgLv __attribute__((unused))  = SPI_NAND_FLASH_DEBUG_LEVEL_2;

	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;

	page_per_block = (ptr_dev_info_t->erase_size / ptr_dev_info_t->page_size);
	start_page = block * page_per_block;

	// read all pages in the block
	for(i = 0; i < page_per_block; i++) {
		rtn_status = spi_nand_read_page(i + start_page, SPI_NAND_FLASH_READ_SPEED_MODE_SINGLE);
		if(rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR) {
			_SPI_NAND_DEBUG_PRINTF(ubiDbgLv, "store_block: block:0x%x page offset:0x%x\n", block, i);
			
			if(isEN7526c && isSpiNandAndCtrlECC) {
				memcpy(block_buf + i * (ptr_dev_info_t->page_size + ptr_dev_info_t->oob_size), 
					   _current_cache_page_data, 
					   ptr_dev_info_t->page_size);
				memcpy(block_buf + i * (ptr_dev_info_t->page_size + ptr_dev_info_t->oob_size) + ptr_dev_info_t->page_size, 
					   _current_cache_page_oob, 
					   ptr_dev_info_t->oob_size);
				_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &_current_cache_page_data[0], (ptr_dev_info_t->page_size));
				_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &_current_cache_page_oob[0], (ptr_dev_info_t->oob_size));
			} else {
				if(_spi_dma_mode == SPI_DMA_MODE_ENABLE) {
					memcpy(block_buf + i * (ptr_dev_info_t->page_size + ptr_dev_info_t->oob_size), 
						   dma_read_page, 
						   ptr_dev_info_t->page_size + ptr_dev_info_t->oob_size);
				} else {
					memcpy(block_buf + i * (ptr_dev_info_t->page_size + ptr_dev_info_t->oob_size), 
						   _current_cache_page, 
						   ptr_dev_info_t->page_size + ptr_dev_info_t->oob_size);
				}
				_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &_current_cache_page[0], ((ptr_dev_info_t->page_size)+(ptr_dev_info_t->oob_size)));
			}
		} else {
			_SPI_NAND_PRINTF("%s: fix blank page 0x%x read error\n",__func__, start_page+i);
			break;
		}
	}

	return rtn_status;
}

SPI_NAND_FLASH_RTN_T restore_block(u32 block, u8 *block_buf, u32 page_number)
{
	struct SPI_NAND_FLASH_INFO_T *ptr_dev_info_t;
	u32 i, j, k, idx;
	u32 page_per_block;
	u32 start_page;
	SPI_NAND_FLASH_RTN_T rtn_status;
	int isBlankData, isBlankOOB = 0;
	u8 *page_buf = NULL;
	u8 oob_mapping[_SPI_NAND_OOB_SIZE];
	struct spi_nand_flash_oobfree 	*ptr_oob_entry_idx;	
	SPI_NAND_FLASH_DEBUG_LEVEL_T ubiDbgLv __attribute__((unused)) = SPI_NAND_FLASH_DEBUG_LEVEL_2;

	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;

	page_per_block = (ptr_dev_info_t->erase_size / ptr_dev_info_t->page_size);
	start_page = block * page_per_block;
	
	// read all pages in the block
	for(i = 0; i < page_per_block; i++) {
		if((i + start_page) == page_number) {
			_SPI_NAND_DEBUG_PRINTF(ubiDbgLv, "restore_block: skip source page block:0x%x page offset:0x%x\n", block, i);
			continue;
		}

		page_buf = (block_buf + i * (ptr_dev_info_t->page_size + ptr_dev_info_t->oob_size));

		isBlankData = 1;
		for(idx = 0; idx < ptr_dev_info_t->page_size; idx++) {
			if(page_buf[idx] != 0xFF) {
				isBlankData = 0;
				break;
			}
		}

		if(isBlankData == 1) {
			isBlankOOB = 1;
			ptr_oob_entry_idx = (struct spi_nand_flash_oobfree*) &( ptr_dev_info_t->oob_free_layout->oobfree );

			idx = 0;
			for(k = 0; (k < SPI_NAND_FLASH_OOB_FREE_ENTRY_MAX) && (ptr_oob_entry_idx[k].len) && (idx < ptr_dev_info_t->oob_free_layout->oobsize); k++)
			{
				for(j = 0; (j < ptr_oob_entry_idx[k].len) && idx < (ptr_dev_info_t->oob_free_layout->oobsize); j++)
				{
					if(page_buf[ptr_dev_info_t->page_size + (ptr_oob_entry_idx[k].offset) + j] != 0xFF) {
						isBlankOOB = 0;
						k = SPI_NAND_FLASH_OOB_FREE_ENTRY_MAX;
						break;
					}
					idx++;
				}
			}
		}

		if((isBlankData == 1) && (isBlankOOB == 1)) {
			_SPI_NAND_DEBUG_PRINTF(ubiDbgLv, "restore_block: skip blank page block:0x%x page offset:0x%x\n", block, i);
			continue;
		} else {
			ptr_oob_entry_idx = (struct spi_nand_flash_oobfree*) &( ptr_dev_info_t->oob_free_layout->oobfree );
			idx = 0;

			memset(oob_mapping, 0xFF, _SPI_NAND_OOB_SIZE);
			for(k = 0; (k < SPI_NAND_FLASH_OOB_FREE_ENTRY_MAX) && (ptr_oob_entry_idx[k].len) && (idx < ptr_dev_info_t->oob_free_layout->oobsize); k++)
			{
				for(j = 0; (j < ptr_oob_entry_idx[k].len) && (idx < ptr_dev_info_t->oob_free_layout->oobsize); j++)
				{
					oob_mapping[idx] = page_buf[ptr_dev_info_t->page_size + (ptr_oob_entry_idx[k].offset) + j];
					idx++;
				}
			}
		}

		rtn_status = spi_nand_write_page(i + start_page, 
										 0, 
										 (block_buf + i * (ptr_dev_info_t->page_size + ptr_dev_info_t->oob_size)), 
										 ptr_dev_info_t->page_size, 
										 0, 
										 oob_mapping, 
										 ptr_dev_info_t->oob_size, 
										 SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE);

		_SPI_NAND_DEBUG_PRINTF(ubiDbgLv, "restore_block: block:0x%x page offset:0x%x\n", block, i);
		
		if(rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR) {
			_SPI_NAND_DEBUG_PRINTF(ubiDbgLv, "fixed page %x\n", start_page + i);
		} else {
			_SPI_NAND_PRINTF("%s: fix_ecc_0 0x%x write error \n",__func__, start_page+i);
			return rtn_status;
		}
	}

	return SPI_NAND_FLASH_RTN_NO_ERROR;
}


UBIFS_BLANK_PAGE_FIXUP_T fix_blank_page(u32 page_number)
{
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	u8 *block_buf;
	u32 page_per_block;
	u32 block;
	SPI_NAND_FLASH_RTN_T rtn_status;

	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;

	page_per_block = (ptr_dev_info_t->erase_size / ptr_dev_info_t->page_size);
	block = page_number / page_per_block;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
	block_buf = (u8 *) kmalloc((ptr_dev_info_t->page_size + ptr_dev_info_t->oob_size) * page_per_block, GFP_KERNEL);
#else
	block_buf = get_block_buf();
#endif

	if(!block_buf) {
		_SPI_NAND_PRINTF("%s:can not allocate buffer\n", __func__);
		return SPI_NAND_FLASH_UBIFS_BLANK_PAGE_FIXUP_FAIL;
	}
	memset(block_buf, 0xff, (ptr_dev_info_t->page_size + ptr_dev_info_t->oob_size) * page_per_block);

	/* store block */
	rtn_status = store_block(block, block_buf);
	if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
		kfree(block_buf);
#endif
		return SPI_NAND_FLASH_UBIFS_BLANK_PAGE_FIXUP_FAIL;
	}

	/* erase block */
	rtn_status = spi_nand_erase_block(block);
	if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
		kfree(block_buf);
#endif
		return SPI_NAND_FLASH_UBIFS_BLANK_PAGE_FIXUP_FAIL;
	}

	/* restore block except page_number */
	rtn_status = restore_block(block, block_buf, page_number);
	if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
		kfree(block_buf);
#endif
		return SPI_NAND_FLASH_UBIFS_BLANK_PAGE_FIXUP_FAIL;
	}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
	kfree(block_buf);
#endif
	return SPI_NAND_FLASH_UBIFS_BLANK_PAGE_FIXUP_SUCCESS;
}
#endif

static SPI_NAND_FLASH_RTN_T spi_nand_write_page(u32 page_number, 
												u32 data_offset,
											  	const u8  *ptr_data, 
											  	u32 data_len, 
											  	u32 oob_offset,
												u8  *ptr_oob , 
												u32 oob_len,
											    SPI_NAND_FLASH_WRITE_SPEED_MODE_T speed_mode)
{
	u8 status_2;
	u32		i = 0;
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	SPI_NAND_FLASH_RTN_T			rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	SPI_NAND_FLASH_RTN_T			prog_exe_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	u16 							write_addr;
	SPI_NFI_CONF_T					spi_nfi_conf_t;
	u32								offset1, offset2, offset3, dma_sec_size;
#if !defined(IMAGE_BL2) && !defined(BOOTROM_EXT) && !defined(LZMA_IMG)
	SPI_NAND_FLASH_DEBUG_LEVEL_T	ubiDbgLv __attribute__((unused)) = SPI_NAND_FLASH_DEBUG_LEVEL_2;
	static int						isUbifsBlankPageFix = 0;
#endif
#if defined(ERASE_WRITE_CNT_LOG) || defined(MULTI_WRITE_DBG)
	unsigned long					block_idx;
#endif
#ifdef ERASE_STATISTICS
	unsigned int					isUpdateEraseStatistic = 0;
#endif

	/* write to write_addr index in the page */
	write_addr = 0;

	SPI_NFI_Get_Configure(&spi_nfi_conf_t);
	
	/* Switch to manual mode*/
	_SPI_NAND_ENABLE_MANUAL_MODE();

	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;
	
#ifdef ERASE_STATISTICS
	if(isSupportEraseCntStatistic) {
		if(erase_statistic_cnt[page_number/PAGE_CNT_PER_BLOCK] != INVALID_ERASE_CNT) {
			/* update erase_statistic_cnt to first page in a block */
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "Write block 0x%04x page 0x%04x\n", page_number / PAGE_CNT_PER_BLOCK, page_number % PAGE_CNT_PER_BLOCK);
			if((page_number % PAGE_CNT_PER_BLOCK) == 0) {
				_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "original oob data:\n");
				_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_1, ptr_oob, oob_len);
				isUpdateEraseStatistic = 1;
			}
		}

		if((page_number % PAGE_CNT_PER_BLOCK) == 0) {
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "write page:0x%x erase cnt:0x%x\n", page_number, erase_statistic_cnt[page_number / PAGE_CNT_PER_BLOCK]);
		}
	}
#endif

#ifdef ERASE_WRITE_CNT_LOG
	block_idx = page_number / PAGE_CNT_PER_BLOCK;
	if(erase_write_flag == SPI_NAND_FLASH_ERASE_WRITE_LOG_ENABLE) {
		b_write_cnt_per_erase[block_idx]++;
		b_write_total_cnt[block_idx]++;
	}
#endif

#ifdef MULTI_WRITE_DBG
	if(multi_write_dbg_flag == SPI_NAND_FLASH_MULTI_WRITE_ENABLE) {
		block_idx = page_number / PAGE_CNT_PER_BLOCK;
		if(page_info[block_idx][page_number % PAGE_CNT_PER_BLOCK].isProgramed != 0) {
			//_SPI_NAND_PRINTF("Error, write block:0x%x page:0x%x without erase.\n", block_idx, page_number % PAGE_CNT_PER_BLOCK);
			//_SPI_NAND_PRINTF("block:0x%x page:0x%x has been writted %d times.\n", block_idx, page_number % PAGE_CNT_PER_BLOCK, page_info[block_idx][page_number % PAGE_CNT_PER_BLOCK]);
			/* Avoid write twice without erase  */
			//return SPI_NAND_FLASH_RTN_NO_ERROR;
			if(page_info[block_idx][page_number % PAGE_CNT_PER_BLOCK].illegal_program < 0x7F) {
				page_info[block_idx][page_number % PAGE_CNT_PER_BLOCK].illegal_program++;
			}
		}
		page_info[block_idx][page_number % PAGE_CNT_PER_BLOCK].isProgramed = 1;
	}
#endif

#if !defined(IMAGE_BL2) && !defined(BOOTROM_EXT) && !defined(LZMA_IMG)
	if(ptr_dev_info_t->feature & SPI_NAND_FLASH_BLANK_PAGE_FIXUP) {
		if(isUbifsBlankPageFix == 0) {
			/* Read Current page data to software cache buffer */
			if(isEN7526c && isSpiNandAndCtrlECC) {
				spi_nfi_conf_t.auto_fdm_t = SPI_NFI_CON_AUTO_FDM_Disable;
				SPI_NFI_Set_Configure(&spi_nfi_conf_t);
			}

			/* Need to disable ECC to check that page had been write blank page for skyhigh */
			if ((!isSpiNandAndCtrlECC) && (ptr_dev_info_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_SKYHIGH)) {
				/* Get current ECC feature address */
				spi_nand_protocol_get_feature(ptr_dev_info_t->ecc_en.ecc_en_addr, &status_2);

				/* Disable ECC feature */
				status_2 = (status_2 & ~(ptr_dev_info_t->ecc_en.ecc_en_mask));
				spi_nand_protocol_set_feature (ptr_dev_info_t->ecc_en.ecc_en_addr, status_2);

				/* Read page and check blank page */
				spi_nand_read_page (page_number, speed_mode);

				/* Enable ECC feature */
				status_2 |= (ptr_dev_info_t->ecc_en.ecc_en_value & ptr_dev_info_t->ecc_en.ecc_en_mask);
				spi_nand_protocol_set_feature (ptr_dev_info_t->ecc_en.ecc_en_addr, status_2);
			} else {
				spi_nand_read_page(page_number, speed_mode);
			}

			if(isEN7526c && isSpiNandAndCtrlECC) {
				spi_nfi_conf_t.auto_fdm_t = SPI_NFI_CON_AUTO_FDM_Enable;
				SPI_NFI_Set_Configure(&spi_nfi_conf_t);
			}
			
			if(check_blank_page(page_number) == SPI_NAND_FLASH_UBIFS_BLANK_PAGE_ECC_MISMATCH) {
				isUbifsBlankPageFix = 1;
				_SPI_NAND_DEBUG_PRINTF(ubiDbgLv, "UBIFS_BLANK_PAGE_FIXUP, page:0x%x\n", page_number);
				fix_blank_page(page_number);
				isUbifsBlankPageFix = 0;
			}
		}
	}
#endif

#if ((defined(TCSUPPORT_CPU_ARMV8)) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)))
	/* get hold of the wdma_addr temporarily to do read */
	dma_sync_single_for_cpu(ecnt_nand->dev, wdma_addr, _SPI_NAND_CACHE_SIZE, DMA_TO_DEVICE);
#endif

	if(data_len > 0) {
		memcpy(&_current_cache_page_data[data_offset], &ptr_data[0], data_len);
		memset(&_current_cache_page_data[data_len], 0xFF, PAGE_SIZE - data_len);
	}

	/* Write data & OOB */
	if((_spi_dma_mode == SPI_DMA_MODE_DISABLE) ||
	  ((_spi_dma_mode == SPI_DMA_MODE_ENABLE) && (!isSpiNandAndCtrlECC) && (spi_nfi_conf_t.auto_fdm_t == SPI_NFI_CON_AUTO_FDM_Disable) && (spi_nfi_conf_t.hw_ecc_t == SPI_NFI_CON_HW_ECC_Disable)))
	{
		if(_spi_dma_mode == SPI_DMA_MODE_DISABLE) {
			memcpy(&_current_cache_page[data_offset], &_current_cache_page_data[0], PAGE_SIZE);
		} else {
			memcpy((u8 *)(&dma_write_page[data_offset]), &_current_cache_page_data[0], PAGE_SIZE);
		}
		
		if(oob_len > 0) {
			/*  When chip->ecc.mode = NAND_ECC_NONE,  linux will automatic get user data from oob_free_layout*/
			memcpy(&_current_cache_page_oob[0], &ptr_oob[0], oob_len);
			memset(&_current_cache_page_oob[oob_len], 0xFF, _current_flash_info_t.oob_size - oob_len);
		}

#ifdef ERASE_STATISTICS
		if(isUpdateEraseStatistic) {
			memcpy((_current_cache_page_oob + MAX_ERASE_CNT_START_OFFSET), &erase_statistic_cnt[page_number / PAGE_CNT_PER_BLOCK], MAX_ERASE_CNT_SIZE);
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "modified oob data:\n");
			_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_1, _current_cache_page_oob, ptr_dev_info_t->oob_size);
		}
#endif
		
		if(_spi_dma_mode == SPI_DMA_MODE_DISABLE) {
			memcpy( &_current_cache_page[ptr_dev_info_t->page_size], &_current_cache_page_oob[0], ptr_dev_info_t->oob_size);
		} else {
			memcpy((u8 *)(&dma_write_page[ptr_dev_info_t->page_size]), &_current_cache_page_oob[0], ptr_dev_info_t->oob_size);
		}
	} else {
		if(spi_nfi_conf_t.auto_fdm_t== SPI_NFI_CON_AUTO_FDM_Disable) {	/* Data and oob alternate */
			if(oob_len > 0) {
				offset1 = 0;
				offset2 = 0;
				for(i = 0; i < spi_nfi_conf_t.sec_num; i++) {
					memcpy( &_current_cache_page_oob[offset1], &ptr_oob[offset2], spi_nfi_conf_t.spare_size_t);
					offset1 += spi_nfi_conf_t.spare_size_t;
					offset2 += spi_nfi_conf_t.spare_size_t;
				}
			}

			offset1 = 0;
			offset2 = 0;
			offset3 = 0;
			dma_sec_size = (spi_nfi_conf_t.page_size_t / spi_nfi_conf_t.sec_num);
			for(i = 0 ; i < spi_nfi_conf_t.sec_num; i++) {													
				memcpy((u8 *)(&dma_write_page[offset2]), &_current_cache_page_data[offset1],	dma_sec_size );
				memcpy((u8 *)(&dma_write_page[offset2+dma_sec_size]), &_current_cache_page_oob[offset3] , (spi_nfi_conf_t.spare_size_t) );
				offset1 += dma_sec_size;
				offset2 += (dma_sec_size+ (spi_nfi_conf_t.spare_size_t));
				offset3 += (spi_nfi_conf_t.spare_size_t);					
			}
		} else {  /* Data inside DRAM , oob inside NFI register */
			if(oob_len > 0) {
				memcpy(&_current_cache_page_oob_mapping[0], &ptr_oob[0], oob_len);
				memset(&_current_cache_page_oob_mapping[oob_len], 0xFF, _current_flash_info_t.oob_size - oob_len);
			}
#ifdef ERASE_STATISTICS
			if(isUpdateEraseStatistic) {
				memcpy((_current_cache_page_oob_mapping + MAX_ERASE_CNT_START_OFFSET), &erase_statistic_cnt[page_number / PAGE_CNT_PER_BLOCK], MAX_ERASE_CNT_SIZE);
				_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "modified oob data:\n");
				_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_1, _current_cache_page_oob_mapping, ptr_dev_info_t->oob_size);
			}
#endif
			/* Set data */
			memcpy((u8 *)(&dma_write_page[0]), &_current_cache_page_data[0], ptr_dev_info_t->page_size);
		}
	}

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spi_nand_write_page: page=0x%x, data_offset=0x%x, data_len=0x%x, oob_offset=0x%x, oob_len=0x%x\n", page_number, data_offset, data_len, oob_offset, oob_len);
	if(_spi_dma_mode == SPI_DMA_MODE_DISABLE) {
		_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &_current_cache_page[0], ((ptr_dev_info_t->page_size) + (ptr_dev_info_t->oob_size)));
	} else {
		_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, (&dma_write_page[0]), ptr_dev_info_t->page_size + ptr_dev_info_t->oob_size);
	}

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spi_nand_write_page: _current_cache_page_data:\n");
	_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &_current_cache_page_data[0], ptr_dev_info_t->page_size);
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spi_nand_write_page: _current_cache_page_oob:\n");
	_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &_current_cache_page_oob[0], ptr_dev_info_t->oob_size);		
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spi_nand_write_page: _current_cache_page_oob_mapping:\n");
	_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &_current_cache_page_oob_mapping[0], (ptr_dev_info_t->oob_free_layout)->oobsize);

	if(ptr_dev_info_t->feature & SPI_NAND_FLASH_PLANE_SELECT_HAVE) {
		_plane_select_bit = ((page_number >> 6) & (0x1));

		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spi_nand_write_page: _plane_select_bit=0x%x\n", _plane_select_bit );
	}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
	/* kernel part */
#ifdef TCSUPPORT_CPU_ARMV8
	/* release the wdma_addr to the device again */
	dma_sync_single_for_device(ecnt_nand->dev, wdma_addr, _SPI_NAND_CACHE_SIZE, DMA_TO_DEVICE);
#else
	dma_cache_wback((unsigned long)dma_write_page, _SPI_NAND_CACHE_SIZE);	
	wdma_addr = K1_TO_PHY(&dma_write_page[0]);
#endif
#else
	/* Bootloader part */
#if defined(TCSUPPORT_CPU_ARMV8) || defined(CONFIG_ECNT_UBOOT)
	flush_dcache_range((unsigned long)dma_write_page, (unsigned long)(dma_write_page + _SPI_NAND_CACHE_SIZE));
#else
	wback_inv_dcache_range((unsigned long)dma_write_page, (unsigned long)(dma_write_page + _SPI_NAND_CACHE_SIZE));
#endif
	wdma_addr = K1_TO_PHY(&dma_write_page[0]);
#endif

	if(_spi_dma_mode == SPI_DMA_MODE_ENABLE) {	
		mb();
	}

#if defined(TCSUPPORT_PARALLEL_NAND)
	if (_parallel_nand_mode)
	{
		rtn_status = parallel_nand_dma_write(write_addr, page_number, (unsigned long *)wdma_addr, oob_len, &_current_cache_page_oob_mapping[0]);
	}
	else
#endif
	{
		spi_nand_select_die ( page_number );
	
		/* Different Manafacture have different prgoram flow and setting */
		if(ptr_dev_info_t->write_en_type == SPI_NAND_FLASH_WRITE_LOAD_FIRST) {
			if(_spi_dma_mode == SPI_DMA_MODE_ENABLE) {
				rtn_status = spi_nand_dma_program_load(write_addr, oob_len, speed_mode);
				if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
					SPI_NAND_Flash_Clear_Read_Cache_Data();
					/* Switch to manual mode*/
					_SPI_NAND_ENABLE_MANUAL_MODE();
					return (rtn_status);
				}
			} else {
				spi_nand_protocol_program_load(write_addr, &_current_cache_page[0], ((ptr_dev_info_t->page_size) + (ptr_dev_info_t->oob_size)), speed_mode);
			}
	
			/*	Enable write_to flash */
			rtn_status = spi_nand_protocol_write_enable();
			if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
				SPI_NAND_Flash_Clear_Read_Cache_Data();
				/* Switch to manual mode*/
				_SPI_NAND_ENABLE_MANUAL_MODE();
				return (rtn_status);
			}
		} else {
			/*	Enable write_to flash */
			rtn_status = spi_nand_protocol_write_enable();
			if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
				SPI_NAND_Flash_Clear_Read_Cache_Data();
				/* Switch to manual mode*/
				_SPI_NAND_ENABLE_MANUAL_MODE();
				return (rtn_status);
			}
		
			if(_spi_dma_mode == SPI_DMA_MODE_ENABLE) {
				rtn_status = spi_nand_dma_program_load(write_addr, oob_len, speed_mode);
				if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
					SPI_NAND_Flash_Clear_Read_Cache_Data();
					/* Switch to manual mode*/
					_SPI_NAND_ENABLE_MANUAL_MODE();
					return (rtn_status);
				}
			} else {
				/* Proram data into buffer of SPI NAND chip */
				spi_nand_protocol_program_load(write_addr, &_current_cache_page[0], ((ptr_dev_info_t->page_size) + (ptr_dev_info_t->oob_size)), speed_mode);
			}
		}

		/*	Execute program data into SPI NAND chip  */
		prog_exe_status = spi_nand_protocol_program_execute ( page_number );
	
		/*. Disable write_flash */
		rtn_status = spi_nand_protocol_write_disable();
		if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
			SPI_NAND_Flash_Clear_Read_Cache_Data();
			/* Switch to manual mode*/
			_SPI_NAND_ENABLE_MANUAL_MODE();
			return (rtn_status);
		}
		if(prog_exe_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
			SPI_NAND_Flash_Clear_Read_Cache_Data();
			/* Switch to manual mode*/
			_SPI_NAND_ENABLE_MANUAL_MODE();
			return (prog_exe_status);
		}
		spi_nand_protocol_get_feature(_SPI_NAND_ADDR_PROTECTION, &status_2);
	
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "[spi_nand_write_page]: status 1 = 0x%x\n", status_2);

		if(!isSpiNandAndCtrlECC)
		{
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_write_page: check ecc status.\n");
			rtn_status = spi_nand_load_page_into_cache(page_number);
			if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
				SPI_NAND_Flash_Clear_Read_Cache_Data();
				/* Switch to manual mode*/
				_SPI_NAND_ENABLE_MANUAL_MODE();
				return (rtn_status);
			}
		}
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	print_dot++;
	if((print_dot % 20) == 0 ) {
		_SPI_NAND_PRINTF(".");
	}
#endif

	SPI_NAND_Flash_Clear_Read_Cache_Data();
	
	return (rtn_status);
}

/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T spi_nand_write_internal( u32    dst_addr,
 *                                                                u32    len,
 *                                                                u32    *ptr_rtn_len,
 *                                                                u8*    ptr_buf      )
 * PURPOSE : To write flash internally.
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : dst_addr     - The dst_addr variable of this function.
 *           len          - The len variable of this function.
 *           ptr_buf      - A pointer to the ptr_buf variable.
 *   OUTPUT: ptr_rtn_len  - A pointer to the ptr_rtn_len variable.
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/19  - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
static SPI_NAND_FLASH_RTN_T spi_nand_write_internal( u32 dst_addr, 
													 u32 len, 
													 u32 *ptr_rtn_len, 
													 u8* ptr_buf, 
													 SPI_NAND_FLASH_WRITE_SPEED_MODE_T speed_mode )
{
	u32					 			remain_len, write_addr, data_len, page_number, physical_dst_addr;
	u32					 			addr_offset;
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	SPI_NAND_FLASH_RTN_T			rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
	unsigned long					spinand_spinlock_flags;
#endif
		
#if	defined(TCSUPPORT_NAND_BMT) && !defined(LZMA_IMG) && !defined(BOOTROM_EXT)
    unsigned short	phy_block_bbt = 0;
	unsigned long	addr_offset_in_block = 0;
	u32				logical_block = 0, physical_block = 0;
	u8				oob_buf[_SPI_NAND_OOB_SIZE]={0xff};
#endif

	*ptr_rtn_len 	= 0;
	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;	
	remain_len 		= len;
	write_addr 		= dst_addr;

#if !defined(LZMA_IMG) && !defined(BOOTROM_EXT)
	rtn_status = spi_nand_boundary_check(write_addr, remain_len);
	if (rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR)
		return rtn_status;
#endif

	_SPI_NAND_SEMAPHORE_LOCK();	

	SPI_NAND_Flash_Clear_Read_Cache_Data();

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_write_internal: remain_len =0x%x\n", remain_len);
	
	while(remain_len > 0)	
	{
		physical_dst_addr = write_addr;
		
#if	defined(TCSUPPORT_NAND_BMT) && !defined(LZMA_IMG) && !defined(BOOTROM_EXT)
		memset(oob_buf, 0xff, _SPI_NAND_OOB_SIZE);

		if(otp_status == NAND_FLASH_OTP_DISABLE) {
			addr_offset_in_block = (write_addr %(ptr_dev_info_t->erase_size) );
			logical_block = (write_addr / (ptr_dev_info_t->erase_size));
			physical_block = get_mapping_block_index(logical_block, &phy_block_bbt);
			physical_dst_addr = (physical_block * (ptr_dev_info_t->erase_size))+ addr_offset_in_block;

			if(physical_block != logical_block) {			
				_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "Bad Block Mapping, from %d block to %d block\n", logical_block, physical_block);
			}
		}
#endif
	
		/* Caculate page number */
		addr_offset = (physical_dst_addr % (ptr_dev_info_t->page_size));
		page_number = (physical_dst_addr / (ptr_dev_info_t->page_size));		
		
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "\nspi_nand_write_internal: addr_offset =0x%x, page_number=0x%x, remain_len=0x%x, page_size=0x%x\n", addr_offset, page_number, remain_len,(ptr_dev_info_t->page_size) );		
		if(((addr_offset + remain_len) > ptr_dev_info_t->page_size)) {  /* data cross over than 1 page range */
			data_len = ((ptr_dev_info_t->page_size) - addr_offset);			
		} else {
			data_len = remain_len;
		}

#if	defined(TCSUPPORT_NAND_BMT) && !defined(LZMA_IMG) && !defined(BOOTROM_EXT)
		if(otp_status == NAND_FLASH_OTP_DISABLE) {
			if(block_is_in_bmt_region(physical_block)) {
				if(ptr_dev_info_t->feature & SPI_NAND_FLASH_OOB_RESERVE_FOR_BMT) {
					memcpy(oob_buf + ooblayout_feature7.oobfree[0 + 1].offset, &phy_block_bbt, OOB_INDEX_SIZE);
				} else {
					memcpy(oob_buf + OOB_INDEX_OFFSET, &phy_block_bbt, OOB_INDEX_SIZE);
				}
			}
		}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
		if(_SPI_NAND_WRITE_FAIL_TEST_FLAG == 0) {
#else
		if(test_write_fail_flag == 0) {
#endif
			rtn_status = spi_nand_write_page(page_number, addr_offset, &(ptr_buf[len - remain_len]), data_len, 0, &oob_buf[0], ptr_dev_info_t->oob_size , speed_mode);
		} else {
			rtn_status = SPI_NAND_FLASH_RTN_PROGRAM_FAIL;
		}

		if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
		    _SPI_NAND_PRINTF("write fail at page: %d \n", page_number);
            if(update_bmt(page_number * ptr_dev_info_t->page_size, UPDATE_WRITE_FAIL, &(ptr_buf[len - remain_len]), oob_buf)) {
                _SPI_NAND_PRINTF("Update BMT success\n");
				rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
           
            } else {
                _SPI_NAND_PRINTF("Update BMT fail\n");
				_SPI_NAND_SEMAPHORE_UNLOCK();	
                return -1;
            }		
		}
#else
		rtn_status = spi_nand_write_page(page_number, addr_offset, &(ptr_buf[len - remain_len]), data_len, 0, NULL, 0 , speed_mode);
#endif
		
		/* 8. Write remain data if neccessary */
		write_addr	+= data_len;
		remain_len  -= data_len;				
		*ptr_rtn_len += data_len;	 //COV, //COV, CID 139184
	}
	
	_SPI_NAND_SEMAPHORE_UNLOCK();
		
	return (rtn_status);
}

/*------------------------------------------------------------------------------------
 * FUNCTION: SPI_NAND_FLASH_RTN_T SPI_NAND_Flash_Write_Nbyte( u32    dst_addr,
 *                                                            u32    len,
 *                                                            u32    *ptr_rtn_len,
 *                                                            u8*    ptr_buf      )
 * PURPOSE : To provide interface for Write N Bytes into SPI NAND Flash.
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : dst_addr - The dst_addr variable of this function.
 *           len      - The len variable of this function.
 *           buf      - The buf variable of this function.
 *   OUTPUT: rtn_len  - The rtn_len variable of this function.
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/15  - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
SPI_NAND_FLASH_RTN_T SPI_NAND_Flash_Write_Nbyte( u32						dst_addr, 
												 u32								len, 
												 u32								*ptr_rtn_len, 
												 u8									*ptr_buf, 
												 SPI_NAND_FLASH_WRITE_SPEED_MODE_T	speed_node )
{		
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;	
	
	rtn_status = spi_nand_write_internal(dst_addr, len, ptr_rtn_len, ptr_buf, speed_node);
	
	*ptr_rtn_len = len ;  /* , tmp modify */
	
	return (rtn_status);
}

int nandflash_write(unsigned long to, unsigned long len, u32 *retlen, unsigned char *buf)
{
	struct SPI_NAND_FLASH_INFO_T	 *ptr_dev_info_t;
	 
	ptr_dev_info_t  = _SPI_NAND_GET_DEVICE_INFO_PTR;
	
	if( SPI_NAND_Flash_Write_Nbyte(to, len, retlen, buf, ptr_dev_info_t->write_mode) == SPI_NAND_FLASH_RTN_NO_ERROR )
	{
		return 0;
	}
	else
	{
		return -1;
	}	
}
#endif



#if !defined(LZMA_IMG) || defined(TCSUPPORT_BB_256KB)
/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T spi_nand_protocol_block_erase( u32   block_idx )
 * PURPOSE : To implement the SPI nand protocol for block erase.
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : block_idx - The block_idx variable of this function.
 *   OUTPUT: None
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/17  - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
static SPI_NAND_FLASH_RTN_T spi_nand_protocol_block_erase( u32 block_idx )
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	rtn_status = spi_nand_protocol_chk_status(_SPI_NAND_VAL_OIP, 0);
	if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
		_SPI_NAND_PRINTF("[Error] erase Timeout.\n");
		return (rtn_status);
	}
	
	/* 1. Chip Select Low */
	_SPI_NAND_READ_CHIP_SELECT_LOW();
	
	/* 2. Write op_cmd 0xD8 (Block Erase) */
	_SPI_NAND_WRITE_ONE_BYTE( _SPI_NAND_OP_BLOCK_ERASE );

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_protocol_block_erase : block idx =0x%x\n", block_idx);
	
	/* 3. Write block number */
	block_idx = block_idx << _SPI_NAND_BLOCK_ROW_ADDRESS_OFFSET; 	/*Row Address format in SPI NAND chip */
	
	_SPI_NAND_WRITE_ONE_BYTE( (block_idx >> 16) & 0xff );		/* dummy byte */
	_SPI_NAND_WRITE_ONE_BYTE( (block_idx >> 8)  & 0xff );
	_SPI_NAND_WRITE_ONE_BYTE(  block_idx & 0xff );
	
	/* 4. Chip Select High */
	_SPI_NAND_READ_CHIP_SELECT_HIGH();
			
	return (rtn_status);
}

SPI_NAND_FLASH_RTN_T spi_nand_erase_block(u32 block_index)
{
	u8								status;
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	SPI_NAND_FLASH_RTN_T			rtn_status	= SPI_NAND_FLASH_RTN_NO_ERROR;
#if defined(ERASE_WRITE_CNT_LOG) || defined(MULTI_WRITE_DBG)
	int								i;
#endif
	
#ifdef ERASE_STATISTICS
	u32								erase_cnt;
	u32								page_no;
#endif

#ifdef ERASE_WRITE_CNT_LOG
	if(erase_write_flag == SPI_NAND_FLASH_ERASE_WRITE_LOG_ENABLE) {
		b_erase_cnt[block_index]++;
		b_write_cnt_per_erase[block_index] = 0;
	}
#endif

#ifdef MULTI_WRITE_DBG
	if(multi_write_dbg_flag == SPI_NAND_FLASH_MULTI_WRITE_ENABLE) {
		for(i = 0; i < 128; i++) {
			page_info[block_index][i].isProgramed = 0;
		}
	}
#endif


	ptr_dev_info_t = _SPI_NAND_GET_DEVICE_INFO_PTR ;

#ifdef ERASE_STATISTICS
	if(isSupportEraseCntStatistic) {
		if(erase_statistic_cnt[block_index] == INVALID_ERASE_CNT) {
			page_no = block_index * PAGE_CNT_PER_BLOCK;
			if(en7512_nand_exec_read_page(page_no, erase_statictic_page_data, erase_statictic_oob_data)) {
				_SPI_NAND_PRINTF("Read erase block counter 0x%x fail.\n", block_index);
			} else {
				memcpy(&erase_cnt, (erase_statictic_oob_data+MAX_ERASE_CNT_START_OFFSET), MAX_ERASE_CNT_SIZE);
				if(erase_cnt == INVALID_ERASE_CNT) {
					erase_statistic_cnt[block_index] = 0;
				} else {
					erase_statistic_cnt[block_index] = erase_cnt;
				}
				erase_statistic_cnt[block_index]++;
			}

			/* Need to clear cache since flash driver read page_no. System erase block after init erase_statistic_cnt. */
			SPI_NAND_Flash_Clear_Read_Cache_Data();
		} else {
			erase_statistic_cnt[block_index]++;
		}
		
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "erase block:0x%x erase cnt:0x%x\n", block_index, erase_statistic_cnt[block_index]);
	}
#endif

#if defined(TCSUPPORT_PARALLEL_NAND)
	if (_parallel_nand_mode)
	{
		rtn_status = parallel_nand_erase(block_index * (ptr_dev_info_t->erase_size / ptr_dev_info_t->page_size));
	}
	else
#endif
	{
		spi_nand_select_die (block_index * (ptr_dev_info_t->erase_size / ptr_dev_info_t->page_size));

		/* 2.2 Enable write_to flash */
		rtn_status = spi_nand_protocol_write_enable();
		if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
			return (SPI_NAND_FLASH_RTN_ERASE_FAIL);
		}

		/* 2.3 Erasing one block */
		spi_nand_protocol_block_erase(block_index);

		/* 2.4 Checking status for erase complete */
		do {
			spi_nand_protocol_get_feature(_SPI_NAND_ADDR_STATUS, &status);
		} while(status & _SPI_NAND_VAL_OIP) ;
				
		/* 2.5 Disable write_flash */
		rtn_status = spi_nand_protocol_write_disable();
		if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
			return (SPI_NAND_FLASH_RTN_ERASE_FAIL);
		}

		/* 2.6 Check Erase Fail Bit */
		if(status & _SPI_NAND_VAL_ERASE_FAIL) {
			_SPI_NAND_PRINTF("spi_nand_erase_block : erase block fail, block=0x%x, status=0x%x\n", block_index, status);
			rtn_status = SPI_NAND_FLASH_RTN_ERASE_FAIL;
		}
	}

	return rtn_status;
}

/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T spi_nand_erase_internal( u32     addr,
 *                                               				  u32     len )
 * PURPOSE : To erase flash internally.
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : addr - The addr variable of this function.
 *           len - The size variable of this function.
 *   OUTPUT: None
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/15  - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
static SPI_NAND_FLASH_RTN_T spi_nand_erase_internal( u32 addr, 
													 u32 len )
{		
	u32						block_index = 0;
	u32						erase_len	= 0;
	SPI_NAND_FLASH_RTN_T	rtn_status  = SPI_NAND_FLASH_RTN_NO_ERROR;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
	unsigned long			spinand_spinlock_flags;
#endif
				
#if	defined(TCSUPPORT_NAND_BMT) && !defined(LZMA_IMG) && !defined(BOOTROM_EXT)
    unsigned short	phy_block_bbt;
	u32				logical_block, physical_block;
#endif	
		
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "\nspi_nand_erase_internal (in): addr=0x%x, len=0x%x\n", addr, len );		

#if !defined(LZMA_IMG) && !defined(BOOTROM_EXT)
	rtn_status = spi_nand_boundary_check(addr, len);
	if (rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR)
		return rtn_status;
#endif

	_SPI_NAND_SEMAPHORE_LOCK();	
	
	/* Switch to manual mode*/
	_SPI_NAND_ENABLE_MANUAL_MODE();

	SPI_NAND_Flash_Clear_Read_Cache_Data();
	
	/* 1. Check the address and len must aligned to NAND Flash block size */
	if( spi_nand_block_aligned_check( addr, len) == SPI_NAND_FLASH_RTN_NO_ERROR) {
		/* 2. Erase block one by one */
		while( erase_len < len ) {
			/* 2.1 Caculate Block index */
			block_index = (addr/(_current_flash_info_t.erase_size));
#if	defined(TCSUPPORT_NAND_BMT) && !defined(LZMA_IMG) && !defined(BOOTROM_EXT)
			logical_block = block_index;
			physical_block = get_mapping_block_index(logical_block, &phy_block_bbt);		
			if( physical_block != logical_block) {			
				_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "Bad Block Mapping, from %d block to %d block\n", logical_block, physical_block);
			}
			block_index = physical_block;
#endif							
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_erase_internal: addr=0x%x, len=0x%x, block_idx=0x%x\n", addr, len, block_index );

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
			if(_SPI_NAND_ERASE_FAIL_TEST_FLAG == 0) {
				rtn_status = spi_nand_erase_block(block_index);
			} else {
				rtn_status = SPI_NAND_FLASH_RTN_ERASE_FAIL;
			}
#else
			rtn_status = spi_nand_erase_block(block_index);
#endif

			/* 2.6 Check Erase Fail Bit */
			if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
#if	defined(TCSUPPORT_NAND_BMT) && !defined(LZMA_IMG) && !defined(BOOTROM_EXT)
				if (update_bmt((block_index * BLOCK_SIZE),UPDATE_ERASE_FAIL, NULL, NULL)) {
					_SPI_NAND_PRINTF("Erase fail at block: %d, update BMT success\n", addr/(_current_flash_info_t.erase_size));
					rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
				} else {
					_SPI_NAND_PRINTF("Erase fail at block: %d, update BMT fail\n", addr/(_current_flash_info_t.erase_size));
					rtn_status = SPI_NAND_FLASH_RTN_ERASE_FAIL;
					break;
				}
#else
				_SPI_NAND_PRINTF("spi_nand_erase_internal : Erase Fail at addr=0x%x, len=0x%x, block_idx=0x%x\n", addr, len, block_index);
				rtn_status = SPI_NAND_FLASH_RTN_ERASE_FAIL;
				break;
#endif
			}
			
			/* 2.7 Erase next block if needed */
			addr		+= _current_flash_info_t.erase_size;
			erase_len	+= _current_flash_info_t.erase_size;
		}		
	} else {
		rtn_status = SPI_NAND_FLASH_RTN_ALIGNED_CHECK_FAIL;
	}	
	
	_SPI_NAND_SEMAPHORE_UNLOCK();
		
	return 	(rtn_status);
}

/*------------------------------------------------------------------------------------
 * FUNCTION: SPI_NAND_FLASH_RTN_T SPI_NAND_Flash_Erase( u32  dst_addr,
 *                                                      u32  len      )
 * PURPOSE : To provide interface for Erase SPI NAND Flash.
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : dst_addr - The dst_addr variable of this function.
 *           len      - The len variable of this function.
 *   OUTPUT: None
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/17  - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
SPI_NAND_FLASH_RTN_T SPI_NAND_Flash_Erase(u32 dst_addr, u32 len)
{
	SPI_NAND_FLASH_RTN_T rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	
	rtn_status = spi_nand_erase_internal(dst_addr, len);
	
	return (rtn_status);
}

int nandflash_erase(unsigned long offset, unsigned long len)
{
	if(SPI_NAND_Flash_Erase(offset, len) == SPI_NAND_FLASH_RTN_NO_ERROR) {
		return 0;
	} else {
		return -1;
	}
}
#endif

#if (!defined(LZMA_IMG)) || defined(TCSUPPORT_BB_256KB)
int en7512_nand_exec_read_page(u32 page, u8* date, u8* oob)
{	
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	SPI_NAND_FLASH_RTN_T			rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;

	rtn_status = spi_nand_read_page(page, ptr_dev_info_t->read_mode);
	
	if(rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR) {
		/* Get data segment and oob segment  */
		memcpy( date, &_current_cache_page_data[0], ptr_dev_info_t->page_size );
		memcpy( oob,  &_current_cache_page_oob_mapping[0], ptr_dev_info_t->oob_size );
		
		return 0;
	} else {
		 _SPI_NAND_PRINTF( "en7512_nand_exec_read_page: read error, page=0x%x\n", page);
		return -1;
	}
}
#endif
#if (!defined(LZMA_IMG) && !defined(BOOTROM_EXT)) || defined(TCSUPPORT_BB_256KB)
int en7512_nand_check_block_bad(u32 offset, u32 bmt_block)
{
	u32								page_number;
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	u8								bbValue = 0;
	SPI_NAND_FLASH_RTN_T			rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	ptr_dev_info_t  = _SPI_NAND_GET_DEVICE_INFO_PTR;	

	if((0xbc000000 <= offset) && (offset <= 0xbfffffff)) {		/* Reserver address area for system */
		if( (offset & 0xbfc00000) == 0xbfc00000) {
			offset &= 0x003fffff;
		} else {
			offset &= 0x03ffffff;
		}
	}	 

	/* Caculate page number */
	page_number = (offset / (ptr_dev_info_t->page_size));		 

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "en7512_nand_check_block_bad: read_addr=0x%x, page_number=0x%x\n", offset, page_number);

	SPI_NAND_Flash_Clear_Read_Cache_Data();

	rtn_status = spi_nand_read_page(page_number, ptr_dev_info_t->read_mode);

      /* Otherwise, the bmt pool size will be changed */
	if (rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "en7512_nand_check_block_bad return error, block:%d\n", offset/ptr_dev_info_t->erase_size);
#if 0 /* No matter what status of spi_nand_read_page, we must check badblock by oob. */
		return 1;
#endif
	}

#if defined(TCSUPPORT_NAND_BMT)
	if(bmt_block) {
		if(ptr_dev_info_t->feature & SPI_NAND_FLASH_OOB_RESERVE_FOR_BMT) {
			bbValue = _current_cache_page_oob_mapping[ooblayout_feature7.oobfree[0].offset] = 0;
		} else {
			bbValue = _current_cache_page_oob_mapping[BMT_BAD_BLOCK_INDEX_OFFSET];
		}

		if(bbValue != 0xff) {
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "Bad block detected at page_addr 0x%x, oob_buf[%d] is 0x%x\n", page_number, BMT_BAD_BLOCK_INDEX_OFFSET,_current_cache_page_oob_mapping[BMT_BAD_BLOCK_INDEX_OFFSET]);
			return 1;
		}
	} else
#endif
	{
		if(_current_cache_page_oob_mapping[0] != 0xff) {
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "Bad block detected at page_addr 0x%x, oob_buf[0] is 0x%x\n", page_number, _current_cache_page_oob_mapping[0]);
			return 1;
		}
	}
#if 0
u8 def_bad_block_ECC[28];
#ifdef TCSUPPORT_SPI_CONTROLLER_ECC
	if(isSpiNandAndCtrlECC) {
		/* This is for check default bad block.
		 * When DMA read with all ECC parity equal to 0xFF,
		 * this will not generate read ECC error. So, it must 
		 * close DMA to check first OOB byte.
		 */
		_SPI_NAND_SEMAPHORE_LOCK();
		SPI_NAND_Flash_Set_DmaMode(0);
		SPI_NAND_Flash_Clear_Read_Cache_Data();
		rtn_status = spi_nand_read_page(page_number, ptr_dev_info_t->read_mode);
		SPI_NAND_Flash_Set_DmaMode(1);
		SPI_NAND_Flash_Clear_Read_Cache_Data();
		_SPI_NAND_SEMAPHORE_UNLOCK();
		
		if (rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "en7512_nand_check_block_bad return error, block:%d\n", offset/ptr_dev_info_t->erase_size);
			SPI_NAND_Flash_Set_DmaMode(1);
			_SPI_NAND_SEMAPHORE_UNLOCK();
			return 1;
		}
		SPI_NAND_Flash_Set_DmaMode(1);
		SPI_NAND_Flash_Clear_Read_Cache_Data();
		_SPI_NAND_SEMAPHORE_UNLOCK();

		memset(def_bad_block_ECC, 0xFF, sizeof(def_bad_block_ECC));
		SPI_NFI_Get_Configure(&spi_nfi_conf_t);

		if(memcmp(def_bad_block_ECC, &(bad_block_indicator[spi_nfi_conf_t.fdm_num]), (spi_nfi_conf_t.spare_size_t - spi_nfi_conf_t.fdm_num)) == 0) {
			if(bmt_block) {
				if(bad_block_indicator[BMT_BAD_BLOCK_INDEX_OFFSET] != 0xff) {
					_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "DMA closed, Bad block detected at page_addr 0x%x, oob_buf[%d] is 0x%x\n", page_number, BMT_BAD_BLOCK_INDEX_OFFSET,_current_cache_page_oob_mapping[BMT_BAD_BLOCK_INDEX_OFFSET]);
					return 1;
				}
			} else {
				if(bad_block_indicator[0] != 0xff) {
					_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "DMA closed, Bad block detected at page_addr 0x%x, oob_buf[0] is 0x%x\n", page_number, _current_cache_page_oob_mapping[0]);
					return 1;
				}
			}
		}
	}
#endif
#endif
	return 0;  /* Good Block*/
}

int en7512_nand_erase(u32 offset)
{
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	SPI_NAND_FLASH_RTN_T			rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	
	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;   

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "en7512_nand_erase: offset =0x%x, erase_size=0x%x\n", offset, (ptr_dev_info_t->erase_size));

	SPI_NAND_Flash_Clear_Read_Cache_Data();
	
	rtn_status = spi_nand_erase_block((offset / ptr_dev_info_t->erase_size));
	
	if(rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR)
	{
		return 0;		
	}
	else
	{
		_SPI_NAND_PRINTF("en7512_nand_erase : Fail \n");
		return -1;
	}
}

int en7512_nand_mark_badblock(u32 offset, u32 bmt_block)
{
	u32 							page_number;
	u8 buf[8];
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	SPI_NAND_FLASH_RTN_T			rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	ptr_dev_info_t  = _SPI_NAND_GET_DEVICE_INFO_PTR;	

	/* Caculate page number */
	page_number = (((offset / BLOCK_SIZE) * BLOCK_SIZE) / PAGE_SIZE);	
  
	memset(buf, 0xFF, 8);
#if defined(TCSUPPORT_NAND_BMT)
	if(bmt_block) {
		if(ptr_dev_info_t->feature & SPI_NAND_FLASH_OOB_RESERVE_FOR_BMT) {
			buf[ooblayout_feature7.oobfree[0].offset] = 0;
		} else {
			buf[BMT_BAD_BLOCK_INDEX_OFFSET] = 0;
		}
	} else
#endif
	{
		buf[0] = 0;
	}

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "en7512_nand_mark_badblock: buf info:\n");
	_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &buf[0], 8);
  
	_SPI_NAND_PRINTF("en7512_nand_mark_badblock: page_num=0x%x\n", page_number);
  
	spi_nand_erase_block((offset / (ptr_dev_info_t->erase_size)));
	rtn_status = spi_nand_write_page(page_number, 0, NULL, 0, 0, &buf[0], 8, ptr_dev_info_t->write_mode);
	
	if( rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}
int en7512_nand_exec_write_page(u32 page, u8 *dat, u8 *oob)
{

	SPI_NAND_FLASH_RTN_T			rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;

	ptr_dev_info_t  = _SPI_NAND_GET_DEVICE_INFO_PTR;

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "en7512_nand_exec_write_page: page=0x%x\n", page);

	rtn_status = spi_nand_write_page(page, 0, dat, ptr_dev_info_t->page_size, 0, oob, ptr_dev_info_t->oob_size, ptr_dev_info_t->write_mode);

	if( rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

#if defined(TCSUPPORT_NAND_BMT)
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
int calc_bmt_pool_size(struct mtd_info *mtd)
#else
int calc_bmt_pool_size(struct ra_nand_chip *ra_chip)
#endif
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
	struct nand_chip *nand = mtd->priv;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
	unsigned int chip_size  = nanddev_target_size(&nand->base);
#else
    unsigned int chip_size = nand->chipsize;
#endif	
    unsigned int block_size = 1 << nand->phys_erase_shift;
#else
    unsigned int chip_size = 1 << ra_chip->flash->chip_shift;
    unsigned int block_size = 1 << ra_chip->flash->erase_shift;
#endif
    unsigned int total_block = chip_size / block_size;
    unsigned int last_block = total_block - 1;
    u16 valid_block_num = 0;
    u16 need_valid_block_num = total_block * POOL_GOOD_BLOCK_PERCENT;

/*#if defined(TCSUPPORT_CT_PON) || defined(TCSUPPORT_OPENWRT)*/
	maximum_bmt_block_count = total_block * MAX_BMT_SIZE_PERCENTAGE_CT;
/*#else
	maximum_bmt_block_count = total_block * MAX_BMT_SIZE_PERCENTAGE;
#endif*/

	nand_flash_avalable_size = chip_size - (maximum_bmt_block_count * block_size);

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "\r\navalable block = %d\n", nand_flash_avalable_size / block_size);

    for(;last_block > 0; --last_block) {
        if(en7512_nand_check_block_bad((last_block*block_size), BAD_BLOCK_RAW)) {
            continue;  
        } else {
            valid_block_num++;
            if(valid_block_num == need_valid_block_num) {
                break;
            }
        }
    }

    return (total_block - last_block);   
}
#endif
#endif //defined(TCSUPPORT_NAND_BMT)

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
#if !defined(BOOTROM_EXT)
#if defined(TCSUPPORT_NAND_BMT) && !defined(LZMA_IMG)
bmt_struct *get_g_bmt(void)
{
	return g_bmt;
}

init_bbt_struct *get_g_bbt(void)
{
	return g_bbt;
}

u32 get_maximum_bmt_block_count(void)
{
	return maximum_bmt_block_count;
}

void set_dma_write_addr(u8 *addr)
{
	dma_write_page = addr;
}

void reset_dma_write_addr(void)
{
	set_dma_write_addr(tmp_dma_write_page + (CACHE_LINE_SIZE - (((uintptr_t)tmp_dma_write_page) % CACHE_LINE_SIZE)));
}
#endif //#if defined(TCSUPPORT_NAND_BMT) && !defined(LZMA_IMG)

SPI_NAND_FLASH_RTN_T get_spi_nand_protocol_read_id (struct SPI_NAND_FLASH_INFO_T *ptr_rtn_flash_id)
{
	return spi_nand_protocol_read_id(ptr_rtn_flash_id);
}

SPI_NAND_FLASH_RTN_T get_spi_nand_protocol_read_id_2 (struct SPI_NAND_FLASH_INFO_T *ptr_rtn_flash_id)
{
	return spi_nand_protocol_read_id_2(ptr_rtn_flash_id);
}

void set_test_write_fail_flag(u32 val)
{
	test_write_fail_flag = val;
}

void set_dma_read_addr(u8 *addr)
{
	dma_read_page = addr;
}

void reset_dma_read_addr(void)
{
	set_dma_read_addr(tmp_dma_read_page + (CACHE_LINE_SIZE - (((uintptr_t)tmp_dma_read_page) % CACHE_LINE_SIZE)));
}

SPI_NAND_FLASH_RTN_T set_spi_nand_load_page_into_cache(u32 page_number)
{
	return spi_nand_load_page_into_cache(page_number);
}
#endif //#if !defined(BOOTROM_EXT)
#endif //#if !(defined(TCSUPPORT_2_6_36_KERNEL) || defined(TCSUPPORT_3_18_21_KERNEL))


/*------------------------------------------------------------------------------------
 * FUNCTION: static void spi_nand_manufacute_init( struct SPI_NAND_FLASH_INFO_T *ptr_device_t )
 * PURPOSE : To init SPI NAND Flash chip
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : None
 *   OUTPUT: None.
 * RETURN  : None.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2015/05/15  - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
static void spi_nand_manufacute_init(struct SPI_NAND_FLASH_INFO_T *ptr_device_t)
{
	unsigned char	feature;
	unsigned char	die;

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1,"spi_nand_manufacute_init: Unlock all block and Enable Quad Mode\n"); 

	for(die = 0; die < ptr_device_t->die_num; die++) {
		spi_nand_direct_select_die(die);

		/* 1. Unlock All block */
		if(ptr_device_t->unlock_block_info.unlock_block_mask != 0x00) {	
			spi_nand_protocol_get_feature(_SPI_NAND_ADDR_PROTECTION, &feature);
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "Before Unlock all block setup, the status register1 = 0x%x\n", feature);
			
			feature = (feature & ~(ptr_device_t->unlock_block_info.unlock_block_mask));
			feature |= (ptr_device_t->unlock_block_info.unlock_block_value & ptr_device_t->unlock_block_info.unlock_block_mask);
			spi_nand_protocol_set_feature(_SPI_NAND_ADDR_PROTECTION, feature);
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "Unlock all block setup, the feature = 0x%x\n", feature);

			spi_nand_protocol_get_feature(_SPI_NAND_ADDR_PROTECTION, &feature);
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "After Unlock all block setup, the status register1 = 0x%x\n", feature);
			/* Set A0[1] be 1 to disable A0 register write protection for skyhigh, then set A0 register again to unlock all block. */
			if (ptr_device_t->mfr_id == _SPI_NAND_MANUFACTURER_ID_SKYHIGH) {
				feature = (feature & ~(ptr_device_t->unlock_block_info.unlock_block_mask));
				feature |= (ptr_device_t->unlock_block_info.unlock_block_value & ptr_device_t->unlock_block_info.unlock_block_mask);
				spi_nand_protocol_set_feature(_SPI_NAND_ADDR_PROTECTION, feature);
				_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "Unlock all block setup, the feature = 0x%x\n", feature);

				spi_nand_protocol_get_feature(_SPI_NAND_ADDR_PROTECTION, &feature);
				_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "After Unlock all block setup, the status register1 = 0x%x\n", feature);
			}
		} else {
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "No need Unlock all block setup.\n");
		}

		/* check continuous read */
		if(ptr_device_t->feature & SPI_NAND_FLASH_DISABLE_CONTINUOUS_RD) {
			_SPI_NAND_PRINTF("Disable CONT_RD\n");
			spi_nand_protocol_get_feature(ptr_device_t->ecc_en.ecc_en_addr, &feature);
			feature &= ~(0x1);
			spi_nand_protocol_set_feature(ptr_device_t->ecc_en.ecc_en_addr, feature);
			/* Value check*/
			spi_nand_protocol_get_feature(ptr_device_t->ecc_en.ecc_en_addr, &feature);
		}
	}

#if defined(TCSUPPORT_NAND_BMT) && !defined(IMAGE_BL2) && ((!defined(LZMA_IMG) && !defined(BOOTROM_EXT)) || defined(TCSUPPORT_BB_256KB))
	if (ptr_device_t->feature & SPI_NAND_FLASH_READ_ECC_ERROR_BIT_CHECK)
	{
		if (ptr_device_t->read_ecc_ceck.type == _SPI_NAND_CHECK_ECC_THROSHOLD_BY_FLASH)
		{
			spi_nand_protocol_set_feature(ptr_device_t->read_ecc_ceck.ext_data[READ_ECC_CHECK_ADDR_INDEX], ptr_device_t->read_ecc_ceck.ext_data[READ_ECC_CHECK_EXT_DATA_INDEX]);
			spi_nand_protocol_get_feature(ptr_device_t->read_ecc_ceck.ext_data[READ_ECC_CHECK_ADDR_INDEX], &feature);

			_SPI_NAND_PRINTF("Set ECC threshold = 0x%x\n", feature);
		}
	}
#endif
}

/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T spi_nand_probe( struct SPI_NAND_FLASH_INFO_T  *ptr_rtn_device_t )
 * PURPOSE : To probe SPI NAND flash id.
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : None
 *   OUTPUT: rtn_index  - The rtn_index variable of this function.
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/12  - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
static SPI_NAND_FLASH_RTN_T spi_nand_probe( struct SPI_NAND_FLASH_INFO_T *ptr_rtn_device_t )
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_PROBE_ERROR;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
	unsigned long			spinand_spinlock_flags;
#endif
	u8 i;

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_probe: start \n");

	/* Protocol for read id */
	_SPI_NAND_SEMAPHORE_LOCK();	
	spi_nand_protocol_read_id(ptr_rtn_device_t );
	_SPI_NAND_SEMAPHORE_UNLOCK();	

	rtn_status = scan_spi_nand_table(ptr_rtn_device_t);

	if ( rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR )
	{
		/* Another protocol for read id  (For example, the GigaDevice SPI NADN chip for Type C */
		_SPI_NAND_SEMAPHORE_LOCK();	
		spi_nand_protocol_read_id_2( (struct SPI_NAND_FLASH_INFO_T *)ptr_rtn_device_t );
		_SPI_NAND_SEMAPHORE_UNLOCK();	

		rtn_status = scan_spi_nand_table(ptr_rtn_device_t);
	}

	_SPI_NAND_PRINTF("spi_nand_probe: mfr_id=0x%x, dev_id=0x%x", ptr_rtn_device_t->mfr_id, ptr_rtn_device_t->dev_id);
	if (ptr_rtn_device_t->extend_dev_id.extend_len)
	{
		for (i=0 ; i<MAX_SPI_NAND_FLASH_EXTEND_DEV_ID_LEN ; i++)
		{
			_SPI_NAND_PRINTF(" 0x%x", ptr_rtn_device_t->extend_dev_id.extend_id[i]);
		}
	}
	_SPI_NAND_PRINTF("\n");

#ifdef BOOTROM_EXT
	prom_puts("===mfr_id:0x");
	prom_print_hex(ptr_rtn_device_t->mfr_id, 2);
	prom_puts(", dev_id:0x");
	prom_print_hex(ptr_rtn_device_t->dev_id, 2);

	if (ptr_rtn_device_t->extend_dev_id.extend_len)
	{
		for (i=0 ; i<MAX_SPI_NAND_FLASH_EXTEND_DEV_ID_LEN ; i++)
		{
			prom_puts(" 0x");
			prom_print_hex(ptr_rtn_device_t->extend_dev_id.extend_id[i], 2);
		}
	}

	prom_puts("===\n");
#endif

#if 0 //defined(IMAGE_BL2)
	NOTICE("mfr_id:0x%02x, dev_id:0x%02x\n", _current_flash_info_t.mfr_id, _current_flash_info_t.dev_id);
	if (ptr_rtn_device_t->extend_dev_id.extend_len) {
		for (i=0 ; i<MAX_SPI_NAND_FLASH_EXTEND_DEV_ID_LEN ; i++) {
			NOTICE(" %02x", ptr_rtn_device_t->extend_dev_id.extend_id[i]);
		}
	}
#endif

	if(rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR)
	{
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_probe: device size:0x%x \n", ptr_rtn_device_t->device_size);
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_probe: erase size :0x%x \n", ptr_rtn_device_t->erase_size);
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_probe: page size  :0x%x \n", ptr_rtn_device_t->page_size);
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_probe: oob size   :%d \n", ptr_rtn_device_t->oob_size);
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_probe: dummy mode :%d \n", ptr_rtn_device_t->dummy_mode);
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_probe: read mode  :%d \n", ptr_rtn_device_t->read_mode);
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_probe: write mode :%d \n", ptr_rtn_device_t->write_mode);
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_probe: feature    :0x%x \n", ptr_rtn_device_t->feature);
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_probe: die num    :%d \n", ptr_rtn_device_t->die_num);
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_probe: write type    :%d \n", ptr_rtn_device_t->write_en_type);
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_probe: ecc check info mask     :%x \n", ptr_rtn_device_t->ecc_fail_check_info.ecc_check_mask);
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_probe: ecc check info value    :%x \n", ptr_rtn_device_t->ecc_fail_check_info.ecc_uncorrected_value);
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_probe: unlock block info mask  :%x \n", ptr_rtn_device_t->unlock_block_info.unlock_block_mask);
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_probe: unlock block info value :%x \n", ptr_rtn_device_t->unlock_block_info.unlock_block_value);
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_probe: quad en info mask       :%x \n", ptr_rtn_device_t->quad_en.quad_en_mask);
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_probe: quad en info value      :%x \n", ptr_rtn_device_t->quad_en.quad_en_value);
		
		_SPI_NAND_SEMAPHORE_LOCK();
		spi_nand_manufacute_init(ptr_rtn_device_t);		
		_SPI_NAND_SEMAPHORE_UNLOCK();	

		if((ptr_rtn_device_t->write_mode == SPI_NAND_FLASH_WRITE_SPEED_MODE_QUAD) || 
			(ptr_rtn_device_t->read_mode == SPI_NAND_FLASH_READ_SPEED_MODE_QUAD)) {
			enable_quad();
		}
	}	
	
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spi_nand_probe: end \n");
	
	return (rtn_status);
}

int enable_dma(SPI_NFI_CONF_T *spi_nfi_conf_t,
				SPI_ECC_ENCODE_CONF_T *encode_conf_t,
				SPI_ECC_DECODE_CONF_T *decode_conf_t)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
	unsigned long spinand_spinlock_flags;
#endif
	
	_SPI_NAND_SEMAPHORE_LOCK();

	/* Set controller to DMA mode */
	SPI_NAND_Flash_Set_DmaMode(SPI_DMA_MODE_ENABLE);

	/* Init DMA */
	SPI_NFI_Init();

	/* config DMA */
	SPI_NFI_Set_Configure(spi_nfi_conf_t);

	/* config ECC */
	if(spi_nfi_conf_t->hw_ecc_t == SPI_NFI_CON_HW_ECC_Enable) {
		if(GET_HIR() >= EN751627_HIR) {
			SPI_NFI_ECC_DATA_SOURCE_INV(SPI_NFI_CONF_ECC_DATA_SOURCE_INV_ENABLE);
		}
		
		/* Init Decode, Encode */
#if !defined(LZMA_IMG) || defined(TCSUPPORT_BB_256KB)
		SPI_ECC_Encode_Init();
#endif
		SPI_ECC_Decode_Init();
			
		/* Setup Encode */
#if !defined(LZMA_IMG) || defined(TCSUPPORT_BB_256KB)
		SPI_ECC_Encode_Set_Configure(encode_conf_t);
#endif	
		/* Setup Decode */
		SPI_ECC_Decode_Set_Configure(decode_conf_t);
	}

	_SPI_NAND_SEMAPHORE_UNLOCK();
	return 0;
}

#if 0
int spinand_ctrlEcc_calibration(SPI_NFI_CONF_T	*spi_nfi_conf_t, 
						 SPI_ECC_ENCODE_CONF_T	*encode_conf_t, 
						 SPI_ECC_DECODE_CONF_T	*decode_conf_t)
{
	unsigned long auto_sizing_magic;
	int auto_ecc_detect_idx;
	int auto_spare_size_idx;
	SPI_NAND_FLASH_DEBUG_LEVEL_T dbg_lv = SPI_NAND_FLASH_DEBUG_LEVEL_1;
	SPI_NAND_FLASH_RTN_T status;
	u32 ecc_ability_array[] = {SPI_ECC_DECODE_ABILITY_4BITS, SPI_ECC_DECODE_ABILITY_6BITS, SPI_ECC_DECODE_ABILITY_8BITS, SPI_ECC_DECODE_ABILITY_10BITS,
							   SPI_ECC_DECODE_ABILITY_12BITS, SPI_ECC_DECODE_ABILITY_14BITS, SPI_ECC_DECODE_ABILITY_16BITS};
	u32 spare_size_array[] = {SPI_NFI_CONF_SPARE_SIZE_16BYTE, SPI_NFI_CONF_SPARE_SIZE_26BYTE, SPI_NFI_CONF_SPARE_SIZE_27BYTE, SPI_NFI_CONF_SPARE_SIZE_28BYTE};
	
	/* auto-detecting ECC ability */
	for(auto_ecc_detect_idx = 0; auto_ecc_detect_idx < (sizeof(ecc_ability_array) / sizeof(ecc_ability_array[0])); auto_ecc_detect_idx++) {
		for(auto_spare_size_idx = 0; auto_spare_size_idx < (sizeof(spare_size_array) / sizeof(spare_size_array[0])); auto_spare_size_idx++) {
			/* Check spare area is enough to store parity */
			if(((spare_size_array[auto_spare_size_idx] - spi_nfi_conf_t->fdm_num) * 8) < (13 * ecc_ability_array[auto_ecc_detect_idx])) {
				_SPI_NAND_DEBUG_PRINTF(dbg_lv, "skip\n");
				_SPI_NAND_DEBUG_PRINTF(dbg_lv, "  ecc  :%02d\n", ecc_ability_array[auto_ecc_detect_idx]);
				_SPI_NAND_DEBUG_PRINTF(dbg_lv, "  spare:%02d\n", spare_size_array[auto_spare_size_idx]);
				continue;
			}

			/* Enable Manual Mode */
			_SPI_NAND_ENABLE_MANUAL_MODE();	

			/* set NFI */
			spi_nfi_conf_t->spare_size_t 		= spare_size_array[auto_spare_size_idx];

			/* Setup Encode */
#if !defined(LZMA_IMG)
			encode_conf_t->encode_ecc_abiliry	= SPI_ECC_ENCODE_ABILITY_4BITS;
#endif

			/* Setup Decode */
			decode_conf_t->decode_ecc_abiliry	= ecc_ability_array[auto_ecc_detect_idx];
			decode_conf_t->decode_block_size 	= (((spi_nfi_conf_t->fdm_ecc_num) + 512) * 8) + ((decode_conf_t->decode_ecc_abiliry) * 13);

			_SPI_NAND_DEBUG_PRINTF(dbg_lv, "testing...\n");
			_SPI_NAND_DEBUG_PRINTF(dbg_lv, "  ecc  :%02d\n", decode_conf_t->decode_ecc_abiliry);
			_SPI_NAND_DEBUG_PRINTF(dbg_lv, "  spare:%02d\n", spi_nfi_conf_t->spare_size_t);

			/* enable DMA */
			enable_dma(spi_nfi_conf_t, encode_conf_t, decode_conf_t);

			SPI_NAND_Flash_Clear_Read_Cache_Data();
			auto_sizing_magic = SPI_NAND_Flash_Read_DWord(PAGE_SIZE_MAGIC_FLASH_ADDR, &status);

			if(status == SPI_NAND_FLASH_RTN_NO_ERROR ) {
				_SPI_NAND_DEBUG_PRINTF(dbg_lv, "magic:0x%x\n", auto_sizing_magic);

				if(auto_sizing_magic == PAGE_SIZE_MAGIC) {
					_SPI_NAND_DEBUG_PRINTF(dbg_lv, "detected:\n");
					_SPI_NAND_DEBUG_PRINTF(dbg_lv, "  ecc  :%02d\n", decode_conf_t->decode_ecc_abiliry);
					_SPI_NAND_DEBUG_PRINTF(dbg_lv, "  spare:%02d\n", spi_nfi_conf_t->spare_size_t);
					return 0;
				}
			}
		} //for auto_spare_size_idx
	} //for auto_ecc_detect_idx

	/* can not find magic number, use default value */
	/* set ECC ability = 4bits */
	/* set spare size = 16B */

	/* set NFI */
	spi_nfi_conf_t->spare_size_t 		= SPI_NFI_CONF_SPARE_SIZE_16BYTE;

	/* Setup Encode */
#if !defined(LZMA_IMG)
	encode_conf_t->encode_ecc_abiliry	= SPI_ECC_ENCODE_ABILITY_4BITS;
#endif
	
	/* Setup Decode */
	decode_conf_t->decode_ecc_abiliry	= SPI_ECC_DECODE_ABILITY_4BITS;
	
	return -1;
}
#endif

#if defined(TCSUPPORT_NAND_BMT)
#if (!defined(LZMA_IMG) && !defined(BOOTROM_EXT)) || defined(TCSUPPORT_BB_256KB)
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
int init_bmt_bbt(struct mtd_info *mtd)
#else
int init_bmt_bbt(struct ra_nand_chip *ra_chip)
#endif
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
	unsigned long spinand_spinlock_flags;
#endif
	
	_SPI_NAND_SEMAPHORE_LOCK();
	
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
	bmt_pool_size = calc_bmt_pool_size(mtd);
#else
	bmt_pool_size = calc_bmt_pool_size(ra_chip);
#endif

	if(bmt_pool_size > maximum_bmt_block_count) {
		_SPI_NAND_PRINTF("Error : bmt pool size: %d > maximum size %d\n", bmt_pool_size, maximum_bmt_block_count);
		_SPI_NAND_PRINTF("Error: init bmt failed \n");
		_SPI_NAND_SEMAPHORE_UNLOCK();
		return -1;
	}

	if(bmt_pool_size > MAX_BMT_SIZE) {
		bmt_pool_size = MAX_BMT_SIZE;
	}

	_SPI_NAND_PRINTF("bmt pool size: %d \n", bmt_pool_size);
	
	if(!g_bmt) {
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
		if (!(g_bmt = init_bmt(mtd, bmt_pool_size)))
#else
		if (!(g_bmt = init_bmt(ra_chip, bmt_pool_size)))
#endif
		{
			_SPI_NAND_PRINTF("Error: init bmt failed \n");
			_SPI_NAND_SEMAPHORE_UNLOCK();
			return -1;
		}
	}

	if(!g_bbt) {
		if (!(g_bbt = start_init_bbt())) {
			_SPI_NAND_PRINTF("Error: init bbt failed \n");
			_SPI_NAND_SEMAPHORE_UNLOCK();
			return -1;
		}
	}

	if(write_bbt_or_bmt_to_flash() != 0) {				
		_SPI_NAND_PRINTF("Error: save bbt or bmt to nand failed \n");
		_SPI_NAND_SEMAPHORE_UNLOCK();
		return -1;
	}
	
	if(create_badblock_table_by_bbt()) {
		_SPI_NAND_PRINTF("Error: create bad block table failed \n");
		_SPI_NAND_SEMAPHORE_UNLOCK();
		return -1;
	}

	_SPI_NAND_PRINTF("BMT & BBT Init Success \n");

	_SPI_NAND_SEMAPHORE_UNLOCK();

	return 0;
}
#endif
#endif

static int nand_probe_init(void)
{
#if ((defined(TCSUPPORT_CPU_ARMV8)) && (LINUX_VERSION_CODE >= KERNEL_VERSION(4,4,0)))
	if (ecnt_nand == NULL)
	{
		_SPI_NAND_PRINTF("\nget spi_dev failed !\n");
		return -ENOMEM;
	}
#endif
#if defined(TCSUPPORT_PARALLEL_NAND)
	if (isParallelNAND)
	{
		_parallel_nand_mode = 1;
		nand_probe = parallel_nand_probe;
		arht_nand_reset = parallel_nand_protocol_reset;
	}
	else
#endif
	{
		nand_probe = spi_nand_probe;
		arht_nand_reset = spi_nand_protocol_reset;
	}
	return 0;
}

__attribute__((unused)) static void set_spi_clock(void)
{
#ifdef TCSUPPORT_CPU_EN7528
	/* 1. set SFC Clock to 40MHZ  */
	spi_set_clock_speed(40);
#else
	/* 1. set SFC Clock to 50MHZ  */
	spi_set_clock_speed(50);
#endif
}

static void debug_config(void)
{
#ifdef ERASE_STATISTICS
	int entry_idx = 0, oob_cnt = 0;
	struct spi_nand_flash_oobfree 	*ptr_oob_entry_idx;	
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t = _SPI_NAND_GET_DEVICE_INFO_PTR;;
#endif

#ifdef ERASE_WRITE_CNT_LOG
		/* init */
		memset(b_erase_cnt, 0, sizeof(b_erase_cnt));
		memset(b_write_total_cnt, 0, sizeof(b_write_total_cnt));
		memset(b_write_cnt_per_erase, 0, sizeof(b_write_cnt_per_erase));
		erase_write_flag = SPI_NAND_FLASH_ERASE_WRITE_LOG_DISABLE;
#endif

#ifdef MULTI_WRITE_DBG
		multi_write_dbg_flag = SPI_NAND_FLASH_MULTI_WRITE_ENABLE;
		memset(page_info, 0, sizeof(page_info));
#endif

#ifdef ERASE_STATISTICS
	if(_current_flash_info_t.feature & SPI_NAND_FLASH_ERASE_STATISTICS) {
		isSupportEraseCntStatistic = 1;
		memset((void *)erase_statistic_cnt, 0xFF, sizeof(erase_statistic_cnt));
		
		/* calculate erase statistics offset */
		oob_cnt = 0;
		MAX_ERASE_CNT_START_OFFSET = 0;
		ptr_oob_entry_idx =  (struct spi_nand_flash_oobfree*) &(ptr_dev_info_t->oob_free_layout->oobfree);

		for(entry_idx = 0; ((entry_idx < SPI_NAND_FLASH_OOB_FREE_ENTRY_MAX) && (ptr_oob_entry_idx[entry_idx].len) && (oob_cnt < ptr_dev_info_t->oob_free_layout->oobsize)); entry_idx++) {
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "ptr_oob_entry_idx[entry_idx].offset:%d\n", ptr_oob_entry_idx[entry_idx].offset);
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "ptr_oob_entry_idx[entry_idx].len:%d\n", ptr_oob_entry_idx[entry_idx].len);
			oob_cnt += ptr_oob_entry_idx[entry_idx].len;
			if(oob_cnt == ptr_dev_info_t->oob_free_layout->oobsize) {
				MAX_ERASE_CNT_START_OFFSET = ptr_oob_entry_idx[entry_idx].offset + ptr_oob_entry_idx[entry_idx].len;
			}
		}	
		
		_SPI_NAND_PRINTF("Support SPI NAND erase statistic.\n");
	}
#endif
}

static int spi_buf_init(void)
{
#if (defined(TCSUPPORT_CPU_ARMV8) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)))
	u32 						rc;
#endif

	/* for cache line alignment */
	if(dma_read_page == NULL) {
#if ((defined(TCSUPPORT_CPU_ARMV8)) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)))
		dma_read_page = (u8 *) kmalloc(_SPI_NAND_CACHE_SIZE, GFP_KERNEL);
        if (dma_read_page == NULL) {
            _SPI_NAND_PRINTF("ERROR: (%s) kmalloc failed for dma_read_page\n", __func__);
            return -1;
        }
		rdma_addr = dma_map_single(ecnt_nand->dev, (void *)dma_read_page, _SPI_NAND_CACHE_SIZE, DMA_FROM_DEVICE);
		rc = dma_mapping_error(ecnt_nand->dev, rdma_addr);
		if (rc) {
			dev_err(ecnt_nand->dev, "dma mapping error\n");
			return -EINVAL;
		}
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "dma_read_page:0x%llx\n", (unsigned long)dma_read_page);
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "rdma_addr:0x%llx\n", (unsigned long)rdma_addr);
#else
		dma_read_page = tmp_dma_read_page + (CACHE_LINE_SIZE - (((uintptr_t)tmp_dma_read_page) % CACHE_LINE_SIZE));
#endif
	}
	
	/* for 32bytes alignment */
#if	!defined(LZMA_IMG) || defined(TCSUPPORT_BB_256KB)
		if(dma_write_page == NULL) {
            #if ((defined(TCSUPPORT_CPU_ARMV8)) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)))
			dma_write_page = (u8 *) kmalloc(_SPI_NAND_CACHE_SIZE, GFP_KERNEL);
            if (dma_write_page == NULL) {
            _SPI_NAND_PRINTF("ERROR: (%s) kmalloc failed for dma_write_page\n", __func__);
				kfree(dma_read_page);
                return -1;
            }
			wdma_addr = dma_map_single(ecnt_nand->dev, (void *)dma_write_page, _SPI_NAND_CACHE_SIZE, DMA_TO_DEVICE);
			rc = dma_mapping_error(ecnt_nand->dev, wdma_addr);
			if (rc) {
				dev_err(ecnt_nand->dev, "dma mapping error\n");
				return -EINVAL;
			}
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "dma_write_page:0x%llx\n", (unsigned long)dma_write_page);
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "wdma_addr:0x%llx\n", (unsigned long)wdma_addr);
            #else
			dma_write_page = tmp_dma_write_page + (CACHE_LINE_SIZE - (((uintptr_t)tmp_dma_write_page) % CACHE_LINE_SIZE));
            #endif
		}
#endif

	return 0;
}

int spi_nand_bad_block_rate(void)
{
	u32 idx, bad_block_cnt, rate;
	struct SPI_NAND_FLASH_INFO_T *ptr_dev_info_t = _SPI_NAND_GET_DEVICE_INFO_PTR;
	u32 block_size = ptr_dev_info_t->erase_size;
	u32 total_block = ptr_dev_info_t->device_size / block_size;

	bad_block_cnt = 0;

	for(idx = 0; idx < total_block; idx++) {
		if(en7512_nand_check_block_bad(idx * block_size, BAD_BLOCK_RAW) || en7512_nand_check_block_bad(idx * block_size, BMT_BADBLOCK_GENERATE_LATER)) {
			bad_block_cnt++;
		}
	}

	rate = bad_block_cnt * 10000 / total_block;
	_SPI_NAND_PRINTF("\nBlock: bad=%d, total=%d, bad rate:(%d / 10000).\n", bad_block_cnt, total_block, rate);

	return rate;
}

/*------------------------------------------------------------------------------------
 * FUNCTION: SPI_NAND_FLASH_RTN_T SPI_NAND_Flash_Init( long  rom_base )
 * PURPOSE : To provide interface for SPI NAND init.
 * AUTHOR  : 
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : rom_base - The rom_base variable of this function.
 *   OUTPUT: None
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/12  - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
SPI_NAND_FLASH_RTN_T SPI_NAND_Flash_Init(u32 rom_base)
{
	SPI_NFI_CONF_T			spi_nfi_conf_t;
	SPI_ECC_ENCODE_CONF_T	encode_conf_t;
	SPI_ECC_DECODE_CONF_T	decode_conf_t;
	int						sec_num;
	SPI_ECC_ENCODE_ABILITY_T	encode_ecc_abiliry;
	SPI_ECC_DECODE_ABILITY_T	decode_ecc_abiliry;
	SPI_NFI_CONF_SPARE_SIZE_T   spare_size_t;
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_PROBE_ERROR;	
	int						ret = 0;

#ifdef TCSUPPORT_DSL_PHYMODE
#if defined(TCSUPPORT_2_6_36_KERNEL) || defined(TCSUPPORT_3_18_21_KERNEL)
	ranand_read_byte  = SPI_NAND_Flash_Read_Byte;
	ranand_read_dword = SPI_NAND_Flash_Read_DWord;
	return SPI_NAND_FLASH_RTN_NO_ERROR;
#endif
#endif

	memset(&spi_nfi_conf_t,0,sizeof(SPI_NFI_CONF_T));
	ret = nand_probe_init();
	if(ret) {
		_SPI_NAND_PRINTF("nand_probe_init fail.\n");
		return ret;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	if(!_parallel_nand_mode) {
		/* set spi controller clock */
		set_spi_clock();
	}
#endif

	/* 2. Enable Manual Mode */
	_SPI_NAND_ENABLE_MANUAL_MODE();	

 	/* 3. Probe flash information */
	if ( nand_probe(&_current_flash_info_t) != SPI_NAND_FLASH_RTN_NO_ERROR ) {
		_SPI_NAND_PRINTF("SPI NAND Flash Detected Error !\n");
	} else {
		debug_config();
		ret = spi_buf_init();
		if(ret) {
			return ret;
		}

		if(_current_flash_info_t.page_size == _SPI_NAND_PAGE_SIZE_2KBYTE) {
			sec_num = 4;
		} else if(_current_flash_info_t.page_size == _SPI_NAND_PAGE_SIZE_4KBYTE) {
			sec_num = 8;
		} else {
			sec_num = 1;
		}

		if(isSpiNandAndCtrlECC) {
			_SPI_NAND_PRINTF("Using SoC ECC.\n");
			/* Disable OnDie ECC */
			SPI_NAND_Flash_Disable_OnDie_ECC();

			if(GET_HIR() >= EN751627_HIR) {
				switch(_current_flash_info_t.soc_ecc_ability) {
					case SPI_ECC_ENCODE_ABILITY_4BITS:
						spare_size_t		= SPI_NFI_CONF_SPARE_SIZE_16BYTE;
						encode_ecc_abiliry	= SPI_ECC_ENCODE_ABILITY_4BITS;
						decode_ecc_abiliry	= SPI_ECC_DECODE_ABILITY_4BITS;
						break;

					case SPI_ECC_ENCODE_ABILITY_8BITS:
						spare_size_t		= SPI_NFI_CONF_SPARE_SIZE_28BYTE;
						encode_ecc_abiliry	= SPI_ECC_ENCODE_ABILITY_8BITS;
						decode_ecc_abiliry	= SPI_ECC_DECODE_ABILITY_8BITS;
						break;

					case SPI_ECC_ENCODE_ABILITY_12BITS:
						spare_size_t		= SPI_NFI_CONF_SPARE_SIZE_28BYTE;
						encode_ecc_abiliry	= SPI_ECC_ENCODE_ABILITY_12BITS;
						decode_ecc_abiliry	= SPI_ECC_DECODE_ABILITY_12BITS;
						break;

					default:
						_SPI_NAND_PRINTF("Error, unknow flash oob size:%u.\n", _current_flash_info_t.soc_ecc_ability);
						return -1;
				}
			} else {
				/* en7526c */
				spare_size_t		= SPI_NFI_CONF_SPARE_SIZE_16BYTE;
				encode_ecc_abiliry	= SPI_ECC_ENCODE_ABILITY_4BITS;
				decode_ecc_abiliry	= SPI_ECC_DECODE_ABILITY_4BITS;
			}
			_SPI_NAND_PRINTF("  ECC ability:%d\n", encode_ecc_abiliry);
			_SPI_NAND_PRINTF("  NFI spare area size:%d\n", spare_size_t);

			/* Setup NFI */
			spi_nfi_conf_t.auto_fdm_t			= SPI_NFI_CON_AUTO_FDM_Enable;
			spi_nfi_conf_t.hw_ecc_t 			= SPI_NFI_CON_HW_ECC_Enable;
			spi_nfi_conf_t.dma_burst_t			= SPI_NFI_CON_DMA_BURST_Enable;
			spi_nfi_conf_t.fdm_num				= 8;
			spi_nfi_conf_t.fdm_ecc_num			= 8;
			spi_nfi_conf_t.page_size_t			= _current_flash_info_t.page_size;
			spi_nfi_conf_t.sec_num				= sec_num;
			spi_nfi_conf_t.cus_sec_size_en_t	= SPI_NFI_CONF_CUS_SEC_SIZE_Disable;
			spi_nfi_conf_t.sec_size 			= 0;
			spi_nfi_conf_t.spare_size_t 		= spare_size_t;

			/* Setup Encode */
#if !defined(LZMA_IMG) || defined(TCSUPPORT_BB_256KB)
			encode_conf_t.encode_en 			= SPI_ECC_ENCODE_ENABLE;
			encode_conf_t.encode_ecc_abiliry	= encode_ecc_abiliry;
			encode_conf_t.encode_block_size 	= 512 + spi_nfi_conf_t.fdm_ecc_num;
#endif
					
			/* Setup Decode */
			decode_conf_t.decode_en 			= SPI_ECC_DECODE_ENABLE;
			decode_conf_t.decode_ecc_abiliry	= decode_ecc_abiliry;
			decode_conf_t.decode_block_size 	= (((spi_nfi_conf_t.fdm_ecc_num) + 512) * 8) + ((decode_conf_t.decode_ecc_abiliry) * 13);

			/* enable DMA */
			enable_dma(&spi_nfi_conf_t, &encode_conf_t, &decode_conf_t);
		} else {
			_SPI_NAND_PRINTF("Using Flash ECC.\n");
			SPI_NAND_Flash_Enable_OnDie_ECC();
#if defined(TCSUPPORT_SPI_NAND_FLASH_ECC_DMA) && !defined(IMAGE_BL2)
			/* BL2 is worked at L2C or FW SRAM, SPI controller DMA does not support these two SRAM */
			if(GET_HIR() >= EN7526C_HIR) {
				/* Setup NFI */
				spi_nfi_conf_t.auto_fdm_t			= SPI_NFI_CON_AUTO_FDM_Disable;
				spi_nfi_conf_t.hw_ecc_t 			= SPI_NFI_CON_HW_ECC_Disable;
				spi_nfi_conf_t.dma_burst_t			= SPI_NFI_CON_DMA_BURST_Enable;
				spi_nfi_conf_t.spare_size_t 		= SPI_NFI_CONF_SPARE_SIZE_16BYTE;
				spi_nfi_conf_t.page_size_t			= _current_flash_info_t.page_size;
				spi_nfi_conf_t.sec_num				= sec_num;
				spi_nfi_conf_t.cus_sec_size_en_t	= SPI_NFI_CONF_CUS_SEC_SIZE_Enable;
				spi_nfi_conf_t.sec_size 			= (_current_flash_info_t.page_size + _current_flash_info_t.oob_size) / sec_num;

				/* Setup Encode */
#if !defined(LZMA_IMG) || defined(TCSUPPORT_BB_256KB)
				encode_conf_t.encode_en 			= SPI_ECC_ENCODE_DISABLE;
#endif
						
				/* Setup Decode */
				decode_conf_t.decode_en 			= SPI_ECC_DECODE_DISABLE;

				/* enable DMA */
				enable_dma(&spi_nfi_conf_t, &encode_conf_t, &decode_conf_t);
			}
#endif
		}

		_SPI_NAND_PRINTF("Detected SPI NAND Flash : %s, Flash Size=0x%x\n", _current_flash_info_t.ptr_name,  _current_flash_info_t.device_size);

		rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
		
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
#if !defined(BOOTROM_EXT) || defined(TCSUPPORT_BB_256KB)
		devinfo.blocksize = (_current_flash_info_t.erase_size)/1024;
		devinfo.totalsize = ((_current_flash_info_t.device_size)/(1024*1024));

		/*  For bootloader to caculate flash size information */
		memset((void*) &ra, 0, sizeof(struct ra_nand_chip));
		memset((void*) &flashInfo, 0, sizeof(struct nand_info));
		
		flashInfo.chip_shift = generic_ffs(_current_flash_info_t.device_size) - 1;
		flashInfo.erase_shift = generic_ffs(_current_flash_info_t.erase_size) - 1;
		flashInfo.page_shift = generic_ffs(_current_flash_info_t.page_size) - 1;
		flashInfo.oob_shift = generic_ffs(MAX_LINUX_USE_OOB_SIZE) - 1;
		ra.flash = &flashInfo;					

/*#if defined(TCSUPPORT_CT_PON) || defined(TCSUPPORT_OPENWRT)*/
reservearea_size = 0x40000;
/*#else
reservearea_size = _current_flash_info_t.erase_size;
#endif*/

#if	defined(TCSUPPORT_NAND_BMT)
#if (!defined(LZMA_IMG) && !defined(BOOTROM_EXT)) || defined(TCSUPPORT_BB_256KB)
		if(init_bmt_bbt(&ra) == -1) {
			return -1;
		}
#endif
#endif //#if defined(TCSUPPORT_NAND_BMT) && !defined(LZMA_IMG) && !defined(BOOTROM_EXT)
#endif //#if !(defined(BOOTROM_EXT) && (defined(TCSUPPORT_CPU_EN7516)||defined(TCSUPPORT_CPU_EN7527)))
#endif //
	}

#if !defined(CONFIG_ECNT_UBOOT)
	bad_block_rate = spi_nand_bad_block_rate();
#endif

   return (rtn_status);
}

int nandflash_init(int rom_base)
{	
	if(SPI_NAND_Flash_Init(rom_base) == SPI_NAND_FLASH_RTN_NO_ERROR) {
		return 0;
	} else {
		return -1;
	}
}

/***********************************************************************************/
/***********************************************************************************/
/***** Modify for SPI NAND linux kernel driver below *******************************/
/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)

/* feature/ status reg */
#define REG_BLOCK_LOCK                  0xa0
#define REG_OTP                         0xb0
#define REG_STATUS                      0xc0/* timing */

/* status */
#define STATUS_OIP_MASK                 0x01
#define STATUS_READY                    (0 << 0)
#define STATUS_BUSY                     (1 << 0)

#define STATUS_E_FAIL_MASK              0x04
#define STATUS_E_FAIL                   (1 << 2)

#define STATUS_P_FAIL_MASK              0x08
#define STATUS_P_FAIL                   (1 << 3)

#define STATUS_ECC_MASK                 0x30
#define STATUS_ECC_1BIT_CORRECTED       (1 << 4)
#define STATUS_ECC_ERROR                (2 << 4)
#define STATUS_ECC_RESERVED             (3 << 4)

/*ECC enable defines*/
#define OTP_ECC_MASK                    0x10
#define OTP_ECC_OFF                     0
#define OTP_ECC_ON                      1

#define ECC_DISABLED
#define ECC_IN_NAND
#define ECC_SOFT

#define SPI_NAND_PROCNAME				"driver/spi_nand_debug"
#define SPI_NAND_TEST					"driver/spi_nand_test"
#define SPI_NAND_BAD_BLOCK_RATE	"driver/spi_nand_bad_block_rate"

#define BUFSIZE (2 * 2048)

#define CONFIG_MTD_SPINAND_ONDIEECC		1

#define MAX_WAIT_JIFFIES  (40 * HZ)

#if defined(CONFIG_MTD_SPINAND_ONDIEECC) && (LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0))
static struct nand_ecclayout spinand_oob_64 = {
	.eccbytes = 0,
};
#endif


/* NAND driver */
int ra_nand_init(void)
{
	return 0;
}

void ra_nand_remove(void)
{

}

static inline struct spinand_state *mtd_to_state(struct mtd_info *mtd)
{
    struct nand_chip *chip = (struct nand_chip *)mtd->priv;
    struct spinand_info *info = (struct spinand_info *)chip->priv;
    struct spinand_state *state = (struct spinand_state *)info->priv;

    return state;
}

/*
 * spinand_read_id- Read SPI Nand ID
 * Description:
 *    Read ID: read two ID bytes from the SPI Nand device
 */
static int spinand_read_id(struct spi_device *spi_nand, u8 *id)
{
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	ptr_dev_info_t		= 	_SPI_NAND_GET_DEVICE_INFO_PTR;	

	id[0] = ptr_dev_info_t->mfr_id;
	id[1] = ptr_dev_info_t->dev_id;
	
	return 0;
}

/*
 * spinand_read_status- send command 0xf to the SPI Nand status register
 * Description:
 *    After read, write, or erase, the Nand device is expected to set the
 *    busy status.
 *    This function is to allow reading the status of the command: read,
 *    write, and erase.
 *    Once the status turns to be ready, the other status bits also are
 *    valid status bits.
 */
static int spinand_read_status(struct spi_device *spi_nand, uint8_t *status)
{
	int ret;
	unsigned long spinand_spinlock_flags;
	
	_SPI_NAND_SEMAPHORE_LOCK();
#if defined(TCSUPPORT_PARALLEL_NAND)
	if (_parallel_nand_mode) {
		ret = parallel_nand_protocol_get_status(status);
		if (((*status)&PARALLEL_NAND_READY) == PARALLEL_NAND_READY) {
			*status = STATUS_READY;
		} else {
			*status = STATUS_BUSY;
		}
	}
	else
#endif
	ret = spi_nand_protocol_get_feature(_SPI_NAND_ADDR_STATUS, status);
	_SPI_NAND_SEMAPHORE_UNLOCK();

	return ret;
}

static int wait_till_ready(struct spi_device *spi_nand)
{
	unsigned long deadline;
	int retval;
	u8 stat = 0;

	deadline = jiffies + MAX_WAIT_JIFFIES;
	do {
		retval = spinand_read_status(spi_nand, &stat);
		if (retval < 0)
			return -1;
		else if (!(stat & 0x1))
			break;

		cond_resched();
	} while (!time_after_eq(jiffies, deadline));

	if ((stat & 0x1) == 0)
		return 0;

	return -1;
}

/**
 * spinand_get_otp- send command 0xf to read the SPI Nand OTP register
 * Description:
 *   There is one bit( bit 0x10 ) to set or to clear the internal ECC.
 *   Enable chip internal ECC, set the bit to 1
 *   Disable chip internal ECC, clear the bit to 0
 */
static int spinand_get_otp(struct spi_device *spi_nand, u8 *otp)
{
	struct SPI_NAND_FLASH_INFO_T *ptr_dev_info_t = _SPI_NAND_GET_DEVICE_INFO_PTR;
	unsigned long spinand_spinlock_flags;
	int ret;

	_SPI_NAND_SEMAPHORE_LOCK();
	ret = spi_nand_protocol_get_feature(ptr_dev_info_t->ecc_en.ecc_en_addr, otp);
	_SPI_NAND_SEMAPHORE_UNLOCK();

	return ret;
}

/**
 * spinand_set_otp- send command 0x1f to write the SPI Nand OTP register
 * Description:
 *   There is one bit( bit 0x10 ) to set or to clear the internal ECC.
 *   Enable chip internal ECC, set the bit to 1
 *   Disable chip internal ECC, clear the bit to 0
 */
__attribute__((unused)) static int spinand_set_otp(struct spi_device *spi_nand, u8 *otp)
{
	struct SPI_NAND_FLASH_INFO_T *ptr_dev_info_t =  _SPI_NAND_GET_DEVICE_INFO_PTR;

	return spi_nand_protocol_set_feature(ptr_dev_info_t->ecc_en.ecc_en_addr, *otp);
}

static SPI_NAND_FLASH_RTN_T spi_nand_write_page_internal(u32 page_number, 
														u32 data_offset,
										  				const u8  *ptr_data, 
													  	u32 data_len, 
													  	u32 oob_offset,
														u8  *ptr_oob , 
														u32 oob_len,
													    SPI_NAND_FLASH_WRITE_SPEED_MODE_T speed_mode )
{
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	SPI_NAND_FLASH_RTN_T			rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	unsigned long					spinand_spinlock_flags;
		
#if	defined(TCSUPPORT_NAND_BMT)
    unsigned short phy_block_bbt;
	u32			   logical_block, physical_block;
	u32			   page_offset_in_block;
#endif

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "[spi_nand_write_page_internal]: enter\n");
	
	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;	
	
	_SPI_NAND_SEMAPHORE_LOCK();	
	
	SPI_NAND_Flash_Clear_Read_Cache_Data();

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "dump ptr_oob oob_len:%d\n", oob_len);	
	_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, ptr_oob, oob_len);
	
#if	defined(TCSUPPORT_NAND_BMT)
		if(otp_status == NAND_FLASH_OTP_DISABLE) {
			page_offset_in_block = ((page_number * (ptr_dev_info_t->page_size))%(ptr_dev_info_t->erase_size))/(ptr_dev_info_t->page_size);
			logical_block = ((page_number * (ptr_dev_info_t->page_size))/(ptr_dev_info_t->erase_size)) ;
			physical_block = get_mapping_block_index(logical_block, &phy_block_bbt);
			if( physical_block != logical_block) {			
				_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "Bad Block Mapping, from %d block to %d block\n", logical_block, physical_block);
			}
			page_number = (page_offset_in_block)+((physical_block*(ptr_dev_info_t->erase_size))/(ptr_dev_info_t->page_size));
		}
#endif
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "dump ptr_oob oob_len:%d\n", ptr_oob, oob_len);	
		_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, ptr_oob, oob_len);	
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "[spi_nand_write_page_internal]: page_number = 0x%x\n", page_number);
		
#if	defined(TCSUPPORT_NAND_BMT)
		if(otp_status == NAND_FLASH_OTP_DISABLE) {
			if(block_is_in_bmt_region(physical_block)) {						
				if(ptr_dev_info_t->feature & SPI_NAND_FLASH_OOB_RESERVE_FOR_BMT) {
					memcpy(ptr_oob + ooblayout_feature7.oobfree[0 + 1].offset, &phy_block_bbt, OOB_INDEX_SIZE);
				} else {
					memcpy(ptr_oob + OOB_INDEX_OFFSET, &phy_block_bbt, OOB_INDEX_SIZE);
				}
				_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "3 dump ptr_oob oob_len:%d\n", ptr_oob, oob_len);	
				_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, ptr_oob, oob_len);			
			}
		}
		
       if(_SPI_NAND_WRITE_FAIL_TEST_FLAG == 0) {
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spi_nand_write_page_internal: data_offset=0x%x, date_len=0x%x, oob_offset=0x%x, oob_len=0x%x\n", data_offset, data_len, oob_offset, oob_len);
			_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &ptr_oob[0], oob_len);

			rtn_status = spi_nand_write_page(page_number, data_offset, ptr_data, data_len, 0, ptr_oob, MAX_USE_OOB_SIZE , speed_mode); //COV, CID 140177
       } else {
	   		rtn_status = SPI_NAND_FLASH_RTN_PROGRAM_FAIL;
	   }
					
		if(rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
		    _SPI_NAND_PRINTF("write fail at page: 0x%x \n", page_number);
            if (update_bmt(page_number * (ptr_dev_info_t->page_size), UPDATE_WRITE_FAIL, (u8 *)ptr_data, ptr_oob)) {
                _SPI_NAND_PRINTF("Update BMT success\n");
				rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
            } else {
                _SPI_NAND_PRINTF("Update BMT fail\n");
            }		
		}
#else
		rtn_status = spi_nand_write_page(page_number, data_offset, ptr_data, data_len, 0, ptr_oob, MAX_USE_OOB_SIZE , speed_mode); //COV, CID 140177
#endif

	SPI_NAND_Flash_Clear_Read_Cache_Data();
	
	_SPI_NAND_SEMAPHORE_UNLOCK();
	
	return (rtn_status);
}

static SPI_NAND_FLASH_RTN_T spi_nand_read_page_internal(u32 page_number, 
														SPI_NAND_FLASH_READ_SPEED_MODE_T speed_mode)
{
	u32 							logical_block, physical_block;
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	SPI_NAND_FLASH_RTN_T			rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	unsigned long					spinand_spinlock_flags;

#if	defined(TCSUPPORT_NAND_BMT)
	unsigned short phy_block_bbt;
	u32 		   page_offset_in_block;
#endif

	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;			

	_SPI_NAND_SEMAPHORE_LOCK(); 	
	
#if	defined(TCSUPPORT_NAND_BMT)
	if(otp_status == NAND_FLASH_OTP_DISABLE) {
		page_offset_in_block = (((page_number * (ptr_dev_info_t->page_size))%(ptr_dev_info_t->erase_size))/ (ptr_dev_info_t->page_size));
		logical_block = ((page_number * (ptr_dev_info_t->page_size))/(ptr_dev_info_t->erase_size)) ;
		physical_block = get_mapping_block_index(logical_block, &phy_block_bbt);
		if( physical_block != logical_block) {			
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "Bad Block Mapping, from %d block to %d block\n", logical_block, physical_block);
		}
		page_number = (page_offset_in_block)+ ((physical_block*(ptr_dev_info_t->erase_size))/(ptr_dev_info_t->page_size));
	}
#endif				

	rtn_status = spi_nand_read_page(page_number, speed_mode);

	_SPI_NAND_SEMAPHORE_UNLOCK();
			
	return (rtn_status);
}

/**
 * spinand_write_enable- send command 0x06 to enable write or erase the
 * Nand cells
 * Description:
 *   Before write and erase the Nand cells, the write enable has to be set.
 *   After the write or erase, the write enable bit is automatically
 *   cleared (status register bit 2)
 *   Set the bit 2 of the status register has the same effect
 */
__attribute__((unused)) static int spinand_write_enable(struct spi_device *spi_nand)
{
	return spi_nand_protocol_write_enable();
}

__attribute__((unused)) static int spinand_read_page_to_cache(struct spi_device *spi_nand, u32 page_id)
{
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "spinand_read_page_to_cache: page_idx=0x%x\n", page_id);
	return spi_nand_protocol_page_read((u32)page_id);
}

/*
 * spinand_read_from_cache- send command 0x03 to read out the data from the
 * cache register(2112 bytes max)
 * Description:
 *   The read can specify 1 to 2112 bytes of data read at the corresponding
 *   locations.
 *   No tRd delay.
 */
__attribute__((unused)) static int spinand_read_from_cache(struct spi_device *spi_nand, u32 page_id,
                u32 byte_id, u32 len, u8 *rbuf)
{
	int					ret;
	u8								status;
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;

	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;
		
	spi_nand_protocol_read_from_cache(byte_id, len, rbuf, ptr_dev_info_t->read_mode, ptr_dev_info_t->dummy_mode);
      
	while (1) {
		ret = spinand_read_status(spi_nand, &status);
		if (ret < 0) {
			_SPI_NAND_PRINTF("err %d read status register\n", ret);
			return ret;
		}

		if ((status & STATUS_OIP_MASK) == STATUS_READY) {
			break;
		}
	}            

	return 0;  /* tmp return success any way */
}

/*
 * spinand_read_page-to read a page with:
 * @page_id: the physical page number
 * @offset:  the location from 0 to 2111
 * @len:     number of bytes to read
 * @rbuf:    read buffer to hold @len bytes
 *
 * Description:
 *   The read includes two commands to the Nand: 0x13 and 0x03 commands
 *   Poll to read status to wait for tRD time.
 */
__attribute__((unused)) static int spinand_read_page(struct spi_device *spi_nand, u64 page_id,
                u32 offset, u32 len, u8 *rbuf)
{
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	SPI_NAND_FLASH_RTN_T			rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	/* for coverity(CID:186337), avoid uninitialized pointer */
	SPI_NAND_FLASH_RTN_T 			status;

	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;

	rtn_status = spi_nand_read_internal((ptr_dev_info_t->page_size * page_id) + offset, len, rbuf, ptr_dev_info_t->read_mode, &status);

	if(rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR) {
		return 0;
	} else {
		_SPI_NAND_PRINTF("spinand_read_page, error\n");
		return -1;
	}
}

/*
 * spinand_program_data_to_cache--to write a page to cache with:
 * @byte_id: the location to write to the cache
 * @len:     number of bytes to write
 * @rbuf:    read buffer to hold @len bytes
 *
 * Description:
 *   The write command used here is 0x84--indicating that the cache is
 *   not cleared first.
 *   Since it is writing the data to cache, there is no tPROG time.
 */
__attribute__((unused)) static int spinand_program_data_to_cache(struct spi_device *spi_nand,
                u32 page_id, u32 byte_id, u32 len, u8 *wbuf)
{
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;

	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;
	
	return spi_nand_protocol_program_load(byte_id, wbuf, len, ptr_dev_info_t->write_mode);
}

/**
 * spinand_program_execute--to write a page from cache to the Nand array with
 * @page_id: the physical page location to write the page.
 *
 * Description:
 *   The write command used here is 0x10--indicating the cache is writing to
 *   the Nand array.
 *   Need to wait for tPROG time to finish the transaction.
 */
__attribute__((unused)) static int spinand_program_execute(struct spi_device *spi_nand, u32 page_id)
{
	return spi_nand_protocol_program_execute(page_id);
}

/**
 * spinand_program_page--to write a page with:
 * @page_id: the physical page location to write the page.
 * @offset:  the location from the cache starting from 0 to 2111
 * @len:     the number of bytes to write
 * @wbuf:    the buffer to hold the number of bytes
 *
 * Description:
 *   The commands used here are 0x06, 0x84, and 0x10--indicating that
 *   the write enable is first sent, the write cache command, and the
 *   write execute command.
 *   Poll to wait for the tPROG time to finish the transaction.
 */
static int spinand_program_page(struct mtd_info *mtd,
                u32 page_id, u32 offset, u32 len, u8 *buf)
{
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	struct spinand_state *state = mtd_to_state(mtd);			
	SPI_NAND_FLASH_RTN_T			rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "[spinand_program_page]: enter, page=0x%x\n", page_id);
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spinand_program_page: _current_cache_page_oob_mapping: state->oob_buf_len=0x%x, state->oob_buf=\n", (state->oob_buf_len));
	_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &(state->oob_buf[0]), (state->oob_buf_len));
	
	rtn_status = spi_nand_write_page_internal(page_id, (state->buf_idx), &state->buf[(state->buf_idx)], (state->buf_len),  0, (&state->oob_buf[0]), (state->oob_buf_len), ptr_dev_info_t->write_mode);

	if( rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR) {
		return 0;
	} else {
		_SPI_NAND_PRINTF("spinand_program_page, error\n");
		return -1;
	}
}

/**
 * spinand_erase_block_erase--to erase a page with:
 * @block_id: the physical block location to erase.
 *
 * Description:
 *   The command used here is 0xd8--indicating an erase command to erase
 *   one block--64 pages
 *   Need to wait for tERS.
 */
__attribute__((unused)) static int spinand_erase_block_erase(struct spi_device *spi_nand, u32 block_id)
{
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1,"[spinand_erase_block_erase]: enter, block id=0x%x \n", block_id);
	return spi_nand_protocol_block_erase(block_id);
}

/**
 * spinand_erase_block--to erase a page with:
 * @block_id: the physical block location to erase.
 *
 * Description:
 *   The commands used here are 0x06 and 0xd8--indicating an erase
 *   command to erase one block--64 pages
 *   It will first to enable the write enable bit (0x06 command),
 *   and then send the 0xd8 erase command
 *   Poll to wait for the tERS time to complete the tranaction.
 */
static int spinand_erase_block(struct spi_device *spi_nand, u32 block_id)
{
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	SPI_NAND_FLASH_RTN_T			rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;

	
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1,"[spinand_erase_block]: enter, block id=0x%x \n", block_id);
	
	rtn_status = spi_nand_erase_internal(block_id * ptr_dev_info_t->erase_size, ptr_dev_info_t->erase_size);

	if(rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR) {
		return 0;
	} else {
		_SPI_NAND_PRINTF("spinand_erase_block, error\n");
		return -1;
	}
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
void spinand_write_buf(struct nand_chip *chip, const uint8_t *buf, int len)
{
    struct mtd_info *mtd = spi_nand_mtd;
	
#else
void spinand_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
#endif
	int		min_oob_size;
    struct spinand_state *state = mtd_to_state(mtd);

	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "[spinand_write_buf]: enter len:%d \n", len);

	if(state->col >= mtd->writesize) {
		/* Write OOB area */
		min_oob_size = MIN(len, (MAX_USE_OOB_SIZE - ((state->col) - (mtd->writesize))));
		memcpy( &(state->oob_buf)[((state->col)-(mtd->writesize))], buf, min_oob_size);
		
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "state->col:%d, mtd->writesize:%d, min_oob_size:%d\n", state->col, mtd->writesize, min_oob_size);	
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "buf:\n"); 
		_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, buf, min_oob_size);		
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "state->oob_buf:\n");	
		_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, state->oob_buf, min_oob_size);		
		state->col += min_oob_size;
		state->oob_buf_len = min_oob_size;
	} else {
		/* Write Data area */
		memcpy( &(state->buf)[state->col], buf, len);
	    state->col += len;
		state->buf_len += len;
	}
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
void spinand_read_buf(struct nand_chip *chip, uint8_t *buf, int len)
{
    struct mtd_info *mtd = spi_nand_mtd;
#else
void spinand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
#endif
    struct spinand_state *state = mtd_to_state(mtd);
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "[spinand_read_buf]: enter, len=0x%x, offset=0x%x\n", len, state->buf_idx);	

	if(	(state->command == NAND_CMD_READID) ||
		(state->command == NAND_CMD_STATUS) ) {
		memcpy(buf, &state->buf, len);
	} else {
		if( (state->buf_idx) < ( ptr_dev_info_t->page_size )) { 	
			/* Read data area */
			memcpy(buf, &_current_cache_page_data[state->buf_idx], len);
		} else {													
			/* Read oob area */
			memcpy(buf, _current_cache_page_oob_mapping, len);
		}			
	}	

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_2, "spinand_read_buf : idx=0x%x, len=0x%x\n", (state->buf_idx), len);
	_SPI_NAND_DEBUG_PRINTF_ARRAY(SPI_NAND_FLASH_DEBUG_LEVEL_2, &buf[0], len);
	
	state->buf_idx += len;
}

/**
 * spinand_enable_ecc- send command 0x1f to write the SPI Nand OTP register
 * Description:
 *   There is one bit( bit 0x10 ) to set or to clear the internal ECC.
 *   Enable chip internal ECC, set the bit to 1
 *   Disable chip internal ECC, clear the bit to 0
 */
static int spinand_enable_ecc(struct spi_device *spi_nand)
{
    SPI_NAND_FLASH_RTN_T flash_rtn;
	
	flash_rtn = SPI_NAND_Flash_Enable_OnDie_ECC();
	if(flash_rtn != SPI_NAND_FLASH_RTN_NO_ERROR) {
		return -1;
	}
	
	return 0;
}

static int spinand_disable_ecc(struct spi_device *spi_nand)
{
	SPI_NAND_FLASH_RTN_T flash_rtn;
	
	flash_rtn = SPI_NAND_Flash_Disable_OnDie_ECC();
	if(flash_rtn != SPI_NAND_FLASH_RTN_NO_ERROR) {
		return -1;
	}
	
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
static int spinand_block_markbad(struct nand_chip *chip, loff_t offset)
#else
static int spinand_block_markbad(struct mtd_info *mtd, loff_t offset)
#endif
{
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "[spinand_block_markbad]: enter , offset=0x%llx\n", offset);

	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
static int spinand_block_bad(struct nand_chip *chip, loff_t ofs)
#else
static int spinand_block_bad(struct mtd_info *mtd, loff_t ofs, int getchip)
#endif
{
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "[spinand_block_bad]: enter \n");
	
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
static void spinand_select_chip(struct nand_chip *chip, int cs)
#else
static void spinand_select_chip(struct mtd_info *mtd, int dev)
#endif
{
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "[spinand_select_chip]: enter \n");	
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
static int spinand_dev_ready(struct nand_chip *chip)
#else
static int spinand_dev_ready(struct mtd_info *mtd)
#endif
{	
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "[spinand_dev_ready]: enter \n");
	return 1;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
static uint8_t spinand_read_byte(struct nand_chip *chip)
{
	struct mtd_info *mtd = spi_nand_mtd;
#else
static uint8_t spinand_read_byte(struct mtd_info *mtd)
{
#endif
	struct spinand_state *state = mtd_to_state(mtd);
	u8 data;

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "[spinand_read_byte]: enter \n");		

	data = state->buf[state->buf_idx];
	state->buf_idx++;
	return data;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
static int spinand_wait(struct nand_chip *chip)
#else
static int spinand_wait(struct mtd_info *mtd, struct nand_chip *chip)
#endif
{
	struct spinand_info *info = (struct spinand_info *)chip->priv;

	unsigned long timeo = jiffies;
	int retval, state;
	u8 status;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
	state = FL_ERASING;
#else
	state = chip->state;
#endif
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "[spinand_wait]: enter \n");		

	if (state == FL_ERASING)
		timeo += (HZ * 400) / 1000;
	else
		timeo += (HZ * 20) / 1000;

	while (time_before(jiffies, timeo)) {
		retval = spinand_read_status(info->spi, &status);
		if ((status & STATUS_OIP_MASK) == STATUS_READY)
			return 0;

		cond_resched();
	}
	
	return 0;
}

/*
 * spinand_reset- send RESET command "0xff" to the Nand device.
 */
static void spinand_reset(struct spi_device *spi_nand)
{
	unsigned long spinand_spinlock_flags;
	
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "[spinand_reset]: enter \n");

	_SPI_NAND_SEMAPHORE_LOCK();
	arht_nand_reset();
	_SPI_NAND_SEMAPHORE_UNLOCK();
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
static void spinand_cmdfunc(struct nand_chip *chip, unsigned int command, int column, int page)
{
	struct mtd_info *mtd = spi_nand_mtd;
#else
static void spinand_cmdfunc(struct mtd_info *mtd, unsigned int command, int column, int page)
{
	struct nand_chip *chip = (struct nand_chip *)mtd->priv;
#endif

	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;

    struct spinand_info *info = (struct spinand_info *)chip->priv;
    struct spinand_state *state = (struct spinand_state *)info->priv;
    u16		block_id;

	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;

	state->command = command;
	
	switch (command) {
	/*
	 * READ0 - read in first  0x800 bytes
	 */
	case NAND_CMD_READ1:
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "[spinand_cmdfunc]: NAND_CMD_READ1 \n");
			break;
	case NAND_CMD_READ0:
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "[spinand_cmdfunc]: NAND_CMD_READ0 \n");	

			state->buf_idx = column;
			spi_nand_read_page_internal(page, ptr_dev_info_t->read_mode);
			
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1,"[spinand_cmdfunc]: NAND_CMD_READ0/1, End\n");

	        break;
	/* READOOB reads only the OOB because no ECC is performed. */
	case NAND_CMD_READOOB:
 			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "[spinand_cmdfunc]: NAND_CMD_READOOB, page=0x%x \n", page);
	        state->buf_idx = column + (ptr_dev_info_t->page_size);
	        spi_nand_read_page_internal(page, ptr_dev_info_t->read_mode);
			
	        break;
	case NAND_CMD_RNDOUT:
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "[spinand_cmdfunc]: NAND_CMD_RNDOUT \n");
	        state->buf_idx = column;
	        break;
	case NAND_CMD_READID:
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "[spinand_cmdfunc]: NAND_CMD_READID \n");
	        state->buf_idx = 0;
	        spinand_read_id(info->spi, (u8 *)state->buf);
	        break;
	/* ERASE1 stores the block and page address */
	case NAND_CMD_ERASE1:
			block_id = page /((mtd->erasesize)/(mtd->writesize));

			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "[spinand_cmdfunc]: NAND_CMD_ERASE1 \n");			
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "erasesize=0x%x, writesiez=0x%x, page=0x%x, block_idx=0x%x\n", (mtd->erasesize), (mtd->writesize), page, block_id);
	        spinand_erase_block(info->spi, block_id);
	        break;
	/* ERASE2 uses the block and page address from ERASE1 */
	case NAND_CMD_ERASE2:
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1,"[spinand_cmdfunc]: NAND_CMD_ERASE2 \n");
	        break;
	/* SEQIN sets up the addr buffer and all registers except the length */
	case NAND_CMD_SEQIN:
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "[spinand_cmdfunc]: NAND_CMD_SEQIN \n");			
	        state->col = column;
	        state->row = page;
	        state->buf_idx = column;
			state->buf_len = 0;			
			state->oob_buf_len = 0 ;
			memset(state->buf, 0xff, BUFSIZE);
			memset(state->oob_buf, 0xff, MAX_USE_OOB_SIZE);
	        break;
	/* PAGEPROG reuses all of the setup from SEQIN and adds the length */
	case NAND_CMD_PAGEPROG:
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "[spinand_cmdfunc]: NAND_CMD_PAGEPROG \n");			
	        spinand_program_page(mtd, state->row, state->col,
	                        state->buf_idx, state->buf);
	        break;
	case NAND_CMD_STATUS:
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "[spinand_cmdfunc]: NAND_CMD_STATUS \n");			
	        spinand_get_otp(info->spi, state->buf);
	        if (!(state->buf[0] & 0x80))
	                state->buf[0] = 0x80;
	        state->buf_idx = 0;
	        break;
	/* RESET command */
	case NAND_CMD_RESET:
			_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "[spinand_cmdfunc]: NAND_CMD_RESET \n");			
	        if (wait_till_ready(info->spi))
	                _SPI_NAND_PRINTF("WAIT timedout!!!\n");
	        /* a minimum of 250us must elapse before issuing RESET cmd*/
	        udelay(250);
	        spinand_reset(info->spi);
	        break;
	default:
	        _SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1,"[spinand_cmdfunc]: Unknown CMD: 0x%x\n", command);
	}
}

struct nand_flash_dev spi_nand_flash_ids[] = {
	{NULL,	{{0, 0}}, 0, 0, 0, 0, 0, 0, {0, 0}, 0},
	{NULL,	{{0, 0}}, 0, 0, 0, 0, 0, 0, {0, 0}, 0}
};

static void free_allcate_memory(struct mtd_info *mtd)
{
	_SPI_NAND_PRINTF("SPI NAND : free_allcate_memory");
	
	if(mtd == NULL)
		return;
	
	if(mtd->priv == NULL)
	{
		kfree(mtd);
		return;
	}
	
	if(((struct spinand_info *)(((struct nand_chip *)(mtd->priv))->priv))->spi) {
		kfree( ((struct spinand_info *)(((struct nand_chip *)(mtd->priv))->priv))->spi );
	}

	if(((struct spinand_info *)(((struct nand_chip *)(mtd->priv))->priv))) {
		kfree( ((struct spinand_info *)(((struct nand_chip *)(mtd->priv))->priv)) );
	}

	if((((struct nand_chip *)(mtd->priv))->priv))	{
		kfree ( (((struct nand_chip *)(mtd->priv))->priv) ) ;
	}

	if((mtd->priv)) {
		kfree((mtd->priv));
	}

	if(mtd) {
		kfree(mtd);
	}	
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
static int nand_ooblayout_free_lp_ecnt(struct mtd_info *mtd, int section,
					  struct mtd_oob_region *oobregion)
{
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;

	if(ptr_dev_info_t->oob_free_layout->oobfree[section].len == 0) {
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "ptr_dev_info_t->oob_free_layout->oobfree[%d].len == 0\n", section);
		return -ERANGE;
	}

	oobregion->offset = ptr_dev_info_t->oob_free_layout->oobfree[section].offset;
	oobregion->length = ptr_dev_info_t->oob_free_layout->oobfree[section].len;
	
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "%s:%d oobregion->offset:0x%d\n", __func__, __LINE__, oobregion->offset);
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "%s:%d oobregion->len:0x%d\n", __func__, __LINE__, oobregion->length);

	return 0;
}

static const struct mtd_ooblayout_ops nand_ooblayout_lp_ecnt_ops = {
	.ecc = NULL,
	.free = nand_ooblayout_free_lp_ecnt,
};
#endif

static int spi_nand_setup(struct mtd_info **ptr_rtn_mtd_address)
{
    struct mtd_info 		*mtd;
    struct nand_chip 		*chip;
    struct spinand_info 	*info;
    struct spinand_state	*state;    
    struct spi_device		*spi_nand;
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	int ret;

	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;

	_SPI_NAND_PRINTF("[spi_nand_setup] : Enter \n");
    
	/* 1. Allocate neccessary struct memory ,and assigned related pointer */
    info  = kzalloc(sizeof(struct spinand_info),GFP_KERNEL);
    if (!info) {
    	_SPI_NAND_PRINTF("spi_nand_setup: allocate info structure error! \n");
		return -ENOMEM;            
    }
            	            
	/* , temp assign. Other function will pass it, but we wil not use it in functions. */	            
	spi_nand = kzalloc(sizeof(struct spinand_info),GFP_KERNEL);	
    if (!spi_nand) {	
	    _SPI_NAND_PRINTF("spi_nand_setup: allocate spi_nand structure error! \n");
		kfree(info); //COV,  CID 140028
		return -ENOMEM;
    }
		
    info->spi = spi_nand;		

    state = kzalloc(sizeof(struct spinand_state),GFP_KERNEL);    
    if (!state) {	
	    _SPI_NAND_PRINTF("spi_nand_setup: allocate state structure error! \n");
		kfree(spi_nand); //COV,  CID 140028
		kfree(info);
		return -ENOMEM;
    }

    info->priv      = state;
    state->buf_idx  = 0;
    state->buf      = kzalloc( BUFSIZE, GFP_KERNEL);			/* Data buffer */
    if (!state->buf) {
    	_SPI_NAND_PRINTF("spi_nand_setup: allocate data buf error! \n");
		kfree(state); //COV,  CID 140028
		kfree(spi_nand);
		kfree(info);
		return -ENOMEM;
    }
    state->oob_buf	= kzalloc( MAX_USE_OOB_SIZE, GFP_KERNEL);	/* OOB buffer */
    if (!state->oob_buf) {
    	_SPI_NAND_PRINTF("spi_nand_setup: allocate oob buf error! \n");
		kfree(state->buf); //COV,  CID 140028
		kfree(state);
		kfree(spi_nand);
		kfree(info);
		return -ENOMEM;
    }	

    chip = kzalloc(sizeof(struct nand_chip),GFP_KERNEL);
    if (!chip) {
	    _SPI_NAND_PRINTF("spi_nand_setup: allocate chip structure error! \n");
	kfree(state->oob_buf); //COV,  CID 140028
	kfree(state->buf);
    	kfree(state);
    	kfree(spi_nand);
    	kfree(info);
    	return -ENOMEM;
    }

	chip->priv				= info;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
    chip->legacy.read_byte      = spinand_read_byte;
    chip->legacy.read_buf           = spinand_read_buf;
    chip->legacy.write_buf      = spinand_write_buf;
    chip->legacy.waitfunc           = spinand_wait;
    chip->legacy.select_chip        = spinand_select_chip;
    chip->legacy.dev_ready      = spinand_dev_ready;
    chip->legacy.cmdfunc            = spinand_cmdfunc;  
	/* bad block managed by driver, 
	 * so the two function block_markbad and block_bad
	 * always return success.
	 */
	chip->legacy.block_markbad	= spinand_block_markbad;   /* tmp null */
	chip->legacy.block_bad		= spinand_block_bad;		/* tmp null */
#else
	chip->read_byte 		= spinand_read_byte;
	chip->read_buf			= spinand_read_buf;
	chip->write_buf 		= spinand_write_buf;
	chip->waitfunc			= spinand_wait;
	chip->select_chip		= spinand_select_chip;
	chip->dev_ready 		= spinand_dev_ready;
	chip->cmdfunc			= spinand_cmdfunc;
	/* bad block managed by driver, 
	 * so the two function block_markbad and block_bad
	 * always return success.
	 */
	chip->block_markbad			= spinand_block_markbad;   /* tmp null */
	chip->block_bad				= spinand_block_bad;		/* tmp null */
#endif	
	chip->options			|= NAND_CACHEPRG;
	
#ifdef CONFIG_MTD_SPINAND_ONDIEECC
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0)
	chip->ecc.mode			= NAND_ECC_NONE;
#else
chip->ecc.engine_type	= NAND_ECC_ENGINE_TYPE_NONE;
#endif
	if(isSpiNandAndCtrlECC) {
		/* Disable OnDie ECC, Using SPI Controller ECC */
		if (spinand_disable_ecc(spi_nand) < 0)
			pr_info("%s: disable ecc failed!\n", __func__);
	} else {
		/* Enable OnDie ECC */
		if (spinand_enable_ecc(spi_nand) < 0)
			pr_info("%s: enable ecc failed!\n", __func__);
	}
#else
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0)
	chip->ecc.mode	= NAND_ECC_SOFT;
#else
chip->ecc.engine_type	= NAND_ECC_ENGINE_TYPE_SOFT;
#endif
	if (spinand_disable_ecc(spi_nand) < 0)
		pr_info("%s: disable ecc failed!\n", __func__);
#endif

	chip->options	|= NAND_NO_SUBPAGE_WRITE;	/* Chip does not allow subpage writes. */
    chip->options	|= NAND_SKIP_BBTSCAN;   	/*To skips the bbt scan during initialization.  */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
	mtd = nand_to_mtd(chip);
#else
    mtd = kzalloc(sizeof(struct mtd_info), GFP_KERNEL);
#endif
    if (!mtd) {
		_SPI_NAND_PRINTF("spi_nand_setup: allocate mtd error! \n");
		kfree(chip); //COV,  CID 140028
		kfree(state->oob_buf);
		kfree(state->buf);
		kfree(state);
		kfree(spi_nand);
		kfree(info);
		return -ENOMEM;
    }

	spi_nand_mtd = mtd;
    mtd->priv = chip;
    mtd->name = "EN7512-SPI_NAND";
    mtd->owner = THIS_MODULE;
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0) && LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0)
    mtd->orig_flags = MTD_WRITEABLE; /* for app to access mtd device with Write attribute */
    #endif
    mtd->oobsize = MAX_LINUX_USE_OOB_SIZE;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)	
	mtd_set_ooblayout(mtd, &nand_ooblayout_lp_ecnt_ops);
#else
	spinand_oob_64.oobavail = MAX_LINUX_USE_OOB_SIZE;
	memcpy(spinand_oob_64.oobfree, _current_flash_info_t.oob_free_layout->oobfree, sizeof(spinand_oob_64.oobfree));
	chip->ecc.layout = &spinand_oob_64;
#if 0	/* test */
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_0, "sizeof(spinand_oob_64.oobfree):%d\n", sizeof(spinand_oob_64.oobfree));
	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_0, "sizeof(_current_flash_info_t->oob_free_layout.oobfree):%d\n", sizeof(_current_flash_info_t.oob_free_layout->oobfree));
	int entry_idx = 0, oob_cnt = 0;
	for(entry_idx = 0; ((entry_idx < SPI_NAND_FLASH_OOB_FREE_ENTRY_MAX) && (chip->ecc.layout->oobfree[entry_idx].length) && (oob_cnt < chip->ecc.layout->oobavail)); entry_idx++) {
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_0, "chip->ecc.layout.oobfree[%d]->offset:%d\n", entry_idx, chip->ecc.layout->oobfree[entry_idx].offset);
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_0, "chip->ecc.layout.oobfree[%d]->length:%d\n", entry_idx, chip->ecc.layout->oobfree[entry_idx].length);
		oob_cnt += chip->ecc.layout->oobfree[entry_idx].length;
		_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_0, "oob_cnt:%d\n", oob_cnt);
	}
#endif
#endif

	spi_nand_flash_ids[0].name 		= (char *)ptr_dev_info_t->ptr_name;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36)
	spi_nand_flash_ids[0].dev_id   		= ptr_dev_info_t->dev_id; 
#else
	spi_nand_flash_ids[0].id   		= ptr_dev_info_t->dev_id; 
#endif
	spi_nand_flash_ids[0].pagesize  = ptr_dev_info_t->page_size; 
	spi_nand_flash_ids[0].chipsize  = ((ptr_dev_info_t->device_size)>>20); 
	spi_nand_flash_ids[0].erasesize = ptr_dev_info_t->erase_size; 
	spi_nand_flash_ids[0].options   = 0; 
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
     spi_nand_flash_ids[0].id_len = 2;
     spi_nand_flash_ids[0].id[0] = ptr_dev_info_t->mfr_id;
     spi_nand_flash_ids[0].id[1] = ptr_dev_info_t->dev_id;
     spi_nand_flash_ids[0].oobsize = ptr_dev_info_t->oob_size;
     ret = nand_scan_with_ids(chip, 1, spi_nand_flash_ids);
     if (!ret) {
        _SPI_NAND_PRINTF("nand_scan_ident ok\n");
        _SPI_NAND_PRINTF("[spi_nand_setup]: chip size =  0x%llx, erase_shift=0x%x\n", nanddev_target_size(&chip->base), chip->phys_erase_shift);
     }
#else
	ret = nand_scan_ident(mtd, 1, spi_nand_flash_ids);
	if (!ret) {
		_SPI_NAND_PRINTF("nand_scan_ident ok\n");
		ret = nand_scan_tail(mtd);
		_SPI_NAND_PRINTF("[spi_nand_setup]: chip size =  0x%llx, erase_shift=0x%x\n", chip->chipsize, chip->phys_erase_shift);
	}
#endif
	else {
		_SPI_NAND_PRINTF("nand_scan_ident fail\n");
        ret = -ENOMEM;
	    goto setup_fail_exit;
	}           

	_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "%s:%d mtd->oobavail:%d\n", __func__, __LINE__, mtd->oobavail);

#if	defined(TCSUPPORT_NAND_BMT)
	if(init_bmt_bbt(mtd) == -1) {
		ret = -1;
        goto setup_fail_exit;
	}

/*#if defined(TCSUPPORT_CT_PON) || defined(TCSUPPORT_OPENWRT)*/
	mtd->size = nand_flash_avalable_size;
/*#else					
	mtd->size = nand_logic_size;					
#endif			*/				
				
#endif
	ranand_read_byte  = SPI_NAND_Flash_Read_Byte;
	ranand_read_dword = SPI_NAND_Flash_Read_DWord;

	*ptr_rtn_mtd_address = mtd;
	
	return 0;
    
setup_fail_exit:
    #if LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0)	
	kfree(mtd); //COV,  CID 140028
    #endif
	kfree(chip);
	kfree(state->oob_buf);
	kfree(state->buf);
	kfree(state);
	kfree(spi_nand);
	kfree(info);
    return ret;
}

__attribute__((unused)) static void dump_bmt(bmt_struct *bmt)
{
    int i;
    
    _SPI_NAND_PRINTF("BMT v%d.", bmt->version);
	_SPI_NAND_PRINTF("bad count:%d\n", g_bmt->bad_count);
    _SPI_NAND_PRINTF("total %d mapping\n", bmt->mapped_count);
    for (i = 0; i < bmt->mapped_count; i++)
    {
        _SPI_NAND_PRINTF("0x%x -> 0x%x \n", bmt->table[i].bad_index, bmt->table[i].mapped_index);
    }
}

static int spi_nand_proc_read(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	int len;

	if (off > 0) {
		return 0;
	}

	len = sprintf(page, "flash size=[%d], SPI NAND DEBUG LEVEL=%d, _SPI_NAND_WRITE_FAIL_TEST_FLAG=%d, _SPI_NAND_ERASE_FAIL_TEST_FLAG=%d\n"
		, _current_flash_info_t.device_size, _SPI_NAND_DEBUG_LEVEL, _SPI_NAND_WRITE_FAIL_TEST_FLAG, _SPI_NAND_ERASE_FAIL_TEST_FLAG);

	return len;
}

static int spi_nand_proc_write(struct file* file, const char* buffer,
	unsigned long count, void *data)
{
	char buf[16];

	int len = count;

	if (copy_from_user(buf, buffer, len)) {
		return -EFAULT;
	}	

	buf[len] = '\0';

	_SPI_NAND_PRINTF("len = 0x%x, buf[0]=%c, buf[1]=%c, buf[2]=%c\n", len , buf[0], buf[1], buf[2]);

	if (buf[0] == '0') {
		_SPI_NAND_PRINTF("Set SPI NAND DEBUG LEVLE to %d\n", SPI_NAND_FLASH_DEBUG_LEVEL_0);
		_SPI_NAND_DEBUG_LEVEL = SPI_NAND_FLASH_DEBUG_LEVEL_0;
	} else if (buf[0] == '1') {
		_SPI_NAND_PRINTF("Set SPI NAND DEBUG LEVLE to %d\n", SPI_NAND_FLASH_DEBUG_LEVEL_1);
		_SPI_NAND_DEBUG_LEVEL = SPI_NAND_FLASH_DEBUG_LEVEL_1;
	} else if (buf[0] == '2') {
		_SPI_NAND_PRINTF("Set SPI NAND DEBUG LEVLE to %d\n", SPI_NAND_FLASH_DEBUG_LEVEL_2);
		_SPI_NAND_DEBUG_LEVEL = SPI_NAND_FLASH_DEBUG_LEVEL_2;
	} else {
		_SPI_NAND_PRINTF("DEBUG LEVEL only up to %d\n", (SPI_NAND_FLASH_DEBUG_LEVEL_DEF_NO -1 ));
	}

	if(buf[1] == '0')
	{
		_SPI_NAND_WRITE_FAIL_TEST_FLAG = 0;
		_SPI_NAND_PRINTF("Set _SPI_NAND_WRITE_FAIL_TEST_FLAG to %d\n", _SPI_NAND_WRITE_FAIL_TEST_FLAG);
	}
	if(buf[1] == '1')
	{
		_SPI_NAND_WRITE_FAIL_TEST_FLAG = 1;
		_SPI_NAND_PRINTF("Set _SPI_NAND_WRITE_FAIL_TEST_FLAG to %d\n", _SPI_NAND_WRITE_FAIL_TEST_FLAG);
	}	
	if(buf[2] == '0')
	{
		_SPI_NAND_ERASE_FAIL_TEST_FLAG = 0;
		_SPI_NAND_PRINTF("Set _SPI_NAND_ERASE_FAIL_TEST_FLAG to %d\n", _SPI_NAND_ERASE_FAIL_TEST_FLAG);
	}	
	if(buf[2] == '1')
	{
		_SPI_NAND_ERASE_FAIL_TEST_FLAG = 1;
		_SPI_NAND_PRINTF("Set _SPI_NAND_ERASE_FAIL_TEST_FLAG to %d\n", _SPI_NAND_ERASE_FAIL_TEST_FLAG);
	}
	if(buf[3] == '0')
	{
		_SPI_NAND_READ_EXCEEDED_THRESHOLD_TEST_FLAG = 0;
		_SPI_NAND_PRINTF("Set _SPI_NAND_READ_EXCEEDED_THRESHOLD_TEST_FLAG to %d\n", _SPI_NAND_READ_EXCEEDED_THRESHOLD_TEST_FLAG);
	}
	if(buf[3] == '1')
	{
		_SPI_NAND_READ_EXCEEDED_THRESHOLD_TEST_FLAG = 1;
		_SPI_NAND_PRINTF("Set _SPI_NAND_READ_EXCEEDED_THRESHOLD_TEST_FLAG to %d\n", _SPI_NAND_READ_EXCEEDED_THRESHOLD_TEST_FLAG);
	}

	return len;
}

static int write_test(void *arg)
{
	struct _SPI_NAND_FLASH_RW_TEST_T param;
	struct SPI_NAND_FLASH_INFO_T	 *ptr_dev_info_t;
	u32								 ptr_rtn_len;
	u8 buf[64], read_buf[64];
	SPI_NAND_FLASH_RTN_T status;
#if defined(CONFIG_MIPS_MT_SMP) || defined(CONFIG_MIPS_MT_SMTC)
	int cpu = smp_processor_id();
    #if LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0)
	int vpe = cpu_data[cpu].vpe_id;
    #endif
#else
	int cpu = 0;
	int vpe __attribute__((unused))= 0;
#endif

	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;

	memcpy(&param, arg, sizeof(struct _SPI_NAND_FLASH_RW_TEST_T));
    #if LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0)
	_SPI_NAND_PRINTF("write_test: run at vpe:%d, cpu:%d\n", vpe, cpu);
    #else
    _SPI_NAND_PRINTF("write_test: run at cpu:%d\n", cpu);
    #endif
	_SPI_NAND_PRINTF("write_test: times=%d, block_idx=%d\n", param.times, param.block_idx);

	while (!kthread_should_stop() && param.times > 0) {
		if(param.times % 10 == 0)
			_SPI_NAND_PRINTF("write_test:%d\n", param.times);
		msleep(1);
		param.times--;
		get_random_bytes(buf, sizeof(buf));
		SPI_NAND_Flash_Erase(param.block_idx * ptr_dev_info_t->erase_size, sizeof(buf));
    	SPI_NAND_Flash_Write_Nbyte(param.block_idx * ptr_dev_info_t->erase_size, sizeof(buf), &ptr_rtn_len, buf, ptr_dev_info_t->write_mode);
		SPI_NAND_Flash_Read_NByte(param.block_idx * ptr_dev_info_t->erase_size, sizeof(read_buf), &ptr_rtn_len, read_buf, ptr_dev_info_t->read_mode, &status);

		if(memcmp(buf, read_buf, sizeof(buf)) != 0) {
			_SPI_NAND_PRINTF("write fail\n");
			return -1;
		}
	}

	_SPI_NAND_PRINTF("write done\n");

	return 0;
}

static int read_test(void *arg)
{
	struct _SPI_NAND_FLASH_RW_TEST_T param;
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	u32								 ptr_rtn_len;
	u8 buf[64];
	SPI_NAND_FLASH_RTN_T status;
#if defined(CONFIG_MIPS_MT_SMP) || defined(CONFIG_MIPS_MT_SMTC)
	int cpu = smp_processor_id();
    #if LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0)
	int vpe = cpu_data[cpu].vpe_id;
    #endif
#else
	int cpu = 0;
	int vpe __attribute__((unused))= 0;
#endif

	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;

	memcpy(&param, arg, sizeof(struct _SPI_NAND_FLASH_RW_TEST_T));
    #if LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0)
	_SPI_NAND_PRINTF("read_test: run at vpe:%d, cpu:%d\n", vpe, cpu);
    #else
    _SPI_NAND_PRINTF("read_test: run at cpu:%d\n", cpu);
    #endif
	_SPI_NAND_PRINTF("read_test: times=%d, block_idx=%d\n", param.times, param.block_idx);

	memset(buf, 0xaa, sizeof(buf));

	while (!kthread_should_stop() && param.times > 0) {
		if(param.times % 10 == 0)
			_SPI_NAND_PRINTF("read_test:%d\n", param.times);
		msleep(1);
		param.times--;
		SPI_NAND_Flash_Read_NByte(param.block_idx * ptr_dev_info_t->erase_size, sizeof(buf), &ptr_rtn_len, buf, ptr_dev_info_t->read_mode, &status);
	}

	_SPI_NAND_PRINTF("read done\n");

	return 0;
}

int spi_nand_get_bad_block_rate(void)
{
	return bad_block_rate;
}

static int spi_nand_bad_block_rate_proc_read(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	int len;
	u32 rate = spi_nand_get_bad_block_rate();

	len = sprintf(page, "%d", rate);

	return len;
}

static int spi_nand_proc_test_write(struct file* file, const char* buffer,
	unsigned long count, void *data)
{
	char 							buf[64], cmd[32];
	u32 							arg1 = 0xFFFFFFFF, arg2;
	struct task_struct 				*thread;
	u32								idx, i, page_no = 0, erase_cnt = 0;
	u32								bad_block_cnt;
	u32								page_size;
	u32								page_cnt;
	u32								block_size;
    u32								total_block;
	char							page_data[_SPI_NAND_PAGE_SIZE];
	char							oob_data[_SPI_NAND_OOB_SIZE];
	struct SPI_NAND_FLASH_INFO_T	*ptr_dev_info_t;
	u8 feature;
	unsigned long			spinand_spinlock_flags;
	u32 					retlen;

	ptr_dev_info_t	= _SPI_NAND_GET_DEVICE_INFO_PTR;

	block_size = ptr_dev_info_t->erase_size;
	page_size = ptr_dev_info_t->page_size;
	page_cnt = block_size / page_size;
    total_block = ptr_dev_info_t->device_size / block_size;

	if (copy_from_user(buf, buffer, count)) {
		return -EFAULT;
	}	

	buf[count] = '\0';

	sscanf(buf, "%s %x %x", cmd, &arg1, &arg2) ;

	_SPI_NAND_PRINTF("cmd:%s, arg1=%u, arg2=%u\n", cmd, arg1, arg2);

	if (!strcmp(cmd, "scan")) {
		_SPI_NAND_SEMAPHORE_LOCK(); 
		bad_block_cnt = 0;
		for(idx = 0; idx < total_block; idx++) {
			if(en7512_nand_check_block_bad(idx * block_size, BAD_BLOCK_RAW) || en7512_nand_check_block_bad(idx * block_size, BMT_BADBLOCK_GENERATE_LATER)) {
				bad_block_cnt++;
				_SPI_NAND_PRINTF("Block 0x%04x is bad ", idx);
				/* check default bad block */
				for (i = 0; i < g_bbt->badblock_count; i++) {
					if(idx == g_bbt->badblock_table[i]) {
						_SPI_NAND_PRINTF("(default)");
					}
					_SPI_NAND_DEBUG_PRINTF(SPI_NAND_FLASH_DEBUG_LEVEL_1, "	0x%04x\n", g_bbt->badblock_table[i]);
				}
				_SPI_NAND_PRINTF("\n");
			}
		}
		_SPI_NAND_SEMAPHORE_UNLOCK();	
		_SPI_NAND_PRINTF("There are %d bad block.\n", bad_block_cnt);
	} else if (!strcmp(cmd, "mark_bad")) {
		_SPI_NAND_SEMAPHORE_LOCK(); 
		if(en7512_nand_check_block_bad(arg1 * block_size, BAD_BLOCK_RAW) || en7512_nand_check_block_bad(arg1 * block_size, BMT_BADBLOCK_GENERATE_LATER)) {
			_SPI_NAND_PRINTF("Block 0x%04x has already bad.\n", arg1);
		} else {
			_SPI_NAND_PRINTF("mark block 0x%x to bad.\n", arg1);
			memset(page_data, 0xFF, _SPI_NAND_PAGE_SIZE);
			memset(oob_data, 0xFF, _SPI_NAND_OOB_SIZE);

			/* migrate arg1 block to good block, and mark arg1 block to bad. */
			if(en7512_nand_exec_read_page(((page_cnt - 1)+ (arg1 * page_cnt)), page_data, oob_data)) {
				_SPI_NAND_PRINTF("Read page 0x%x fail.\nmark block 0x%x to bad fail.\n", ((page_cnt - 1)+ (arg1 * page_cnt)), arg1);
			} else {
				if (update_bmt((((page_cnt - 1) * page_size) + (arg1 * block_size)), UPDATE_WRITE_FAIL, page_data, oob_data)) {
					_SPI_NAND_PRINTF("Mark block 0x%x to bad success.\n", arg1);
				} else {
					_SPI_NAND_PRINTF("Mark block 0x%x to bad fail.\n", arg1);
				}
			}
		}
		_SPI_NAND_SEMAPHORE_UNLOCK();	
	}
#ifdef ERASE_STATISTICS
	else if(!strcmp(cmd, "dbg_erase_update")) {
			memset(erase_statistic_cnt, 0, sizeof(erase_statistic_cnt));
			for(i = 0; i < total_block; i++) {
				page_no = i * page_cnt;
				if(en7512_nand_exec_read_page(page_no, page_data, oob_data)) {
					_SPI_NAND_PRINTF("Read page 0x%x fail.\nBlock 0x%04xInit erase counter fail.\n", page_no, i);
					erase_statistic_cnt[i] = INVALID_ERASE_CNT;
				} else {
					memcpy(&erase_cnt, (oob_data+MAX_ERASE_CNT_START_OFFSET), MAX_ERASE_CNT_SIZE);
					if(erase_cnt == INVALID_ERASE_CNT) {
						erase_statistic_cnt[i] = 0;
					} else {
						erase_statistic_cnt[i] = erase_cnt;
					}
				}
			}
	} else if(!strcmp(cmd, "dbg_erase_statistic")) {
		_SPI_NAND_PRINTF("Erase total count:\n");
		if(total_block > MAX_BLOCK) {
			total_block = MAX_BLOCK;
		}
		if(0xFFFFFFFF == arg1) {
			for(idx = 0; idx < total_block; idx++) {
				if(erase_statistic_cnt[idx] != INVALID_ERASE_CNT) {
					_SPI_NAND_PRINTF("block[0x%04x] erased %08u ", idx, erase_statistic_cnt[idx]);
				} else {
					_SPI_NAND_PRINTF("block[0x%04x] never count erased.\n", idx);
					continue;
				}
				/* check default/runtime bad block */
				if(check_default_bad(idx)) {
					_SPI_NAND_PRINTF("(default bad)\n");
				} else if(check_runtime_bad(idx)) {
					_SPI_NAND_PRINTF("(bad)\n");
				} else {
					_SPI_NAND_PRINTF("\n");
				}
			}
		} else if(arg1 < total_block) {
			if(erase_statistic_cnt[arg1] != INVALID_ERASE_CNT) {
				_SPI_NAND_PRINTF("block[0x%04x] erased %08u ", arg1, erase_statistic_cnt[arg1]);

				/* check default/runtime bad block */
				if(check_default_bad(arg1)) {
					_SPI_NAND_PRINTF("(default bad)\n");
				} else if(check_runtime_bad(arg1)) {
					_SPI_NAND_PRINTF("(bad)\n");
				} else {
					_SPI_NAND_PRINTF("\n");
				}
			} else {
				_SPI_NAND_PRINTF("block[0x%04x] never count erased.\n", arg1);
			}
		} else {
			_SPI_NAND_PRINTF("block number is more than 0x%04x.\n", total_block);
		}
	} else 
#endif
	if (!strcmp(cmd, "rw_test")) {
		rw_test_param.times = arg1;
		rw_test_param.block_idx = arg2;

		thread = kthread_create(write_test, (void *)&rw_test_param, "write_test");
#if defined(CONFIG_MIPS_MT_SMP)
		kthread_bind(thread, 0);
#else
		kthread_bind(thread, 3);
#endif
		wake_up_process(thread);
		thread = kthread_create(read_test, (void *)&rw_test_param, "read_test");
#if defined(CONFIG_MIPS_MT_SMP)
		kthread_bind(thread, 1);
#else
		kthread_bind(thread, 2);
#endif

		wake_up_process(thread);
 	} else if (!strcmp(cmd, "read")) {
 		SPI_NAND_Flash_Clear_Read_Cache_Data();
 		spi_nand_read_page(arg1, SPI_NAND_FLASH_READ_SPEED_MODE_SINGLE); 
	} else if (!strcmp(cmd, "erase")) {
 		_SPI_NAND_PRINTF("erase block:0x%x\n", arg1);
		spi_nand_erase_block(arg1);
 	} else if (!strcmp(cmd, "program")) {
 		_SPI_NAND_PRINTF("program page:0x%x\n", arg1);
		/* program without erase for testing programming ECC */
		get_random_bytes(page_data, page_size);
		nandflash_write(arg1 * page_size, page_size, &retlen, page_data);
 	} else if (!strcmp(cmd, "getFeature")) {
		spi_nand_protocol_get_feature(arg1, &feature);
		_SPI_NAND_PRINTF("get feature:0x%x=0x%x\n", arg1, feature);
 	} else if (!strcmp(cmd, "setFeature")) {
		_SPI_NAND_SEMAPHORE_LOCK();
		spi_nand_protocol_set_feature(arg1, arg2);
		spi_nand_protocol_get_feature(arg1, &feature);
		_SPI_NAND_PRINTF("get feature:0x%x=0x%x\n", arg1, feature);
		_SPI_NAND_SEMAPHORE_UNLOCK();
 	}
	else if (!strcmp(cmd, "dumpBmt")) {
		dump_bmt_info(1, g_bmt);
 	} else if (!strcmp(cmd, "dumpBbt")) {
		dump_bbt_info(1, g_bbt);
 	} else if (!strcmp(cmd, "clrCache")) {
		_current_page_num = UNKNOW_PAGE;
		_SPI_NAND_PRINTF("cache has beed cleared, _current_page_num:0x%x\n", _current_page_num);
#ifdef ERASE_WRITE_CNT_LOG
	} else if(!strcmp(cmd, "dbg_erase_write")) {
		if(arg1 == 0 || arg1 == 1) {
			/* set debug flag */
			erase_write_flag = (ERASE_WRITE_LOG_T)arg1;
			_SPI_NAND_PRINTF("erase_write_flag = %d\n\n", erase_write_flag);
		} else {
			_SPI_NAND_PRINTF("arg1 error, arg1 must 0 or 1\n");
		}
	} else if(!strcmp(cmd, "dbg_erase_write_clean")) {
		for(idx = 0; idx < MAX_BLOCK; idx++) {
			b_erase_cnt[idx] = 0;
			b_write_total_cnt[idx] = 0;
			b_write_cnt_per_erase[idx] = 0;
		}
	} else if(!strcmp(cmd, "dbg_erase_write_dump")) {
		_SPI_NAND_PRINTF("Erase total count:\n");
		for(idx = 0; idx < MAX_BLOCK; idx++) {
			if(b_erase_cnt[idx] != 0) {
				_SPI_NAND_PRINTF("block[0x%04x] = 0x%lx\n", idx, b_erase_cnt[idx]);
			}
		}

		_SPI_NAND_PRINTF("Write total count:\n");
		for(idx = 0; idx < MAX_BLOCK; idx++) {
			if(b_write_total_cnt[idx] != 0) {
				_SPI_NAND_PRINTF("block[0x%04x] = 0x%lx\n", idx, b_write_total_cnt[idx]);
			}
		}

		_SPI_NAND_PRINTF("Write per erase count:\n");
		for(idx = 0; idx < MAX_BLOCK; idx++) {
			if(b_write_cnt_per_erase[idx] != 0) {
				_SPI_NAND_PRINTF("block[0x%04x] = 0x%lx\n", idx, b_write_cnt_per_erase[idx]);
			}
		}

		_SPI_NAND_PRINTF("dbg_erase_write_dump finish\n");
#endif
#ifdef MULTI_WRITE_DBG
	} else if(!strcmp(cmd, "dbg_multi_write")) {
		if(arg1 == 0 || arg1 == 1) {
			/* set debug flag */
			multi_write_dbg_flag = (MULTI_WRITE_LOG_T)arg1;
			_SPI_NAND_PRINTF("multi_write_dbg_flag = %d\n\n", multi_write_dbg_flag);
			if(arg1 == 0) {
				memset(page_info, 0, sizeof(page_info));
			}
		} else {
			_SPI_NAND_PRINTF("arg1 error, arg1 must 0 or 1\n");
		}
	} else if(!strcmp(cmd, "dbg_multi_write_clean")) {
		memset(page_info, 0, sizeof(page_info));
	} else if(!strcmp(cmd, "dbg_multi_write_dump")) {
		if(multi_write_dbg_flag == SPI_NAND_FLASH_MULTI_WRITE_ENABLE) {
			_SPI_NAND_PRINTF("Block 0x%x total count:\n", arg1);
			for(idx = 0; idx < 128; idx++) {
				_SPI_NAND_PRINTF("page illegal program:0x%02x = %d\n", idx, page_info[arg1][idx].illegal_program);
			}
			_SPI_NAND_PRINTF("dbg_multi_write_dump finish\n");
		} else {
			_SPI_NAND_PRINTF("multi_write_dbg_flag is disabled.\n");
		}
#endif
 	} else if(!strcmp(cmd, "int_test")) {
 		_SPI_NAND_PRINTF("spi auto read & manual read at the same time test.\n");
		if(arg1 == 0) {
			/* Switch to manual mode*/
			_SPI_NAND_ENABLE_MANUAL_MODE();
#ifdef TCSUPPORT_CPU_ARMV8
			spi_auto_read(arg2);
#else
			VPint(arg2);
#endif
		} else {
			_SPI_NAND_PRINTF("error arg1.\n");
		}
 	} else {
		_SPI_NAND_PRINTF("input not defined.\n");
	}

	return count;
}


static struct mtd_info *spi_nand_probe_kernel(struct map_info *map)
{
	struct mtd_info *mtd_address = NULL;
	int	rtn_status;

	_SPI_NAND_PRINTF("EN7512 mtd init: spi nand probe enter\n");		

	rtn_status = spi_nand_setup(&mtd_address);

	if(rtn_status == 0) {  /* Probe without error */
		return (mtd_address);
	} else {
		_SPI_NAND_PRINTF("[spi_nand_probe_kernel] probe fail !\n");
		return NULL;
	}
}

static void spi_nand_destroy_kernel(struct mtd_info *mtd)
{
	free_allcate_memory(mtd);	
}

static struct mtd_chip_driver spi_nand_chipdrv = {
	.probe	 = spi_nand_probe_kernel,
	.destroy = spi_nand_destroy_kernel,
	.name	 = "nandflash_probe",
	.module	 = THIS_MODULE
};

static int __init linux_spi_nand_flash_init(void)
{
	struct proc_dir_entry *entry;

	_SPI_NAND_PRINTF("IS_SPIFLASH=0x%x, IS_NANDFLASH=0x%x, (0xBFA10114)=0x%x)\n", (unsigned int)IS_SPIFLASH, (unsigned int)IS_NANDFLASH, get_sfc_strap());
	
				
	SPI_NAND_Flash_Init(0);
	
	_SPI_NAND_PRINTF("spi nand flash\n");
	register_mtd_chip_driver(&spi_nand_chipdrv);

	entry = create_proc_entry(SPI_NAND_PROCNAME, 0666, NULL);
	if (entry == NULL) {
		_SPI_NAND_PRINTF("SPI NAND  unable to create /proc entry\n");
		return -ENOMEM;
	}
	entry->read_proc = spi_nand_proc_read;
	entry->write_proc = spi_nand_proc_write;

	entry = create_proc_entry(SPI_NAND_TEST, 0666, NULL);
	if (entry == NULL) {
		_SPI_NAND_PRINTF("SPI NAND  unable to create /proc entry\n");
		return -ENOMEM;
	}
	entry->write_proc = spi_nand_proc_test_write;

	entry = create_proc_entry(SPI_NAND_BAD_BLOCK_RATE, 0666, NULL);
	if (entry == NULL) {
		_SPI_NAND_PRINTF("SPI NAND  unable to create /proc entry bad block rate\n");
		return -ENOMEM;
	}
	entry->read_proc = spi_nand_bad_block_rate_proc_read;

#if (defined(TCSUPPORT_CPU_EN7516) || defined(TCSUPPORT_CPU_EN7527) || defined(TCSUPPORT_CPU_EN7580)) && defined(TCSUPPORT_AUTOBENCH)	
	if(ecnt_register_hook(&ecnt_spi_nand_op)) {
		_SPI_NAND_PRINTF("ecnt_flash_op register fail\n");
		return -ENODEV ;
	}
#endif
	
	return 0;
}

static void __exit linux_spi_nand_flash_exit(void)
{

#if (defined(TCSUPPORT_CPU_EN7516)||defined(TCSUPPORT_CPU_EN7527)) && defined(TCSUPPORT_AUTOBENCH)
	ecnt_unregister_hook(&ecnt_spi_nand_op);
#endif
	unregister_mtd_chip_driver(&spi_nand_chipdrv);

	remove_proc_entry(SPI_NAND_PROCNAME, NULL);
	remove_proc_entry(SPI_NAND_TEST, NULL);
	remove_proc_entry(SPI_NAND_BAD_BLOCK_RATE, NULL);

#if	((defined(TCSUPPORT_CPU_ARMV8)) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)))
	dma_unmap_single(ecnt_nand->dev, rdma_addr, _SPI_NAND_CACHE_SIZE, DMA_FROM_DEVICE);
	dma_unmap_single(ecnt_nand->dev, wdma_addr, _SPI_NAND_CACHE_SIZE, DMA_TO_DEVICE);
#endif

	if(dma_read_page) {
		kfree(dma_read_page);
	}
	if(dma_write_page) {
		kfree(dma_write_page);
	}

    return;
}
#if !defined(TCSUPPORT_CPU_ARMV8)
module_init(linux_spi_nand_flash_init);
module_exit(linux_spi_nand_flash_exit);
#endif
unsigned long spinand_lock(void)
{
	unsigned long spinand_spinlock_flags = 0;
	_SPI_NAND_SEMAPHORE_LOCK();

	return spinand_spinlock_flags;
}

void spinand_unlock(unsigned long spinand_spinlock_flags)
{
	_SPI_NAND_SEMAPHORE_UNLOCK();
}

EXPORT_SYMBOL(spinand_lock);
EXPORT_SYMBOL(spinand_unlock);

#endif
#if defined(CONFIG_ECNT_UBOOT)
extern bool recover_bmt(u32 bad_offset);
extern bool is_beyond_total_block_count(u32 offset);

int spi_nand_recover_bad_block(u32 bad_offset)
{
	if(is_beyond_total_block_count(bad_offset))
	{
		_SPI_NAND_PRINTF("Recovered block 0x%08x exceeds the device size.\n", bad_offset);
		return 0;
	}

	return recover_bmt(bad_offset);
}

int spi_nand_mark_bad_block(u32 offset)
{
	struct SPI_NAND_FLASH_INFO_T *ptr_dev_info_t = _SPI_NAND_GET_DEVICE_INFO_PTR;
	u32 block_size = ptr_dev_info_t->erase_size;
	u32 page_size = ptr_dev_info_t->page_size;
	u32 page_cnt = block_size / page_size;
	u8							page_data[_SPI_NAND_PAGE_SIZE];
	u8							oob_data[_SPI_NAND_OOB_SIZE];
	u32 							block_index = 0xFFFFFFFF;

	block_index = offset / block_size;

	if(is_beyond_total_block_count(offset))
	{
		_SPI_NAND_PRINTF("Block 0x%08x exceeds the device size.\n", offset);
		return 1;
	}

	if(en7512_nand_check_block_bad(block_index * block_size, BAD_BLOCK_RAW) || en7512_nand_check_block_bad(block_index * block_size, BMT_BADBLOCK_GENERATE_LATER)) {
		_SPI_NAND_PRINTF("Block 0x%08x has already bad.\n", offset);
	} else {
		memset(page_data, 0xFF, _SPI_NAND_PAGE_SIZE);
		memset(oob_data, 0xFF, _SPI_NAND_OOB_SIZE);

		/* migrate block_index block to good block, and mark block_index block to bad. */
		if(en7512_nand_exec_read_page(((page_cnt - 1)+ (block_index * page_cnt)), page_data, oob_data)) {
			_SPI_NAND_PRINTF("Read page 0x%x fail.\nmark block 0x%x to bad fail.\n", ((page_cnt - 1)+ (block_index * page_cnt)), block_index);
			return 1;
		} else {
			if (update_bmt((((page_cnt - 1) * page_size) + (block_index * block_size)), UPDATE_WRITE_FAIL, page_data, oob_data)) {
			} else {
				_SPI_NAND_PRINTF("block 0x%08x failed marked as bad.\n", offset);
				return 1;
			}
		}
	}

	return 0;
}

int spi_nand_bad_block_info(void)
{
	u32 idx, bad_block_cnt;
	struct SPI_NAND_FLASH_INFO_T *ptr_dev_info_t = _SPI_NAND_GET_DEVICE_INFO_PTR;
	u32 block_size = ptr_dev_info_t->erase_size;
	u32 total_block = ptr_dev_info_t->device_size / block_size;

	bad_block_cnt = 0;

	for(idx = 0; idx < total_block; idx++) {
		if(en7512_nand_check_block_bad(idx * block_size, BAD_BLOCK_RAW) || en7512_nand_check_block_bad(idx * block_size, BMT_BADBLOCK_GENERATE_LATER)) {
			bad_block_cnt++;
			_SPI_NAND_PRINTF("0x%08x ", idx * block_size);
		}
	}

	_SPI_NAND_PRINTF("\nThere are %d bad blocks.\n", bad_block_cnt);

	return 0;
}
#endif

#ifdef TCSUPPORT_CPU_ARMV8
static const struct of_device_id ecnt_nand_of_ids[] = {
	{ .compatible = "econet,ecnt-nand"},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ecnt_nand_of_ids);

static int ecnt_nand_drv_probe(struct platform_device *pdev)
{
	int ret = 0;

	if (!pdev->dev.of_node) {
		dev_err(&pdev->dev, "No DT found\n");
		return -EINVAL;
	}

	ecnt_nand = devm_kzalloc(&pdev->dev, sizeof(struct ecnt_nand), GFP_KERNEL);
	if (!ecnt_nand) {
		_SPI_NAND_PRINTF("%s devm_kzalloc fail\n", __func__);
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, ecnt_nand);

	/* Get SPI controller address */
	ret = init_spi_ctrl_base(pdev);
	if(ret != 0) {
		dev_err(&pdev->dev, "%s failed to init spi controller\n", __func__);
		return ret;
	}

	/* Init SPI nfi2spi */
	ret = init_spi_nfi2spi_base(pdev);
	if(ret != 0) {
		dev_err(&pdev->dev, "%s failed to init spi nfi\n", __func__);
		return ret;
	}

	/* Init SPI ECC */
	ret = init_spi_ecc_base(pdev);
	if(ret != 0) {
		dev_err(&pdev->dev, "%s failed to init spi ecc\n", __func__);
		return ret;
	}

	ret = dma_set_mask(&pdev->dev, DMA_BIT_MASK(32));
	if (ret) {
		dev_err(&pdev->dev, "failed to set dma mask\n");
	}

	ecnt_nand->dev = &pdev->dev;
    #if defined(TCSUPPORT_CPU_ARMV8)
    if(IS_NANDFLASH) { 	/* For boot from SPI NOR, then mount NAND as a MTD partition */
        linux_spi_nand_flash_init();
    }
    #endif
	return ret;
}

static int ecnt_nand_drv_remove(struct platform_device *pdev)
{
    #if defined(TCSUPPORT_CPU_ARMV8)
    if(!(IS_SPIFLASH)) {
		linux_spi_nand_flash_exit();
	}
    #endif
	return 0;
}

static struct platform_driver ecnt_nand_driver = {
	.probe = ecnt_nand_drv_probe,
	.remove = ecnt_nand_drv_remove,
	.driver = {
		.name = "ecnt-nand",
		//.pm = MTK_NOR_DEV_PM_OPS,
		.of_match_table = ecnt_nand_of_ids,
	},
};
module_platform_driver(ecnt_nand_driver);
MODULE_DESCRIPTION("EcoNet NAND Flash Driver");
#endif

#if defined(TCSUPPORT_PARALLEL_NAND)
SPI_CONTROLLER_RTN_T spi_nand_enable_manual_mode(void)
{
	if (_parallel_nand_mode)
		return 0;
	else
		return SPI_CONTROLLER_Enable_Manual_Mode();
}

#endif

int mtk_plat_nand_setup(size_t *page_size, size_t *block_size, uint32_t *size)
{
	*page_size = _current_flash_info_t.page_size;
	*block_size = _current_flash_info_t.erase_size;
	*size = _current_flash_info_t.device_size;
	return 0;
}

/* End of [spi_nand_flash.c] package */
