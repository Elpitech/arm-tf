/*
 * Copyright (c) 2018-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>

#include <bm1000_cmu.h>
#include <bm1000_def.h>
#include <bm1000_private.h>

#define MMUSB_RESET				(MMUSB_GPR_BASE + 0x00)
#define MMUSB_RESET_NIC400_CFG_SLV		BIT(0)
#define MMUSB_RESET_NIC400_CFG_MSTR		BIT(1)
#define MMUSB_RESET_NIC400_FNC_SLV		BIT(2)
#define MMUSB_RESET_NIC400_FNC_MSTR		BIT(3)
#define MMUSB_RESET_USB2_PHY0			BIT(4)
#define MMUSB_RESET_USB2_PHY1			BIT(5)
#define MMUSB_RESET_USB2			BIT(6)
#define MMUSB_RESET_USB3_PHY0			BIT(8)
#define MMUSB_RESET_USB3_PHY1			BIT(9)
#define MMUSB_RESET_USB3_PHY2			BIT(10)
#define MMUSB_RESET_USB3_PHY3			BIT(11)
#define MMUSB_RESET_USB3			BIT(12)
#define MMUSB_RESET_SATA_PHY			BIT(16)
#define MMUSB_RESET_SATA_CTRL2			BIT(17)
#define MMUSB_RESET_SATA_CTRL1			BIT(18)
#define MMUSB_RESET_DMAC			BIT(20)
#define MMUSB_RESET_GIC				BIT(22)
#define MMUSB_RESET_SMMU			BIT(24)

/* AXI */
#define MMUSB_SATA0_AXI_SB			(MMUSB_GPR_BASE + 0x10)
#define MMUSB_SATA0_AXI				(MMUSB_GPR_BASE + 0x40)
#define MMUSB_SATA1_AXI_SB			(MMUSB_GPR_BASE + 0x18)
#define MMUSB_SATA1_AXI				(MMUSB_GPR_BASE + 0x48)
#define MMUSB_USB2_AXI_SB			(MMUSB_GPR_BASE + 0x20)
#define MMUSB_USB2_AXI				(MMUSB_GPR_BASE + 0x50)
#define MMUSB_USB3_AXI_SB			(MMUSB_GPR_BASE + 0x28)
#define MMUSB_USB3_AXI				(MMUSB_GPR_BASE + 0x58)
#define MMUSB_DMA330_AXI_SB			(MMUSB_GPR_BASE + 0x30)

/* SATA */
#define MMUSB_SATA_PHY_CLK			(MMUSB_GPR_BASE + 0x80)
#define MMUSB_SATA_PHY_CLK_REFPAD		BIT(0)
#define MMUSB_SATA_PHY_CLK_CGPEN		BIT(1)
#define MMUSB_SATA_PHY_CLK_CGMEN		BIT(2)
#define MMUSB_SATA_PHY_CLK_REFSSPEN		BIT(12)

/* USB2 */
#define MMUSB_USB2_PHY_CTL			(MMUSB_GPR_BASE + 0xc0)
#define MMUSB_USB2_PHY_CTL_PHS0RCLS_MASK	GENMASK(    1,  0)
#define MMUSB_USB2_PHY_CTL_PHS0RCLS(x)		SETMASK(x,  1,  0)
#define MMUSB_USB2_PHY_CTL_PHS0FSEL_MASK	GENMASK(    4,  2)
#define MMUSB_USB2_PHY_CTL_PHS0FSEL(x)		SETMASK(x,  4,  2)
#define MMUSB_USB2_PHY_CTL_PHS1RCLS_MASK	GENMASK(   17, 16)
#define MMUSB_USB2_PHY_CTL_PHS1RCLS(x)		SETMASK(x, 17, 16)
#define MMUSB_USB2_PHY_CTL_PHS1FSEL_MASK	GENMASK(   20, 18)
#define MMUSB_USB2_PHY_CTL_PHS1FSEL(x)		SETMASK(x, 20, 18)
#define USB2_PHY_REFCLK_CRYSTAL			0x0
#define USB2_PHY_REFCLK_EXTERNAL		0x1
#define USB2_PHY_REFCLK_CLKCORE			0x2
#define USB2_PHY_FSEL_12MHZ			0x2
#define USB2_PHY_FSEL_50MHZ			0x7
#define USB2_PHY_REFCLK_VALUE			USB2_PHY_REFCLK_CLKCORE
#define USB2_PHY_FSEL_VALUE			USB2_PHY_FSEL_50MHZ /* picoPHY uses clock core 50 MHz */

