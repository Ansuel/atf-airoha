
#include <assert.h>
#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/stat.h>
#include <openssl/conf.h>
#include <firmware_image_package.h>

#include "cmd_opt.h"
#include "debug.h"

#define NUM_ELEM(x)			((sizeof(x)) / (sizeof(x[0])))
#define HELP_OPT_MAX_LEN		128

#define KEY_STRING_SIZE_256	64
#define KEY_STRING_SIZE_128	32

#define ENV_CRC_VALUE_LENGTH	4

extern const char build_msg[];
extern const char platform_msg[];


enum {
	ENC_ALG_NONE,
	ENC_ALG_AES128,
	ENC_ALG_AES256,
};

enum {
	HASH_ALG_SHA256,
	HASH_ALG_SHA512,
};

static const char *enc_algs_str[] = {
	[ENC_ALG_NONE] = "none",
	[ENC_ALG_AES128] = "aes128",
	[ENC_ALG_AES256] = "aes256"
};

static const char *hash_algs_str[] = {
	[HASH_ALG_SHA256] = "sha256",
	[HASH_ALG_SHA512] = "sha512",
};

static secure_efuse_t secure_data;

static int get_file_size(char *filename)
{
	struct stat st;

	if (stat(filename, &st) == 0)
		return st.st_size;

	return -1;
}

static int get_enc_alg(const char *enc_alg_str)
{
	int i;

	for (i = 0 ; i < NUM_ELEM(enc_algs_str) ; i++) {
		if (strcmp(enc_alg_str, enc_algs_str[i]) == 0) {
			return i;
		}
	}

	return -1;
}

static int get_hash_alg(const char *hash_alg_str)
{
	int i;

	for (i = 0 ; i < NUM_ELEM(hash_algs_str) ; i++) {
		if (0 == strcmp(hash_alg_str, hash_algs_str[i])) {
			return i;
		}
	}

	return -1;
}

static void print_help(const char *cmd, const struct option *long_opt)
{
	int rem, i = 0;
	const struct option *opt;
	char line[HELP_OPT_MAX_LEN];
	char *p;

	assert(cmd != NULL);
	assert(long_opt != NULL);

	printf("\n");
	printf("Usage:\n");
	printf("\t%s [OPTIONS]\n\n", cmd);

	printf("Available options:\n");
	opt = long_opt;
	while (opt->name)
	{
		p = line;
		rem = HELP_OPT_MAX_LEN;
		if (isalpha(opt->val))
		{
			/* Short format */
			sprintf(p, "-%c,", (char)opt->val);
			p += 3;
			rem -= 3;
		}
		snprintf(p, rem, "--%s %s", opt->name,
			 (opt->has_arg == required_argument) ? "<arg>" : "");
		printf("\t%-32s %s\n", line, cmd_opt_get_help_msg(i));
		opt++;
		i++;
	}
	printf("\n");
}

/* Common command line options */
static const cmd_opt_t common_cmd_opt[] = {
	{
		{ "help", no_argument, NULL, 'h' },
		"Print this message and exit"
	},
	{
		{ "aes-alg", required_argument, NULL, 'a' },
		"AES algorithm : 'none (default)', 'aes128', 'aes256''"
	},
	{
		{ "hash-alg", required_argument, NULL, 's' },
		"Hash algorithm : 'sha256' (default), 'sha512'"
	},
	{
		{ "key", required_argument, NULL, 'k' },
		"AES key."
	},
	{
		{ "2nd_key", required_argument, NULL, 'K' },
		"AES key."
	},
	{
		{ "rotpk", required_argument, NULL, 'r' },
		"Root Of Trust key filename."
	},
	{
		{ "2nd_rotpk", required_argument, NULL, 'R' },
		"Root Of Trust key filename."
	},
	{
		{ "out", required_argument, NULL, 'o' },
		"Efuse output filename."
	},

};

