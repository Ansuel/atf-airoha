// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2024 AIROHA Inc
*/

/*======================================================================================
 * MODULE NAME: NAND
 * FILE NAME: nand_flash_otp.h
 * VERSION: 1.00
 * PURPOSE: To Provide NAND OTP Access interface.
 * NOTES:
 *
 * FUNCTIONS  
 *
 * DEPENDENCIES
 *
 * * $History: $
 * MODIFICTION HISTORY:
 * ** This is the first versoin for creating to support the functions of
 *    current module.
 *
 *======================================================================================
 */

#ifndef __ECNT_NAND_FLASH_OTP_H__
#define __ECNT_NAND_FLASH_OTP_H__

/* INCLUDE FILE DECLARATIONS --------------------------------------------------------- */

/* MACRO DECLARATIONS ---------------------------------------------------------------- */

/* TYPE DECLARATIONS ----------------------------------------------------------------- */
typedef struct {
	unsigned int otp_locked		: 1;
	unsigned int otp_enabled	: 1;
	unsigned int resve			: 30;
} NAND_FLASH_OTP_STS_T;

typedef enum {
	NAND_FLASH_OTP_DISABLE = 0,
	NAND_FLASH_OTP_ENABLE,
} NAND_FLASH_OTP_ENABLE_T;

typedef enum {
	NAND_FLASH_OTP_HAS_NO_RSA_PUBKEY = 0,
	NAND_FLASH_OTP_HAS_RSA_PUBKEY,
} NAND_FLASH_OTP_RSA_PUBKEY_T;

/* EXPORTED SUBPROGRAM SPECIFICATION ------------------------------------------------- */
#if defined(TCSUPPORT_SECURE_BOOT_FLASH_OTP)
int nand_otp_read_rsa_pub_key(u8 *data, u32 len);
NAND_FLASH_OTP_RSA_PUBKEY_T nand_otp_is_rsa_pub_key(u32 len);
int nand_otp_write_rsa_pub_key(u8 *data, u32 len);
#endif

#endif /* ifndef __ECNT_NAND_FLASH_OTP_H__ */

