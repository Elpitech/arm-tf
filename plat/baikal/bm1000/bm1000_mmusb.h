/*
 * Copyright (c) 2021-2022, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BM1000_MMUSB_H
#define BM1000_MMUSB_H

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

/* DMAC */
#define MMUSB_DMAC_BOOT_MNGR_NS			(MMUSB_GPR_BASE + 0x68)
#define MMUSB_DMAC_BOOT_MNGR_NS_MNGNS		BIT(0)
#define MMUSB_DMAC_IRQ_NS			(MMUSB_GPR_BASE + 0x70)
#define MMUSB_DMAC_IRQ_NS_IRQNS_MASK		GENMASK(    7,  0)

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
#define USB2_PHY_FSEL_VALUE			USB2_PHY_FSEL_50MHZ

#define MMUSB_USB2_PHY_TUNE0			(MMUSB_GPR_BASE + 0xc8)
#define MMUSB_USB2_PHY_TUNE1			(MMUSB_GPR_BASE + 0xd0)


/* USB3 */
#define MMUSB_USB3_PHY0_CTL0			(MMUSB_GPR_BASE + 0xe0)
#define MMUSB_USB3_PHY1_CTL0			(MMUSB_GPR_BASE + 0x100)
#define MMUSB_USB3_PHY_CTL0_REFPAD		BIT(0)
#define MMUSB_USB3_PHY_CTL0_MPLLMUL_MASK	GENMASK(    7,  1)
#define MMUSB_USB3_PHY_CTL0_MPLLMUL(x)		SETMASK(x,  7,  1)
#define MMUSB_USB3_PHY_CTL0_FSEL_MASK		GENMASK(   13,  8)
#define MMUSB_USB3_PHY_CTL0_FSEL(x)		SETMASK(x, 13,  8)
#define MMUSB_USB3_PHY_CTL0_REFDIV2		BIT(14)
#define MMUSB_USB3_PHY_CTL0_REFSSPEN		BIT(15)
#define MMUSB_USB3_PHY_CTL0_SSCRCLKS_MASK	GENMASK(   24, 16)
#define MMUSB_USB3_PHY_CTL0_SSCRCLKS(x)		SETMASK(x, 24, 16)
#define MMUSB_USB3_PHY_CTL0_SSCRANGE_MASK	GENMASK(   27, 25)
#define MMUSB_USB3_PHY_CTL0_SSCRANGE(x)		SETMASK(x, 27, 25)
#define MMUSB_USB3_PHY_CTL0_CGPEN		BIT(28)
#define MMUSB_USB3_PHY_CTL0_CGMEN		BIT(29)

#define MMUSB_USB3_PHY0_CTL1			(MMUSB_GPR_BASE + 0xe8)
#define MMUSB_USB3_PHY0_TUNE0			(MMUSB_GPR_BASE + 0xf0)
#define MMUSB_USB3_PHY0_TUNE1			(MMUSB_GPR_BASE + 0xf8)
#define MMUSB_USB3_PHY0_CRW			(MMUSB_GPR_BASE + 0x120)
#define MMUSB_USB3_PHY0_CRR			(MMUSB_GPR_BASE + 0x124)

#define MMUSB_USB3_PHY1_CTL1			(MMUSB_GPR_BASE + 0x108)
#define MMUSB_USB3_PHY1_TUNE0			(MMUSB_GPR_BASE + 0x110)
#define MMUSB_USB3_PHY1_TUNE1			(MMUSB_GPR_BASE + 0x118)
#define MMUSB_USB3_PHY1_CRW			(MMUSB_GPR_BASE + 0x128)
#define MMUSB_USB3_PHY1_CRR			(MMUSB_GPR_BASE + 0x12c)

/* SATA */
#define SATA_GHC				0x4
#define SATA_GHC_RESET				BIT(0)
#define SATA_TIMER1MS				0xe0
#define SATA_CAP				0x0
#define SATA_CAP_SSS				BIT(27)
#define SATA_CAP_SMPS				BIT(28)
#define SATA_PI					0x0c
#define SATA_PORT0_SCTL				0x12c
#define SATA_PORT0_STS				0x128

