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
 * FILE NAME: spi_controller.c
 * DATE: 2014/12/16
 * VERSION: 1.00
 * PURPOSE: To Provide SPI Controller Access interface.
 * NOTES:
 *
 * AUTHOR : Chuck Kuo         REVIEWED by
 *
 * FUNCTIONS
 *
 *      SPI_CONTROLLER_Enable_Manual_Mode To provide interface for Enable SPI Controller Manual Mode.
 *      SPI_CONTROLLER_Write_One_Byte     To provide interface for write one byte to SPI bus. 
 *      SPI_CONTROLLER_Write_NByte        To provide interface for write N bytes to SPI bus. 
 *      SPI_CONTROLLER_Read_NByte         To provide interface for read N bytes from SPI bus. 
 *      SPI_CONTROLLER_Chip_Select_Low    To provide interface for set chip select low in SPI bus. 
 *      SPI_CONTROLLER_Chip_Select_High   To provide interface for set chip select high in SPI bus. 
 *
 * DEPENDENCIES
 *
 * * $History: $
 * MODIFICTION HISTORY:
 * Version 1.00 - Date 2014/12/16
 * ** This is the first versoin for creating to support the functions of
 *    current module.
 *
 *======================================================================================
 */

/* INCLUDE FILE DECLARATIONS --------------------------------------------------------- */
#include <asm/types.h>
#include <asm/io.h>
#include <asm/types.h>
#include <linux/types.h>
#include <linux/version.h>

#include <common/ecnt_chip_id.h>

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
#include <linux/irqreturn.h>
#include <linux/interrupt.h>
#include <asm/tc3162/tc3162.h>
/* for coverity(CID:199298), fix parse EXPORT_SYMBOL() warning */
#include <linux/module.h>
#define SPI_CONTROLLER_DEBUG
#else
#include <asm/tc3162.h>
#include "flashhal.h"
#endif

#if defined(SPI_CONTROLLER_DEBUG)
#include <linux/stdarg.h>
#endif

#include "spi/spi_controller.h"
#include "newspiflash.h"

#if defined(CONFIG_ECNT_UBOOT) /* U-boot likes MIPS bootram, don't follow ARMV8 path */
#undef TCSUPPORT_CPU_ARMV8
#endif

/* NAMING CONSTANT DECLARATIONS ------------------------------------------------------ */

/* SPI Controller Register Definition */
#ifdef TCSUPPORT_CPU_ARMV8
#define _SPI_CONTROLLER_REGS_BASE					0x00000000
#else
#if defined(CONFIG_ECNT_UBOOT) /* Only for EN7523 */
#define _SPI_CONTROLLER_REGS_BASE					0x1FA10000
#else
#define _SPI_CONTROLLER_REGS_BASE					0xBFA10000
#endif
#endif
#define _SPI_CONTROLLER_REGS_READ_MODE     			(_SPI_CONTROLLER_REGS_BASE + 0x0000)
#define _SPI_CONTROLLER_REGS_READ_IDLE_EN     		(_SPI_CONTROLLER_REGS_BASE + 0x0004)
#define _SPI_CONTROLLER_REGS_SIDLY		     		(_SPI_CONTROLLER_REGS_BASE + 0x0008)
#define _SPI_CONTROLLER_REGS_CSHEXT					(_SPI_CONTROLLER_REGS_BASE + 0x000C)
#define _SPI_CONTROLLER_REGS_CSLEXT					(_SPI_CONTROLLER_REGS_BASE + 0x0010)
#define _SPI_CONTROLLER_REGS_MTX_MODE_TOG			(_SPI_CONTROLLER_REGS_BASE + 0x0014)
#define _SPI_CONTROLLER_REGS_RDCTL_FSM				(_SPI_CONTROLLER_REGS_BASE + 0x0018)
#define _SPI_CONTROLLER_REGS_MACMUX_SEL				(_SPI_CONTROLLER_REGS_BASE + 0x001C)
#define _SPI_CONTROLLER_REGS_MANUAL_EN				(_SPI_CONTROLLER_REGS_BASE + 0x0020)
#define _SPI_CONTROLLER_REGS_MANUAL_OPFIFO_EMPTY	(_SPI_CONTROLLER_REGS_BASE + 0x0024)
#define _SPI_CONTROLLER_REGS_MANUAL_OPFIFO_WDATA	(_SPI_CONTROLLER_REGS_BASE + 0x0028)
#define _SPI_CONTROLLER_REGS_MANUAL_OPFIFO_FULL		(_SPI_CONTROLLER_REGS_BASE + 0x002C)
#define _SPI_CONTROLLER_REGS_MANUAL_OPFIFO_WR		(_SPI_CONTROLLER_REGS_BASE + 0x0030)
#define _SPI_CONTROLLER_REGS_MANUAL_DFIFO_FULL		(_SPI_CONTROLLER_REGS_BASE + 0x0034)
#define _SPI_CONTROLLER_REGS_MANUAL_DFIFO_WDATA		(_SPI_CONTROLLER_REGS_BASE + 0x0038)
#define _SPI_CONTROLLER_REGS_MANUAL_DFIFO_EMPTY		(_SPI_CONTROLLER_REGS_BASE + 0x003C)
#define _SPI_CONTROLLER_REGS_MANUAL_DFIFO_RD		(_SPI_CONTROLLER_REGS_BASE + 0x0040)
#define _SPI_CONTROLLER_REGS_MANUAL_DFIFO_RDATA		(_SPI_CONTROLLER_REGS_BASE + 0x0044)
#define _SPI_CONTROLLER_REGS_DUMMY					(_SPI_CONTROLLER_REGS_BASE + 0x0080)
#define _SPI_CONTROLLER_REGS_PROBE_SEL				(_SPI_CONTROLLER_REGS_BASE + 0x0088)
#define _SPI_CONTROLLER_REGS_INTERRUPT				(_SPI_CONTROLLER_REGS_BASE + 0x0090)
#define _SPI_CONTROLLER_REGS_INTERRUPT_EN			(_SPI_CONTROLLER_REGS_BASE + 0x0094)
#define _SPI_CONTROLLER_REGS_SI_CK_SEL				(_SPI_CONTROLLER_REGS_BASE + 0x009C)
#define _SPI_CONTROLLER_REGS_SW_CFGNANDADDR_VAL		(_SPI_CONTROLLER_REGS_BASE + 0x010C)
#define _SPI_CONTROLLER_REGS_SW_CFGNANDADDR_EN		(_SPI_CONTROLLER_REGS_BASE + 0x0110)
#define _SPI_CONTROLLER_REGS_SFC_STRAP				(_SPI_CONTROLLER_REGS_BASE + 0x0114)
#define _SPI_CONTROLLER_REGS_NFI2SPI_EN				(_SPI_CONTROLLER_REGS_BASE + 0x0130)

#ifndef TCSUPPORT_CPU_ARMV8
#if defined(TCSUPPORT_CPU_EN7523) || defined(TCSUPPORT_CPU_AN7552) /* Only for EN7523 */
#define _SPI_FREQUENCY_ADJUST_REG			(0x1FA201C4)
#define IOMUX_CONTROL1						(0x1FA20214)
#define IOMUX_CONTROL2						(0x1FA20218)
#elif defined(TCSUPPORT_CPU_EN7580)
#define _SPI_FREQUENCY_ADJUST_REG			(0xBFA201B8)
#define IOMUX_CONTROL1						(0xBFA20214)
#elif defined(TCSUPPORT_CPU_EN7516)||defined(TCSUPPORT_CPU_EN7527)
#define _SPI_FREQUENCY_ADJUST_REG			(0xBFA2011C)
#define IOMUX_CONTROL1						(0xBFA2015C)
#else
#define _SPI_FREQUENCY_ADJUST_REG			(0xBFA200CC)
#define IOMUX_CONTROL1						(0xBFA20104)
#endif
#endif

/* Register Value Definition */
#define	_SPI_CONTROLLER_VAL_OP_LEN_MAX				(0x1ff)
#define	_SPI_CONTROLLER_VAL_OP_LEN_ONE				(1)
#define	_SPI_CONTROLLER_VAL_OP_LEN_TWO				(2)
#define	_SPI_CONTROLLER_VAL_OP_LEN_THREE			(3)
#define	_SPI_CONTROLLER_VAL_OP_LEN_FOUR				(4)
#define	_SPI_CONTROLLER_VAL_OP_LEN_FIVE				(5)
#define _SPI_CONTROLLER_VAL_OP_CMD_MASK				(0x1f)
#define _SPI_CONTROLLER_VAL_OP_LEN_MASK				(0x1ff)
#define _SPI_CONTROLLER_VAL_OP_SHIFT				(0x9)
#define _SPI_CONTROLLER_VAL_OP_ENABLE				(0x1)
#define _SPI_CONTROLLER_VAL_DFIFO_MASK				(0xff)
#define _SPI_CONTROLLER_VAL_READ_IDLE_DISABLE		(0x0)
#define _SPI_CONTROLLER_VAL_MANUAL_MTXMODE			(0x9)
#define _SPI_CONTROLLER_VAL_MANUAL_MANUALEN			(0x1)
#define _SPI_CONTROLLER_VAL_DDATA_ENABLE			(0x1)
#define _SPI_CONTROLLER_VAL_AUTO_MTXMODE			(0x0)
#define _SPI_CONTROLLER_VAL_MANUAL_MANUALDISABLE	(0x0)
#define _SPI_CONTROLLER_VAL_NFI2SPI_ENABLE			(1)
#define _SPI_CONTROLLER_VAL_NFI2SPI_DISABLE			(0)

#define _SPI_CONTROLLER_VAL_AUTO_MANUAL_INTR_EN		(0x1)

#define _SPI_CONTROLLER_CHECK_TIMES					(10000)

/* FUNCTION DECLARATIONS ------------------------------------------------------ */
#if !defined(IMAGE_BL2)
extern void * memcpy(void * dest,const void *src,size_t count);
#endif

/* MACRO DECLARATIONS ---------------------------------------------------------------- */
#ifndef VPint
#define VPint										*(volatile u32 *)
#endif



#if !defined(SPI_CONTROLLER_DEBUG)
	#define _SPI_CONTROLLER_PRINTF(args...)
	#define _SPI_CONTROLLER_DEBUG_PRINTF(args...)
	#define _SPI_CONTROLLER_DEBUG_PRINTF_ARRAY(args...)
#else
	#ifdef SPRAM_IMG
	#define _SPI_CONTROLLER_PRINTF(fmt, args...)		prom_puts(fmt)		/* Always print information */
	#define _SPI_CONTROLLER_DEBUG_PRINTF_HEX			prom_print_hex
	#else
		#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
		#define _SPI_CONTROLLER_PRINTF						printk
		#else
		#define _SPI_CONTROLLER_PRINTF						prom_printf			/* Always print information */
		#endif
	#endif
#define _SPI_CONTROLLER_DEBUG_PRINTF				spi_controller_debug_printf	
#define _SPI_CONTROLLER_DEBUG_PRINTF_ARRAY			spi_controller_debug_printf_array
#endif
#define _SPI_CONTROLLER_GET_CONF_PTR				&(_spi_controller_conf_t)
#define _SPI_CONTROLLER_MEMCPY						memcpy



/* TYPE DECLARATIONS ----------------------------------------------------------------- */

/* STATIC VARIABLE DECLARATIONS ------------------------------------------------------ */
u8		_SPI_CONTROLLER_DEBUG_FLAG= 0;	/* For control printf debug message or not */
SPI_CONTROLLER_CONF_T	_spi_controller_conf_t;
#ifdef TCSUPPORT_CPU_ARMV8
struct ecnt_spi_ctrl {
	struct device *dev;
	void __iomem *base;	/* spi controller base address */
	void __iomem *autoread_base;	/* spi controller autoread base address */
	int irq;
};

struct ecnt_spi_ctrl *ecnt_spi_ctrl = NULL;
static int isInit = 0;

#define SPI_CTRL_WRL(reg, data)	(writel(data, ecnt_spi_ctrl->base + reg))
#define SPI_CTRL_RDL(reg)		(readl(ecnt_spi_ctrl->base + reg))

#else
#define	WriteReg(reg, data)							(VPint(reg) = data)
#define	ReadReg(reg)								(VPint(reg))
#define	bReadReg(reg, mask)							(VPint(reg) & mask)

#define SPI_CTRL_WRL(reg, data)	(WriteReg(reg, data))
#define SPI_CTRL_RDL(reg)		(ReadReg(reg))
#endif

