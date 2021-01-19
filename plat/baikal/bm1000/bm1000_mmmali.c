/*
 * Copyright (c) 2018-2020, Baikal Electronics JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <assert.h>
#include <bm1000_private.h>
#include <drivers/arm/gicv3.h>
#include <drivers/console.h>
#include <lib/mmio.h>
#include <plat/arm/common/plat_arm.h>
#include <plat/common/platform.h>
#include <platform_def.h>

#define MMMALI_GPR(ind) (MALI_LCRU + LCRU_GPR + ind)

/* Resets */
#define MMMALI_RST MMMALI_GPR(0)
#define MMMALI_RST_PVT			CTL_BIT(1)
#define MMMALI_RST_NIC			CTL_BIT(0)

/* AXI */
#define MMMALI_PROT_CTL0_REG		MMMALI_GPR(0x18)
#define MMMALI_PROT_CTL1_REG		MMMALI_GPR(0x20)
#define MMMALI_PROT_CTL_REG_AWPROT_MASK	CTL_BIT_MASK(2,0)
#define MMMALI_PROT_CTL_REG_AWPROT(x)	CTL_BIT_SET(x, 2,0)
#define MMMALI_PROT_CTL_REG_ARPROT_MASK	CTL_BIT_MASK(5,3)
#define MMMALI_PROT_CTL_REG_ARPROT(x)	CTL_BIT_SET(x, 5,3)

/* Clocks */
#define MMMALI_CCH_0       0
#define MMMALI_CCH_ON(id, divider) CLKCH_ON(MALI_LCRU + LCRU_CMU0 + LCRU_CLKCH_OFFSET(id), divider)

static const PllCtlInitValues MALI_LCRU_PLL_INIT = {
	0, 0, 0x68, 0x00, 0x00, 0x2c, 0x00, 0x2c
};

void mmmali_on(void)
{
	uint32_t gpr;
	/* 1200MHz */
	pll_on(MALI_LCRU, &MALI_LCRU_PLL_INIT, "MALI_LCRU timeout!");
	/* Clock channels */
	MMMALI_CCH_ON(MMMALI_CCH_0, 2); /* 600MHz */
	/* Remove reset off */
	gpr = mmio_read_32(MMMALI_RST);
	gpr &= ~(
		MMMALI_RST_NIC |
		MMMALI_RST_PVT
		);
	mmio_write_32(MMMALI_RST, gpr);
}

void mmmali_toNSW(void)
{
	/* Protection control */
	mmio_write_32(MMMALI_PROT_CTL0_REG,
			MMMALI_PROT_CTL_REG_AWPROT(2) |
			MMMALI_PROT_CTL_REG_ARPROT(2));
	mmio_write_32(MMMALI_PROT_CTL1_REG,
			MMMALI_PROT_CTL_REG_AWPROT(2) |
			MMMALI_PROT_CTL_REG_ARPROT(2));
	/* Secure mode */
	SECURE_MODE(BAIKAL_NIC_MALI_TCU, SECURE_MODE_OFF);
	SECURE_MODE(BAIKAL_NIC_MALI, SECURE_MODE_OFF);
}
