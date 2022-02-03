/*
 * Copyright (c) 2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <drivers/delay_timer.h>

#include <ndelay.h>

#include "ddr_baikal.h"
#include "ddr_lcru.h"
#include "ddr_master.h"

static int baikal_ddrc_set_registers(const unsigned port, struct ctl_content *ddrc)
{
	BM_DDRC_WRITE(port, DDRC_MSTR, ddrc->MSTR_);
	if (BM_DDRC_READ(port, DDRC_MSTR) != ddrc->MSTR_) {
		return 1;
	}

	BM_DDRC_WRITE(port, DDRC_SCHED, ddrc->SCHED_);
	BM_DDRC_WRITE(port, DDRC_CRCPARCTL0, ddrc->CRCPARCTL0_);
	if (BM_DDRC_READ(port, DDRC_CRCPARCTL0) != ddrc->CRCPARCTL0_) {
		return 2;
	}

	BM_DDRC_WRITE(port, DDRC_CRCPARCTL1, ddrc->CRCPARCTL1_);
	if (BM_DDRC_READ(port, DDRC_CRCPARCTL1) != ddrc->CRCPARCTL1_) {
		return 3;
	}

	BM_DDRC_WRITE(port, DDRC_DBICTL, ddrc->DBICTL_);
	if (BM_DDRC_READ(port, DDRC_DBICTL) != ddrc->DBICTL_) {
		return 4;
	}

	BM_DDRC_WRITE(port, DDRC_DIMMCTL, ddrc->DIMMCTL_);
	if (BM_DDRC_READ(port, DDRC_DIMMCTL) != ddrc->DIMMCTL_) {
		return 5;
	}

	BM_DDRC_WRITE(port, DDRC_PCCFG, ddrc->PCCFG_);
	BM_DDRC_WRITE(port, DDRC_INIT0, ddrc->INIT0_);
	if (BM_DDRC_READ(port, DDRC_INIT0) != ddrc->INIT0_) {
		return 6;
	}

	BM_DDRC_WRITE(port, DDRC_INIT0, ddrc->INIT0_);
	BM_DDRC_WRITE(port, DDRC_INIT3, ddrc->INIT3_);
	BM_DDRC_WRITE(port, DDRC_INIT4, ddrc->INIT4_);
	BM_DDRC_WRITE(port, DDRC_INIT6, ddrc->INIT6_);
	BM_DDRC_WRITE(port, DDRC_INIT7, ddrc->INIT7_);
	BM_DDRC_WRITE(port, DDRC_RANKCTL, ddrc->RANKCTL_);
	if (BM_DDRC_READ(port, DDRC_RANKCTL) != ddrc->RANKCTL_) {
		return 7;
	}

	/* Program DFI timings */
	BM_DDRC_WRITE(port, DDRC_DFITMG0, ddrc->DFITMG0_);
	BM_DDRC_WRITE(port, DDRC_DFITMG1, ddrc->DFITMG1_);

	/* Program DDRC DRAM timings */
	BM_DDRC_WRITE(port, DDRC_DRAMTMG0, ddrc->DRAMTMG0_);
	BM_DDRC_WRITE(port, DDRC_DRAMTMG1, ddrc->DRAMTMG1_);
	BM_DDRC_WRITE(port, DDRC_DRAMTMG2, ddrc->DRAMTMG2_);
	BM_DDRC_WRITE(port, DDRC_DRAMTMG3, ddrc->DRAMTMG3_);
	BM_DDRC_WRITE(port, DDRC_DRAMTMG4, ddrc->DRAMTMG4_);
	BM_DDRC_WRITE(port, DDRC_DRAMTMG5, ddrc->DRAMTMG5_);
	BM_DDRC_WRITE(port, DDRC_DRAMTMG8, ddrc->DRAMTMG8_);
	BM_DDRC_WRITE(port, DDRC_DRAMTMG9, ddrc->DRAMTMG9_);
	BM_DDRC_WRITE(port, DDRC_DRAMTMG11, ddrc->DRAMTMG11_);
	BM_DDRC_WRITE(port, DDRC_DRAMTMG12, ddrc->DRAMTMG12_);

	/* Program address map */
	BM_DDRC_WRITE(port, DDRC_ADDRMAP0, ddrc->ADDRMAP0_);
	BM_DDRC_WRITE(port, DDRC_ADDRMAP1, ddrc->ADDRMAP1_);
	BM_DDRC_WRITE(port, DDRC_ADDRMAP2, ddrc->ADDRMAP2_);
	BM_DDRC_WRITE(port, DDRC_ADDRMAP3, ddrc->ADDRMAP3_);
	BM_DDRC_WRITE(port, DDRC_ADDRMAP4, ddrc->ADDRMAP4_);
	BM_DDRC_WRITE(port, DDRC_ADDRMAP5, ddrc->ADDRMAP5_);
	BM_DDRC_WRITE(port, DDRC_ADDRMAP6, ddrc->ADDRMAP6_);
	BM_DDRC_WRITE(port, DDRC_ADDRMAP7, ddrc->ADDRMAP7_);
	BM_DDRC_WRITE(port, DDRC_ADDRMAP8, ddrc->ADDRMAP8_);

	/* Program SAR regs */
	BM_DDRC_WRITE(port, DDRC_SARBASE(0), ddrc->SARBASE0_);
	BM_DDRC_WRITE(port, DDRC_SARSIZE(0), ddrc->SARSIZE0_);
	BM_DDRC_WRITE(port, DDRC_SARBASE(1), ddrc->SARBASE1_);
	BM_DDRC_WRITE(port, DDRC_SARSIZE(1), ddrc->SARSIZE1_);
	BM_DDRC_WRITE(port, DDRC_SARBASE(2), ddrc->SARBASE2_);
	BM_DDRC_WRITE(port, DDRC_SARSIZE(2), ddrc->SARSIZE2_);

	/* Set refresh period */
	BM_DDRC_WRITE(port, DDRC_RFSHTMG, ddrc->RFSHTMG_);

	/* Program ODT timings */
	BM_DDRC_WRITE(port, DDRC_ODTCFG, ddrc->ODTCFG_);
	BM_DDRC_WRITE(port, DDRC_ODTMAP, ddrc->ODTMAP_);

	/* Program DQ map */
	BM_DDRC_WRITE(port, DDRC_DQMAP0, ddrc->DQMAP0_);
	BM_DDRC_WRITE(port, DDRC_DQMAP1, ddrc->DQMAP1_);
	BM_DDRC_WRITE(port, DDRC_DQMAP2, ddrc->DQMAP2_);
	BM_DDRC_WRITE(port, DDRC_DQMAP3, ddrc->DQMAP3_);
	BM_DDRC_WRITE(port, DDRC_DQMAP4, ddrc->DQMAP4_);
	BM_DDRC_WRITE(port, DDRC_DQMAP5, ddrc->DQMAP5_);

	/* Program ZQ regs */
	BM_DDRC_WRITE(port, DDRC_ZQCTL0, ddrc->ZQCTL0_);

	/* Program ECC */
	BM_DDRC_WRITE(port, DDRC_ECCCFG0, ddrc->ECCCFG0_);
	if (BM_DDRC_READ(port, DDRC_ECCCFG0) != ddrc->ECCCFG0_) {
		return 8;
	}

	/* Disable DFI updates (for PHY trainings) */
	BM_DDRC_WRITE(port, DDRC_DFIUPD0, ddrc->DFIUPD0_);

	/* Enable support for acknowledging PHY-initiated updates */
	BM_DDRC_WRITE(port, DDRC_DFIUPD2, ddrc->DFIUPD2_);
	return 0;
}

