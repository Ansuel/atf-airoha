/*
 * Copyright (c) 2016-2022, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>

#include <firmware_image_package.h>

#include "tbbr_config.h"

/* The images used depends on the platform. */
toc_entry_t toc_entries[] = {
#if defined(CONFIG_ECNT)
	{
		.name = "Trusted Boot Firmware BL2",
		.uuid = UUID_TRUSTED_BOOT_FIRMWARE_BL2,
		.cmdline_name = "tb-fw"
	},
	{
		.name = "Trusted Boot Firmware BL2 certificate",
		.uuid = UUID_TRUSTED_BOOT_FW_CERT,
		.cmdline_name = "tb-fw-cert"
	},
	{
		.name = "Trusted key certificate",
		.uuid = UUID_TRUSTED_KEY_CERT,
		.cmdline_name = "trusted-key-cert"
	},
	{
		.name = "SoC Firmware key certificate",
		.uuid = UUID_SOC_FW_KEY_CERT,
		.cmdline_name = "soc-fw-key-cert"
	},
	{
		.name = "Non-Trusted Firmware key certificate",
		.uuid = UUID_NON_TRUSTED_FW_KEY_CERT,
		.cmdline_name = "nt-fw-key-cert"
	},
	{
		.name = "SoC Firmware content certificate",
		.uuid = UUID_SOC_FW_CONTENT_CERT,
		.cmdline_name = "soc-fw-cert"
	},
	{
		.name = "Non-Trusted Firmware content certificate",
		.uuid = UUID_NON_TRUSTED_FW_CONTENT_CERT,
		.cmdline_name = "nt-fw-cert"
	},
	{
		.name = "EL3 Runtime Firmware BL31",
		.uuid = UUID_EL3_RUNTIME_FIRMWARE_BL31,
		.cmdline_name = "soc-fw"
	},
	{
		.name = "Non-Trusted Firmware BL33",
		.uuid = UUID_NON_TRUSTED_FIRMWARE_BL33,
		.cmdline_name = "nt-fw"
	},
#endif
	{
		.name = "SCP Firmware Updater Configuration FWU SCP_BL2U",
		.uuid = UUID_TRUSTED_UPDATE_FIRMWARE_SCP_BL2U,
		.cmdline_name = "scp-fwu-cfg"
	},
	{
		.name = "AP Firmware Updater Configuration BL2U",
		.uuid = UUID_TRUSTED_UPDATE_FIRMWARE_BL2U,
		.cmdline_name = "ap-fwu-cfg"
	},
	{
		.name = "Firmware Updater NS_BL2U",
		.uuid = UUID_TRUSTED_UPDATE_FIRMWARE_NS_BL2U,
		.cmdline_name = "fwu"
	},
	{
		.name = "Non-Trusted Firmware Updater certificate",
		.uuid = UUID_TRUSTED_FWU_CERT,
		.cmdline_name = "fwu-cert"
	},
#if !defined(CONFIG_ECNT)
	{
		.name = "Trusted Boot Firmware BL2",
		.uuid = UUID_TRUSTED_BOOT_FIRMWARE_BL2,
		.cmdline_name = "tb-fw"
	},
#endif
	{
		.name = "SCP Firmware SCP_BL2",
		.uuid = UUID_SCP_FIRMWARE_SCP_BL2,
		.cmdline_name = "scp-fw"
	},
#if !defined(CONFIG_ECNT)
	{
		.name = "EL3 Runtime Firmware BL31",
		.uuid = UUID_EL3_RUNTIME_FIRMWARE_BL31,
		.cmdline_name = "soc-fw"
	},
#endif
	{
		.name = "Secure Payload BL32 (Trusted OS)",
		.uuid = UUID_SECURE_PAYLOAD_BL32,
		.cmdline_name = "tos-fw"
	},
	{
		.name = "Secure Payload BL32 Extra1 (Trusted OS Extra1)",
		.uuid = UUID_SECURE_PAYLOAD_BL32_EXTRA1,
		.cmdline_name = "tos-fw-extra1"
	},
	{
		.name = "Secure Payload BL32 Extra2 (Trusted OS Extra2)",
		.uuid = UUID_SECURE_PAYLOAD_BL32_EXTRA2,
		.cmdline_name = "tos-fw-extra2"
	},
#if !defined(CONFIG_ECNT)
	{
		.name = "Non-Trusted Firmware BL33",
		.uuid = UUID_NON_TRUSTED_FIRMWARE_BL33,
		.cmdline_name = "nt-fw"
	},
#endif
	{
		.name = "Realm Monitor Management Firmware",
		.uuid = UUID_REALM_MONITOR_MGMT_FIRMWARE,
		.cmdline_name = "rmm-fw"
	},
	/* Dynamic Configs */
	{
		.name = "FW_CONFIG",
		.uuid = UUID_FW_CONFIG,
		.cmdline_name = "fw-config"
	},
	{
		.name = "HW_CONFIG",
		.uuid = UUID_HW_CONFIG,
		.cmdline_name = "hw-config"
	},
	{
		.name = "TB_FW_CONFIG",
		.uuid = UUID_TB_FW_CONFIG,
		.cmdline_name = "tb-fw-config"
	},
	{
		.name = "SOC_FW_CONFIG",
		.uuid = UUID_SOC_FW_CONFIG,
		.cmdline_name = "soc-fw-config"
	},
	{
		.name = "TOS_FW_CONFIG",
		.uuid = UUID_TOS_FW_CONFIG,
		.cmdline_name = "tos-fw-config"
	},
	{
		.name = "NT_FW_CONFIG",
		.uuid = UUID_NT_FW_CONFIG,
		.cmdline_name = "nt-fw-config"
	},
	/* Key Certificates */
	{
		.name = "Root Of Trust key certificate",
		.uuid = UUID_ROT_KEY_CERT,
		.cmdline_name = "rot-cert"
	},
#if !defined(CONFIG_ECNT)
	{
		.name = "Trusted key certificate",
		.uuid = UUID_TRUSTED_KEY_CERT,
		.cmdline_name = "trusted-key-cert"
	},
#endif
	{
		.name = "SCP Firmware key certificate",
		.uuid = UUID_SCP_FW_KEY_CERT,
		.cmdline_name = "scp-fw-key-cert"
	},
#if !defined(CONFIG_ECNT)
	{
		.name = "SoC Firmware key certificate",
		.uuid = UUID_SOC_FW_KEY_CERT,
		.cmdline_name = "soc-fw-key-cert"
	},
#endif
	{
		.name = "Trusted OS Firmware key certificate",
		.uuid = UUID_TRUSTED_OS_FW_KEY_CERT,
		.cmdline_name = "tos-fw-key-cert"
	},
#if !defined(CONFIG_ECNT)
	{
		.name = "Non-Trusted Firmware key certificate",
		.uuid = UUID_NON_TRUSTED_FW_KEY_CERT,
		.cmdline_name = "nt-fw-key-cert"
	},
#endif
#if !defined(CONFIG_ECNT)
	/* Content certificates */
	{
		.name = "Trusted Boot Firmware BL2 certificate",
		.uuid = UUID_TRUSTED_BOOT_FW_CERT,
		.cmdline_name = "tb-fw-cert"
	},
#endif
	{
		.name = "SCP Firmware content certificate",
		.uuid = UUID_SCP_FW_CONTENT_CERT,
		.cmdline_name = "scp-fw-cert"
	},
#if !defined(CONFIG_ECNT)
	{
		.name = "SoC Firmware content certificate",
		.uuid = UUID_SOC_FW_CONTENT_CERT,
		.cmdline_name = "soc-fw-cert"
	},
#endif
	{
		.name = "Trusted OS Firmware content certificate",
		.uuid = UUID_TRUSTED_OS_FW_CONTENT_CERT,
		.cmdline_name = "tos-fw-cert"
	},
#if !defined(CONFIG_ECNT)
	{
		.name = "Non-Trusted Firmware content certificate",
		.uuid = UUID_NON_TRUSTED_FW_CONTENT_CERT,
		.cmdline_name = "nt-fw-cert"
	},
#endif
	{
		.name = "SiP owned Secure Partition content certificate",
		.uuid = UUID_SIP_SECURE_PARTITION_CONTENT_CERT,
		.cmdline_name = "sip-sp-cert"
	},
	{
		.name = "Platform owned Secure Partition content certificate",
		.uuid = UUID_PLAT_SECURE_PARTITION_CONTENT_CERT,
		.cmdline_name = "plat-sp-cert"
	},
	{
		.name = "CCA Content Certificate",
		.uuid = UUID_CCA_CONTENT_CERT,
		.cmdline_name = "cca-cert"
	},
	{
		.name = "Core Secure World Key Certificate",
		.uuid = UUID_CORE_SWD_KEY_CERT,
		.cmdline_name = "core-swd-cert"
	},
	{
		.name = "Platform Key Certificate",
		.uuid = UUID_PLAT_KEY_CERT,
		.cmdline_name = "plat-key-cert"
	},
	{
		.name = NULL,
		.uuid = { {0} },
		.cmdline_name = NULL,
	}
};
