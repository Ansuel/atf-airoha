/*
 * Copyright (c) 2016-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef ECNT_PLAT_COMMON_H
#define ECNT_PLAT_COMMON_H

#include <stdint.h>

#include <common/bl_common.h>
#include <common/param_header.h>

/*******************************************************************************
 * Function and variable prototypes
 ******************************************************************************/
#define DEVINFO_SIZE 4
#define LINUX_KERNEL_32 0
#define SMC32_PARAM_MASK		(0xFFFFFFFF)

#define AVS_OP_FREQ_DOWN 0
#define AVS_OP_FREQ_RECOVER 1

#define FREQ_RECOVER	0
#define FREQ_DOWN	1

/* added for cpu freq */
#define CPU_FREQ_OP
#define AVS_OP_FREQ_DYN_ADJ    (0xddddddd1)
#define AVS_OP_GET_FREQ    (0xddddddd2)
struct atf_arg_t {
	unsigned int atf_magic;
	unsigned int tee_support;
	unsigned int tee_entry;
	unsigned int tee_boot_arg_addr;
	unsigned int hwuid[4];     /* HW Unique id for t-base used */
	unsigned int HRID[2];      /* HW random id for t-base used */
	unsigned int atf_log_port;
	unsigned int atf_log_baudrate;
	unsigned int atf_log_buf_start;
	unsigned int atf_log_buf_size;
	unsigned int atf_irq_num;
	unsigned int devinfo[DEVINFO_SIZE];
	unsigned int atf_aee_debug_buf_start;
	unsigned int atf_aee_debug_buf_size;
};

struct kernel_info {
	uint64_t pc;
	uint64_t r0;
	uint64_t r1;
	uint64_t r2;
	uint64_t k32_64;
};

struct mtk_bl_param_t {
	uint64_t bootarg_loc;
	uint64_t bootarg_size;
	uint64_t bl33_start_addr;
	uint64_t tee_info_addr;
};

struct mtk_bl31_params {
       param_header_t h;
       image_info_t *bl31_image_info;
       entry_point_info_t *bl32_ep_info;
       image_info_t *bl32_image_info;
       entry_point_info_t *bl33_ep_info;
       image_info_t *bl33_image_info;
};

/* Declarations for mtk_plat_common.c */
uint32_t plat_get_spsr_for_bl32_entry(void);
uint32_t plat_get_spsr_for_bl33_entry(void);
void clean_top_32b_of_param(uint32_t smc_fid, u_register_t *x1,
				u_register_t *x2,
				u_register_t *x3,
				u_register_t *x4);
void bl31_prepare_kernel_entry(uint64_t k32_64);
void enable_ns_access_to_cpuectlr(void);
void boot_to_kernel(uint64_t x1, uint64_t x2, uint64_t x3, uint64_t x4);
uint64_t get_kernel_info_pc(void);
uint64_t get_kernel_info_r0(void);
uint64_t get_kernel_info_r1(void);
uint64_t get_kernel_info_r2(void);

uint64_t ecnt_efuse_handler(uint32_t r1, uint32_t r2, uint32_t r3);
uint64_t ecnt_test_handler(uint32_t r1, uint32_t r2, uint32_t r3);
uint64_t ecnt_verify_handler(uint32_t id, uint32_t r1, uint32_t r2, uint32_t r3);
uint64_t ecnt_decrypt_handler(uint32_t id, uint32_t r1, uint32_t r2, uint32_t r3);
uint64_t ecnt_password_verify_handler(uint32_t r1, uint32_t r2, uint32_t r3);
uint64_t ecnt_decrypt_dm_key_handler(uint32_t r1, uint32_t r2, uint32_t r3);

#if defined(IMAGE_BL31)
int ecnt_mbedtls_pkcs5_pbkdf2_compare (char *account, char *password, unsigned char *login_auth, unsigned int iter_time, unsigned int key_length, unsigned int hash_algo);
#endif

uint64_t ecnt_avs_handler(uint32_t r1, uint32_t r2, uint32_t r3);
uint64_t ecnt_sref_handler(uint32_t r1);
uint64_t phy_efuse_handler(uint32_t r1);
uint64_t DDR_TX_DLY_handler(uint32_t r1, uint32_t r2);
uint64_t DDR_RX_DLY_handler(uint32_t r1, uint32_t r2);


extern struct atf_arg_t gteearg;
#endif /* ECNT_PLAT_COMMON_H */