static int baikal_ddrphy_set_registers(const unsigned port, struct phy_content *phy)
{
	BM_DDR_PUB_WRITE(port, DDR_PUB_DCR, phy->DCR_);

	/* Set PLL referece freq range */
	BM_DDR_PUB_WRITE(port, DDR_PUB_PLLCR, phy->PLLCR_);

	/* Set DX8 byte line usage (for ECC) */
	BM_DDR_PUB_WRITE(port, DDR_PUB_DX8GCR0, phy->DX8GCR0_);

	/* Set refresh period */
	BM_DDR_PUB_WRITE(port, DDR_PUB_PGCR2, phy->PGCR2_);

	/* Set user BIST pattern */
	BM_DDR_PUB_WRITE(port, DDR_PUB_PGCR4, phy->PGCR4_);

	/* Set VT compensation settings */
	BM_DDR_PUB_WRITE(port, DDR_PUB_PGCR6, phy->PGCR6_);

	/* Program PHY timings */
	BM_DDR_PUB_WRITE(port, DDR_PUB_PTR1, phy->PTR1_);
	BM_DDR_PUB_WRITE(port, DDR_PUB_PTR3, phy->PTR3_);
	BM_DDR_PUB_WRITE(port, DDR_PUB_PTR4, phy->PTR4_);

	/* Program DRAM timings */
	BM_DDR_PUB_WRITE(port, DDR_PUB_DTPR0, phy->DTPR0_);
	BM_DDR_PUB_WRITE(port, DDR_PUB_DTPR1, phy->DTPR1_);
	BM_DDR_PUB_WRITE(port, DDR_PUB_DTPR2, phy->DTPR2_);
	BM_DDR_PUB_WRITE(port, DDR_PUB_DTPR3, phy->DTPR3_);
	BM_DDR_PUB_WRITE(port, DDR_PUB_DTPR4, phy->DTPR4_);
	BM_DDR_PUB_WRITE(port, DDR_PUB_DTPR5, phy->DTPR5_);

	/* Program DRAM mode registers */
	BM_DDR_PUB_WRITE(port, DDR_PUB_MR0, phy->MR0_);
	BM_DDR_PUB_WRITE(port, DDR_PUB_MR1, phy->MR1_);
	BM_DDR_PUB_WRITE(port, DDR_PUB_MR2, phy->MR2_);
	BM_DDR_PUB_WRITE(port, DDR_PUB_MR3, phy->MR3_);
	BM_DDR_PUB_WRITE(port, DDR_PUB_MR4, phy->MR4_);
	BM_DDR_PUB_WRITE(port, DDR_PUB_MR5, phy->MR5_);
	BM_DDR_PUB_WRITE(port, DDR_PUB_MR6, phy->MR6_);

	ndelay(40);

	if (phy->RDIMMGCR0_ & 0x1) {
		BM_DDR_PUB_WRITE(port, DDR_PUB_RDIMMGCR0, phy->RDIMMGCR0_);
		BM_DDR_PUB_WRITE(port, DDR_PUB_RDIMMGCR1, phy->RDIMMGCR1_);
		BM_DDR_PUB_WRITE(port, DDR_PUB_RDIMMGCR2, phy->RDIMMGCR2_);
	}

	BM_DDR_PUB_WRITE(port, DDR_PUB_DTCR0, phy->DTCR0_);
	BM_DDR_PUB_WRITE(port, DDR_PUB_DTCR1, phy->DTCR1_);
	BM_DDR_PUB_WRITE(port, DDR_PUB_DSGCR, phy->DSGCR_);
	BM_DDR_PUB_WRITE(port, DDR_PUB_ZQCR, phy->ZQCR_);

	if (phy->dbus_half) {
		uint32_t DXnGCR0_val = BM_DDR_PUB_READ(port, DDR_PUB_DX4GCR0);
		DXnGCR0_val &= ~(0x1U);
		BM_DDR_PUB_WRITE(port, DDR_PUB_DX4GCR0, DXnGCR0_val);
		BM_DDR_PUB_WRITE(port, DDR_PUB_DX5GCR0, DXnGCR0_val);
		BM_DDR_PUB_WRITE(port, DDR_PUB_DX6GCR0, DXnGCR0_val);
		BM_DDR_PUB_WRITE(port, DDR_PUB_DX7GCR0, DXnGCR0_val);
	}

	return 0;
}