int check_argument(int enc_alg, int hash_alg, char *key, char *rotpk)
{
	int key_string_len = 0;
	int key_len = 0, rotpk_len = 0;

	if(!key || !rotpk)
	{
		printf("*** Does not input the key, skip parsing key***\n");
		return 0;
	}
	if (enc_alg != ENC_ALG_NONE)
	{
		key_string_len = strlen(key);
		if ((enc_alg == ENC_ALG_AES128 && key_string_len != KEY_STRING_SIZE_128) ||
			(enc_alg == ENC_ALG_AES256 && key_string_len != KEY_STRING_SIZE_256))
		{
			ERROR("AES key length should be %d, but input is %d\n",
				(enc_alg == ENC_ALG_AES128) ? KEY_STRING_SIZE_128 : KEY_STRING_SIZE_256,
				key_string_len);
			return -1;
		}
		key_len = key_string_len >> 1;
	}

	rotpk_len = get_file_size(rotpk);

	if ((hash_alg == HASH_ALG_SHA256 && rotpk_len != FILE_SIZE_SHA256) ||
		(hash_alg == HASH_ALG_SHA512 && rotpk_len != FILE_SIZE_SHA512))
	{
		ERROR("Root Of Trust key length should be %d, but input is %d\n",
			(hash_alg == HASH_ALG_SHA256) ? FILE_SIZE_SHA256 : FILE_SIZE_SHA512,
			rotpk_len);
		return -1;
	}

	printf("key_string_len %d rotpk_len %d\n", key_len, rotpk_len);
	return 0;
}

int efuse_create(int enc_alg, int hash_alg, char *key, char *rotpk, char *key_2nd, char *rotpk_2nd)
{
	FILE *ip_file = NULL;
	int i = 0, j = 0, byte = 0;
	unsigned int key_len = KEY_SIZE_128, rotpk_len = FILE_SIZE_SHA256;
#if defined(TCSUPPORT_CPU_AN7552)
	unsigned char aes_key[KEY_SIZE_128] = {0}, hash_sha[FILE_SIZE_SHA256] = {0};
#else
	unsigned char aes_key[KEY_SIZE_256] = {0}, hash_sha[FILE_SIZE_SHA512] = {0};
#endif

	memset(&secure_data, 0, sizeof(secure_efuse_t));

#if !defined(TCSUPPORT_CPU_AN7552)
	if (hash_alg == HASH_ALG_SHA512)
	{
		secure_data.hash_mode = HASH_MODE_SHA512;
		rotpk_len = FILE_SIZE_SHA512;
	}
#endif

	if (enc_alg != ENC_ALG_NONE)
	{

#if !defined(TCSUPPORT_CPU_AN7552)
		if (enc_alg == ENC_ALG_AES256)
		{
			secure_data.enc_mode = ENC_MODE_AES256;
			key_len = KEY_SIZE_256;
		}
#endif
		printf("aes key:\n");
		for (i = 0, j = 0; i < key_len; i++, j += 2)
		{
			if (sscanf(&key[j], "%02hhx", &aes_key[i]) != 1)
			{
				ERROR("Incorrect key format\n");
				return -1;
			}
			printf("%x ", aes_key[i]);
		}
		printf("\n");
		memcpy(secure_data.ssk, aes_key, key_len);

#if defined(TCSUPPORT_CPU_AN7583)
		if((key_2nd != NULL))
		{
			printf("2nd aes key:\n");
			for (i = 0, j = 0; i < key_len; i++, j += 2)
			{
				if (sscanf(&key_2nd[j], "%02hhx", &aes_key[i]) != 1)
				{
					ERROR("Incorrect key format\n");
					return -1;
				}
				printf("%x ", aes_key[i]);
			}
			printf("\n");
			memcpy(secure_data.ssk_dual, aes_key, key_len);
		}
#endif

	}

	ip_file = fopen(rotpk, "rb");
	if (ip_file == NULL)
	{
		ERROR("Cannot read %s\n", rotpk);
		return -1;
	}

	byte = fread(hash_sha, 1, rotpk_len, ip_file);
	fclose(ip_file);
	if (byte != rotpk_len)
	{
		ERROR("Read length error %d\n", byte);
		return -1;
	}
	memcpy(secure_data.rotpk, hash_sha, rotpk_len);
	printf("Root of trusted public key hashed value:\n");

	for (i = 0; i < rotpk_len; i++)
	{
		printf("%x ", hash_sha[i]);
	}
	printf("\n");
	
#if defined(TCSUPPORT_CPU_AN7583)
	if((rotpk_2nd != NULL))
	{
		ip_file = fopen(rotpk_2nd, "rb");
		if (ip_file == NULL)
		{
			ERROR("Cannot read %s\n", rotpk_2nd);
			return -1;
		}

		byte = fread(hash_sha, 1, rotpk_len, ip_file);
		fclose(ip_file);
		if (byte != rotpk_len)
		{
			ERROR("Read length error %d\n", byte);
			return -1;
		}
		memcpy(secure_data.rotpk_dual, hash_sha, rotpk_len);
		printf("2nd Root of trusted public key hashed value:\n");

		for (i = 0; i < rotpk_len; i++)
		{
			printf("%x ", hash_sha[i]);
		}
		printf("\n");	
	}
#endif

	/*secure_data.vaild = SECURE_VAILD;*/

	return 0;
}