#if 0
extern u32 GET_SPI_CLK(void);
extern void SET_SPI_CLK(u32 val);
extern u32 GET_SPI_QUAD_SHARED_PIN_RG(void);
extern void SET_SPI_QUAD_SHARED_PIN_RG(u32 val);
#endif
extern u32 GET_NP_SCU_EMMC(void);
/* LOCAL SUBPROGRAM BODIES------------------------------------------------------------ */
#if defined(SPI_CONTROLLER_DEBUG)
#ifdef SPRAM_IMG
static void spi_nand_flash_debug_printf_array(char *buf, u32 len )
{
	u32	idx_for_debug;

	if( _SPI_CONTROLLER_DEBUG_FLAG = 1 )
	{
		for(idx_for_debug=0; idx_for_debug< len; idx_for_debug++)
		{
			if( ((idx_for_debug) % 8 == 0) )
			{
				_SPI_CONTROLLER_PRINTF("  ");
			}
			
			if( ((idx_for_debug) % 16 == 0) )
			{
				_SPI_CONTROLLER_PRINTF("\n");
				_SPI_CONTROLLER_DEBUG_PRINTF_HEX(idx_for_debug, 8);
				_SPI_CONTROLLER_PRINTF(": ");
			}
			_SPI_CONTROLLER_DEBUG_PRINTF_HEX((unsigned char)(buf[idx_for_debug]), 2);
			_SPI_CONTROLLER_PRINTF(" ");
		}			
		_SPI_CONTROLLER_PRINTF("\n");		
	}
}
#else
void spi_controller_debug_printf_array (u8 *buf, u32 len)
{
	u32 idx_for_debug;
	
	if( _SPI_CONTROLLER_DEBUG_FLAG == 1 )
	{
		for(idx_for_debug=0; idx_for_debug< len; idx_for_debug++)
		{				
			if( ((idx_for_debug) %8 == 0) )
			{
				_SPI_CONTROLLER_PRINTF("\n%04x: ", (idx_for_debug));
			}
			_SPI_CONTROLLER_PRINTF("%02x ", (unsigned char)(buf[idx_for_debug]));
		}			
		_SPI_CONTROLLER_PRINTF("\n"); 	
	}	
}
#endif

static void spi_controller_debug_printf( char *fmt, ... )
{
	if( _SPI_CONTROLLER_DEBUG_FLAG == 1 )
	{
#ifdef SPRAM_IMG
		_SPI_CONTROLLER_PRINTF(fmt);
#else
		unsigned char 		str_buf[100];	
		va_list 			argptr;
		int 				cnt;		
	
		va_start(argptr, fmt);
		cnt = vsprintf(str_buf, fmt, argptr);
		va_end(argptr);
				
		_SPI_CONTROLLER_PRINTF("%s", str_buf);	
#endif
	}
}
#endif

/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_CONTROLLER_RTN_T spi_controller_set_opfifo( u8  op_cmd,
 *                                                           u32  op_len )
 * PURPOSE : To setup SPI Controller opfifo.
 * AUTHOR  : Chuck Kuo
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : op_cmd - The op_cmd variable of this function.
 *           op_len - The op_len variable of this function.
 *   OUTPUT: None
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/16 by Chuck Kuo - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
static SPI_CONTROLLER_RTN_T spi_controller_set_opfifo(u8 op_cmd, u32 op_len)
{
#if !defined(IMAGE_BL2)
	u32						check_idx;
#endif
	SPI_CONTROLLER_RTN_T	rtn_status = SPI_CONTROLLER_RTN_NO_ERROR; 
		
	_SPI_CONTROLLER_DEBUG_PRINTF("spi_controller_set_opfifo: set op_cmd =0x%x, op_len=0x%x\n", op_cmd, op_len);	
	 
   	/* 1. Write op_cmd to register OPFIFO_WDATA */
    SPI_CTRL_WRL( _SPI_CONTROLLER_REGS_MANUAL_OPFIFO_WDATA, ((((op_cmd) & _SPI_CONTROLLER_VAL_OP_CMD_MASK) << _SPI_CONTROLLER_VAL_OP_SHIFT) | ((op_len) & _SPI_CONTROLLER_VAL_OP_LEN_MASK)));

    /* 2. Wait until opfifo is not full */
    while(SPI_CTRL_RDL( _SPI_CONTROLLER_REGS_MANUAL_OPFIFO_FULL ));      	      
     
  	/* 3. Enable write from register OPFIFO_WDATA to opfifo */  	 
    SPI_CTRL_WRL( _SPI_CONTROLLER_REGS_MANUAL_OPFIFO_WR, _SPI_CONTROLLER_VAL_OP_ENABLE);
      
	/* 4. Wait until opfifo is empty */
    while(!SPI_CTRL_RDL( _SPI_CONTROLLER_REGS_MANUAL_OPFIFO_EMPTY ));	      
    
    return(rtn_status);    
}

/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_CONTROLLER_RTN_T spi_controller_read_data_fifo( u8      *ptr_rtn_data,
 *                                                               u32     data_len  )
 * PURPOSE : To read data from SPI Controller data pfifo.
 * AUTHOR  : Chuck Kuo
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : data_len  - The data_len variable of this function.
 *   OUTPUT: ptr_rtn_data  - The ptr_rtn_data variable of this function.
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/16 by Chuck Kuo - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
static SPI_CONTROLLER_RTN_T spi_controller_read_data_fifo( u8 *ptr_rtn_data, u32 data_len)
{
	u32						idx;
	SPI_CONTROLLER_RTN_T	rtn_status = SPI_CONTROLLER_RTN_NO_ERROR; 	

	for( idx =0 ; idx<data_len ; idx ++)
	{	
		 /* 1. wait until dfifo is not empty */
		 while(SPI_CTRL_RDL( _SPI_CONTROLLER_REGS_MANUAL_DFIFO_EMPTY ));
	 
		 /* 2. read from dfifo to register DFIFO_RDATA */       	 

		 *(ptr_rtn_data+idx) = (SPI_CTRL_RDL( _SPI_CONTROLLER_REGS_MANUAL_DFIFO_RDATA )) &_SPI_CONTROLLER_VAL_DFIFO_MASK;

#if 0		  
		 _SPI_CONTROLLER_DEBUG_PRINTF(" spi_controller_read_data_fifo : read_data = 0x%x\n", *(ptr_rtn_data+idx));
#endif
		 /* 3. enable register DFIFO_RD to read next byte */
		 SPI_CTRL_WRL( _SPI_CONTROLLER_REGS_MANUAL_DFIFO_RD, _SPI_CONTROLLER_VAL_DDATA_ENABLE);
	}
	
	return(rtn_status);   
}

