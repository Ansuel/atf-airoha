/*
 * Copyright (c) 2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <common/desc_image_load.h>
#include <common/debug.h>
#include <drivers/console.h>
#include <lib/mmio.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <plat/common/platform.h>

#include <ecnt_plat_common.h>
#include <ecnt_sip_svc.h>
#include <plat_private.h>
#include <drivers/io/io_storage.h>
#include <ecnt_cpufreq.h>
//#include "./drivers/ddr_cal/an7583/inc/dramc_selfrefresh_api.h"

extern void set_fip_block_spec(size_t offset, size_t length);
extern uint8_t read_iddq(void);

typedef enum
{
    NORM_FREQ = 0,	//Lance for NORM_FREQ use
    LOW_FREQ		//Lance for LOW_FREQ use
}SRF_FREQ_T;
#pragma weak Enter_SelfRefresh_api

unsigned char Enter_SelfRefresh_api(SRF_FREQ_T freq_sel)
{    
	printf("build Enter_SelfRefresh_api is fail\n");
	return 1;
}

#pragma weak phy_config_efuse_load_new
void phy_config_efuse_load_new(uint32_t base_addr)
{    	
	printf("build phy_config_efuse_load_new is fail\n");	
}

#pragma weak TX_DQ_delay_shift
void TX_DQ_delay_shift(unsigned char bit_num, int dly)
{    	
	printf("build TX_DQ_delay_shift is fail\n");	
}

#pragma weak RX_DQ_delay_shift
void RX_DQ_delay_shift(unsigned char bit_num, int dly)
{    	
	printf("build TX_DQ_delay_shift is fail\n");	
}

uintptr_t image_hash_base = 0;

#define SR_TEST					(0)

uint64_t read_scu(uint32_t r2, uint32_t r3);
uint64_t write_scu(uint32_t r2, uint32_t r3);
typedef	struct
{
	const uint32_t r1;
	uint64_t (*func)(uint32_t, uint32_t);
} testFunction_t;

testFunction_t test_func[] = 
{
	{0x55435352, read_scu},
	{0x55435357, write_scu},
	{0, NULL}
};

struct atf_arg_t gteearg;

void clean_top_32b_of_param(uint32_t smc_fid,
				u_register_t *px1,
				u_register_t *px2,
				u_register_t *px3,
				u_register_t *px4)
{
	/* if parameters from SMC32. Clean top 32 bits */
	if (0 == (smc_fid & SMC_AARCH64_BIT)) {
		*px1 = *px1 & SMC32_PARAM_MASK;
		*px2 = *px2 & SMC32_PARAM_MASK;
		*px3 = *px3 & SMC32_PARAM_MASK;
		*px4 = *px4 & SMC32_PARAM_MASK;
	}
}

#if ECNT_SIP_KERNEL_BOOT_ENABLE
static struct kernel_info k_info;

static void save_kernel_info(uint64_t pc,
			uint64_t r0,
			uint64_t r1,
			uint64_t k32_64)
{
	k_info.k32_64 = k32_64;
	k_info.pc = pc;

	if (LINUX_KERNEL_32 ==  k32_64) {
		/* for 32 bits kernel */
		k_info.r0 = 0;
		/* machtype */
		k_info.r1 = r0;
		/* tags */
		k_info.r2 = r1;
	} else {
		/* for 64 bits kernel */
		k_info.r0 = r0;
//		k_info.r1 = r1;
	}
}

uint64_t get_kernel_info_pc(void)
{
	return k_info.pc;
}

uint64_t get_kernel_info_r0(void)
{
	return k_info.r0;
}

uint64_t get_kernel_info_r1(void)
{
	return k_info.r1;
}

uint64_t get_kernel_info_r2(void)
{
	return k_info.r2;
}

void boot_to_kernel(uint64_t x1, uint64_t x2, uint64_t x3, uint64_t x4)
{
	static uint8_t kernel_boot_once_flag;
	/* only support in booting flow */
	if (0 == kernel_boot_once_flag) {
		kernel_boot_once_flag = 1;

		INFO("save kernel info\n");
		save_kernel_info(x1, x2, x3, x4);
		bl31_prepare_kernel_entry(x4);
		INFO("el3_exit\n");
	}
}
#endif

