/*
 * Copyright (c) 2018-2020, Baikal Electronics JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <assert.h>
#include <bm1000_private.h>
#include <bm1000_smmu.h>
#include <lib/mmio.h>
#include <platform_def.h>

/* Resets */
#define MMVDEC_RST                  MMVDEC_GPR(0)
#define MMVDEC_SLAVE_RST            CTL_BIT(0)
#define MMVDEC_VDEC_RST             CTL_BIT(1)
#define MMVDEC_MMU_RST              CTL_BIT(2)
#define MMVDEC_MMU_SLV_RST          CTL_BIT(3)

/* Configuration */
#define MMVDEC_CFG_REG                     MMVDEC_GPR(0x8)
#define MMVDEC_CFG_REG_BPASIDLE            (1 << 20)
#define MMVDEC_CFG_REG_REG_PROT            (1 << 16)
#define MMVDEC_CFG_REG_VP8_EN              (1 << 11)
#define MMVDEC_CFG_REG_VP6_EN              (1 << 10)
#define MMVDEC_CFG_REG_RV_EN               (1 << 9)
#define MMVDEC_CFG_REG_SORENSON_EN         (1 << 8)
#define MMVDEC_CFG_REG_AVS_EN              (1 << 7)
#define MMVDEC_CFG_REG_MPEG4_EN            (1 << 6)
#define MMVDEC_CFG_REG_MPEG2_EN            (1 << 5)
#define MMVDEC_CFG_REG_MPEG1_EN            (1 << 4)
#define MMVDEC_CFG_REG_WMV9_EN             (1 << 3)
#define MMVDEC_CFG_REG_VC1_EN              (1 << 2)
#define MMVDEC_CFG_REG_H264_EN             (1 << 1)
#define MMVDEC_CFG_REG_HEVC_EN             (1 << 0)

/* Clocks */
#define MMVDEC_CCH_0       0
#define MMVDEC_CCH_ON(id, divider) CLKCH_ON(VDEC_LCRU + LCRU_CMU0 + LCRU_CLKCH_OFFSET(id), divider)

/* Set PLL clock to 1.42 GHz - will be further divided by 2 by a clock channel to minimize jitter */
static const PllCtlInitValues VDEC_LCRU_PLL_INIT = {
	0, 0x18, 0xb18, 0x00, 0x00, 0x2c, 0x00, 0x2c
};

void mmvdec_on(void)
{
	uint32_t vdec_gpr;
	pll_on(VDEC_LCRU, &VDEC_LCRU_PLL_INIT, "VDEC_LCRU timeout!");

	/* 1.42 GHz / 2 = 710 MHz */
	MMVDEC_CCH_ON(MMVDEC_CCH_0, 2);

	/* Remove reset signals */
	vdec_gpr = mmio_read_32(MMVDEC_RST);
	vdec_gpr &= ~(
			MMVDEC_SLAVE_RST   |
			MMVDEC_VDEC_RST    |
			MMVDEC_MMU_RST     |
			MMVDEC_MMU_SLV_RST
			);
	mmio_write_32(MMVDEC_RST, vdec_gpr);

	mmvdec_smmu_set_normalize(0);
	mmvdec_smmu_set_domain_cache(0x2, 0x2, 0x7);
	mmvdec_smmu_set_qos(0xf, 0xf);
}

void mmvdec_toNSW(void)
{
	uint32_t vdec_gpr;

	/* Secure mode */
	SECURE_MODE(BAIKAL_NIC_VDEC_TCU, SECURE_MODE_OFF);
	SECURE_MODE(BAIKAL_NIC_VDEC_CTL, SECURE_MODE_OFF);

	/* Setup AxPROT */
	vdec_gpr = mmio_read_32(MMVDEC_SMMU_CFG1_REG);
	vdec_gpr &= ~(
			MMVDEC_SMMU_CFG1_REG_AWPROT_MASK |
			MMVDEC_SMMU_CFG1_REG_ARPROT_MASK
			);
	vdec_gpr |= (
			MMVDEC_SMMU_CFG1_REG_AWPROT(0x2) |
			MMVDEC_SMMU_CFG1_REG_ARPROT(0x2)
			);
	mmio_write_32(MMVDEC_SMMU_CFG1_REG, vdec_gpr);
}