static int baikal_ddrphy_pir_training(const unsigned port, uint32_t mode)
{
	int ret = 0;
	uint64_t timeout;

	/*
	 * Trigger PHY initialization
	 *
	 * DDR PHY DataBook, p.84, PIR[0]:
	 * It is recommended that this bit be set 1b1 in a separate config write step
	 * after other bits in this register have been programmed to avoid any race condition.
	 */
	BM_DDR_PUB_WRITE(port, DDR_PUB_PIR, mode & (~0x1UL));
	BM_DDR_PUB_WRITE(port, DDR_PUB_PIR, mode | 0x1);

	/*
	 * Wait for DDR PHY initialization done (IDONE)
	 *
	 * DDR PHY DataBook, p.84:
	 * Note that PGSR0[IDONE] is not cleared immediately after PIR[INIT] is set,
	 * and therefore software must wait a minimum of 10 configuration clock cycles
	 * from when PIR[INIT] is set to when it starts polling for PGSR0[IDONE]
	 */
	ndelay(40);

	for (timeout = timeout_init_us(10000);;) {
		const uint32_t reg = BM_DDR_PUB_READ(port, DDR_PUB_PGSR0);
		if (reg & 0x1) {
			break;
		} else if (timeout_elapsed(timeout)) {
			ret = reg;
			ERROR("%s: failed, 0x%x\n", __func__, ret);
			break;
		}
	}

	udelay(1);
	return ret;
}

