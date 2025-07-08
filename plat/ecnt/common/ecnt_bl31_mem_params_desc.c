/*
 * Copyright (c) 2016-2020, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform_def.h>

#include <common/bl_common.h>
#include <common/desc_image_load.h>
#include <mbedtls/md.h>

/*******************************************************************************
 * Following descriptor provides BL image/ep information that gets used
 * by BL2 to load the images and also subset of this information is
 * passed to next BL image. The image loading sequence is managed by
 * populating the images in required loading order. The image execution
 * sequence is managed by populating the `next_handoff_image_id` with
 * the next executable image id.
 ******************************************************************************/

static bl_mem_params_node_t bl31_mem_params_descs[] = {
	{
		.image_id = BL2_IMAGE_ID,
		SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP,
			VERSION_2, entry_point_info_t,
			SECURE | NON_EXECUTABLE),
		.ep_info.pc = 0,

		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP,
			VERSION_2, image_info_t, 0),
		.image_info.image_base = TZRAM2_BASE,
		.image_info.image_size = 0,
		.image_info.image_max_size = TZRAM2_SIZE,

		.next_handoff_image_id = INVALID_IMAGE_ID,
	},

    {
		.image_id = BL33_IMAGE_ID,
		SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP,
				      VERSION_2, entry_point_info_t,
				      NON_SECURE | NON_EXECUTABLE),
		.ep_info.pc = 0,

		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP,
			VERSION_2, image_info_t, 0),
		.image_info.image_base = TZRAM2_BASE,
		.image_info.image_size = 0,
		.image_info.image_max_size = TZRAM2_SIZE,

		.next_handoff_image_id = INVALID_IMAGE_ID,
    }
};

REGISTER_BL_IMAGE_DESCS(bl31_mem_params_descs)
