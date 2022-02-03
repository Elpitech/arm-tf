/*
 * Copyright (c) 2018-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/mmio.h>
#include <lib/utils_def.h>

#include <bm1000_cmu.h>
#include <bm1000_def.h>
#include <bm1000_private.h>
#include <bm1000_smmu.h>

#define MMVDEC_RST			(MMVDEC_GPR_BASE + 0x00)
#define MMVDEC_SLAVE_RST		BIT(0)
#define MMVDEC_VDEC_RST			BIT(1)
#define MMVDEC_SMMU_RST			BIT(2)
#define MMVDEC_SMMU_SLV_RST		BIT(3)

#define MMVDEC_CFG_REG			(MMVDEC_GPR_BASE + 0x08)
#define MMVDEC_CFG_REG_HEVC_EN		BIT(0)
#define MMVDEC_CFG_REG_H264_EN		BIT(1)
#define MMVDEC_CFG_REG_VC1_EN		BIT(2)
#define MMVDEC_CFG_REG_WMV9_EN		BIT(3)
#define MMVDEC_CFG_REG_MPEG1_EN		BIT(4)
#define MMVDEC_CFG_REG_MPEG2_EN		BIT(5)
#define MMVDEC_CFG_REG_MPEG4_EN		BIT(6)
#define MMVDEC_CFG_REG_AVS_EN		BIT(7)
#define MMVDEC_CFG_REG_SORENSON_EN	BIT(8)
#define MMVDEC_CFG_REG_RV_EN		BIT(9)
#define MMVDEC_CFG_REG_VP6_EN		BIT(10)
#define MMVDEC_CFG_REG_VP8_EN		BIT(11)
#define MMVDEC_CFG_REG_REG_PROT		BIT(16)
#define MMVDEC_CFG_REG_BPASIDLE		BIT(20)

void mmvdec_init(void)
{
	/*
	 * Set PLL clock to 1.2 GHz. It will be further divided by 3
	 * by a clock channel to minimize jitter.
	 */
	const cmu_pll_ctl_vals_t mmvdec_cmu0_pll_ctls = {
		0, 0, 0x60, 0, 0, 0x2c, 0, 0x2c
	};

	cmu_pll_on(MMVDEC_CMU0_BASE, &mmvdec_cmu0_pll_ctls);

	/* 1.2 GHz / 3 = 400 MHz */
	cmu_clkch_enable_by_base(MMVDEC_CMU0_CLKCHCTL_SYS, 3);

	/* Deassert reset signals */
	mmio_clrbits_32(MMVDEC_RST,
			MMVDEC_SLAVE_RST |
			MMVDEC_VDEC_RST	 |
			MMVDEC_SMMU_RST	 |
			MMVDEC_SMMU_SLV_RST);

	mmvdec_smmu_set_normalize(0);
	mmvdec_smmu_set_domain_cache(0x2, 0x2, 0x7);
	mmvdec_smmu_set_qos(0xf, 0xf);

	/* Enable non-secure access */
	mmio_write_32(MMVDEC_NIC_CFG_GPV_REGIONSEC_VDEC, NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(MMVDEC_NIC_CFG_GPV_REGIONSEC_SMMU, NIC_GPV_REGIONSEC_NONSECURE);

	mmio_clrsetbits_32(MMVDEC_SMMU_CFG1_REG,
			   MMVDEC_SMMU_CFG1_REG_AWPROT_MASK |
			   MMVDEC_SMMU_CFG1_REG_ARPROT_MASK,
			   MMVDEC_SMMU_CFG1_REG_AWPROT(0x2) |
			   MMVDEC_SMMU_CFG1_REG_ARPROT(0x2));
}