static int baikal_ddrphy_vref_training(const unsigned port, unsigned clock_mhz)
{
	uint32_t vtcr0_val = BM_DDR_PUB_READ(port, DDR_PUB_VTCR0);
	vtcr0_val &= ~GENMASK(31, 29);
	vtcr0_val |= GENMASK(31, 29) & ((3 * clock_mhz / 640) << 29); /* Number of ctl_clk supposed to be (> 150ns) during DRAM VREF training */
	vtcr0_val &= ~GENMASK(5, 0);
	vtcr0_val |= 0x8; /* DRAM Vref initial value (this one is for DDR0 channel speedbin DDR4-2133) */
	vtcr0_val |= 1 << 27; /* DRAM Vref PDA mode */
	BM_DDR_PUB_WRITE(port, DDR_PUB_VTCR0, vtcr0_val);

	/* D4MV I/Os with PVREF_DAC settings (PUB DataBook p.353) */
	uint32_t vtcr1_val = BM_DDR_PUB_READ(port, DDR_PUB_VTCR1);
	vtcr1_val &= ~GENMASK(7, 5);
	vtcr1_val |= GENMASK(7, 5) & ((clock_mhz / 160) << 5); /* Number of ctl_clk supposed to be (> 200ns) during Host IO VREF training */
	vtcr1_val &= ~(0x1U << 8);
	BM_DDR_PUB_WRITE(port, DDR_PUB_VTCR1, vtcr1_val);

	/* 4.4.8.6 Steps to Run VREF Training (PUB DataBook p.363) */
	const uint32_t dtcr0_stored = BM_DDR_PUB_READ(port, DDR_PUB_DTCR0);
	uint32_t dtcr0_val = dtcr0_stored;
	dtcr0_val &= ~GENMASK(31, 28); /* Disable refresh during training */
	BM_DDR_PUB_WRITE(port, DDR_PUB_DTCR0, dtcr0_val);

	/* Program number of LCDL eye points for which VREF training is repeated (ENUM) by writing VTCR1[2] */
	vtcr1_val = BM_DDR_PUB_READ(port, DDR_PUB_VTCR1);
	vtcr1_val |= 1 << 2; /* Allow to program number of LCDL eye points for which VREF training is repeated */
	vtcr1_val &= ~GENMASK(31, 28);
	vtcr1_val |= 0x1 << 28; /* Program HOST VREF step size used during VREF training (HVSS) VTCR1[31:28] = 0x1 */
	vtcr1_val |= 0xf << 12; /* Program VREF word count (VWCR) VTCR1[15:12] = 0xf */
	BM_DDR_PUB_WRITE(port, DDR_PUB_VTCR1, vtcr1_val);

	int ret = baikal_ddrphy_pir_training(port, DDR_PUB_PIR_VREF); /* run vref training itself */

	BM_DDR_PUB_WRITE(port, DDR_PUB_DTCR0, dtcr0_stored); /* restore DTCR0 value */

	return ret;
}