/* USB3 */
#define MMUSB_USB3_PHY0_CTL			(MMUSB_GPR_BASE + 0xe0)
#define MMUSB_USB3_PHY1_CTL			(MMUSB_GPR_BASE + 0x100)
#define MMUSB_USB3_PHY_CTL_REFPAD		BIT(0)
#define MMUSB_USB3_PHY_CTL_MPLLMUL_MASK		GENMASK(    7,  1)
#define MMUSB_USB3_PHY_CTL_MPLLMUL(x)		SETMASK(x,  7,  1)
#define MMUSB_USB3_PHY_CTL_FSEL_MASK		GENMASK(   13,  8)
#define MMUSB_USB3_PHY_CTL_FSEL(x)		SETMASK(x, 13,  8)
#define MMUSB_USB3_PHY_CTL_REFDIV2		BIT(14)
#define MMUSB_USB3_PHY_CTL_REFSSPEN		BIT(15)
#define MMUSB_USB3_PHY_CTL_SSCRCLKS_MASK	GENMASK(   24, 16)
#define MMUSB_USB3_PHY_CTL_SSCRCLKS(x)		SETMASK(x, 24, 16)
#define MMUSB_USB3_PHY_CTL_SSCRANGE_MASK	GENMASK(   27, 25)
#define MMUSB_USB3_PHY_CTL_SSCRANGE(x)		SETMASK(x, 27, 25)
#define MMUSB_USB3_PHY_CTL_CGPEN		BIT(28)
#define MMUSB_USB3_PHY_CTL_CGMEN		BIT(29)

static void mmusb_init_sata_ch(const uintptr_t base);

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
	reg = mmio_read_32(MMUSB_USB3_PHY0_CTL);
	reg &= ~(MMUSB_USB3_PHY_CTL_CGMEN |
		 MMUSB_USB3_PHY_CTL_CGPEN);

	reg |= MMUSB_USB3_PHY_CTL_REFPAD;
	mmio_write_32(MMUSB_USB3_PHY0_CTL, reg);
	udelay(1);
	reg |= MMUSB_USB3_PHY_CTL_REFSSPEN;
	mmio_write_32(MMUSB_USB3_PHY0_CTL, reg);

	reg = mmio_read_32(MMUSB_USB3_PHY1_CTL);
	reg &= ~(MMUSB_USB3_PHY_CTL_CGMEN |
		 MMUSB_USB3_PHY_CTL_CGPEN);

	reg |= MMUSB_USB3_PHY_CTL_REFPAD;
	mmio_write_32(MMUSB_USB3_PHY1_CTL, reg);
	udelay(1);
	reg |= MMUSB_USB3_PHY_CTL_REFSSPEN;
	mmio_write_32(MMUSB_USB3_PHY1_CTL, reg);

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

	/* Deassert reset signals */
	mmusb_gpr0 = mmio_read_32(MMUSB_RESET);
	mmusb_gpr0 &= ~(MMUSB_RESET_SMMU	    |
			MMUSB_RESET_GIC		    |
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
}

#define SATA_GHC		0x4
#define SATA_GHC_RESET		BIT(0)
#define SATA_TIMER1MS		0xe0
#define SATA_CAP		0x0
#define SATA_CAP_SSS		BIT(27)
#define SATA_CAP_SMPS		BIT(28)
#define SATA_PI			0x0c
#define SATA_PORT0_SCTL		0x12c
#define SATA_PORT0_STS		0x128

static void mmusb_init_sata_ch(const uintptr_t base)
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