/* global */
#define MMUSB_USB3_G_CFG0			(MMUSB_USB3_BASE + 0xc100)
#define MMUSB_USB3_G_CFG1			(MMUSB_USB3_BASE + 0xc104)
#define MMUSB_USB3_G_TX_THR			(MMUSB_USB3_BASE + 0xc108)
#define MMUSB_USB3_G_RX_THR			(MMUSB_USB3_BASE + 0xc10c)
#define MMUSB_USB3_G_CTL			(MMUSB_USB3_BASE + 0xc110)
#define MMUSB_USB3_G_PM_STS			(MMUSB_USB3_BASE + 0xc114)
#define MMUSB_USB3_G_STS			(MMUSB_USB3_BASE + 0xc118)
#define MMUSB_USB3_G_UCTL1			(MMUSB_USB3_BASE + 0xc11c)
#define MMUSB_USB3_G_SNPS_ID			(MMUSB_USB3_BASE + 0xc120)
#define MMUSB_USB3_G_GPIO			(MMUSB_USB3_BASE + 0xc124)
#define MMUSB_USB3_G_UID			(MMUSB_USB3_BASE + 0xc128)
#define MMUSB_USB3_G_UCTL0			(MMUSB_USB3_BASE + 0xc12c)
#define MMUSB_USB3_G_ERR_ADR0			(MMUSB_USB3_BASE + 0xc130)
#define MMUSB_USB3_G_ERR_ADR1			(MMUSB_USB3_BASE + 0xc134)
#define MMUSB_USB3_G_PRT_MAP0			(MMUSB_USB3_BASE + 0xc138)
#define MMUSB_USB3_G_PRT_MAP1			(MMUSB_USB3_BASE + 0xc13c)
#define MMUSB_USB3_G_ERR_INJ1			(MMUSB_USB3_BASE + 0xc194)
#define MMUSB_USB3_G_ERR_INJ2			(MMUSB_USB3_BASE + 0xc198)
#define MMUSB_USB3_G_PIPE_CTL0			(MMUSB_USB3_BASE + 0xc2c0)
#define MMUSB_USB3_G_PIPE_CTL1			(MMUSB_USB3_BASE + 0xc2c4)
#define MMUSB_USB3_G_EVENT_BUF_ADR0		(MMUSB_USB3_BASE + 0xc400)
#define MMUSB_USB3_G_EVENT_BUF_ADR1		(MMUSB_USB3_BASE + 0xc404)
#define MMUSB_USB3_G_EVENT_BUF_SIZE0		(MMUSB_USB3_BASE + 0xc408)
#define MMUSB_USB3_G_EVENT_BUF_SIZE1		(MMUSB_USB3_BASE + 0xc40c)
#define MMUSB_USB3_G_CFLADJ			(MMUSB_USB3_BASE + 0xc630)

typedef struct {
	uint32_t refpad		:1;		// 0
	uint32_t mpllmul	:7-1+1;		// 1
	uint32_t fsel		:13-8+1;	// 13-8
	uint32_t refdiv2	:1;		// 14
	uint32_t refsspen	:1;		// 15
	uint32_t sscrclks	:24-16+1;	// 24-16
	uint32_t sscrange	:27-25+1;	// 27-25
	uint32_t cgpen		:1;		// 28
	uint32_t cgmen		:1;		// 29
	uint32_t _		:31-30+1;	// 31-30
} usb3_phy_ss_ctl0_t;

typedef struct {
	uint32_t txvbstl	:2-0+1;		// 0
	uint32_t _		:1;		// 3
	uint32_t losbias	:6-4+1;		// 4
	uint32_t __		:15-7+1;	// 7
	uint32_t acjtlvl	:20-16+1;	// 16
	uint32_t ___		:26-21+1;	// 21
	uint32_t lpben		:1;		// 27
	uint32_t vaten		:29-28+1;	// 29-28
	uint32_t pwrdhsp	:1;		// 30
	uint32_t pwrdssp	:1;		// 31
} usb3_phy_ss_ctl1_t;

typedef struct {
	uint32_t cmpdistn	:2-0+1;		// 0
	uint32_t otgtn		:5-3+1;		// 3
	uint32_t sqrxtn		:8-6+1;		// 6
	uint32_t txfsltn	:12-9+1;	// 9
	uint32_t txhsxvtn	:14-13+1;	// 13
	uint32_t txpectn	:16-15+1;	// 16
	uint32_t txpeptn	:1;		// 17
	uint32_t txrestn	:19-18+1;	// 18
	uint32_t txrisetn	:21-20+1;	// 20
	uint32_t txvreftn	:25-22+1;	// 22
	uint32_t _		:31-26+1;	// 26
} usb3_phy_ss_tune0_t;

typedef struct {
	uint32_t pcsrxlosm	:9-0+1;		// 0
	uint32_t pcstxde35	:15-10+1;	// 10
	uint32_t pcstxde6	:21-16+1;	// 16
	uint32_t pcstxswf	:28-22+1;	// 22
	uint32_t _		:31-29+1;	// 29
} usb3_phy_ss_tune1_t;

typedef struct {
	uint32_t elastic_buffer_mode			:1;		// 0
	uint32_t tx_de_epphasis				:2-1+1;		// 2:1
	uint32_t tx_margin				:5-3+1;		// 5:3
	uint32_t tx_swing				:1;		// 6
	uint32_t ssic_en				:1;		// 7
	uint32_t rx_detect_to_polling_lfps_control	:1;		// 8
	uint32_t lfps_filter				:1;		// 9
	uint32_t p3_ex_sig_p2				:1;		// 10
	uint32_t p3_p2_tran_ok				:1;		// 11
	uint32_t lfps_p0_align				:1;		// 12
	uint32_t skip_rx_det				:1;		// 13
	uint32_t abort_rx_det_in_u2			:1;		// 14
	uint32_t dat_width				:16-15+1;	// 16:15
	uint32_t suspend_enable				:1;		// 17
	uint32_t delay_p1_trans				:1;		// 18
	uint32_t delay_p1_p2_p3				:21-19+1;	// 21:19
	uint32_t dis_rx_det_u3_rx_det			:1;		// 22
	uint32_t start_rx_det_u3_rx_det			:1;		// 23
	uint32_t request_p1_p2_p3			:1;		// 24
	uint32_t u1_u2_exit_fail_to_recov		:1;		// 25
	uint32_t ping_enhancement_en			:1;		// 26
	uint32_t ux_exit_in_px				:1;		// 27
	uint32_t dis_rx_det_p3				:1;		// 28
	uint32_t u2_ss_inact_p3_ok			:1;		// 29
	uint32_t hst_prt_cmpl				:1;		// 30
	uint32_t phy_soft_reset				:1;		// 31
} usb3_g_pipe_t;

typedef struct {
	uint32_t clock_gating	:1;	// 0
	uint32_t hibernation	:1;	// 1
	uint32_t u2exit_lfps	:1;	// 2
	uint32_t scramble	:1;	// 3
	uint32_t scale_down	:5-4+1;	// 5:4
	uint32_t ram_clk	:7-6+1;	// 7:6
	uint32_t attach		:1;	// 8
	uint32_t u1u2_timer	:1;	// 9
	uint32_t sofitp_sync	:1;	// 10
	uint32_t coresoft_reset	:1;	// 11
	// ...
} usb3_g_ctl_t;

typedef struct {
	uint32_t disconnect	: 2-0+1;
	uint32_t _		: 1;
	uint32_t squelch	: 6-4+1;
	uint32_t __		: 1;
	uint32_t impedance_fs	: 11-8+1;
	uint32_t voltage	: 15-12+1;
	uint32_t crossover	: 17-16+1;
	uint32_t rise_fall	: 19-18+1;
	uint32_t impedance_hs	: 21-20+1;
	uint32_t current	: 23-22+1;
	uint32_t duration	: 1;
} usb2_phy_tune_t;



#endif /* BM1000_MMUSB_H */

