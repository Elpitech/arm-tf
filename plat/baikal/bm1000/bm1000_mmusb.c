/*
 * Copyright (c) 2018-2022, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>

#include <bm1000_cmu.h>
#include <bm1000_def.h>
#include <bm1000_private.h>

#include "bm1000_mmusb.h"

void mmusb_init_sata_ch(const uintptr_t base)
{
	uint64_t timeout;

	mmio_setbits_32(base + SATA_GHC, SATA_GHC_RESET);
	timeout = timeout_init_us(100);
	while (mmio_read_32(base + SATA_GHC) & SATA_GHC_RESET) {
		if (timeout_elapsed(timeout)) {
			ERROR("%s(0x%lx): GHC.RESET timeout\n", __func__, base);
			break;
		}
	}

	mmio_clrsetbits_32(base + SATA_CAP,
			   SATA_CAP_SSS,
			   SATA_CAP_SMPS);

	/* SATA timer 150 MHz */
	mmio_write_32(base + SATA_TIMER1MS, 150 * 1000);
	mmio_write_32(base + SATA_PI, 1); /* 1 port */
	mmio_write_32(base + SATA_PORT0_SCTL, 0x10);
	timeout = timeout_init_us(1000);
	while (!(mmio_read_32(base + SATA_PORT0_STS) & 0x11) &&
	       !timeout_elapsed(timeout));
}

void mmusb_init_usb_global(void)
{
	uint32_t raw = mmio_read_32(MMUSB_USB3_G_PIPE_CTL0);
	usb3_g_pipe_t *pipe = (void*) &raw;

	pipe-> dis_rx_det_p3 = 1;

	mmio_write_32(MMUSB_USB3_G_PIPE_CTL0, raw);
	mmio_write_32(MMUSB_USB3_G_PIPE_CTL1, raw);
}