#ifdef TCSUPPORT_UBOOT_64BIT
/* copy from plat/common/arm_common.c */
uint32_t plat_get_spsr_for_bl33_entry(void)
{
	unsigned int mode;
	uint32_t spsr;

	/* Figure out what mode we enter the non-secure world in */
	INFO("Secondary bootloader is AArch64\n");
	mode = (el_implemented(2) != EL_IMPL_NONE) ? MODE_EL2 : MODE_EL1;
	/*
	 * TODO: Consider the possibility of specifying the SPSR in
	 * the FIP ToC and allowing the platform to have a say as
	 * well.
	 */
	spsr = SPSR_64(mode, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);
	return spsr;
}
#else
uint32_t plat_get_spsr_for_bl33_entry(void)
{
	unsigned int mode;
	uint32_t spsr;
	unsigned int ee;
	unsigned long daif;

	INFO("Secondary bootloader is AArch32\n");
	mode = MODE32_svc;
	ee = 0;
	/*
	 * TODO: Choose async. exception bits if HYP mode is not
	 * implemented according to the values of SCR.{AW, FW} bits
	 */
	daif = DAIF_ABT_BIT | DAIF_IRQ_BIT | DAIF_FIQ_BIT;

	spsr = SPSR_MODE32(mode, 0, ee, daif);
	return spsr;
}
#endif

uint64_t read_scu(uint32_t r2, uint32_t r3)
{
	uintptr_t addr = EN7523_SEC_SSR;
	switch (r2)
	{
		case 0x0:
		case 0x4:
		case 0x10:
		case 0x14:
		case 0x18:
			addr += r2;
			break;
		default:
			return ECNT_SIP_E_NOT_SUPPORTED;
	}

	printf("0x%lx: %x\n", addr, mmio_read_32(addr));
	return ECNT_SIP_E_SUCCESS;
}

uint64_t write_scu(uint32_t r2, uint32_t r3)
{
	uintptr_t addr = EN7523_SEC_SSR;
	switch (r2)
	{
		case 0x0:
		case 0x4:
		case 0x10:
		case 0x14:
		case 0x18:
			addr += r2;
			break;
		default:
			return ECNT_SIP_E_NOT_SUPPORTED;
	}

	mmio_write_32(addr, r3);
	return ECNT_SIP_E_SUCCESS;
}



uint64_t ecnt_efuse_handler(uint32_t r1, uint32_t r2, uint32_t r3)
{
	int status = 0;
	size_t size = 0;

	#ifdef TCSUPPORT_CPU_AN7583
	unsigned int board_idx;
	uintptr_t mmap_base = 0;
	uintptr_t mmap_size = 0;
	#endif

	switch (r1)
	{
		case 0x44494B50: /* PKID */
			status = efuse_write_pkgid((uint8_t) r2, (uint8_t) r3);
			break;

		case 0x59454B53: /* SKEY */
			size = (size_t) (r3 + (PAGE_SIZE - (r3 % PAGE_SIZE)));
			status = mmap_add_dynamic_region((unsigned long long) r2,
					(uintptr_t) r2, size, (MT_RO_DATA | MT_NS));

			if (status == 0)
			{
				status = efuse_write_secure_key((uint8_t *) ((uintptr_t) r2));
				mmap_remove_dynamic_region((uintptr_t) r2, size);
			}
			break;

		case 0x54534554: /* TEST */
			status = efuse_test((uint8_t) r2, (uint8_t) r3);
			break;

		
		#ifdef TCSUPPORT_CPU_AN7583
		case 0x67626564: /* debg */
			#if(0)
			status = mmap_add_dynamic_region((unsigned long long) 0x81800000,
					(uintptr_t) 0x81800000, PAGE_SIZE, (MT_RO_DATA | MT_NS));
			
			if (status == 0){
				printf(" r2=0x%x, r3=0x%x  \n", r2, r3 );
				status = efuse_write_dbg(2 ,r2,r3,   (uint8_t *)0x81800000);
				printf(" debg status = 0x%x\n",status );
			}
			#endif
			break;

		case 0x44414552: /* READ */
			mmap_base = page_align((uintptr_t) r2, DOWN);
			mmap_size = (page_align((uintptr_t) (r2 + r3), UP) - mmap_base);
			
			status = mmap_add_dynamic_region((unsigned long long) mmap_base,
					(uintptr_t) mmap_base, mmap_size, (MT_RW_DATA | MT_NS));
			
			if(status){
				printf(" mmap fail, err=%d \n", status);
			}else{
				status = efuse_read_out((uint8_t *) ((uintptr_t) r2));
				mmap_remove_dynamic_region((uintptr_t) mmap_base, mmap_size);
			}	
			
			break;

		case 0x44493142: /* B1ID */
		case 0x44493242: /* B2ID */	
			
			board_idx=(r1&0x1ff)>>8;
			printf(" board idx=%d \n", board_idx);			
			status = efuse_write_board_id((uint8_t) board_idx,(uint8_t) r2, (uint8_t) r3);
			break;

		case 0x56324C42: /* BL2V */
			status = efuse_bl2_version(r2, r3);
			printf(" version = 0x%x\n", r2);
			break;
		
		case 0x594B5544: /* DUKY */
			status = efuse_enable_dual_key();
			break;
			
		case 0x544F424D: /* MBOT */
			status = efuse_enable_multi_boot();
			break;
		
		case 0x53505942: /* BYPS */
			status = efuse_enable_hw_bypass();
			break;

		#endif

		case 0x59454B43: /* CKEY */
			status = efuse_check_secure_key((uint8_t) r2);
			break;




#if 0
		case 0x524B5344: /* DSKR, Device Specific key read */
			status = efuse_dsk_read();
			if (status){
				printf(" [DSKR] efuse_dsk_read fail with status=0x%x \n",status);
			}
			break;


		case 0x574B5344: /* DSKW, Device Specific key Write */
			status = mmap_add_dynamic_region((unsigned long long) r2,
					(uintptr_t) r2, PAGE_SIZE, (MT_RO_DATA | MT_NS));

			if (status == 0){
				status = efuse_dsk_write((uint8_t *) ((uintptr_t) r2));
				mmap_remove_dynamic_region((uintptr_t) r2, size);
			}else{
				printf(" [DSKW] mmap_add_dynamic_region fail with status=0x%x \n",status);
			}
			
			break;
#endif



		default:
			return ECNT_SIP_E_NOT_SUPPORTED;
	}

	return status;
}

