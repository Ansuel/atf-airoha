/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

	.global ecnt_npu_img
	.global ecnt_npu_img_end
	.global ecnt_npu_data
	.global ecnt_npu_data_end
	
ecnt_npu_img:
	.incbin NPU_IMAGE_BIN
ecnt_npu_img_end:
ecnt_npu_data:
	.incbin NPU_DATA_BIN
ecnt_npu_data_end: