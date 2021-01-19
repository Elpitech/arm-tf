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

#define MMUSB_GPR(ind) (USB_LCRU + LCRU_GPR + ind)

// Resets
#define MMUSB_RESET MMUSB_GPR(0)
#define MMUSB_RESET_MMU					CTL_BIT(24)
#define MMUSB_RESET_GIC					CTL_BIT(22)
#define MMUSB_RESET_DMAC				CTL_BIT(20)
#define MMUSB_RESET_SATA_CTRL1				CTL_BIT(18)
#define MMUSB_RESET_SATA_CTRL2				CTL_BIT(17)
#define MMUSB_RESET_SATA_PHY				CTL_BIT(16)
#define MMUSB_RESET_USB3				CTL_BIT(12)
#define MMUSB_RESET_USB3_PHY3				CTL_BIT(11)
#define MMUSB_RESET_USB3_PHY2				CTL_BIT(10)
#define MMUSB_RESET_USB3_PHY1				CTL_BIT(9)
#define MMUSB_RESET_USB3_PHY0				CTL_BIT(8)
#define MMUSB_RESET_USB2				CTL_BIT(6)
#define MMUSB_RESET_USB2_PHY1				CTL_BIT(5)
#define MMUSB_RESET_USB2_PHY0				CTL_BIT(4)
#define MMUSB_RESET_NIC400_FNC_MSTR			CTL_BIT(3)
#define MMUSB_RESET_NIC400_FNC_SLV			CTL_BIT(2)
#define MMUSB_RESET_NIC400_CFG_MSTR			CTL_BIT(1)
#define MMUSB_RESET_NIC400_CFG_SLV			CTL_BIT(0)

// AXI
#define MMUSB_SATA0_AXI_SB	MMUSB_GPR(0x10)
#define MMUSB_SATA0_AXI		MMUSB_GPR(0x40)

#define MMUSB_SATA1_AXI_SB	MMUSB_GPR(0x18)
#define MMUSB_SATA1_AXI		MMUSB_GPR(0x48)

#define MMUSB_USB2_AXI_SB	MMUSB_GPR(0x20)
#define MMUSB_USB2_AXI		MMUSB_GPR(0x50)

#define MMUSB_USB3_AXI_SB	MMUSB_GPR(0x28)
#define MMUSB_USB3_AXI		MMUSB_GPR(0x58)

#define MMUSB_DMA330_AXI_SB	MMUSB_GPR(0x30)

// USB2
#define MMUSB_USB2_PHY_CTL	MMUSB_GPR(0xC0)
#define MMUSB_USB2_PHY_CTL_PHS1FSEL_MASK	CTL_BIT_MASK(20,18)
#define MMUSB_USB2_PHY_CTL_PHS1FSEL(x)		CTL_BIT_SET(x, 20,18)
#define MMUSB_USB2_PHY_CTL_PHS1RCLS_MASK	CTL_BIT_MASK(17, 16)
#define MMUSB_USB2_PHY_CTL_PHS1RCLS(x)		CTL_BIT_SET(x, 17, 16)
#define MMUSB_USB2_PHY_CTL_PHS0FSEL_MASK	CTL_BIT_MASK(4,2)
#define MMUSB_USB2_PHY_CTL_PHS0FSEL(x)		CTL_BIT_SET(x, 4,2)
#define MMUSB_USB2_PHY_CTL_PHS0RCLS_MASK	CTL_BIT_MASK(1,0)
#define MMUSB_USB2_PHY_CTL_PHS0RCLS(x)		CTL_BIT_SET(x, 1,0)
#define USB2_PHY_REFCLK_CRYSTAL   0x0
#define USB2_PHY_REFCLK_EXTERNAL  0x1
#define USB2_PHY_REFCLK_CLKCORE   0x2
#define USB2_PHY_FSEL_12MHZ       0x2
#define USB2_PHY_FSEL_50MHZ       0x7
#define USB2_PHY_REFCLK_VALUE         USB2_PHY_REFCLK_CLKCORE
#define USB2_PHY_FSEL_VALUE           USB2_PHY_FSEL_50MHZ      /* picoPHY use clock core 50 MHz*/

// USB3
#define MMUSB_USB3_PHY0_CTL	MMUSB_GPR(0xE0)
#define MMUSB_USB3_PHY1_CTL	MMUSB_GPR(0x100)
#define MMUSB_USB3_PHY_CTL_CGMEN		CTL_BIT(29)
#define MMUSB_USB3_PHY_CTL_CGPEN		CTL_BIT(28)
#define MMUSB_USB3_PHY_CTL_SSCRANGE_MASK	CTL_BIT_MASK(27, 25)
#define MMUSB_USB3_PHY_CTL_SSCRANGE(x)		CTL_BIT_SET(x, 27, 25)
#define MMUSB_USB3_PHY_CTL_SSCRCLKS_MASK	CTL_BIT_MASK(24, 16)
#define MMUSB_USB3_PHY_CTL_SSCRCLKS(x)		CTL_BIT_SET(x, 24, 16)
#define MMUSB_USB3_PHY_CTL_REFSSPEN		CTL_BIT(15)
#define MMUSB_USB3_PHY_CTL_REFDIV2		CTL_BIT(14)
#define MMUSB_USB3_PHY_CTL_FSEL_MASK		CTL_BIT_MASK(13, 8)
#define MMUSB_USB3_PHY_CTL_FSEL(x)		CTL_BIT_SET(x, 13, 8)
#define MMUSB_USB3_PHY_CTL_MPLLMUL_MASK		CTL_BIT_MASK( 7, 1)
#define MMUSB_USB3_PHY_CTL_MPLLMUL(x)		CTL_BIT_SET(x, 7, 1)
#define MMUSB_USB3_PHY_CTL_REFPAD		CTL_BIT(0)

// Clock channels
#define MMUSB_CCH_SATA_PHY_REF       0
#define MMUSB_CCH_SATA_ACLK_CTRL0    1
#define MMUSB_CCH_SATA_ACLK_CTRL1    2
#define MMUSB_CCH_USB2_PHY0_REF      3
#define MMUSB_CCH_USB2_PHY1_REF      4
#define MMUSB_CCH_USB2_ACLK          5
#define MMUSB_CCH_USB2_CLK_SOFITP    6
#define MMUSB_CCH_USB3_PHY0_REF      7
#define MMUSB_CCH_USB3_PHY1_REF      8
#define MMUSB_CCH_USB3_PHY2_REF      9
#define MMUSB_CCH_USB3_PHY3_REF      10
#define MMUSB_CCH_USB3_ACLK          11
#define MMUSB_CCH_USB3_CLK_SOFITP    12
#define MMUSB_CCH_USB3_CLK_SUSPEND   13
#define MMUSB_CCH_MMU_ACLK           14
#define MMUSB_CCH_DMAC_ACLK          15
#define MMUSB_CCH_GIC_ACLK           16

#define MMUSB_CCH_ON(id, divider) CLKCH_ON(USB_LCRU + LCRU_CMU0 + LCRU_CLKCH_OFFSET(id), divider)

// 800MHz
static const PllCtlInitValues USB_LCRU_PLL_INIT = {
	1, 0, 0x80, 0x00, 0x00, 0x2c, 0x00, 0x2c
};

void mmusb_on(void)
{
	uint32_t mmusb_gpr0, reg;
	// PLL 800MHz
	pll_on(USB_LCRU, &USB_LCRU_PLL_INIT, "USB_LCRU timeout!");
	// Clock channels
	MMUSB_CCH_ON(MMUSB_CCH_SATA_ACLK_CTRL0 , 8 ); // 100MHz differrential reference clock
	MMUSB_CCH_ON(MMUSB_CCH_SATA_ACLK_CTRL1 , 8 ); // 100Mhz axi clk controller #0 (50-100Mhz)
	MMUSB_CCH_ON(MMUSB_CCH_SATA_PHY_REF    , 8 ); // 100Mhz axi clk controller #1 (50-100Mhz)
	MMUSB_CCH_ON(MMUSB_CCH_USB2_PHY0_REF   , 16); // 50MHz  max reference phy #0 clock
	MMUSB_CCH_ON(MMUSB_CCH_USB2_PHY1_REF   , 16); // 50MHz  max reference phy #1 clock
	MMUSB_CCH_ON(MMUSB_CCH_USB2_ACLK       , 16); // 50MHz  axi clock
	MMUSB_CCH_ON(MMUSB_CCH_USB2_CLK_SOFITP , 8 ); // 100MHZ reference clock for SOF and ITP counter (16.129 - 125MHZ)
	MMUSB_CCH_ON(MMUSB_CCH_USB3_PHY0_REF   , 8 ); // 100MHz differrential reference clock phy #0
	MMUSB_CCH_ON(MMUSB_CCH_USB3_PHY1_REF   , 8 ); // 100MHz differrential reference clock phy #1
	MMUSB_CCH_ON(MMUSB_CCH_USB3_PHY2_REF   , 16); // 50MHz  max reference phy #2 clock
	MMUSB_CCH_ON(MMUSB_CCH_USB3_PHY3_REF   , 16); // 50MHz  max reference phy #3 clock
	MMUSB_CCH_ON(MMUSB_CCH_USB3_ACLK       , 6 ); // 133MHz axi clock
	MMUSB_CCH_ON(MMUSB_CCH_USB3_CLK_SOFITP , 8 ); // 100MHZ reference clock for SOF and ITP counter (16.129 - 125MHZ)
	MMUSB_CCH_ON(MMUSB_CCH_USB3_CLK_SUSPEND, 8 ); // 100MHZ suspend clock for low power state (P3)  (32Khz  - 125MHZ)
	MMUSB_CCH_ON(MMUSB_CCH_MMU_ACLK        , 2 ); // 400Mhz axi clk (200 - 400 Mhz)
	MMUSB_CCH_ON(MMUSB_CCH_DMAC_ACLK       , 4 ); // 200MHz - temp value (250 Mhz axi clk (100 - 250 Mhz))
	MMUSB_CCH_ON(MMUSB_CCH_GIC_ACLK        , 5 ); // 160Mhz

	// USB3 Phy clocks
	reg = mmio_read_32(MMUSB_USB3_PHY0_CTL);
	reg &=	~(
		MMUSB_USB3_PHY_CTL_CGMEN	|
		MMUSB_USB3_PHY_CTL_CGPEN
		);
	reg |= MMUSB_USB3_PHY_CTL_REFPAD;
	mmio_write_32(MMUSB_USB3_PHY0_CTL, reg);
	WAIT_DELAY(1, 1000, ); // delay 1us
	reg |= MMUSB_USB3_PHY_CTL_REFSSPEN;
	mmio_write_32(MMUSB_USB3_PHY0_CTL, reg);
	reg = mmio_read_32(MMUSB_USB3_PHY1_CTL);
	reg &=	~(
		MMUSB_USB3_PHY_CTL_CGMEN	|
		MMUSB_USB3_PHY_CTL_CGPEN
		);
	reg |= MMUSB_USB3_PHY_CTL_REFPAD;
	mmio_write_32(MMUSB_USB3_PHY1_CTL, reg);
	WAIT_DELAY(1, 1000, ); // delay 1us
	reg |= MMUSB_USB3_PHY_CTL_REFSSPEN;
	mmio_write_32(MMUSB_USB3_PHY1_CTL, reg);

	// USB2 Phy clocks
	reg = mmio_read_32(MMUSB_USB2_PHY_CTL);
	reg &=	~(MMUSB_USB2_PHY_CTL_PHS1RCLS_MASK |
		MMUSB_USB2_PHY_CTL_PHS1FSEL_MASK |
		MMUSB_USB2_PHY_CTL_PHS0RCLS_MASK |
		MMUSB_USB2_PHY_CTL_PHS0FSEL_MASK);
	reg |=	MMUSB_USB2_PHY_CTL_PHS1RCLS(USB2_PHY_REFCLK_VALUE) |
		MMUSB_USB2_PHY_CTL_PHS1FSEL(USB2_PHY_FSEL_VALUE) |
		MMUSB_USB2_PHY_CTL_PHS0RCLS(USB2_PHY_REFCLK_VALUE) |
		MMUSB_USB2_PHY_CTL_PHS0FSEL(USB2_PHY_FSEL_VALUE);
	mmio_write_32(MMUSB_USB2_PHY_CTL, reg);

	// remove resets
	mmusb_gpr0 = mmio_read_32(MMUSB_RESET);
	mmusb_gpr0 &= ~(
			MMUSB_RESET_MMU			|
			MMUSB_RESET_GIC			|
//			MMUSB_RESET_DMAC		|
			MMUSB_RESET_SATA_CTRL1		|
			MMUSB_RESET_SATA_CTRL2		|
			MMUSB_RESET_SATA_PHY		|
			MMUSB_RESET_USB3		|
			MMUSB_RESET_USB2		|
			MMUSB_RESET_NIC400_FNC_MSTR	|
			MMUSB_RESET_NIC400_FNC_SLV	|
			MMUSB_RESET_NIC400_CFG_MSTR	|
			MMUSB_RESET_NIC400_CFG_SLV
			);
	mmio_write_32(MMUSB_RESET, mmusb_gpr0);

	WAIT_DELAY(1, 50000, ); // delay 50us

	mmusb_gpr0 &= ~(
			MMUSB_RESET_USB3_PHY3		|
			MMUSB_RESET_USB3_PHY2		|
			MMUSB_RESET_USB3_PHY1		|
			MMUSB_RESET_USB3_PHY0		|
			MMUSB_RESET_USB2_PHY1		|
			MMUSB_RESET_USB2_PHY0
			);
	mmio_write_32(MMUSB_RESET, mmusb_gpr0);
}

void mmusb_chc(void)
{
	// SATA0
        mmio_write_32(MMUSB_SATA0_AXI_SB, 0x0A0000);    // ARDOMAIN 2 / AWDOMAIN 2 (0xa0000 = Coherent ReadOnce + WriteUnique)
        mmio_write_32(MMUSB_SATA0_AXI, 0x0b70012);	// ARCACHE 7, AWCACHE 7, AxPROT = 2
	// SATA1
        mmio_write_32(MMUSB_SATA1_AXI_SB, 0x0A0000);    // ARDOMAIN 2 / AWDOMAIN 2 (0xa0000 = Coherent ReadOnce + WriteUnique)
        mmio_write_32(MMUSB_SATA1_AXI, 0x0b70012);	// ARCACHE 7, AWCACHE 7, AxPROT = 2
	// USB2
        mmio_write_32(MMUSB_USB2_AXI_SB, 0x0A0000);	// ARDOMAIN 2 / AWDOMAIN 2 (0xa0000 = Coherent ReadOnce + WriteUnique)
        mmio_write_32(MMUSB_USB2_AXI, 0x0000012);	// AxPROT = 2
	// USB3
        mmio_write_32(MMUSB_USB3_AXI_SB, 0x0A0000);	// ARDOMAIN 2 / AWDOMAIN 2 (0xa0000 = Coherent ReadOnce + WriteUnique)
        mmio_write_32(MMUSB_USB3_AXI, 0x0000012);	// AxPROT = 2
}

// Prepare for non secure world (EFI, rich os)
void mmusb_toNSW(void)
{
	SECURE_MODE(BAIKAL_NIC_USB_TCU, SECURE_MODE_OFF);
	SECURE_MODE(BAIKAL_NIC_USB_0,	SECURE_MODE_OFF);
	SECURE_MODE(BAIKAL_NIC_USB_1,	SECURE_MODE_OFF);
	SECURE_MODE(BAIKAL_NIC_SATA_0,	SECURE_MODE_OFF);
	SECURE_MODE(BAIKAL_NIC_SATA_1,	SECURE_MODE_OFF);
	SECURE_MODE(BAIKAL_NIC_USB_GIC, SECURE_MODE_OFF);
//	SECURE_MODE(BAIKAL_NIC_USB_DMA330_0, SECURE_MODE_OFF);
//	SECURE_MODE(BAIKAL_NIC_USB_DMA330_1, SECURE_MODE_OFF);
}

//////////////////////////////////////////////////////
// SATA tests
#define SATA_0			0x2c600000
#define SATA_1			0x2c610000

#define SATA_GHC		0x4
#define SATA_GHC_RESET		CTL_BIT(0)

#define SATA_TIMER1MS		0xe0

#define SATA_CAP		0x0
#define SATA_CAP_SMPS		CTL_BIT(28)
#define SATA_CAP_SSS		CTL_BIT(27)

#define SATA_PI			0x0c

#define SATA_PORT0_SCTL		0x12c
#define SATA_PORT0_STS		0x128

int wait_break_on = 1;
void wait_break(void) {
	while(wait_break_on) {} ;
}

void mmusb_init_sata_ch(uintptr_t base)
{
	uint32_t reg;
//	ERROR("SATA!\n");
//	wait_break();
//	WAIT_SEC_VERBOSE(3);

	reg = mmio_read_32(base + SATA_GHC);
	reg |= SATA_GHC_RESET;
	mmio_write_32(base + SATA_GHC, reg);
	WAIT_DELAY(
		((mmio_read_32(base + SATA_GHC) & SATA_GHC_RESET) == 1),
		100000, // 100ms
		WARN("SATA_GHC 0x%lx timeout!\n", base)
		);

	reg = mmio_read_32(base + SATA_CAP);
	reg |= SATA_CAP_SMPS;
	reg &= ~SATA_CAP_SSS;
	mmio_write_32(base + SATA_CAP, reg);

	mmio_write_32(base + SATA_TIMER1MS, 150*1000); // timer SATA 150MHz
	mmio_write_32(base + SATA_PI, 1);		 // 1 port

	mmio_write_32(base + SATA_PORT0_SCTL, 0x10);
	WAIT_DELAY(
		((mmio_read_32(base + SATA_PORT0_STS) & 0x11) == 0),
		1000000, // 1000ms
		WARN("SATA_STS 0x%lx timeout!\n", base)
		);

// SMMU
//        reg = mmio_read_32(0x2c080000);
//	reg =   0xa5f0001;
//	mmio_write_32(0x2c080000, reg);

//	reg =   0xa5f0001;
//	mmio_write_32(0x2c080400, reg);
}

void mmusb_initSATA(void)
{
	mmusb_init_sata_ch(SATA_0);
	mmusb_init_sata_ch(SATA_1);
}
