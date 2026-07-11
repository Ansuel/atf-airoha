/*
 * Copyright (c) 2015, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <common/debug.h>
#include <common/runtime_svc.h>
#include <drivers/console.h>
#include <lib/mmio.h>
#include <tools_share/uuid.h>

#include <plat_private.h>
#include <ecnt_plat_common.h>
#include <ecnt_sip_svc.h>
#include <plat_sip_calls.h>
#include <lib/xlat_tables/xlat_tables_v2.h>

/* Mediatek SiP Service UUID */
DEFINE_SVC_UUID2(ecnt_sip_svc_uid,
	0xf6dea3cf, 0x1933, 0x4052, 0xa1, 0x77,
	0x93, 0x18, 0xc3, 0x20, 0x59, 0x20);

#pragma weak ecnt_plat_sip_handler
uintptr_t ecnt_plat_sip_handler(uint32_t smc_fid,
				u_register_t x1,
				u_register_t x2,
				u_register_t x3,
				u_register_t x4,
				void *cookie,
				void *handle,
				u_register_t flags)
{
	ERROR("%s: unhandled SMC (0x%x)\n", __func__, smc_fid);
	SMC_RET1(handle, SMC_UNK);
}

/*
 * This function handles Mediatek defined SiP Calls */
uintptr_t ecnt_sip_handler(uint32_t smc_fid,
			u_register_t x1,
			u_register_t x2,
			u_register_t x3,
			u_register_t x4,
			void *cookie,
			void *handle,
			u_register_t flags)
{
	uint32_t ns = 0;
	uint64_t ret = 0;
	int status = 0;
	uintptr_t mmap_base = 0;
	uintptr_t mmap_size = 0;


	/* if parameter is sent from SMC32. Clean top 32 bits */
	clean_top_32b_of_param(smc_fid, &x1, &x2, &x3, &x4);

	/* Determine which security state this SMC originated from */
	ns = is_caller_non_secure(flags);
	if (!ns)
	{
		/* SiP SMC service secure world's call */
	}
	else
	{
		/* SiP SMC service normal world's call */
		switch (smc_fid) {
			case ECNT_SIP_EFUSE_HANDLE:
			{
				ret = ecnt_efuse_handler((uint32_t)x1, (uint32_t)x2, (uint32_t)x3);
				SMC_RET1(handle, ret);
			}
			break;

			case ECNT_SIP_TEST_HANDLE:
			{
				ret = ecnt_test_handler((uint32_t)x1, (uint32_t)x2, (uint32_t)x3);
				SMC_RET1(handle, ret);
			}
			break;

			case ECNT_SIP_GET_ALG_HANDLE:
			{
				ret = get_hash_mode_by_efuse();
				SMC_RET1(handle, ret);
			}
			break;

#if defined(IMAGE_BL31)
			case ECNT_SIP_VERIFY_PW_HANDLE:
			{
				ret = ecnt_password_verify_handler((uint32_t)x1, (uint32_t)x2, (uint32_t)x3);
				SMC_RET1(handle, ret);
			}
			break;
#endif

			case ECNT_SIP_VERIFY_BL2_HANDLE:
			case ECNT_SIP_VERIFY_BL33_HANDLE:
			{
				ret = ecnt_verify_handler((uint32_t)(smc_fid & 0xF), (uint32_t)x1, (uint32_t)x2, (uint32_t)x3);
				SMC_RET1(handle, ret);
			}
			break;

			/* support IV and tag in the header,
			 naming as format 1*/
			case ECNT_SIP_DCRYPT_GCM_F1_HANDLE:	
			{	
				mmap_base = page_align((uintptr_t) x1, DOWN);
				mmap_size = (page_align((uintptr_t) (x1 + x2), UP) - mmap_base);
				
				status = mmap_add_dynamic_region((unsigned long long) mmap_base,(uintptr_t) mmap_base, mmap_size, (MT_RW_DATA | MT_NS));
				if(status){
					printf(" mmap fail, err=%d \n", status);
				}else{
					ret = decrypt_gcm_data((uint8_t *)x1, (uint32_t)x2);
					mmap_remove_dynamic_region((uintptr_t) mmap_base, mmap_size);
					SMC_RET1(handle, ret);
				}
			}
			break;

			case ECNT_SIP_DCRYPT_DM_KEY_HANDLE:
			{
				ret = ecnt_decrypt_dm_key_handler((uint32_t)x1, (uint32_t)x2, (uint32_t)x3);
				SMC_RET1(handle, ret);
			}
			break;

			case ECNT_SIP_DCRYPT_VERIFY_BL2_HANDLE:
			case ECNT_SIP_DCRYPT_VERIFY_BL33_HANDLE:
			{
				ret = ecnt_decrypt_handler((uint32_t)(smc_fid & 0xF), (uint32_t)x1, (uint32_t)x2, (uint32_t)x3);
				SMC_RET1(handle, ret);
			}
			break;

			case ECNT_SIP_AVS_HANDLE:
			{
                ret = ecnt_avs_handler((uint32_t)x1, (uint32_t)x2, (uint32_t)x3);
				SMC_RET1(handle, ret);
			}
			break;
			
			case ECNT_SIP_SERF_HANDLE:
			{
                ret = ecnt_sref_handler((uint32_t)x1);
				SMC_RET1(handle, ret);
			}
			break;
			
			case ECNT_SIP_PHY_EFUSE_HANDLE:			
			{                
				ret = phy_efuse_handler((uint32_t)x1);				
				SMC_RET1(handle, ret);			
			}
			break;

			case ECNT_SIP_DDR_TX_DLY_HANDLE:			
			{                
				ret = DDR_TX_DLY_handler((uint32_t)x1, (uint32_t)x2);				
				SMC_RET1(handle, ret);			
			}
			break;

			case ECNT_SIP_DDR_RX_DLY_HANDLE:			
			{                
				ret = DDR_RX_DLY_handler((uint32_t)x1, (uint32_t)x2);				
				SMC_RET1(handle, ret);			
			}
			break;

#if ECNT_SIP_KERNEL_BOOT_ENABLE
			case ECNT_SIP_KERNEL_BOOT_AARCH32:
			case ECNT_SIP_KERNEL_BOOT_AARCH64:
			{
				boot_to_kernel(x1, x2, x3, x4);
				SMC_RET0(handle);
			}
			break;
#endif
			default:
			/* Do nothing in default case */
				break;
		}
	}

	return ecnt_plat_sip_handler(smc_fid, x1, x2, x3, x4,
					cookie, handle, flags);

}