uint64_t ecnt_avs_handler(uint32_t r1, uint32_t r2, uint32_t r3)
{
	unsigned int pllindex;
	static unsigned int oriarmpll = 0;
	uint64_t iddq64;
	uint64_t ret;

	iddq64 =  (0xff & (uint64_t) read_iddq());
    #if !defined(CPU_FREQ_OP)	
	if(iddq64 <= r2)
		return iddq64;
    #endif
	switch (r1)
	{
		case AVS_OP_FREQ_DOWN:
			oriarmpll = curr_armpll_clk_get();
			ret = (uint64_t) en7523_armpll_set(cpu_freq_500M);

			break;
		case AVS_OP_FREQ_RECOVER:
			if (oriarmpll == 0) {
				printf("Warning : Can not get the correct oriarmpll !!\n");
				return ECNT_SIP_E_NOT_SUPPORTED;
			}
			pllindex = cpu_freq_enum_get(oriarmpll);
			ret = (uint64_t) en7523_armpll_set(pllindex);

			break;
			
				/* New for Freq Scaling */
	    case AVS_OP_FREQ_DYN_ADJ:
             en7523_armpll_set(r3);
             return ECNT_SIP_E_NOT_SUPPORTED;

				/* Get current clock frequency */
	    case AVS_OP_GET_FREQ:	
             oriarmpll = curr_armpll_clk_get();
             return oriarmpll;				
			
		default:
			printf("Warning : ecnt_avs_handler r1 not support\n");
			return ECNT_SIP_E_NOT_SUPPORTED;
	}
	return (iddq64 | ret);
}

uint64_t ecnt_sref_handler(uint32_t r1)
{
	uint32_t status = 0;
	switch (r1)
	{
		case NORM_FREQ:
			status = Enter_SelfRefresh_api(NORM_FREQ);
			break;
			
		case LOW_FREQ:
			status = Enter_SelfRefresh_api(LOW_FREQ);
			break;

		default:
			printf("Warning : ecnt_sref_handler r1 not support\n");
			return ECNT_SIP_E_NOT_SUPPORTED;
	}
	return status;
}

