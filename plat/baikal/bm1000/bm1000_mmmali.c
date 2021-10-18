/*
 * Copyright (c) 2018-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <bm1000_cmu.h>
#include <bm1000_def.h>
#include <bm1000_private.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>

#define MMMALI_GPR(offset)	(MMMALI_GPR_BASE + offset)

/* Resets */
#define MMMALI_RST			MMMALI_GPR(0)
#define MMMALI_RST_NIC			BIT(0)
#define MMMALI_RST_PVT			BIT(1)

/* AXI */
#define MMMALI_PROT_CTL0_REG		MMMALI_GPR(0x18)
#define MMMALI_PROT_CTL1_REG		MMMALI_GPR(0x20)
#define MMMALI_PROT_CTL_REG_AWPROT(x)	SETMASK(x, 2, 0)
#define MMMALI_PROT_CTL_REG_ARPROT(x)	SETMASK(x, 5, 3)

void mmmali_init(void)
{
	static const cmu_pll_ctl_vals_t mali_lcru_pll_init = {
		0, 0, 0x68, 0, 0, 0x2c, 0, 0x2c
	};

	cmu_pll_on(MMMALI_CMU0_BASE, &mali_lcru_pll_init);
	cmu_clkch_enable_by_base(MMMALI_CMU0_CLKCHCTL_AXI, 2);
	mmio_clrbits_32(MMMALI_RST,
			MMMALI_RST_NIC |
			MMMALI_RST_PVT);
}

void mmmali_ns_access(void)
{
	mmio_write_32(MMMALI_PROT_CTL0_REG,
		      MMMALI_PROT_CTL_REG_AWPROT(2) |
		      MMMALI_PROT_CTL_REG_ARPROT(2));

	mmio_write_32(MMMALI_PROT_CTL1_REG,
		      MMMALI_PROT_CTL_REG_AWPROT(2) |
		      MMMALI_PROT_CTL_REG_ARPROT(2));

	mmio_write_32(MMMALI_NIC_CFG_GPV_REGIONSEC_MALI, NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(MMMALI_NIC_CFG_GPV_REGIONSEC_SMMU, NIC_GPV_REGIONSEC_NONSECURE);
}