/*
 * This function is responsible for handling all SiP calls from the NS world
 */
uintptr_t sip_smc_handler(uint32_t smc_fid,
			 u_register_t x1,
			 u_register_t x2,
			 u_register_t x3,
			 u_register_t x4,
			 void *cookie,
			 void *handle,
			 u_register_t flags)
{
	switch (smc_fid) {
	case SIP_SVC_CALL_COUNT:
		/* Return the number of Mediatek SiP Service Calls. */
		SMC_RET1(handle,
			 ECNT_COMMON_SIP_NUM_CALLS + ECNT_PLAT_SIP_NUM_CALLS);

	case SIP_SVC_UID:
		/* Return UID to the caller */
		SMC_UUID_RET(handle, ecnt_sip_svc_uid);

	case SIP_SVC_VERSION:
		/* Return the version of current implementation */
		SMC_RET2(handle, ECNT_SIP_SVC_VERSION_MAJOR,
			ECNT_SIP_SVC_VERSION_MINOR);

	default:
		return ecnt_sip_handler(smc_fid, x1, x2, x3, x4,
			cookie, handle, flags);
	}
}

/* Define a runtime service descriptor for fast SMC calls */
DECLARE_RT_SVC(
	ecnt_sip_svc,
	OEN_SIP_START,
	OEN_SIP_END,
	SMC_TYPE_FAST,
	NULL,
	sip_smc_handler
);
