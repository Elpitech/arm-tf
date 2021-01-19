/*
 * Copyright (c) 2018-2020, Baikal Electronics JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <bm1000_private.h>
#include <lib/mmio.h>

#define MMPCI_GPR(ind)	(PCI_LCRU + LCRU_GPR + ind)
#define MMPCI_SREG(ind)	(PCI_LCRU + LCRU_SREG + ind)

// PCIe LCRU GPR registers
#define BM1000_LCRU_GPR_PCIE_X4_0_RESET_REG		MMPCI_GPR(0x0)
#define BM1000_LCRU_GPR_PCIE_X4_1_RESET_REG		MMPCI_GPR(0x20)
#define BM1000_LCRU_GPR_PCIE_X8_RESET_REG		MMPCI_GPR(0x40)

#define BM1000_LCRU_GPR_PCIE_MM_RESET_REG		MMPCI_GPR(0x80)
#define BM1000_LCRU_GPR_PCIE_MM_RESET_NIC_SLV_X4_0_RST	CTL_BIT(0)
#define BM1000_LCRU_GPR_PCIE_MM_RESET_NIC_SLV_X4_1_RST	CTL_BIT(1)
#define BM1000_LCRU_GPR_PCIE_MM_RESET_NIC_SLV_X8_RST	CTL_BIT(2)
#define BM1000_LCRU_GPR_PCIE_MM_RESET_NIC_SLV_SLV_RST	CTL_BIT(3)
#define BM1000_LCRU_GPR_PCIE_MM_RESET_NIC_CFG_X4_0_RST	CTL_BIT(4)
#define BM1000_LCRU_GPR_PCIE_MM_RESET_NIC_CFG_X4_1_RST	CTL_BIT(5)
#define BM1000_LCRU_GPR_PCIE_MM_RESET_NIC_CFG_X8_RST	CTL_BIT(6)
#define BM1000_LCRU_GPR_PCIE_MM_RESET_NIC_CFG_SLV_RST	CTL_BIT(7)
#define BM1000_LCRU_GPR_PCIE_MM_RESET_NIC_CFG_TCU_RST	CTL_BIT(8)

#define BM1000_LCRU_GPR_PCIE_X4_0_SID_PROT_CTL_REG	MMPCI_GPR(0xD0)
#define BM1000_LCRU_GPR_PCIE_X4_1_SID_PROT_CTL_REG	MMPCI_GPR(0xD8)
#define BM1000_LCRU_GPR_PCIE_X8_SID_PROT_CTL_REG	MMPCI_GPR(0xE0)
#define BM1000_LCRU_GPR_PCIE_SID_PROT_CTL_AWPROT(x)	CTL_BIT_SET(x, 2, 0)
#define BM1000_LCRU_GPR_PCIE_SID_PROT_CTL_WSID_RWON	CTL_BIT(4)
#define BM1000_LCRU_GPR_PCIE_SID_PROT_CTL_WSID_MODE	CTL_BIT(5)
#define BM1000_LCRU_GPR_PCIE_SID_PROT_CTL_ARPROT(x)	CTL_BIT_SET(x, 10, 8)
#define BM1000_LCRU_GPR_PCIE_SID_PROT_CTL_RSID_RWON	CTL_BIT(12)
#define BM1000_LCRU_GPR_PCIE_SID_PROT_CTL_RSID_MODE	CTL_BIT(13)

#define BM1000_LCRU_GPR_PCIE_MSI_TRANS_CTL0_REG		MMPCI_GPR(0xE8)
#define BM1000_LCRU_GPR_PCIE_MSI_TRANS_CTL0_MSI_AWUSER_MASK	CTL_BIT_MASK(3, 0)

#define BM1000_LCRU_GPR_PCIE_MSI_TRANS_CTL1_REG		MMPCI_GPR(0xF0)

#define BM1000_LCRU_GPR_PCIE_MSI_TRANS_CTL2_REG		MMPCI_GPR(0xF8)
#define BM1000_LCRU_GPR_PCIE_MSI_TRANS_CTL2_MSI_RCNUM_PCIE_X4_0_MASK	CTL_BIT_MASK(1, 0)
#define BM1000_LCRU_GPR_PCIE_MSI_TRANS_CTL2_MSI_RCNUM_PCIE_X4_1_MASK	CTL_BIT_MASK(3, 2)
#define BM1000_LCRU_GPR_PCIE_MSI_TRANS_CTL2_MSI_RCNUM_PCIE_X8_MASK	CTL_BIT_MASK(5, 4)
#define BM1000_LCRU_GPR_PCIE_MSI_TRANS_CTL2_PCIE_X4_0_MSI_TRANS_EN	CTL_BIT(9)
#define BM1000_LCRU_GPR_PCIE_MSI_TRANS_CTL2_PCIE_X4_1_MSI_TRANS_EN	CTL_BIT(10)
#define BM1000_LCRU_GPR_PCIE_MSI_TRANS_CTL2_PCIE_X8_MSI_TRANS_EN	CTL_BIT(11)

// Clock channels
#define MMPCI_X4_0_MSTR		0
#define MMPCI_X4_0_SLV		1
#define MMPCI_X4_0_CFG		2 // DBI
#define MMPCI_X4_1_MSTR		3
#define MMPCI_X4_1_SLV		4
#define MMPCI_X4_1_CFG		5 // DBI
#define MMPCI_X8_MSTR		6
#define MMPCI_X8_SLV		7
#define MMPCI_X8_CFG		8 // DBI
#define MMPCI_TCU_CFG		9
#define MMPCI_TBU0		10
#define MMPCI_TBU1		11
#define MMPCI_TBU2		12

#define MMPCI_CCH_ON(id, divider) CLKCH_ON(PCI_LCRU + LCRU_CMU0 + LCRU_CLKCH_OFFSET(id), divider)

static const PllCtlInitValues PCI_LCRU_PLL_INIT = {
	0, 0, 0x78, 0x00, 0x00, 0x2c, 0x00, 0x2c
};

void mmpci_on(void)
{
	uint32_t gpr;

	pll_on(PCI_LCRU, &PCI_LCRU_PLL_INIT, "PCI_LCRU timeout!");

	MMPCI_CCH_ON(MMPCI_X4_0_MSTR,  4); // 375 MHz
	MMPCI_CCH_ON(MMPCI_X4_0_SLV,   4); // 375 MHz
	MMPCI_CCH_ON(MMPCI_X4_0_CFG,  30); //  50 MHz
	MMPCI_CCH_ON(MMPCI_X4_1_MSTR,  4); // 375 MHz
	MMPCI_CCH_ON(MMPCI_X4_1_SLV,   4); // 375 MHz
	MMPCI_CCH_ON(MMPCI_X4_1_CFG,  30); //  50 MHz
	MMPCI_CCH_ON(MMPCI_X8_MSTR,    3); // 500 MHz
	MMPCI_CCH_ON(MMPCI_X8_SLV,     3); // 500 MHz
	MMPCI_CCH_ON(MMPCI_X8_CFG,    30); //  50 MHz
	MMPCI_CCH_ON(MMPCI_TCU_CFG,    4); // 375 MHz
	MMPCI_CCH_ON(MMPCI_TBU0,       4); // 375 MHz
	MMPCI_CCH_ON(MMPCI_TBU1,       4); // 375 MHz
	MMPCI_CCH_ON(MMPCI_TBU2,       3); // 500 MHz

	// Deassert resets
	gpr = mmio_read_32(BM1000_LCRU_GPR_PCIE_MM_RESET_REG);
	gpr &= ~(BM1000_LCRU_GPR_PCIE_MM_RESET_NIC_CFG_X4_0_RST	|
		 BM1000_LCRU_GPR_PCIE_MM_RESET_NIC_CFG_X4_1_RST	|
		 BM1000_LCRU_GPR_PCIE_MM_RESET_NIC_CFG_X8_RST	|
		 BM1000_LCRU_GPR_PCIE_MM_RESET_NIC_CFG_TCU_RST);

	mmio_write_32(BM1000_LCRU_GPR_PCIE_MM_RESET_REG, gpr);
	gpr &= ~BM1000_LCRU_GPR_PCIE_MM_RESET_NIC_CFG_SLV_RST;
	mmio_write_32(BM1000_LCRU_GPR_PCIE_MM_RESET_REG, gpr);

	gpr &= ~(BM1000_LCRU_GPR_PCIE_MM_RESET_NIC_SLV_X4_0_RST |
		 BM1000_LCRU_GPR_PCIE_MM_RESET_NIC_SLV_X4_1_RST |
		 BM1000_LCRU_GPR_PCIE_MM_RESET_NIC_SLV_X8_RST);

	mmio_write_32(BM1000_LCRU_GPR_PCIE_MM_RESET_REG, gpr);
	gpr &= ~BM1000_LCRU_GPR_PCIE_MM_RESET_NIC_SLV_SLV_RST;
	mmio_write_32(BM1000_LCRU_GPR_PCIE_MM_RESET_REG, gpr);

	// Set MSI_AWUSER to 0
	gpr = mmio_read_32(BM1000_LCRU_GPR_PCIE_MSI_TRANS_CTL0_REG);
	gpr &= ~BM1000_LCRU_GPR_PCIE_MSI_TRANS_CTL0_MSI_AWUSER_MASK;
	mmio_write_32(BM1000_LCRU_GPR_PCIE_MSI_TRANS_CTL0_REG, gpr);

	// Enable MSI translations, RC number for all MSI transactions is 0
	gpr = mmio_read_32(BM1000_LCRU_GPR_PCIE_MSI_TRANS_CTL2_REG);
	gpr &= ~(BM1000_LCRU_GPR_PCIE_MSI_TRANS_CTL2_MSI_RCNUM_PCIE_X4_0_MASK |
		 BM1000_LCRU_GPR_PCIE_MSI_TRANS_CTL2_MSI_RCNUM_PCIE_X4_1_MASK |
		 BM1000_LCRU_GPR_PCIE_MSI_TRANS_CTL2_MSI_RCNUM_PCIE_X8_MASK);

	gpr |=	BM1000_LCRU_GPR_PCIE_MSI_TRANS_CTL2_PCIE_X4_0_MSI_TRANS_EN |
		BM1000_LCRU_GPR_PCIE_MSI_TRANS_CTL2_PCIE_X4_1_MSI_TRANS_EN |
		BM1000_LCRU_GPR_PCIE_MSI_TRANS_CTL2_PCIE_X8_MSI_TRANS_EN;

	mmio_write_32(BM1000_LCRU_GPR_PCIE_MSI_TRANS_CTL2_REG, gpr);

	// TODO: security open LCRU GPR reg
	mmio_write_32(MMPCI_SREG(0), 0x001F1F1F);
#ifdef NOT_YET
	mmio_write_32(MMPCI_SREG(4), 0x01501501);
#else
	/* Preserve compatibility with older kernel versions which
	 * access "MSI_TRANS" registers.
	 */
	mmio_write_32(MMPCI_SREG(4), 0x45501501);
