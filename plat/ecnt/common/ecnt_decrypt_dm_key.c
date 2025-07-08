#include <errno.h>
#include <common/debug.h>
#include <plat/common/platform.h>
#include <tools_share/firmware_image_package.h>
#include <tools_share/firmware_encrypted.h>
#include <mbedtls/aes.h>
#include <drivers/auth/crypto_mod.h>
#include <drivers/auth/mbedtls/mbedtls_common.h>
#include <drivers/auth/mbedtls/mbedtls_config-3.h>

/* mbed TLS headers */
#include <mbedtls/gcm.h>
#include <mbedtls/md.h>
#include <mbedtls/memory_buffer_alloc.h>
#include <mbedtls/oid.h>
#include <mbedtls/platform.h>
#include <mbedtls/version.h>
#include <mbedtls/x509.h>

#define DEC_OP_BUF_SIZE		256
#define ENC_KEY_SIZE		256
#define AIROHA_KEY_SIZE		32

int decrypt_dm_key(uint8_t *p_buf, unsigned int size){
	enum fw_enc_status_t fw_enc_status;
	mbedtls_aes_context ctx;
	uint8_t enc_key[ENC_KEY_SIZE], airoha_key[AIROHA_KEY_SIZE], decrypt_key_buf[DEC_OP_BUF_SIZE];
	size_t airoha_key_len = AIROHA_KEY_SIZE;
	int i, rc;
	unsigned int key_flags = 0;

	memcpy(enc_key, p_buf, size);

	fw_enc_status = 0;
	plat_get_enc_key_info(fw_enc_status, airoha_key, &airoha_key_len, &key_flags, 0, 0);

	mbedtls_aes_init(&ctx);
	rc = mbedtls_aes_setkey_dec(&ctx, airoha_key, airoha_key_len * 8);
	if (rc != 0) {
		printf("SETKEY Failed\n");
		rc = -1;
		goto exit_aes;
	}

	for(i = 0; i < size; i += 16)
		mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_DECRYPT, (enc_key + i), (decrypt_key_buf + i));

	/* AES decryption success */
	rc = CRYPTO_SUCCESS;

	memcpy(p_buf, decrypt_key_buf, DEC_OP_BUF_SIZE);
	printf("decryption success\n");

exit_aes:
	mbedtls_aes_free(&ctx);
	return rc;
}

static inline int _is_valid_header(struct fw_enc_hdr *header)
{
	if (header->magic == ENC_HEADER_MAGIC)
		return 1;
	else
		return 0;
}

void _dump_raw (unsigned char* p, int len){
	int i=0;
	
	printf(" ----------------------start dump\n");
	for (i=0;i<len;i++){
		printf("0x%x \n", *(p+i));
	}
	printf(" ----------------------finish dump\n");
	
	return ;
}

int aes_gcm_decrypt(void *data_ptr, size_t len, const void *key,
			   unsigned int key_len, const void *iv,
			   unsigned int iv_len, const void *tag,
			   unsigned int tag_len)
{
	mbedtls_gcm_context ctx;
	mbedtls_cipher_id_t cipher = MBEDTLS_CIPHER_ID_AES;
	unsigned char buf[DEC_OP_BUF_SIZE];
	unsigned char tag_buf[16];
	unsigned char *pt = data_ptr;
	size_t dec_len;
	int diff, i, rc;
	size_t output_length ;

	mbedtls_gcm_init(&ctx);

	rc = mbedtls_gcm_setkey(&ctx, cipher, key, key_len * 8);
	if (rc != 0) {
		rc = CRYPTO_ERR_DECRYPTION;
		goto exit_gcm;
	}

	rc = mbedtls_gcm_starts(&ctx, MBEDTLS_GCM_DECRYPT, iv, iv_len);
	
	if (rc != 0) {
		rc = CRYPTO_ERR_DECRYPTION;
		goto exit_gcm;
	}

	while (len > 0) {
		dec_len = MIN(sizeof(buf), len);

		rc = mbedtls_gcm_update(&ctx, pt, dec_len, buf, sizeof(buf), &output_length);

		if (rc != 0) {
			rc = CRYPTO_ERR_DECRYPTION;
			goto exit_gcm;
		}

		memcpy(pt, buf, dec_len);
		pt += dec_len;
		len -= dec_len;
	}

	rc = mbedtls_gcm_finish(&ctx, NULL, 0, &output_length, tag_buf, sizeof(tag_buf));

	if (rc != 0) {
		rc = CRYPTO_ERR_DECRYPTION;
		goto exit_gcm;
	}

	/* Check tag in "constant-time" */
	for (diff = 0, i = 0; i < tag_len; i++)
		diff |= ((const unsigned char *)tag)[i] ^ tag_buf[i];

	if (diff != 0) {
		rc = CRYPTO_ERR_DECRYPTION;
		goto exit_gcm;
	}

	/* GCM decryption success */
	rc = CRYPTO_SUCCESS;

exit_gcm:
	mbedtls_gcm_free(&ctx);
	return rc;
}



int decrypt_gcm_data(uint8_t *buffer, unsigned int size){
	int result;
	struct fw_enc_hdr* p_header;
	void* payload = (void*) (buffer +sizeof(struct fw_enc_hdr));

	enum fw_enc_status_t fw_enc_status;
	uint8_t  airoha_key[AIROHA_KEY_SIZE];
	size_t airoha_key_len = AIROHA_KEY_SIZE;
	unsigned int key_flags = 0;

	NOTICE("debug: buffer=0x%x, payload=%p, sizeof fw_hdr=0x%x.\n", buffer, payload, sizeof(struct fw_enc_hdr));	
	int len;

	p_header = (struct fw_enc_hdr*) buffer;
	
	if (!_is_valid_header(p_header)) {
		printf("Encryption header check failed.\n");
		return -ENOENT;
	}

	printf("Encryption header looks OK.\n");
	
	fw_enc_status = p_header->flags & FW_ENC_STATUS_FLAG_MASK;

	if ((p_header->iv_len > ENC_MAX_IV_SIZE) ||
	    (p_header->tag_len > ENC_MAX_TAG_SIZE)) {

		#if (1)
		printf("\033[33;1m header->iv_len = 0x%x", p_header->iv_len);
		printf("\033[33;1m header->tag_len = 0x%x \033[0m", p_header->tag_len);
		#else
		printf("Incorrect IV or tag length\n");
		return -ENOENT;
		#endif
	}

	#if(0)
	printf("dump iv:\n");
	_dump_raw(p_header->iv,p_header->iv_len);
	printf("dump tag:\n");
	_dump_raw(p_header->tag,p_header->tag_len);
	#endif
	

	plat_get_enc_key_info(fw_enc_status, airoha_key, &airoha_key_len, &key_flags, 0, 0);

	#if(0)
	printf("dump key:\n");
	_dump_raw(airoha_key,airoha_key_len);
	#endif

	len = size - sizeof(struct fw_enc_hdr);
	
	printf("gcm len=%d (%d - %d)\n",len, size, sizeof(struct fw_enc_hdr));
	
	result = aes_gcm_decrypt(payload, len, airoha_key, airoha_key_len, p_header->iv, p_header->iv_len,
				     p_header->tag, p_header->tag_len);

	if(result==0){
		//printf("moving back..\n");
		memmove(payload-sizeof(struct fw_enc_hdr), payload, len);
	}

	return result;
}