/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_CONTROLLER_RTN_T spi_controller_write_data_fifo( u8     *ptr_data,
 *                                                                u32    data_len )
 * PURPOSE : To write data from SPI Controller data pfifo.
 * AUTHOR  : Chuck Kuo
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : ptr_data     - The data variable of this function.
 *           data_len - The data_len variable of this function.
 *   OUTPUT: None
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/16 by Chuck Kuo - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
static SPI_CONTROLLER_RTN_T spi_controller_write_data_fifo(u8 *ptr_data, u32 data_len)
{	
	u32						idx;
	SPI_CONTROLLER_RTN_T	rtn_status = SPI_CONTROLLER_RTN_NO_ERROR; 
	
	_SPI_CONTROLLER_DEBUG_PRINTF("spi_controller_write_data_fifo : len=0x%x, data: 0x%x\n", data_len, *ptr_data);
	_SPI_CONTROLLER_DEBUG_PRINTF_ARRAY(ptr_data, data_len);
	
	for( idx =0 ; idx<data_len ; idx++)
	{
		 /* 1. Wait until dfifo is not full */	
		 while(SPI_CTRL_RDL( _SPI_CONTROLLER_REGS_MANUAL_DFIFO_FULL )); 
		  
		 /* 2. Write data  to register DFIFO_WDATA */
		 SPI_CTRL_WRL( _SPI_CONTROLLER_REGS_MANUAL_DFIFO_WDATA, ((*(ptr_data+idx)) & _SPI_CONTROLLER_VAL_DFIFO_MASK));
		  

		 _SPI_CONTROLLER_DEBUG_PRINTF(" spi_controller_write_data_fifo: write data =0x%x\n", ((*(ptr_data+idx)) & _SPI_CONTROLLER_VAL_DFIFO_MASK));

		  
		 /* 3. Wait until dfifo is not full */						
		 while(SPI_CTRL_RDL( _SPI_CONTROLLER_REGS_MANUAL_DFIFO_FULL ));           

    }
    
    return(rtn_status);
}

/* EXPORTED SUBPROGRAM BODIES -------------------------------------------------------- */
SPI_CONTROLLER_RTN_T SPI_CONTROLLER_Set_Configure( SPI_CONTROLLER_CONF_T *ptr_spi_controller_conf_t )
{
	SPI_CONTROLLER_CONF_T	*ptr_spi_conf_t;

	ptr_spi_conf_t = _SPI_CONTROLLER_GET_CONF_PTR;

	/* Store new setting */
	_SPI_CONTROLLER_MEMCPY(ptr_spi_conf_t, ptr_spi_controller_conf_t, sizeof(SPI_CONTROLLER_CONF_T) );	

	/* Setting Mode */	
	if( (ptr_spi_conf_t->mode) == SPI_CONTROLLER_MODE_AUTO )		
	{
		_SPI_CONTROLLER_DEBUG_PRINTF("SPI_CONTROLLER_Set_Configure: AUTO Mode\n");
		
		/* Switch out DMA circuit  */
		if(GET_HIR() >= EN7526C_HIR) {
			SPI_CTRL_WRL( _SPI_CONTROLLER_REGS_NFI2SPI_EN, _SPI_CONTROLLER_VAL_NFI2SPI_DISABLE);
		}
		
		/* manaul mode -> auto mode */
		/*Set 0  to SF_MTX_MODE_TOG */		
		SPI_CTRL_WRL( _SPI_CONTROLLER_REGS_MTX_MODE_TOG, _SPI_CONTROLLER_VAL_AUTO_MTXMODE);
		
		/*Enable Auto Mode */
		SPI_CTRL_WRL( _SPI_CONTROLLER_REGS_MANUAL_EN, _SPI_CONTROLLER_VAL_MANUAL_MANUALDISABLE);					
	}
	if( (ptr_spi_conf_t->mode) == SPI_CONTROLLER_MODE_MANUAL)
	{
		_SPI_CONTROLLER_DEBUG_PRINTF("SPI_CONTROLLER_Set_Configure: Manual Mode\n");

		/* Switch out DMA circuit  */
		if(GET_HIR() >= EN7526C_HIR) {
			SPI_CTRL_WRL( _SPI_CONTROLLER_REGS_NFI2SPI_EN, _SPI_CONTROLLER_VAL_NFI2SPI_DISABLE);
		}
		
		/* disable read_idle_enable */
		SPI_CTRL_WRL( _SPI_CONTROLLER_REGS_READ_IDLE_EN , _SPI_CONTROLLER_VAL_READ_IDLE_DISABLE);
		
		/*wait until auto read status is IDLE */
		while(SPI_CTRL_RDL( _SPI_CONTROLLER_REGS_RDCTL_FSM ));
		
		/*auto mode -> manaul mode */
		/*Set 9  to SF_MTX_MODE_TOG */
		SPI_CTRL_WRL( _SPI_CONTROLLER_REGS_MTX_MODE_TOG, _SPI_CONTROLLER_VAL_MANUAL_MTXMODE);
		
		/*Enable Manual Mode */
		SPI_CTRL_WRL( _SPI_CONTROLLER_REGS_MANUAL_EN, _SPI_CONTROLLER_VAL_MANUAL_MANUALEN);
	}
	if( (ptr_spi_conf_t->mode) == SPI_CONTROLLER_MODE_DMA)
	{
		_SPI_CONTROLLER_DEBUG_PRINTF("SPI_CONTROLLER_Set_Configure: DMA Mode\n");
	
		/* Switch into DMA circuit  */
		if(GET_HIR() >= EN7526C_HIR) {
			SPI_CTRL_WRL( _SPI_CONTROLLER_REGS_NFI2SPI_EN, _SPI_CONTROLLER_VAL_NFI2SPI_ENABLE);
		}
		
		/* manaul mode -> auto mode */
		/*Set 0  to SF_MTX_MODE_TOG */
		SPI_CTRL_WRL( _SPI_CONTROLLER_REGS_MTX_MODE_TOG, _SPI_CONTROLLER_VAL_AUTO_MTXMODE);
		
		/*Enable Auto Mode */
		SPI_CTRL_WRL( _SPI_CONTROLLER_REGS_MANUAL_EN, _SPI_CONTROLLER_VAL_MANUAL_MANUALDISABLE);

	}

	/* Set dummy byte number */
	SPI_CTRL_WRL(_SPI_CONTROLLER_REGS_DUMMY, (ptr_spi_conf_t->dummy_byte_num) );	

	return (SPI_CONTROLLER_RTN_NO_ERROR);	
}