#endif
}

void mmpci_toNSW(void)
{
	mmio_write_32(BM1000_LCRU_GPR_PCIE_X4_0_SID_PROT_CTL_REG,
		      BM1000_LCRU_GPR_PCIE_SID_PROT_CTL_AWPROT(2) |
		      BM1000_LCRU_GPR_PCIE_SID_PROT_CTL_ARPROT(2) |
		      BM1000_LCRU_GPR_PCIE_SID_PROT_CTL_WSID_MODE |
		      BM1000_LCRU_GPR_PCIE_SID_PROT_CTL_RSID_MODE);

	mmio_write_32(BM1000_LCRU_GPR_PCIE_X4_1_SID_PROT_CTL_REG,
		      BM1000_LCRU_GPR_PCIE_SID_PROT_CTL_AWPROT(2) |
		      BM1000_LCRU_GPR_PCIE_SID_PROT_CTL_ARPROT(2) |
		      BM1000_LCRU_GPR_PCIE_SID_PROT_CTL_WSID_MODE |
		      BM1000_LCRU_GPR_PCIE_SID_PROT_CTL_RSID_MODE);

	mmio_write_32(BM1000_LCRU_GPR_PCIE_X8_SID_PROT_CTL_REG,
		      BM1000_LCRU_GPR_PCIE_SID_PROT_CTL_AWPROT(2) |
		      BM1000_LCRU_GPR_PCIE_SID_PROT_CTL_ARPROT(2) |
		      BM1000_LCRU_GPR_PCIE_SID_PROT_CTL_WSID_MODE |
		      BM1000_LCRU_GPR_PCIE_SID_PROT_CTL_RSID_MODE);

	SECURE_MODE(BAIKAL_NIC_PCIE_CFG_X4_0, SECURE_MODE_OFF);
	SECURE_MODE(BAIKAL_NIC_PCIE_CFG_X4_1, SECURE_MODE_OFF);
	SECURE_MODE(BAIKAL_NIC_PCIE_CFG_X8,   SECURE_MODE_OFF);
	SECURE_MODE(BAIKAL_NIC_PCIE_CFG_TCU,  SECURE_MODE_OFF);

	SECURE_MODE(BAIKAL_NIC_PCIE_X4_0, SECURE_MODE_OFF);
	SECURE_MODE(BAIKAL_NIC_PCIE_X4_1, SECURE_MODE_OFF);
	SECURE_MODE(BAIKAL_NIC_PCIE_X8,   SECURE_MODE_OFF);
}