int main(int argc, char *argv[])
{
	int i = 0, enc_alg = ENC_ALG_NONE, hash_alg = HASH_ALG_SHA256;
	int c = 0, opt_idx = 0;
	char *key = NULL;
	char *rotpk = NULL;
	char *key_2nd = NULL;
	char *rotpk_2nd = NULL;
	char *out_fn = NULL;
	const struct option *cmd_opt =NULL;
	FILE *op_file = NULL;
#ifdef TCSUPPORT_ARM_SECURE_BOOT_FLASH_KEY
	char out_flash_fn[128];
#endif

	NOTICE("ECONET Cert Tool: %s\n", build_msg);

	/* Add common command line options */
	for (i = 0; i < NUM_ELEM(common_cmd_opt); i++)
	{
		cmd_opt_add(&common_cmd_opt[i]);
	}

	/* Get the command line options populated during the initialization */
	cmd_opt = cmd_opt_get_array();

	while (1)
	{
		/* getopt_long stores the option index here. */
		c = getopt_long(argc, argv, "h:a:s:k:K:r:R:o:", cmd_opt, &opt_idx);

		/* Detect the end of the options. */
		if (c == -1)
		{
			break;
		}

		switch (c)
		{
		case 'a':
			enc_alg = get_enc_alg(optarg);
			if (enc_alg < 0) {
				ERROR("Invalid encrypt algorithm '%s'\n", optarg);
				exit(1);
			}
			break;
		case 's':
			hash_alg = get_hash_alg(optarg);
			if (hash_alg < 0) {
				ERROR("Invalid hash algorithm '%s'\n", optarg);
				exit(1);
			}
			break;
		case 'k':
			key = optarg;
			break;
		case 'K':
			key_2nd = optarg;
			break;
		case 'r':
			rotpk = optarg;
			break;
		case 'R':
			rotpk_2nd = optarg;
			break;
		case 'o':
			out_fn = optarg;
			break;
		case 'h':
			print_help(argv[0], cmd_opt);
			exit(0);
		case '?':
		default:
			print_help(argv[0], cmd_opt);
			exit(1);
		}
	}

	if (enc_alg)
	{
		if (!key) {
			ERROR("AES key must not be NULL\n");
			exit(1);
		}
	}

	if (!rotpk)
	{
		ERROR("Root Of Trust key must not be NULL\n");
		exit(1);
	}

	if (!out_fn)
	{
		ERROR("Output filename must not be NULL\n");
		exit(1);
	}

	if (check_argument(enc_alg, hash_alg, key, rotpk))
	{
		ERROR("Input argument length mismatch\n");
		exit(1);
	}

#ifdef TCSUPPORT_CPU_AN7583
	if (check_argument(enc_alg, hash_alg, key_2nd, rotpk_2nd))
	{
		ERROR("Input argument length mismatch\n");
		exit(1);
	}
#endif

	if (efuse_create(enc_alg, hash_alg, key, rotpk, key_2nd, rotpk_2nd))
	{
		ERROR("secure efuse create error\n");
		exit(1);
	}
	else
	{

	}

	op_file = fopen(out_fn, "wb");
	if (op_file == NULL)
	{
		ERROR("Cannot write %s\n", out_fn);
		return -1;
	}
	if (fseek(op_file, 0, SEEK_SET))
	{
		fclose(op_file);

		ERROR("fseek failed\n");
		exit(1);
	}
	fwrite(&secure_data, 1, sizeof(secure_efuse_t), op_file);
	fclose(op_file);

#ifdef TCSUPPORT_ARM_SECURE_BOOT_FLASH_KEY
	/* New file for tcboot.bin */
	sprintf (out_flash_fn, "%s_flash", out_fn);

	op_file = fopen(out_flash_fn, "wb");
	if (op_file == NULL)
	{
		ERROR("Cannot write %s\n", out_flash_fn);
		return -1;
	}
	if (fseek(op_file, 0, SEEK_SET))
	{
		fclose(op_file);

		ERROR("fseek failed\n");
		exit(1);
	}

	secure_data.vaild = SECURE_VAILD;

	/* Add padding 4 bytes for env CRC value */
	fwrite(&secure_data, 1, (sizeof(secure_efuse_t)+ENV_CRC_VALUE_LENGTH), op_file);
	fclose(op_file);
#endif

	return 0;
}