SPI_CONTROLLER_RTN_T SPI_CONTROLLER_Get_Configure( SPI_CONTROLLER_CONF_T *ptr_rtn_spi_controller_conf_t )
{

	SPI_CONTROLLER_CONF_T	*ptr_spi_controller_conf_info_t;

	ptr_spi_controller_conf_info_t = _SPI_CONTROLLER_GET_CONF_PTR;
	_SPI_CONTROLLER_MEMCPY( ptr_rtn_spi_controller_conf_t, ptr_spi_controller_conf_info_t, sizeof(SPI_CONTROLLER_CONF_T) );

	return (SPI_CONTROLLER_RTN_NO_ERROR);
}

/*------------------------------------------------------------------------------------
 * FUNCTION: SPI_CONTROLLER_RTN_T SPI_CONTROLLER_Enable_Manual_Mode( void )
 * PURPOSE : To provide interface for enable SPI Controller Manual Mode Enable.
 * AUTHOR  : Chuck Kuo
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
 * Date 2014/12/16 by Chuck Kuo - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
SPI_CONTROLLER_RTN_T SPI_CONTROLLER_Enable_Manual_Mode( void )
{	
	SPI_CONTROLLER_RTN_T	rtn_status = SPI_CONTROLLER_RTN_NO_ERROR; 
	SPI_CONTROLLER_CONF_T	spi_conf_t;

	/* Switch to manual mode*/
	spi_conf_t.dummy_byte_num = 1 ;
	spi_conf_t.mode = SPI_CONTROLLER_MODE_MANUAL;
	SPI_CONTROLLER_Set_Configure(&spi_conf_t);
	
	return (rtn_status);
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
SPI_CONTROLLER_RTN_T SPI_CONTROLLER_Enable_Auto_Mode( void )
{	
	SPI_CONTROLLER_RTN_T	rtn_status = SPI_CONTROLLER_RTN_NO_ERROR; 
	SPI_CONTROLLER_CONF_T	spi_conf_t;

	/* Switch to manual mode*/
	spi_conf_t.dummy_byte_num = 1 ;
	spi_conf_t.mode = SPI_CONTROLLER_MODE_AUTO;
	SPI_CONTROLLER_Set_Configure(&spi_conf_t);
	
	return (rtn_status);
}
EXPORT_SYMBOL(SPI_CONTROLLER_Enable_Auto_Mode);
#endif

/*------------------------------------------------------------------------------------
 * FUNCTION: SPI_CONTROLLER_RTN_T SPI_CONTROLLER_Write_One_Byte_With_Cmd( u8  data )
 * PURPOSE : To provide interface for write one byte to SPI bus.
 * AUTHOR  : Chuck Kuo
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : data - The data variable of this function.
 *   OUTPUT: None
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/16 - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
SPI_CONTROLLER_RTN_T SPI_CONTROLLER_Write_One_Byte_With_Cmd(u8 op_cmd,	u8 data )
{
	SPI_CONTROLLER_RTN_T	rtn_status = SPI_CONTROLLER_RTN_NO_ERROR; 
	
	_SPI_CONTROLLER_DEBUG_PRINTF("SPI_CONTROLLER_Write_One_Byte : data=0x%x\n", data);
	
	/* 1. Set opcode to SPI Controller */
	spi_controller_set_opfifo( op_cmd, _SPI_CONTROLLER_VAL_OP_LEN_ONE);
	
	/* 2. Write data to SPI Controller */
	spi_controller_write_data_fifo( &data, _SPI_CONTROLLER_VAL_OP_LEN_ONE);
	
	return (rtn_status);	
}


SPI_CONTROLLER_RTN_T SPI_CONTROLLER_Write_One_Byte( u8 data )
{
	return SPI_CONTROLLER_Write_One_Byte_With_Cmd(_SPI_CONTROLLER_VAL_OP_OUTS, data);	
}

#if !defined(LZMA_IMG) || defined(TCSUPPORT_BB_256KB) 
/*------------------------------------------------------------------------------------
 * FUNCTION: SPI_CONTROLLER_RTN_T SPI_CONTROLLER_WRITE_NBYTES( u8                        *ptr_data,
 *                                                             u32                       len,
 *                                                             SPI_CONTROLLER_SPEED_T    speed )
 * PURPOSE : To provide interface for write N bytes to SPI bus.
 * AUTHOR  : Chuck Kuo
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : ptr_data  - The data variable of this function.
 *           len   - The len variable of this function.
 *           speed - The speed variable of this function.
 *   OUTPUT: None
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/16 by Chuck Kuo - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
SPI_CONTROLLER_RTN_T SPI_CONTROLLER_Write_NByte( u8 *ptr_data, u32 len, SPI_CONTROLLER_SPEED_T speed )
{
	u8						op_cmd;
	u32						data_len, remain_len;
	SPI_CONTROLLER_RTN_T	rtn_status = SPI_CONTROLLER_RTN_NO_ERROR; 
	
	_SPI_CONTROLLER_DEBUG_PRINTF("SPI_CONTROLLER_Write_NByte: len=0x%x\n", len );
	_SPI_CONTROLLER_DEBUG_PRINTF_ARRAY(ptr_data, len);
	
	/* 1. Mapping the op code */
	switch( speed )
	{
		case SPI_CONTROLLER_SPEED_SINGLE :
			op_cmd = _SPI_CONTROLLER_VAL_OP_OUTS;
			break;
			
		case SPI_CONTROLLER_SPEED_DUAL :
			op_cmd = _SPI_CONTROLLER_VAL_OP_OUTD;
			break;
			
		case SPI_CONTROLLER_SPEED_QUAD :
			op_cmd = _SPI_CONTROLLER_VAL_OP_OUTQ;			
			break;
		default:
			return SPI_CONTROLLER_RTN_SET_OPFIFO_ERROR;
	}
	
	remain_len = len; 
	while (remain_len > 0)
	{
		if( remain_len > _SPI_CONTROLLER_VAL_OP_LEN_MAX )		/*Controller at most process limitation one time */
		{
			data_len = _SPI_CONTROLLER_VAL_OP_LEN_MAX;
		}
		else
		{
			data_len = remain_len;
		}
		/* 2. Set opcode to SPI Controller */
		spi_controller_set_opfifo( op_cmd, data_len);
	
		/* 3. Write data to SPI Controller */
		spi_controller_write_data_fifo( &ptr_data[len - remain_len], data_len );
		
		remain_len -= data_len;
	}
	
	return (rtn_status);
}
#endif