uint64_t phy_efuse_handler(uint32_t r1)
{
	int err = 0;
	uintptr_t base = 0;
	size_t size = 4096; //Set large enough size 

	size = page_align(size, UP);
	
	base = page_align((uintptr_t) r1, DOWN);
	//printf("base0 = %ld\n", base);

	err = mmap_add_dynamic_region((unsigned long long) base,
				(uintptr_t) base, (size_t) size, (MT_RW_DATA | MT_NS));

	//printf("err = %d\n", err);
	//printf("base1 = %ld\n", base);
	//printf("base2 = %ld\n", (uintptr_t)base);
	
	if(err == 0)
	{
		phy_config_efuse_load_new(r1);
		mmap_remove_dynamic_region((uintptr_t) base, (size_t) size);
	}
	else
	{
		printf("Warning : phy_efuse_handler r1 not support\n");
		return ECNT_SIP_E_NOT_SUPPORTED;
	}
	
	return ECNT_SIP_E_SUCCESS;
}

uint64_t DDR_TX_DLY_handler(uint32_t r1, uint32_t r2)
{
	TX_DQ_delay_shift(r1, r2);

	return ECNT_SIP_E_SUCCESS;
}

uint64_t DDR_RX_DLY_handler(uint32_t r1, uint32_t r2)
{
	RX_DQ_delay_shift(r1, r2);

	return ECNT_SIP_E_SUCCESS;
}

uint64_t ecnt_test_handler(uint32_t r1, uint32_t r2, uint32_t r3)
{
#if SR_TEST
	testFunction_t *ptr = &test_func[0];
	while (ptr->func != NULL)
	{
		if (ptr->r1 == r1)
		{
			ptr->func(r2, r3);
			return ECNT_SIP_E_SUCCESS;
		}
		ptr++;
	}
#endif
	return ECNT_SIP_E_INVALID_PARAM;
}

uint64_t ecnt_verify_handler(uint32_t id, uint32_t r1, uint32_t r2, uint32_t r3)
{
	bl_mem_params_node_t *bl_mem_params = NULL;
	int err = 0;
	uintptr_t base = 0;
	uintptr_t size = 0;
	uintptr_t hash_base = 0;
	uintptr_t hash_size = 0;

	bl_mem_params = get_bl_mem_params_node(id);

	if ((bl_mem_params != NULL) && (r1 != 0) && (r2 != 0))
	{
		base = page_align((uintptr_t) r2, DOWN);
		size = (page_align((uintptr_t) (r2 + r3), UP) - base);
		err = mmap_add_dynamic_region((unsigned long long) base,
				(uintptr_t) base, (size_t) size, (MT_RO_DATA | MT_NS));

		if (err == 0)
		{
			set_fip_block_spec((size_t) r2, (size_t) r3);

			hash_base = page_align((uintptr_t) r1, DOWN);
			hash_size = (page_align((uintptr_t) (r1 + PAGE_SIZE), UP) - hash_base);

			err = mmap_add_dynamic_region((unsigned long long) hash_base,
					(uintptr_t) hash_base, (size_t) hash_size, (MT_RO_DATA | MT_NS));
			if (err == 0)
			{
				image_hash_base = (uintptr_t) r1;

				err = load_auth_image(bl_mem_params->image_id, &(bl_mem_params->image_info));
				mmap_remove_dynamic_region((uintptr_t) hash_base, (size_t) hash_size);
			}

			set_fip_block_spec(0, 0);

			mmap_remove_dynamic_region((uintptr_t) base, (size_t) size);
		}

		if (err != 0)
		{
			ERROR("BL31: Failed to verify image id %d (%i)\n",
			      id, err);
			//plat_error_handler(err);
		}
		else
			return ECNT_SIP_E_SUCCESS;
	}

	return ECNT_SIP_E_INVALID_PARAM;
}