static int baikal_ddrphy_dram_init(const unsigned port, unsigned clock_mhz)
{
	uint64_t timeout;

	/* Disable PHY IO ODT and DQS PU/PD for normal operation */
	BM_DDR_PUB_WRITE(port, DDR_PUB_DXCCR, 0x20401004);

	/* Disable DDR Controller auto-refreshes during trainings */
	BM_DDRC_WRITE(port, DDRC_RFSHCTL3, 0x1);
	ndelay(40);
	if (BM_DDRC_READ(port, DDRC_RFSHCTL3) != 0x1) {
		return 1;
	}

	if (BM_DDR_PUB_READ(port, DDR_PUB_RDIMMGCR0) & 0x1) {
		baikal_ddrphy_pir_training(port, DDR_PUB_PIR_DRAM_STEP0 | DDR_PUB_PIR_RDIMM);
	} else {
		baikal_ddrphy_pir_training(port, DDR_PUB_PIR_DRAM_STEP0);
	}

	/*
	 * DDR4 multiPHY Utility Block (PUB) Databook (p.317):
	 * During DDR4 write leveling, dynamic ODT must be disabled (RTT_WR in MR2 register).
	 */
	const uint16_t mr2_val = BM_DDRC_READ(port, DDRC_INIT4) >> 16;
	const uint16_t rttWR = mr2_val & (0x3 << 9);
	if (rttWR) {
		BM_DDR_PUB_WRITE(port, DDR_PUB_MR2, mr2_val & ~(0x3 << 9));
	}

	baikal_ddrphy_pir_training(port, DDR_PUB_PIR_DRAM_STEP1);
	if (rttWR) {
		BM_DDR_PUB_WRITE(port, DDR_PUB_MR2, mr2_val);
		uint32_t tMRD = BM_DDR_PUB_READ(port, DDR_PUB_DTPR1) & 0x1f;
		uint64_t cmu_cmd = ((uint64_t)tMRD << 39) | (1ULL << 37) | (1ULL << 33) | (2 << 27) | (mr2_val << 9);
		BM_DDR_PUB_WRITE(port, DDR_PUB_DCUAR, 0x0);
		BM_DDR_PUB_WRITE(port, DDR_PUB_DCUDR, (uint32_t)cmu_cmd);
		BM_DDR_PUB_WRITE(port, DDR_PUB_DCUAR, 1 << 4);
		BM_DDR_PUB_WRITE(port, DDR_PUB_DCUDR, (uint32_t)(cmu_cmd >> 32));
		BM_DDR_PUB_WRITE(port, DDR_PUB_DCURR, 0x0);
		BM_DDR_PUB_WRITE(port, DDR_PUB_DCURR, 1 << 0);

		for (timeout = timeout_init_us(10000);;) {
			const uint32_t reg = BM_DDR_PUB_READ(port, DDR_PUB_DCUSR0);
			if (reg & 0x1) {
				break;
			} else if (timeout_elapsed(timeout)) {
				ERROR("%s: *rttWR* failed, 0x%x\n", __func__, reg);
				break;
			}
		}
	}

	baikal_ddrphy_pir_training(port, DDR_PUB_PIR_DRAM_STEP2);
	baikal_ddrphy_vref_training(port, clock_mhz);

	/* Enable DDR Controller auto-refreshes after training */
	BM_DDRC_WRITE(port, DDRC_RFSHCTL3, 0x0);
	ndelay(40);
	if (BM_DDRC_READ(port, DDRC_RFSHCTL3) != 0x0) {
		return 2;
	}

	/* Enable DFI update request */
	uint32_t dsgcr_val = BM_DDR_PUB_READ(port, DDR_PUB_DSGCR);
	dsgcr_val |= DDR_PUB_DSGCR_PUREN;
	BM_DDR_PUB_WRITE(port, DDR_PUB_DSGCR, dsgcr_val);

	/* Enable AXI port */
	BM_DDRC_WRITE(port, DDRC_PCTRL(0), 0x1);
	ndelay(40);
	if (BM_DDRC_READ(port, DDRC_PCTRL(0)) != 0x1) {
		return 3;
	}

	int ret = 0;

	for (timeout = timeout_init_us(10000);;) {
		const uint32_t reg = BM_DDRC_READ(port, DDRC_STAT);
		if ((reg & 0x3) == 0x1) {
			break;
		} else if (timeout_elapsed(timeout)) {
			ret = ~reg;
			ERROR("%s: failed, 0x%x\n", __func__, ret);
			break;
		}
	}

	ndelay(40);

	return ret;
}