/*------------------------------------------------------------------------------------
 * FUNCTION: SPI_CONTROLLER_RTN_T SPI_CONTROLLER_READ_NBYTES( u8                         *ptr_rtn_data,
 *                                                            u8                         len,
 *                                                            SPI_CONTROLLER_SPEED_T     speed     )
 * PURPOSE : To provide interface for read N bytes from SPI bus.
 * AUTHOR  : Chuck Kuo
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : len       - The len variable of this function.
 *           speed     - The speed variable of this function.
 *   OUTPUT: ptr_rtn_data  - The ptr_rtn_data variable of this function.
 * RETURN  : SPI_RTN_NO_ERROR - Successful.   Otherwise - Failed.
 * NOTES   :
 * MODIFICTION HISTORY:
 * Date 2014/12/16 by Chuck Kuo - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
SPI_CONTROLLER_RTN_T SPI_CONTROLLER_Read_NByte(u8 *ptr_rtn_data, u32 len, SPI_CONTROLLER_SPEED_T speed)
{
	u8						op_cmd;
	u32						data_len, remain_len;
	SPI_CONTROLLER_RTN_T	rtn_status = SPI_CONTROLLER_RTN_NO_ERROR; 
	
	_SPI_CONTROLLER_DEBUG_PRINTF("SPI_CONTROLLER_Read_NByte : \n");
	
	/* 1. Mapping the op code */
	switch( speed )
	{
		case SPI_CONTROLLER_SPEED_SINGLE :
			op_cmd = _SPI_CONTROLLER_VAL_OP_INS;
			break;
			
		case SPI_CONTROLLER_SPEED_DUAL :
			op_cmd = _SPI_CONTROLLER_VAL_OP_IND;
			break;
			
		case SPI_CONTROLLER_SPEED_QUAD :
			op_cmd = _SPI_CONTROLLER_VAL_OP_INQ;			
			break;
		default:
			return SPI_CONTROLLER_RTN_SET_OPFIFO_ERROR;
	}

	remain_len = len;
	while (remain_len > 0)
	{		
		if( remain_len > _SPI_CONTROLLER_VAL_OP_LEN_MAX )		/*Controller at most process limitation one time */
		{
			data_len = _SPI_CONTROLLER_VAL_OP_LEN_MAX;
		}
		else
		{
			data_len = remain_len;
		}
		/* 2. Set opcode to SPI Controller */
		spi_controller_set_opfifo( op_cmd, data_len);
	
		/* 3. Read data through SPI Controller */
		spi_controller_read_data_fifo( &ptr_rtn_data[len - remain_len], data_len );
		
		remain_len -= data_len;
	}
	
	return (rtn_status);
}

/*------------------------------------------------------------------------------------
 * FUNCTION: SPI_CONTROLLER_RTN_T SPI_CONTROLLER_Chip_Select_Low( void )
 * PURPOSE : To provide interface for set chip select low in SPI bus.
 * AUTHOR  : Chuck Kuo
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
 * Date 2014/12/16 by Chuck Kuo - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
SPI_CONTROLLER_RTN_T SPI_CONTROLLER_Chip_Select_Low(void)
{
	SPI_CONTROLLER_RTN_T	rtn_status = SPI_CONTROLLER_RTN_NO_ERROR; 
	
	spi_controller_set_opfifo( _SPI_CONTROLLER_VAL_OP_CSL, _SPI_CONTROLLER_VAL_OP_LEN_ONE);
	//spi_controller_set_opfifo( _SPI_CONTROLLER_VAL_OP_CSL, _SPI_CONTROLLER_VAL_OP_LEN_ONE);
	
	return (rtn_status);		
}

/*------------------------------------------------------------------------------------
 * FUNCTION: SPI_CONTROLLER_RTN_T SPI_CONTROLLER_Chip_Select_High( void )
 * PURPOSE : To provide interface for set chip select high in SPI bus.
 * AUTHOR  : Chuck Kuo
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
 * Date 2014/12/16 by Chuck Kuo - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
SPI_CONTROLLER_RTN_T SPI_CONTROLLER_Chip_Select_High(void)
{
	SPI_CONTROLLER_RTN_T	rtn_status = SPI_CONTROLLER_RTN_NO_ERROR; 
	
	spi_controller_set_opfifo( _SPI_CONTROLLER_VAL_OP_CSH, _SPI_CONTROLLER_VAL_OP_LEN_ONE);
	//spi_controller_set_opfifo( _SPI_CONTROLLER_VAL_OP_CK, _SPI_CONTROLLER_VAL_OP_LEN_FIVE);
	
	return (rtn_status);
}


void SPI_CONTROLLER_DEBUG_ENABLE( void )
{	
	_SPI_CONTROLLER_DEBUG_FLAG = 1;	
}

/*------------------------------------------------------------------------------------
 * FUNCTION: void SPI_NAND_DEBUG_DISABLE( void )
 * PURPOSE : To disable to printf debug message of SPI NAND driver.
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
 * Date 2015/01/20 by Chuck Kuo - The first revision for this function.
 *
 *------------------------------------------------------------------------------------
 */
void SPI_CONTROLLER_DEBUG_DISABLE( void )
{	
	_SPI_CONTROLLER_DEBUG_FLAG = 0;	
}

u32 get_sfc_strap(void)
{
	return SPI_CTRL_RDL(_SPI_CONTROLLER_REGS_SFC_STRAP);
}

#ifndef TCSUPPORT_CPU_ARMV8
u32 GET_SPI_CLK(void)
{
	return SPI_CTRL_RDL(_SPI_FREQUENCY_ADJUST_REG);
}

void SET_SPI_CLK(u32 val)
{
	SPI_CTRL_WRL(_SPI_FREQUENCY_ADJUST_REG, val);
}

u32 GET_SPI_QUAD_SHARED_PIN_RG(void)
{
#if defined(TCSUPPORT_CPU_EN7580) || defined(TCSUPPORT_CPU_EN7523) || defined(TCSUPPORT_CPU_AN7552)
#if defined(TCSUPPORT_CPU_AN7552)
	return SPI_CTRL_RDL(IOMUX_CONTROL2);
#else
	return SPI_CTRL_RDL(CR_CHIP_SCU_IOMUX_CTRL_2);
#endif
#elif defined(TCSUPPORT_CPU_EN7516)||defined(TCSUPPORT_CPU_EN7527)
	return SPI_CTRL_RDL(CR_CHIP_SCU_IOMUX_CTRL_1);
#else
	return SPI_CTRL_RDL(CR_CHIP_SCU_IOMUX_CTRL_1);
#endif
}

void SET_SPI_QUAD_SHARED_PIN_RG(u32 val)
{
#if defined(TCSUPPORT_CPU_EN7580) || defined(TCSUPPORT_CPU_EN7523) || defined(TCSUPPORT_CPU_AN7552)
#if defined(TCSUPPORT_CPU_AN7552)
	SPI_CTRL_WRL(IOMUX_CONTROL2, val);
#else
	SPI_CTRL_WRL(CR_CHIP_SCU_IOMUX_CTRL_2, val);
#endif
#elif defined(TCSUPPORT_CPU_EN7516)||defined(TCSUPPORT_CPU_EN7527)
	SPI_CTRL_WRL(CR_CHIP_SCU_IOMUX_CTRL_1, val);
#else
	SPI_CTRL_WRL(CR_CHIP_SCU_IOMUX_CTRL_1, val);
#endif
}
#endif

/*------------------------------------------------------------------------------------
 * FUNCTION: spi_set_clock_speed(u32 clock_factor)
 * PURPOSE : To set SPI clock. 
 *                 clock_factor = 0
 *                   EN7512: SPI clock = 500MHz / 40 = 12.5MHz
 *                   EN7522: SPI clock = 400MHz / 40 = 10MHz
 *                 clock_factor > 0
 *                   EN7512: SPI clock = 500MHz / (clock_factor * 2)
 *                   EN7522: SPI clock = 400MHz / (clock_factor * 2)
 * CALLED BY
 *   -
 * CALLS
 *   -
 * PARAMs  :
 *   INPUT : clock_factor - The SPI clock divider.
 * RETURN  : NONE.
 * NOTES   :
 * MODIFICTION HISTORY:
 *
 *------------------------------------------------------------------------------------
 */
void spi_set_clock_speed(u32 clk)
{
	u32 val;
	u32 dividend;
	u32 clock_factor;

	if(!isFPGA) {
		if(GET_HIR() >= EN7526C_HIR) {
			dividend = 400;
		} else {
			dividend = 500;
		}

		clock_factor = (dividend / (clk * 2));
		
		val  = GET_SPI_CLK();
		val &= 0xffff0000;
		SET_SPI_CLK(val);

		val |= (((clock_factor) << 8)|1);
		SET_SPI_CLK(val);

		_SPI_CONTROLLER_PRINTF("Set SPI Clock to %u Mhz\n", (dividend / (clock_factor * 2)));
	} else {
		_SPI_CONTROLLER_PRINTF("FPGA can not Set SPI Clock, FPGA SPI Clock is fixed to 10 Mhz\n");
	}
}

void set_spi_quad_mode_shared_pin(void)
{
	u32 val;
	
	if(!isFPGA) {
		if(isEN7526c) {
			val = GET_SPI_QUAD_SHARED_PIN_RG();
			val |= (1 << 19);
			val &= ~((1 << 18) | (1 << 11) | (1 << 8) | (1 << 7) | (1 << 3));
			SET_SPI_QUAD_SHARED_PIN_RG(val);
		} else if(isEN751627) {
			val = GET_SPI_QUAD_SHARED_PIN_RG();
			val |= (1 << 27);
			val &= ~((1 << 24) | (1 << 17) | (1 << 14));
			SET_SPI_QUAD_SHARED_PIN_RG(val);
		} else if(isEN7580) {
			val = GET_SPI_QUAD_SHARED_PIN_RG();
			val |= (1 << 27);
			SET_SPI_QUAD_SHARED_PIN_RG(val);
		} else if(isAN7552) {
			val = GET_SPI_QUAD_SHARED_PIN_RG();
			val |= (1 << 4);
			SET_SPI_QUAD_SHARED_PIN_RG(val);
		} else {
			_SPI_CONTROLLER_PRINTF("can not config share pin.\n");
			while(1);
		}
	}
}

#ifdef TCSUPPORT_CPU_EN7528
void SPI_Clock_Edges_Set(unsigned int valSet)
{
	unsigned int val;

	val = ReadReg(_SPI_CONTROLLER_REGS_SI_CK_SEL);
	val &= (~0x7);
	val |= (valSet&0x7);
	WriteReg(_SPI_CONTROLLER_REGS_SI_CK_SEL, val);
}
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
static u32 get_spi_ctrl_data(u32 reg)
{
	return SPI_CTRL_RDL(reg);
}

static void set_spi_ctrl_data(u32 reg, u32 val)
{
	SPI_CTRL_WRL(reg, val); 
}
#endif

#ifdef TCSUPPORT_CPU_ARMV8
void ISSUE_OPFIFO_WRITE_CMD(u32 op_cmd, u32 op_len)
{ 
    /* write op_cmd to register OPFIFO_WDATA */
    set_spi_ctrl_data(SF_MANUAL_OPFIFO_WDATA, ((((op_cmd) & OP_CMD_MASK) << OP_SHIFT) | ((op_len) & OP_LEN_MASK))); 
    /* wait until opfifo is not full */
    while(get_spi_ctrl_data(SF_MANUAL_OPFIFO_FULL)) ; 
     /* enable write from register OPFIFO_WDATA to opfifo */
    set_spi_ctrl_data(SF_MANUAL_OPFIFO_WR, OP_ENABLE); 
    /* wait until opfifo is empty */
    while(!get_spi_ctrl_data(SF_MANUAL_OPFIFO_EMPTY));	
}

void ISSUE_DFIFO_WRITE_CMD(u8 data)
{ 
    /* wait until dfifo is not full */	
    while(get_spi_ctrl_data(SF_MANUAL_DFIFO_FULL)) ; 
     /* write data  to register DFIFO_WDATA */
    set_spi_ctrl_data(SF_MANUAL_DFIFO_WDATA, ((data) & DDATA_MASK));
}

void ISSUE_AUTO_MANUAL_CMD(u32 isAuto2Manual)
{
    if (isAuto2Manual) {
         /* set 9 to SF_MTX_MODE_TOG */	
        set_spi_ctrl_data(SF_MTX_MODE_TOG, MANUAL_MTXMODE);
         /* enable Manual mode */
        set_spi_ctrl_data(SF_MANUAL_EN, MANUAL_MANUALEN);
    }
    else { /* Manual2Auto */
        /* set 0 to SF_MTX_MODE_TOG */ 
       set_spi_ctrl_data(SF_MTX_MODE_TOG, AUTO_MTXMODE);
        /* enable auto mode */
       set_spi_ctrl_data(SF_MANUAL_EN, AUTO_MANUALEN); 
    }
}

