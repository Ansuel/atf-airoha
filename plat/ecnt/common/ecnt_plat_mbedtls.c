/*
 * Copyright (c) 2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <ecnt_plat_common.h>
#include <string.h>

#include "mbedtls/pkcs5.h"
#include "mbedtls/md.h"

#define ARHT_HASH_ALGO_256	(256)
#define ARHT_HASH_ALGO_512	(512)

#if defined(IMAGE_BL31)
int ecnt_mbedtls_pkcs5_pbkdf2_compare (char *account, char *password, unsigned char *login_auth, unsigned int iter_time, unsigned int key_length, unsigned int hash_algo)
{
	const mbedtls_md_info_t *info_sha;
	int type = MBEDTLS_MD_SHA256, ret = MBEDTLS_ERR_PKCS5_BAD_INPUT_DATA;

	if (ARHT_HASH_ALGO_512 == hash_algo)
	{
		type = MBEDTLS_MD_SHA512;
	}

	info_sha = mbedtls_md_info_from_type( type );
	if( NULL != info_sha)
	{
		mbedtls_md_context_t sha_ctx;

		mbedtls_md_init( &sha_ctx );
		if( 0 == mbedtls_md_setup (&sha_ctx, info_sha, 1))
		{
			unsigned char key[MBEDTLS_MD_MAX_BLOCK_SIZE] = {0};

			ret = mbedtls_pkcs5_pbkdf2_hmac( &sha_ctx, (unsigned char *)password, strlen(password), (unsigned char *)account,
			strlen(account), iter_time, key_length, key );

			if ((0 == ret) && (0 != memcmp (key, login_auth, key_length)))
			{
				ret = MBEDTLS_ERR_PKCS5_PASSWORD_MISMATCH;
			}
		}

		mbedtls_md_free( &sha_ctx );
	}

	return ret;
}
#endif