int ddr_init(int port, bool dual_mode, struct ddr_configuration *info)
{
	struct ctl_content ddrc = {0};
	struct phy_content phy = {0};

	ddrc_set_default_vals(&ddrc);
	phy_set_default_vals(&phy);

	ddrc_config_content(&ddrc, info);
	phy_config_content(&phy, info);

	ddr_lcru_clkch_rst(port, LCRU_DDR_CMU_CLKCH0_ACLK, 1);
	ddr_lcru_clkch_rst(port, LCRU_DDR_CMU_CLKCH1_CORE, 1);

	if (dual_mode) {
		ddr_lcru_dual_mode(port, DDR_DUAL_MODE);
	}

	ndelay(40);

	int ret = baikal_ddrc_set_registers(port, &ddrc);
	if (ret) {
		ERROR("->\t%s: failed to config ddrc registers; error: %d\n", __func__, ret);
		return -1;
	}

	ndelay(40);

	/* Clear LCU reset. (DDR CTRL & DDR AXI clocks on) */
	ddr_lcru_clkch_rst(port, LCRU_DDR_CMU_CLKCH0_ACLK, 0);
	ddr_lcru_clkch_rst(port, LCRU_DDR_CMU_CLKCH1_CORE, 0);

	ndelay(40);

	baikal_ddrphy_set_registers(port, &phy);

	ret = baikal_ddrphy_pir_training(port, DDR_PUB_PIR_PHY_INIT);
	if (ret) {
		ERROR("->\t%s: baikal_ddrphy_pir_training error: %d\n", __func__, ret);
		return ret;
	}

	ret = baikal_ddrphy_dram_init(port, info->clock_mhz);
	if (ret) {
		ERROR("->\t%s: baikal_ddrphy_dram_init error: %d\n", __func__, ret);
	}

	return ret;
}

int ddr_init_ecc_memory(int port)
{
	uint32_t reg;
	uint64_t timeout;

	BM_DDRC_WRITE(port, DDRC_PCTRL(0), 0); /* Disable AXI port */
	reg = BM_DDRC_READ(port, DDRC_SBRCTL);

	/* zero scrub_interval value and set scrub_mode to perform writes */
	reg &= ~GENMASK(20, 8);
	reg |= 1 << 2;
	BM_DDRC_WRITE(port, DDRC_SBRCTL, reg);

	BM_DDRC_WRITE(port, DDRC_SBRWDATA0, 0);
	BM_DDRC_WRITE(port, DDRC_SBRWDATA1, 0);

	/* enable scrub in a separate step to avoid race conditions */
	BM_DDRC_WRITE(port, DDRC_SBRCTL, reg | 0x1);

	/* it may take up to 3 seconds to init 32Gb of memory */
	for (timeout = timeout_init_us(3000000);;) {
		reg = BM_DDRC_READ(port, DDRC_SBRSTAT);
		if (reg & 0x2) {
			break;
		} else if (timeout_elapsed(timeout)) {
			ERROR("%s: failed to init memory (undone)\n", __func__);
			goto exit;
		}
	}
	for (timeout = timeout_init_us(10000);;) {
		reg = BM_DDRC_READ(port, DDRC_SBRSTAT);
		if (!(reg & 0x1)) {
			break;
		} else if (timeout_elapsed(timeout)) {
			ERROR("%s: failed to init memory (busy)\n", __func__);
			goto exit;
		}
	}

exit:
	reg = BM_DDRC_READ(port, DDRC_SBRCTL);
	/* set maximum scrub_interval value and set scrub_mode to perform reads */
	reg |= 0x1fff << 8;
	reg &= ~(0x5);
	BM_DDRC_WRITE(port, DDRC_SBRCTL, reg);
	/* enable scrub in a separate step to avoid race conditions */
	BM_DDRC_WRITE(port, DDRC_SBRCTL, reg | 0x1);
	BM_DDRC_WRITE(port, DDRC_PCTRL(0), 0x1); /* Enable AXI port */

	return 0;
}
