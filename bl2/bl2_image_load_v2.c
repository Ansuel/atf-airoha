/*
 * Copyright (c) 2016-2022, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <stdint.h>

#include <arch.h>
#include <arch_helpers.h>
#include "bl2_private.h"
#include <common/bl_common.h>
#include <common/debug.h>
#include <common/desc_image_load.h>
#include <drivers/auth/auth_mod.h>
#include <plat/common/platform.h>

#include <platform_def.h>

#if defined(IMAGE_BL23)
extern void bl2_mem_params_backup(void);
extern void bl2_mem_params_restore(void);
extern void plat_ecnt_io_switch_to_memmap(void);
extern int fip_image_xmodem_load(void *loadaddr, int max_size);
#endif

static int bl2_do_load_images(void)
{
	bl_load_info_t *bl2_load_info;
	const bl_load_info_node_t *bl2_node_info;
	static int plat_setup_done = 0;
	int err;

	/*
	 * Get information about the images to load.
	 */
	bl2_load_info = plat_get_bl_image_load_info();
	assert(bl2_load_info != NULL);
	assert(bl2_load_info->head != NULL);
	assert(bl2_load_info->h.type == PARAM_BL_LOAD_INFO);
	assert(bl2_load_info->h.version >= VERSION_2);
	bl2_node_info = bl2_load_info->head;

	while (bl2_node_info != NULL) {
		/*
		 * Perform platform setup before loading the image,
		 * if indicated in the image attributes AND if NOT
		 * already done before.
		 */
		if ((bl2_node_info->image_info->h.attr &
		    IMAGE_ATTRIB_PLAT_SETUP) != 0U) {
			if (plat_setup_done != 0) {
				WARN("BL2: Platform setup already done!!\n");
			} else {
				INFO("BL2: Doing platform setup\n");
				bl2_platform_setup();
				plat_setup_done = 1;
			}
		}

		err = bl2_plat_handle_pre_image_load(bl2_node_info->image_id);
		if (err != 0) {
			ERROR("BL2: Failure in pre image load handling (%i)\n", err);
			return err;
		}

		if ((bl2_node_info->image_info->h.attr &
		    IMAGE_ATTRIB_SKIP_LOADING) == 0U) {
			INFO("BL2: Loading image id %u\n", bl2_node_info->image_id);
			err = load_auth_image(bl2_node_info->image_id,
				bl2_node_info->image_info);
			if (err != 0) {
				ERROR("BL2: Failed to load image id %u (%i)\n",
				      bl2_node_info->image_id, err);
				return err;
			}
		} else {
			INFO("BL2: Skip loading image id %u\n", bl2_node_info->image_id);
		}

		/* Allow platform to handle image information. */
		err = bl2_plat_handle_post_image_load(bl2_node_info->image_id);
		if (err != 0) {
			ERROR("BL2: Failure in post image load handling (%i)\n", err);
			return err;
		}

		/* Go to next image */
		bl2_node_info = bl2_node_info->next_load_info;
	}

	return 0;
}

/*******************************************************************************
 * This function loads SCP_BL2/BL3x images and returns the ep_info for
 * the next executable image.
 ******************************************************************************/
struct entry_point_info *bl2_load_images()
{
	bl_params_t *bl2_to_next_bl_params;
	int err;

#if !defined(IMAGE_BL23)
	err = bl2_do_load_images();
	if (err != 0)
		plat_error_handler(err);
#else
	while (1)  {
		bl2_mem_params_backup();
		err = bl2_do_load_images();
		if (!err)
			break;

		/*
		 * bl2_plat_handle_pre_image_load() and load_auth_image()
		 * may change contents of bl2_node_info->image_info, thus
		 * next call of bl2_do_load_images() will finish by panic.
		 *
		 * Restore original values of bl2_node_info->image_info to
		 * avoid an issue.
		 */
		bl2_mem_params_restore();

		/*
		 * In the case of booting from UBI, ubi I/O driver will be
		 * used to read BL31/U-Boot FIP image. This is not suitable.
		 *
		 * Switch to memmap based driver suitable for image reading
		 * via xmodem.
		 */
		plat_ecnt_io_switch_to_memmap();

		printf("BL31 + U-Boot FIP is damaged, loading via xmodem\n");
		fip_image_xmodem_load((void*)(uintptr_t)PLAT_ECNT_FIP_BASE,
				      PLAT_ECNT_FIP_MAX_SIZE);
	}
#endif

	/*
	 * Get information to pass to the next image.
	 */
	bl2_to_next_bl_params = plat_get_next_bl_params();
	assert(bl2_to_next_bl_params != NULL);
	assert(bl2_to_next_bl_params->head != NULL);
	assert(bl2_to_next_bl_params->h.type == PARAM_BL_PARAMS);
	assert(bl2_to_next_bl_params->h.version >= VERSION_2);
	assert(bl2_to_next_bl_params->head->ep_info != NULL);

	/* Populate arg0 for the next BL image if not already provided */
	if (bl2_to_next_bl_params->head->ep_info->args.arg0 == (u_register_t)0)
		bl2_to_next_bl_params->head->ep_info->args.arg0 =
					(u_register_t)bl2_to_next_bl_params;

	/* Flush the parameters to be passed to next image */
	plat_flush_next_bl_params();

	return bl2_to_next_bl_params->head->ep_info;
}