uint64_t ecnt_decrypt_handler(uint32_t id, uint32_t r1, uint32_t r2, uint32_t r3)
{
	bl_mem_params_node_t *bl_mem_params = NULL;
	int err = 0;
	uintptr_t base = 0;
	uintptr_t size = 0;

	uintptr_t img_base = 0;
	uintptr_t img_size = 0;


	bl_mem_params = get_bl_mem_params_node(id);

	if ((bl_mem_params != NULL) && (r1 != 0) && (r2 != 0))
	{
			base = page_align((uintptr_t) r2, DOWN);
			size = (page_align((uintptr_t) (0x2000 + r2 + r1), UP) - base);			
			err = mmap_add_dynamic_region((unsigned long long) base,
					(uintptr_t) base, (size_t) size, (MT_RO_DATA | MT_NS));
			if (err == 0)
			{

				img_base = page_align((uintptr_t) r3, DOWN);
				img_size = (page_align((uintptr_t) (r1 + r3), UP) - img_base);			
				err = mmap_add_dynamic_region((unsigned long long) img_base,
						(uintptr_t) img_base, (size_t) img_size, (MT_RW_DATA | MT_NS));
				if (err == 0)
				{
					bl_mem_params->image_info.image_size = r1;
					bl_mem_params->image_info.image_base = r3;
					bl_mem_params->image_info.image_max_size = 0x10000000;
					set_fip_block_spec((size_t) r2, (size_t) 0x2000);

					err = load_auth_image(bl_mem_params->image_id, &(bl_mem_params->image_info));
				}
				else
				{
					printf("mmap_add_dynamic_region error ! (%d)\n",err);
					mmap_remove_dynamic_region((uintptr_t) base, (size_t) size);
					plat_error_handler(err);
				}
			}
			else
			{
				printf("mmap_add_dynamic_region error ! (%d)\n",err);
				plat_error_handler(err);
			}
			set_fip_block_spec(0,0);
			bl_mem_params->image_info.image_base = TZRAM2_BASE;
			bl_mem_params->image_info.image_size = 0;
			bl_mem_params->image_info.image_max_size = TZRAM2_SIZE;

			mmap_remove_dynamic_region((uintptr_t) img_base, (size_t) img_size);
			mmap_remove_dynamic_region((uintptr_t) base, (size_t) size);
		if (err != 0)
		{
			ERROR("BL31: Failed to verify image id %d (%i)\n",
			      id, err);
			//plat_error_handler(err);
		}
		else
			return ECNT_SIP_E_SUCCESS;
	}

	return ECNT_SIP_E_INVALID_PARAM;
}

#if defined(IMAGE_BL31)
unsigned int iter_time = 0;
unsigned int key_length = 0;
unsigned int hash_algo = 0;

extern int mbedtls_pkcs5_pbkdf2_compare (char *account, char *password, unsigned char *login_auth, unsigned int iter_time, unsigned int key_length, unsigned int hash_algo);

uint64_t ecnt_password_verify(uint32_t r2, uint32_t r3)
{
	uintptr_t base = page_align((uintptr_t) r2, DOWN);
	uintptr_t size = (size_t) (r3 + (PAGE_SIZE - (r3 % PAGE_SIZE)));
	int ret = mmap_add_dynamic_region((unsigned long long) base, (uintptr_t) base, size, (MT_RO_DATA | MT_NS));

	if (ECNT_SIP_E_SUCCESS == ret)
	{
		char *username = ((char *) ((uintptr_t) base));
		char *password = ((char *) ((uintptr_t) base + r3));
		unsigned char *login_info = ((unsigned char *) ((uintptr_t) base + (r3*2)));

		if (mbedtls_pkcs5_pbkdf2_compare (username, password, login_info, iter_time, key_length, hash_algo) != 0)
		{
			ret = ECNT_SIP_E_INVALID_PARAM;
		}

		mmap_remove_dynamic_region((uintptr_t) base, size);
	}

	return ret;
}

uint64_t ecnt_pbkdf2_setting (uint32_t r2, uint32_t r3)
{
	iter_time = r2;
	key_length = r3 & 0xffff;
	hash_algo = ((r3 & 0xffff0000) >> 16);

	return ECNT_SIP_E_SUCCESS;
}

uint64_t ecnt_password_verify_handler(uint32_t r1, uint32_t r2, uint32_t r3)
{
	int status = 0;

	switch (r1)
	{
		case 0x504D4350: /* PCMP */
			if (iter_time && key_length && hash_algo)
			{
				status = ecnt_password_verify(r2, r3);
			}
			else
			{
				status = ECNT_SIP_E_INVALID_PARAM;
			}
			break;

		case 0x464E4353: /* SCNF */
			status = ecnt_pbkdf2_setting(r2, r3);
			break;

		default:
			return ECNT_SIP_E_NOT_SUPPORTED;
	}

	return status;
}

uint64_t ecnt_decrypt_dm_key_handler(uint32_t r1, uint32_t r2, uint32_t r3)
{
	int ret = 0;

	ret = decrypt_dm_key((uint8_t *) ((uintptr_t) r1), r2);

	if(ret != 0)
	{
		printf("Failed: Decrypt dm key failed \n");
	}

	return ret;
}
#endif