void mmusb_init(void)
{
	uint32_t mmusb_gpr0, reg;

	/* PLL 800 MHz */
	const cmu_pll_ctl_vals_t mmusb_cmu0_pll_ctls = {
		1, 0, 0x80, 0, 0, 0x2c, 0, 0x2c
	};

	cmu_pll_on(MMUSB_CMU0_BASE, &mmusb_cmu0_pll_ctls);

	cmu_clkch_enable_by_base(MMUSB_CMU0_CLKCHCTL_SATA_REF,       8); /* 100 MHz axi clk controller #1 (50 - 100 MHz) */
	cmu_clkch_enable_by_base(MMUSB_CMU0_CLKCHCTL_SATA0_AXI,      8); /* 100 MHz differential reference clock */
	cmu_clkch_enable_by_base(MMUSB_CMU0_CLKCHCTL_SATA1_AXI,      8); /* 100 MHz axi clk controller #0 (50 - 100 MHz) */
	cmu_clkch_enable_by_base(MMUSB_CMU0_CLKCHCTL_USB2_PHY0_REF, 16); /*  50 MHz max reference PHY #0 clock */
	cmu_clkch_enable_by_base(MMUSB_CMU0_CLKCHCTL_USB2_PHY1_REF, 16); /*  50 MHz max reference PHY #1 clock */
	cmu_clkch_enable_by_base(MMUSB_CMU0_CLKCHCTL_USB2_AXI,	    16); /*  50 MHz axi clock */
	cmu_clkch_enable_by_base(MMUSB_CMU0_CLKCHCTL_USB2_SOFITP,    8); /* 100 MHz reference clock for SOF and ITP counter (16.129 - 125 MHz) */
	cmu_clkch_enable_by_base(MMUSB_CMU0_CLKCHCTL_USB3_PHY0_REF,  8); /* 100 MHz differential reference clock PHY #0 */
	cmu_clkch_enable_by_base(MMUSB_CMU0_CLKCHCTL_USB3_PHY1_REF,  8); /* 100 MHz differential reference clock PHY #1 */
	cmu_clkch_enable_by_base(MMUSB_CMU0_CLKCHCTL_USB3_PHY2_REF, 16); /*  50 MHz max reference PHY #2 clock */
	cmu_clkch_enable_by_base(MMUSB_CMU0_CLKCHCTL_USB3_PHY3_REF, 16); /*  50 MHz max reference PHY #3 clock */
	cmu_clkch_enable_by_base(MMUSB_CMU0_CLKCHCTL_USB3_AXI,	     6); /* 133 MHz axi clock */
	cmu_clkch_enable_by_base(MMUSB_CMU0_CLKCHCTL_USB3_SOFITP,    8); /* 100 MHz reference clock for SOF and ITP counter (16.129 - 125 MHz) */
	cmu_clkch_enable_by_base(MMUSB_CMU0_CLKCHCTL_USB3_SUSPEND,   8); /* 100 MHz suspend clock for low power state (P3) (32 kHz - 125 MHz) */
	cmu_clkch_enable_by_base(MMUSB_CMU0_CLKCHCTL_SMMU,	     2); /* 400 MHz axi clk (200 - 400 MHz) */
	cmu_clkch_enable_by_base(MMUSB_CMU0_CLKCHCTL_DMAC,	     4); /* 200 MHz - temp value (250 MHz axi clk (100 - 250 MHz)) */
	cmu_clkch_enable_by_base(MMUSB_CMU0_CLKCHCTL_GIC,	     5); /* 160 MHz */

	/* SATA clocks */
	reg = mmio_read_32(MMUSB_SATA_PHY_CLK);
	reg &= ~(MMUSB_SATA_PHY_CLK_REFSSPEN |
		 MMUSB_SATA_PHY_CLK_CGMEN |
		 MMUSB_SATA_PHY_CLK_CGPEN);
	reg |= MMUSB_SATA_PHY_CLK_REFPAD;
	mmio_write_32(MMUSB_SATA_PHY_CLK, reg);
	udelay(1);
	reg |= MMUSB_SATA_PHY_CLK_REFSSPEN;
	mmio_write_32(MMUSB_SATA_PHY_CLK, reg);

	/* USB3 PHY clocks */
	reg = mmio_read_32(MMUSB_USB3_PHY0_CTL0);
	reg &= ~(MMUSB_USB3_PHY_CTL0_CGMEN |
		 MMUSB_USB3_PHY_CTL0_CGPEN);
	reg |= MMUSB_USB3_PHY_CTL0_REFPAD;
	mmio_write_32(MMUSB_USB3_PHY0_CTL0, reg);
	udelay(1);
	reg |= MMUSB_USB3_PHY_CTL0_REFSSPEN;
	mmio_write_32(MMUSB_USB3_PHY0_CTL0, reg);

	reg = mmio_read_32(MMUSB_USB3_PHY1_CTL0);
	reg &= ~(MMUSB_USB3_PHY_CTL0_CGMEN |
		 MMUSB_USB3_PHY_CTL0_CGPEN);
	reg |= MMUSB_USB3_PHY_CTL0_REFPAD;
	mmio_write_32(MMUSB_USB3_PHY1_CTL0, reg);
	udelay(1);
	reg |= MMUSB_USB3_PHY_CTL0_REFSSPEN;
	mmio_write_32(MMUSB_USB3_PHY1_CTL0, reg);

	/* USB2 PHY clocks */
	mmio_clrsetbits_32(MMUSB_USB2_PHY_CTL,
			   MMUSB_USB2_PHY_CTL_PHS0RCLS_MASK |
			   MMUSB_USB2_PHY_CTL_PHS0FSEL_MASK |
			   MMUSB_USB2_PHY_CTL_PHS1RCLS_MASK |
			   MMUSB_USB2_PHY_CTL_PHS1FSEL_MASK,
			   MMUSB_USB2_PHY_CTL_PHS0RCLS(USB2_PHY_REFCLK_VALUE) |
			   MMUSB_USB2_PHY_CTL_PHS0FSEL(USB2_PHY_FSEL_VALUE)   |
			   MMUSB_USB2_PHY_CTL_PHS1RCLS(USB2_PHY_REFCLK_VALUE) |
			   MMUSB_USB2_PHY_CTL_PHS1FSEL(USB2_PHY_FSEL_VALUE));

	/* Enable DMAC non-secure manager thread */
	mmio_setbits_32(MMUSB_DMAC_BOOT_MNGR_NS,
			MMUSB_DMAC_BOOT_MNGR_NS_MNGNS);
	/* Enable DMAC non-secure interrupts */
	mmio_setbits_32(MMUSB_DMAC_IRQ_NS,
			MMUSB_DMAC_IRQ_NS_IRQNS_MASK);

	/* Deassert reset signals */
	mmusb_gpr0 = mmio_read_32(MMUSB_RESET);
	mmusb_gpr0 &= ~(MMUSB_RESET_SMMU	    |
			MMUSB_RESET_GIC		    |
			MMUSB_RESET_DMAC	    |
			MMUSB_RESET_SATA_CTRL1	    |
			MMUSB_RESET_SATA_CTRL2	    |
			MMUSB_RESET_SATA_PHY	    |
			MMUSB_RESET_USB3	    |
			MMUSB_RESET_USB2	    |
			MMUSB_RESET_NIC400_FNC_MSTR |
			MMUSB_RESET_NIC400_FNC_SLV  |
			MMUSB_RESET_NIC400_CFG_MSTR |
			MMUSB_RESET_NIC400_CFG_SLV);
	mmio_write_32(MMUSB_RESET, mmusb_gpr0);
	udelay(50);
	mmusb_gpr0 &= ~(MMUSB_RESET_USB3_PHY3 |
			MMUSB_RESET_USB3_PHY2 |
			MMUSB_RESET_USB3_PHY1 |
			MMUSB_RESET_USB3_PHY0 |
			MMUSB_RESET_USB2_PHY1 |
			MMUSB_RESET_USB2_PHY0);
	mmio_write_32(MMUSB_RESET, mmusb_gpr0);

	/* Enable non-secure access */
	mmio_write_32(MMUSB_NIC_CFG_GPV_REGIONSEC_USB2,	 NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(MMUSB_NIC_CFG_GPV_REGIONSEC_USB3,	 NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(MMUSB_NIC_CFG_GPV_REGIONSEC_SATA0, NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(MMUSB_NIC_CFG_GPV_REGIONSEC_SATA1, NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(MMUSB_NIC_CFG_GPV_REGIONSEC_SMMU,	 NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(MMUSB_NIC_CFG_GPV_REGIONSEC_GIC,	 NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(MMUSB_NIC_CFG_GPV_REGIONSEC_DMACN, NIC_GPV_REGIONSEC_NONSECURE);

	/* CHC */
	/* SATA0 */
	mmio_write_32(MMUSB_SATA0_AXI_SB, 0xa0000);  /* ARDOMAIN 2 / AWDOMAIN 2 (0xa0000 = Coherent ReadOnce + WriteUnique) */
	mmio_write_32(MMUSB_SATA0_AXI,	  0xb70012); /* ARCACHE 7, AWCACHE 7, AxPROT = 2 */
	/* SATA1 */
	mmio_write_32(MMUSB_SATA1_AXI_SB, 0xa0000);  /* ARDOMAIN 2 / AWDOMAIN 2 (0xa0000 = Coherent ReadOnce + WriteUnique) */
	mmio_write_32(MMUSB_SATA1_AXI,	  0xb70012); /* ARCACHE 7, AWCACHE 7, AxPROT = 2 */
	/* USB2 */
	mmio_write_32(MMUSB_USB2_AXI_SB,  0xa0000);  /* ARDOMAIN 2 / AWDOMAIN 2 (0xa0000 = Coherent ReadOnce + WriteUnique) */
	mmio_write_32(MMUSB_USB2_AXI,	  0x000012); /* AxPROT = 2 */
	/* USB3 */
	mmio_write_32(MMUSB_USB3_AXI_SB,  0xa0000);  /* ARDOMAIN 2 / AWDOMAIN 2 (0xa0000 = Coherent ReadOnce + WriteUnique) */
	mmio_write_32(MMUSB_USB3_AXI,	  0x000012); /* AxPROT = 2 */

	/* Init SATA */
	mmusb_init_sata_ch(MMUSB_SATA0_BASE);
	mmusb_init_sata_ch(MMUSB_SATA1_BASE);

	/* Init USB */
	mmusb_init_usb_global();
}