// change auto mode to 3Byte or 3Byte address
void ISSUE_AUTO_3B_4B_CMD(u32 isAuto3BAddr)
{
    if (isAuto3BAddr) {
         /* enable auto address change */
        set_spi_ctrl_data(SF_CFG3B4B_EN, CFG3B4B_EN);
         /* aauto address is 3Byte */	
        set_spi_ctrl_data(SF_ADDR_3B4B, AUTO_3BMODE);
    }
    else { /* change to auto 4B address */
        /* enable auto address change */
       set_spi_ctrl_data(SF_CFG3B4B_EN, CFG3B4B_EN);
        /* auto address is 4Byte */
       set_spi_ctrl_data(SF_ADDR_3B4B, AUTO_4BMODE);
    }
}

// change Tn Value
void ISSUE_TN_CMD(u8 data)
{	
     /* set data to SF_SI_CK_SEL */	
    set_spi_ctrl_data(SF_SI_CK_SEL, ((data) & SI_SEL_MASK));
}

// Enable or Disable Auto Mode Read Idle Capability
void ISSUE_READ_IDLE_EN_DIS_CMD(u32 enable)
{
    if (enable) {
         /* Enalbe Read IDLE */	
        set_spi_ctrl_data(SF_READ_IDLE_EN, RD_IDLE_EN);	
    }
    else {
         /* Disable Read IDLE */
        set_spi_ctrl_data(SF_READ_IDLE_EN, RD_IDLE_DIS);
    }
}

// change auto mode to Read/ Fast Read/ Fast Read Dual Output/ Fast Read Dual IO
void ISSUE_AUTO_MODE_CMD(enum auto_modes mode)
{
    if (mode==auto_read) {
         /* auto mode is Read */
        set_spi_ctrl_data(SF_READ_MODE, AUTO_READ);	
    }
    else if (mode==auto_fastread) {
        /* auto mode is Fast Read */
        set_spi_ctrl_data(SF_READ_MODE, AUTO_FAST_READ);
    }
    else if (mode==auto_fastread_dualout) {
        /* auto mode is Fast Read Dual Output */
        set_spi_ctrl_data(SF_READ_MODE, AUTO_FAST_READ_DUALOUT);
    }
    else /* mode==auto_fastread_dualio */ {
        /* auto mode is Fast Read Dual IO */
        set_spi_ctrl_data(SF_READ_MODE, AUTO_FAST_READ_DUALIO);
    }
}
#endif

SPI_CONTROLLER_SPI_TYPE_T spi_type(void)
{
	u32 val;

	if(GET_NP_SCU_EMMC()) {
		return SPI_CONTROLLER_EMMC;
	} else {
		val = SPI_CTRL_RDL(_SPI_CONTROLLER_REGS_SFC_STRAP);
		if(val & 0x2) {
			return SPI_CONTROLLER_SPI_NAND;
		} else {
			return SPI_CONTROLLER_SPI_NOR;
		}
	}
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
/* for coverity(CID:441609), fix parse EXPORT_SYMBOL() warning since bootrom does not have kernel version and does not need to export symbol. */
EXPORT_SYMBOL(spi_type);

static irqreturn_t sfc_AutoManualInterrupt_handler(int irq, void *dev_id)
{
	if(get_spi_ctrl_data(_SPI_CONTROLLER_REGS_INTERRUPT) == 1) {
		_SPI_CONTROLLER_PRINTF("\n>>> Auto Mode and Manual Mode access in the same time\n");
		set_spi_ctrl_data(_SPI_CONTROLLER_REGS_INTERRUPT, 1);
	} else {
		_SPI_CONTROLLER_PRINTF("\n>>> Strange!! should not come here\n");
	}

	return IRQ_HANDLED;
}

int register_interrupt(void)
{
	int ret = 0;

#ifdef TCSUPPORT_CPU_ARMV8
	ret = request_irq(ecnt_spi_ctrl->irq, sfc_AutoManualInterrupt_handler, 0,
	    "SFC_AutoManual", ecnt_spi_ctrl->dev);
#else
	ret = request_irq(AUTO_MANUAL_INT, sfc_AutoManualInterrupt_handler, 0,
	    "SFC_AutoManual", NULL);
#endif

	return ret;
}
#endif

#ifdef TCSUPPORT_CPU_ARMV8
u32 spi_auto_read(u32 addr)
{
	printk("=== spi_auto_read, addr:0x%x ===\n", addr);
	return (readl(ecnt_spi_ctrl->autoread_base + addr));
}

static int init_ecnt_spi_ctrl(struct platform_device *pdev)
{
	struct resource *res = NULL;
	int ret = 0;
	int irq;
	
	ecnt_spi_ctrl = devm_kzalloc(&pdev->dev, sizeof(struct ecnt_spi_ctrl), GFP_KERNEL);
	if (!ecnt_spi_ctrl)
		return -ENOMEM;
	platform_set_drvdata(pdev, ecnt_spi_ctrl);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ecnt_spi_ctrl->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(ecnt_spi_ctrl->base))
		return PTR_ERR(ecnt_spi_ctrl->base);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	ecnt_spi_ctrl->autoread_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(ecnt_spi_ctrl->base))
		return PTR_ERR(ecnt_spi_ctrl->base);

	irq = platform_get_irq(pdev, 0);
	if (irq <= 0)
		return irq;
	ecnt_spi_ctrl->irq = irq;

	ecnt_spi_ctrl->dev = &pdev->dev;

#ifdef TCSUPPORT_NEW_SPIFLASH
    register_spi_ctrl_data_cb_funcs(get_spi_ctrl_data, set_spi_ctrl_data);
#endif

	return ret;
}

int init_spi_ctrl_base(struct platform_device *pdev)
{
	struct device_node *np = NULL;
	struct platform_device *pdev_spiCtrl = NULL;
	int ret = 0;
	
	if(isInit == 0) {
		np = of_parse_phandle(pdev->dev.of_node, "spi-controller", 0);
		if (np) {
			pdev_spiCtrl = of_find_device_by_node(np);
			ret = init_ecnt_spi_ctrl(pdev_spiCtrl);
			if(ret == 0) {
				isInit = 1;
			}
			of_node_put(np);
		} else {
			return -1;
		}
	}

	return ret;
}
#endif

/* End of [spi_controller.c] package */
