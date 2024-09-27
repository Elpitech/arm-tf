/*
 * Copyright (c) 2023-2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/mmio.h>
#include <lib/utils_def.h>

#include <bm1000_def.h>
#include <bm1000_private.h>

#define MMCORESIGHT_TSGEN_BASE		(MMCORESIGHT_CORESIGHT_BASE + 0x3000)
#define MMCORESIGHT_TSGEN_CNTCR		(MMCORESIGHT_TSGEN_BASE)
#define MMCORESIGHT_TSGEN_CNTCR_EN	BIT(0)

void mmcoresight_init(void)
{
	/* Deassert reset signals */
	mmio_clrbits_32(MMCORESIGHT_GPR_MMRST,
			MMCORESIGHT_GPR_MMRST_CFG_NIC |
			MMCORESIGHT_GPR_MMRST_CSSYS   |
			MMCORESIGHT_GPR_MMRST_CC0     |
			MMCORESIGHT_GPR_MMRST_CC1     |
			MMCORESIGHT_GPR_MMRST_CC2     |
			MMCORESIGHT_GPR_MMRST_CC3);

	/* Enable non-secure access */
	mmio_write_32(MMCORESIGHT_NIC_CFG_SECURITY_SMMU,  NIC_SECURITY_REGION0_NONSECURE);
	mmio_write_32(MMCORESIGHT_NIC_CFG_SECURITY_APBIC, NIC_SECURITY_REGION0_NONSECURE);

	/* Enable TSGEN */
	mmio_setbits_32(MMCORESIGHT_TSGEN_CNTCR,
			MMCORESIGHT_TSGEN_CNTCR_EN);
}
