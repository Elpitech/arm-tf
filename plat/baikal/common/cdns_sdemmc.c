/*
 * Copyright (c) 2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <stdint.h>

#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <drivers/mmc.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>

#define HRS00				0x000
#define HRS00_SWR			BIT(0)

#define HRS04				0x010
#define HRS05				0x014

#define HRS06				0x018
#define HRS06_EMM_SDR			2

#define HRS07				0x01c
#define HRS07_IDELAY_VAL(x)		((x) << 0)
#define HRS07_RW_COMPENSATE(x)		((x) << 16)

#define HRS09				0x024
#define HRS09_PHY_SW_RST		BIT(0)
#define HRS09_PHY_INIT_COMPLETE		BIT(1)
#define HRS09_EXTENDED_RD_MODE		BIT(2)
#define HRS09_EXTENDED_WR_MODE		BIT(3)
#define HRS09_RD_CMD_EN			BIT(15)
#define HRS09_RD_DATA_EN		BIT(16)

#define HRS10				0x028
#define HRS10_HCSDCLKADJ(x)		((x) << 16)

#define HRS11				0x02c
#define HRS11_EMMC_RST			BIT(0)

#define HRS16				0x040
#define HRS16_WRCMD0_DLY(x)		((x) << 0)
#define HRS16_WRCMD1_DLY(x)		((x) << 4)
#define HRS16_WRDATA0_DLY(x)		((x) << 8)
#define HRS16_WRDATA1_DLY(x)		((x) << 12)
#define HRS16_WRCMD0_SDCLK_DLY(x)	((x) << 16)
#define HRS16_WRCMD1_SDCLK_DLY(x)	((x) << 20)
#define HRS16_WRDATA0_SDCLK_DLY(x)	((x) << 24)
#define HRS16_WRDATA1_SDCLK_DLY(x)	((x) << 28)

#define SRS01				0x204
#define SRS02				0x208

#define SRS03				0x20c
#define SRS03_RTS_NONE			(0 << 16)
#define SRS03_RTS_136BIT		(1 << 16)
#define SRS03_RTS_48BIT			(2 << 16)
#define SRS03_RTS_48BIT_BUSY		(3 << 16)
#define SRS03_DTDS			BIT(4)
#define SRS03_CRCCE			BIT(19)
#define SRS03_CICE			BIT(20)
#define SRS03_DPS			BIT(21)
#define SRS03_CIDX(x)			((x) << 24)

#define SRS04				0x210
#define SRS08				0x220

#define SRS09				0x224
#define SRS09_CICMD			BIT(0)
#define SRS09_CIDAT			BIT(1)
#define SRS09_BRE			BIT(11)
#define SRS09_CI			BIT(16)

#define SRS10				0x228
#define SRS10_DTW			BIT(1)
#define SRS10_EDTW			BIT(5)
#define SRS10_BP			BIT(8)
#define SRS10_BVS_1V8			(5 << 9)

#define SRS11				0x22c
#define SRS11_ICE			BIT(0)
#define SRS11_ICS			BIT(1)
#define SRS11_SDCE			BIT(2)
#define SRS11_DTCV29			(14 << 16)

#define SRS12				0x230
#define SRS12_CC			BIT(0)
#define SRS12_TC			BIT(1)
#define SRS12_BRR			BIT(5)

#define SRS13				0x234

#define PHY_DQ_TIMING_REG				0x2000
#define PHY_DQ_TIMING_DATA_SELECT_OE_END(x)		((x) << 0)
#define PHY_DQ_TIMING_IO_MASK_START(x)			((x) << 24)
#define PHY_DQ_TIMING_IO_MASK_END(x)			((x) << 27)
#define PHY_DQ_TIMING_IO_MASK_ALWAYS_ON			BIT(31)

#define PHY_DQS_TIMING_REG				0x2004
#define PHY_DQS_TIMING_USE_PHONY_DQS_CMD		BIT(19)
#define PHY_DQS_TIMING_USE_PHONY_DQS			BIT(20)
#define PHY_DQS_TIMING_USE_LPBK_DQS			BIT(21)
#define PHY_DQS_TIMING_USE_EXT_LPBK_DQS			BIT(22)

#define PHY_GATE_LPBK_CTRL_REG				0x2008
#define PHY_GATE_LPBK_CTRL_GATE_CFG_ALWAYS_ON		BIT(6)
#define PHY_GATE_LPBK_CTRL_UNDERRUN_SUPPRESS		BIT(18)
#define PHY_GATE_LPBK_CTRL_RD_DEL_SEL(x)		((x) << 19)
#define PHY_GATE_LPBK_CTRL_SYNC_METHOD			BIT(31)

#define PHY_DLL_MASTER_CTRL_REG				0x200c
#define PHY_DLL_MASTER_CTRL_DLL_START_POINT(x)		((x) << 0)
#define PHY_DLL_MASTER_CTRL_PHASE_DETECT_SEL(x)		((x) << 20)
#define PHY_DLL_MASTER_CTRL_DLL_BYPASS_MODE		BIT(23)

#define PHY_DLL_SLAVE_CTRL_REG				0x2010
#define PHY_DLL_SLAVE_CTRL_READ_DQS_DELAY(x)		((x) << 0)
#define PHY_DLL_SLAVE_CTRL_CLK_WR_DELAY(x)		((x) << 8)
#define PHY_DLL_SLAVE_CTRL_CLK_WRDQS_DELAY(x)		((x) << 16)
#define PHY_DLL_SLAVE_CTRL_READ_DQS_CMD_DELAY(x)	((x) << 24)

#define PHY_CTRL_REG					0x2080
#define PHY_CTRL_PHONY_DQS_TIMING(x)			((x) << 4)

#define OCR_1V8			BIT(7)
#define OCR_ACCESS_MODE_SECTOR	(2 << 29)

static bool cdns_sdemmc_card_inserted(uintptr_t hc);
static void cdns_sdemmc_card_init(uintptr_t hc);
static void cdns_sdemmc_data_read(const uintptr_t hc, const uintptr_t base, uint32_t *buf, size_t sz);
static void cdns_sdemmc_dfi_init(uintptr_t hc, unsigned int mode);
static void cdns_sdemmc_init(void);
static int cdns_sdemmc_prepare(int lba, uintptr_t buf, size_t size);
static int cdns_sdemmc_read(int lba, uintptr_t buf, size_t size);
static int cdns_sdemmc_send_cmd(struct mmc_cmd *cmd);
static void cdns_sdemmc_set_clk(uintptr_t hc, uint32_t freq);
static int cdns_sdemmc_set_ios(unsigned int clk, unsigned int width);
static int cdns_sdemmc_write(int lba, uintptr_t buf, size_t size);

static const struct mmc_ops cdns_sdemmc_ops = {
	.init		= cdns_sdemmc_init,
	.send_cmd	= cdns_sdemmc_send_cmd,
	.set_ios	= cdns_sdemmc_set_ios,
	.prepare	= cdns_sdemmc_prepare,
	.read		= cdns_sdemmc_read,
	.write		= cdns_sdemmc_write,
};

static uintptr_t ghc;
static bool sector_mode;

static void cdns_sdemmc_card_init(const uintptr_t hc)
{
	unsigned int counter;

	/* Reset the card to idle state */
	mmio_write_32(hc + SRS02, 0x00000000);
	mmio_write_32(hc + SRS03, SRS03_CIDX(0));

	counter = 3;
	do {
		uint32_t ocr;
		uint64_t timeout;

		/* Clear error/interrupt status */
		mmio_write_32(hc + SRS12, 0xbff00ff);

		/* Ask the card to send its Operating Conditions Register */
		mmio_write_32(hc + SRS02, OCR_ACCESS_MODE_SECTOR | OCR_1V8);
		mmio_write_32(hc + SRS03, SRS03_CIDX(1) |
					  SRS03_RTS_48BIT);

		timeout = timeout_init_us(100000);
		do {
			if (mmio_read_32(hc + SRS12) &
					      SRS12_CC) {
				ocr = mmio_read_32(hc + SRS04);
				if (!(ocr & BIT(31))) {
					INFO("mmc@%lx: OCR:0x%08x: busy\n", hc, ocr);

					/* Clear error/interrupt status */
					mmio_write_32(hc + SRS12, 0xbff00ff);

					/* Ask the card to send its Operating Conditions Register */
					mmio_write_32(hc + SRS02, OCR_ACCESS_MODE_SECTOR | OCR_1V8);
					mmio_write_32(hc + SRS03, SRS03_CIDX(1) |
								  SRS03_RTS_48BIT);
				} else if (!(ocr & BIT(7))) {
					INFO("mmc@%lx: OCR:0x%08x: unsupported card\n", hc, ocr);
					return;
				} else if (ocr == 0x80ff8080 || ocr == 0xc0ff8080) {
					INFO("mmc@%lx: OCR:0x%08x: ready\n", hc, ocr);
					if (((ocr >> 29) & 0x3) != 0) {
						sector_mode = true;
					}

					counter = 0;
					break;
				} else {
					INFO("mmc@%lx: OCR:0x%08x: unknown state\n", hc, ocr);
					return;
				}
			}
		} while (!timeout_elapsed(timeout));

		if (counter > 0) {
			--counter;
		}
	} while (counter > 0);

	/* CMD2 - get card ID - go to "IDENT" state */
	mmio_write_32(hc + SRS12, 0xbff00ff);
	mmio_write_32(hc + SRS02, 0x00000000);
	mmio_write_32(hc + SRS03, SRS03_CIDX(2) |
				  SRS03_RTS_136BIT);

	while (!(mmio_read_32(hc + SRS12) &
				   SRS12_CC))
		;

	/* CMD3 - send RCA to eMMC card and go to "STANDBY" state */
	mmio_write_32(hc + SRS12, 0xbff00ff);
	mmio_write_32(hc + SRS02, 0x1234 << 16);
	mmio_write_32(hc + SRS03, SRS03_CIDX(3) |
				  SRS03_CICE    |
				  SRS03_CRCCE   |
				  SRS03_RTS_48BIT);

	while (!(mmio_read_32(hc + SRS12) &
				   SRS12_CC))
		;

	/* Select the card */
	mmio_write_32(hc + SRS12, 0xbff00ff);
	mmio_write_32(hc + SRS02, 0x1234 << 16);
	mmio_write_32(hc + SRS03, SRS03_CIDX(7) |
				  SRS03_CICE    |
				  SRS03_CRCCE   |
				  SRS03_RTS_48BIT);

	while (!(mmio_read_32(hc + SRS12) &
				   SRS12_CC))
		;

	/* Set eMMC mode: SDR legacy, clock = 25 MHz */

	/* Select bus width in SDHC */
	mmio_setbits_32(hc + SRS10,
			     SRS10_EDTW);

	/* CMD6 - SWITCH, set extended CSD register: select bus width and SDR in eMMC card */
	mmio_write_32(hc + SRS12, 0xbff00ff);
	mmio_write_32(hc + SRS02, (3 << 24) | (183 << 16) | (2 << 8));
	mmio_write_32(hc + SRS03, SRS03_CIDX(6) |
				  SRS03_CICE    |
				  SRS03_CRCCE   |
				  SRS03_RTS_48BIT_BUSY);

	while (!(mmio_read_32(hc + SRS12) &
				   SRS12_CC))
		;

	/* Response R1b - busy signal is transmitted on the DAT0 line */
	while (!(mmio_read_32(hc + SRS12) &
				   SRS12_TC))
		;
	/* Set PHY timings for 25 MHz, SDR */
	cdns_sdemmc_dfi_init(hc, 1);

	/* Disable SDCE - it will be re-enabled by sdhc_set_clk() */
	mmio_write_32(hc + SRS11, 0x00000000);
	udelay(10);

	/* Set eMMC mode in SDHC */
	mmio_write_32(hc + HRS06,
			   HRS06_EMM_SDR);

	/* Change (and enable) clock frequency (eMMC clock = 25 MHz) */
	cdns_sdemmc_set_clk(hc, 25000000);

	INFO("mmc@%lx: successfully initialized\n", hc);

	/* Clear partition access */
	mmio_write_32(hc + SRS12, 0xbff00ff);
	mmio_write_32(hc + SRS02, (179 << 16) | (0x7 << 8) | (2 << 24));
	mmio_write_32(hc + SRS03, SRS03_CIDX(6) |
				  SRS03_CICE	 |
				  SRS03_CRCCE	 |
				  SRS03_RTS_48BIT_BUSY);

	while (!(mmio_read_32(hc + SRS12) &
				   SRS12_CC))
		;

	while (!(mmio_read_32(hc + SRS12) &
				   SRS12_TC))
		;

	/* Set partition access to the boot partition (1 or 2) */
	mmio_write_32(hc + SRS12, 0xbff00ff);
	mmio_write_32(hc + SRS02, (179 << 16) | (0x1 << 8) | (1 << 24));
	mmio_write_32(hc + SRS03, SRS03_CIDX(6) |
				  SRS03_CICE	 |
				  SRS03_CRCCE	 |
				  SRS03_RTS_48BIT_BUSY);

	while (!(mmio_read_32(hc + SRS12) &
				   SRS12_CC))
		;

	while (!(mmio_read_32(hc + SRS12) &
				   SRS12_TC))
		;

	/* Set transfer data block length */
	mmio_write_32(hc + SRS12, 0xbff00ff);
	mmio_write_32(hc + SRS02, MMC_BLOCK_SIZE);
	mmio_write_32(hc + SRS03, SRS03_CIDX(16) |
				  SRS03_CICE	 |
				  SRS03_CRCCE	 |
				  SRS03_RTS_48BIT);

	while (!(mmio_read_32(hc + SRS12) &
				   SRS12_CC))
		;

	/* Get card status */
	mmio_write_32(hc + SRS12, 0xbff00ff);
	mmio_write_32(hc + SRS02, 0x1234 << 16);
	mmio_write_32(hc + SRS03, SRS03_CIDX(13) |
				  SRS03_CICE	 |
				  SRS03_CRCCE	 |
				  SRS03_RTS_48BIT);

	while (!(mmio_read_32(hc + SRS12) &
				   SRS12_CC))
		;

	INFO("mmc@%lx: card status: 0x%x\n", hc, mmio_read_32(hc + SRS04));
}

static bool cdns_sdemmc_card_inserted(const uintptr_t hc)
{
	return mmio_read_32(hc + SRS09) &
				 SRS09_CI;
}

static void cdns_sdemmc_data_read(const uintptr_t hc, const uintptr_t base, uint32_t *buf, size_t sz)
{
	unsigned int address;

	if (sector_mode) {
		/* Sector access mode */
		unsigned int sector = base / MMC_BLOCK_SIZE;

		if (sector * MMC_BLOCK_SIZE != base) {
			ERROR("mmc@%lx: unsupported address\n", hc);
			return;
		}

		address = sector;
	} else {
		/* Byte access mode */
		address = base;
	}

	const unsigned int block_cnt = sz / MMC_BLOCK_SIZE;

	if (block_cnt * MMC_BLOCK_SIZE != sz) {
		ERROR("mmc@%lx: unsupported data length\n", hc);
		return;
	}

	/* Set transfer block size (must be 512 byte as card default parameter after reset) */
	mmio_write_32(hc + SRS01, MMC_BLOCK_SIZE);

	for (unsigned int block = 0; block < block_cnt; block++) {
		/* Wait for SDHC ready for new command (CMD and DAT lines) */
		while (mmio_read_32(hc + SRS09) &
					(SRS09_CICMD |
					 SRS09_CIDAT))
			;

		/* CMD17 - "READ SINGLE BLOCK" */
		mmio_write_32(hc + SRS12, 0xbff00ff);
		mmio_write_32(hc + SRS02, address);
		mmio_write_32(hc + SRS03, SRS03_CIDX(17) |
					  SRS03_DPS	 |
					  SRS03_CICE	 |
					  SRS03_CRCCE	 |
					  SRS03_DTDS	 |
					  SRS03_RTS_48BIT);

		while (!(mmio_read_32(hc + SRS12) &
					   SRS12_CC))
			;

		while (!(mmio_read_32(hc + SRS12) &
					   SRS12_BRR))
			;

		/* Read data block */
		for (int i = 0; i < MMC_BLOCK_SIZE / 4; i++) {
			*buf++ = mmio_read_32(hc + SRS08);
		}

		/* Wait for transfer complete flag */
		while (!(mmio_read_32(hc + SRS12) &
					   SRS12_TC))
			;

		address += sector_mode ? 1 : MMC_BLOCK_SIZE;
	}
}

static void cdns_sdemmc_dfi_init(const uintptr_t hc, const unsigned int mode)
{
	uint32_t val;

	/* Assert PHY reset */
	mmio_clrbits_32(hc + HRS09,
			     HRS09_PHY_SW_RST);

	/* Select phy_dqs_timing_reg, enable phony dqs for card initialization */
	mmio_write_32(hc + HRS04, PHY_DQS_TIMING_REG);
	mmio_write_32(hc + HRS05, PHY_DQS_TIMING_USE_PHONY_DQS_CMD |
				  PHY_DQS_TIMING_USE_PHONY_DQS	   |
				  PHY_DQS_TIMING_USE_LPBK_DQS	   |
				  PHY_DQS_TIMING_USE_EXT_LPBK_DQS);

	mmio_write_32(hc + HRS04, PHY_GATE_LPBK_CTRL_REG);
	mmio_write_32(hc + HRS05, PHY_GATE_LPBK_CTRL_GATE_CFG_ALWAYS_ON	|
				  PHY_GATE_LPBK_CTRL_UNDERRUN_SUPPRESS	|
				  PHY_GATE_LPBK_CTRL_RD_DEL_SEL(52)	|
				  PHY_GATE_LPBK_CTRL_SYNC_METHOD);

	mmio_write_32(hc + HRS04, PHY_DLL_MASTER_CTRL_REG);
	val  = mmio_read_32(hc + HRS05);
	val &= ~((0x7 << 20) | 0xff);
	val |= PHY_DLL_MASTER_CTRL_DLL_START_POINT(4)  |
	       PHY_DLL_MASTER_CTRL_PHASE_DETECT_SEL(2) |
	       PHY_DLL_MASTER_CTRL_DLL_BYPASS_MODE;
	mmio_write_32(hc + HRS05, val);

	mmio_write_32(hc + HRS04, PHY_DLL_SLAVE_CTRL_REG);
	mmio_write_32(hc + HRS05, PHY_DLL_SLAVE_CTRL_READ_DQS_DELAY(0)	|
				  PHY_DLL_SLAVE_CTRL_CLK_WR_DELAY(0)	|
				  PHY_DLL_SLAVE_CTRL_CLK_WRDQS_DELAY(0)	|
				  PHY_DLL_SLAVE_CTRL_READ_DQS_CMD_DELAY(0));

	if (mode == 0) { /* initial SDHC settings */
		mmio_write_32(hc + HRS04, PHY_CTRL_REG);
		val  = mmio_read_32(hc + HRS05);
		val &= ~0x3f;
		val |= PHY_CTRL_PHONY_DQS_TIMING(val);
		mmio_write_32(hc + HRS05, val);
	}

	/* Deassert PHY reset */
	mmio_setbits_32(hc + HRS09,
			     HRS09_PHY_SW_RST);

	while (!(mmio_read_32(hc + HRS09) &
				   HRS09_PHY_INIT_COMPLETE))
		;

	mmio_write_32(hc + HRS04, PHY_DQ_TIMING_REG);
	val  = mmio_read_32(hc + HRS05);
	val &= ~(BIT(31) | (0x7 << 27) | (0x7 << 24) | (0x7 << 0));
	val |= PHY_DQ_TIMING_DATA_SELECT_OE_END(1) |
	       PHY_DQ_TIMING_IO_MASK_END(1);
	mmio_write_32(hc + HRS05, val);

	mmio_setbits_32(hc + HRS09,
			     HRS09_EXTENDED_RD_MODE |
			     HRS09_EXTENDED_WR_MODE |
			     HRS09_RD_CMD_EN	    |
			     HRS09_RD_DATA_EN);

	if (mode == 0) {
		mmio_clrsetbits_32(hc + HRS10,
					~(0xf << 16),
					HRS10_HCSDCLKADJ(1));
	} else {
		mmio_clrsetbits_32(hc + HRS10,
					~(0xf << 16),
					HRS10_HCSDCLKADJ(2));
	}

	mmio_write_32(hc + HRS16,
			   HRS16_WRCMD0_DLY(1)	      |
			   HRS16_WRCMD1_DLY(0)	      |
			   HRS16_WRDATA0_DLY(1)	      |
			   HRS16_WRDATA1_DLY(0)	      |
			   HRS16_WRCMD0_SDCLK_DLY(0)  |
			   HRS16_WRCMD1_SDCLK_DLY(0)  |
			   HRS16_WRDATA0_SDCLK_DLY(0) |
			   HRS16_WRDATA1_SDCLK_DLY(0));

	mmio_write_32(hc + HRS07,
			   HRS07_IDELAY_VAL(1) |
			   HRS07_RW_COMPENSATE(9));
}

void cdns_sdemmc_hc_init(const uintptr_t hc)
{
	static struct mmc_device_info info;

	info.mmc_dev_type = MMC_IS_EMMC;

	/* Software reset */
	mmio_write_32(hc + HRS00,
			   HRS00_SWR);

	/* Wait for software reset to complete */
	while (mmio_read_32(hc + HRS00) &
				 HRS00_SWR)
		;

	if (cdns_sdemmc_card_inserted(hc)) {
		/* Assert DFI reset */
		mmio_write_32(hc + HRS11, 0);
		mdelay(1);

		/* Deassert DFI reset */
		mmio_write_32(hc + HRS11,
				   HRS11_EMMC_RST);

		/* Initialize DFI-PHY */
		cdns_sdemmc_dfi_init(hc, 0);

		/* Enable SD bus power for VDD1 */
		mmio_write_32(hc + SRS10,
				   SRS10_BVS_1V8 |
				   SRS10_BP);

		/* Power-up time for 1.8 V should be less than 25 ms (JEDEC Standard No. 84-B51) */
		mdelay(25);

		cdns_sdemmc_set_clk(hc, 400000);

		/* Enable statuses */
		mmio_write_32(hc + SRS13, 0xbff61ff);

		cdns_sdemmc_card_init(hc);
	}

	ghc = hc;
	mmc_init(&cdns_sdemmc_ops, 25000000, MMC_BUS_WIDTH_8, 0, &info);
}

static void cdns_sdemmc_init(void)
{
}

static int cdns_sdemmc_prepare(const int lba, const uintptr_t buf, const size_t size)
{
	return 0;
}

static int cdns_sdemmc_read(const int lba, const uintptr_t buf, const size_t size)
{
	cdns_sdemmc_data_read(ghc, lba * 512, (uint32_t *)buf, size);

	return 0;
}

static int cdns_sdemmc_send_cmd(struct mmc_cmd *cmd)
{
	switch (cmd->cmd_idx) {
	case MMC_CMD(1):
		cmd->resp_data[0] = OCR_POWERUP;
		break;
	case MMC_CMD(9):
		cmd->resp_data[3] = 0x10;
		break;
	case MMC_CMD(13):
		mmio_write_32(ghc + SRS12, 0xbff00ff);
		mmio_write_32(ghc + SRS02, 0x1234 << 16);
		mmio_write_32(ghc + SRS03, SRS03_CIDX(13) |
					   SRS03_CICE	 |
					   SRS03_CRCCE	 |
					   SRS03_RTS_48BIT);

		while (!(mmio_read_32(ghc + SRS12) &
					    SRS12_CC))
			;

		cmd->resp_data[0] = mmio_read_32(ghc + SRS04);
		break;
	}

	return 0;
}

#define SDMCLK 200000000

static void cdns_sdemmc_set_clk(const uintptr_t hc, const uint32_t freq)
{
	const uint32_t sdcfs = (SDMCLK / 2 + freq - 1) / freq;

	/* Set clock */
	mmio_write_32(hc + SRS11,
			   SRS11_DTCV29			|
			   (((sdcfs >> 0) & 0xff) << 8)	|
			   (((sdcfs >> 8) & 0x3)  << 6)	|
			   SRS11_ICE);

	/* Wait for ICS */
	while (!(mmio_read_32(hc + SRS11) &
				   SRS11_ICS))
		;

	/* Assert PHY reset */
	mmio_clrbits_32(hc + HRS09,
			     HRS09_PHY_SW_RST);

	if (sdcfs > 0) {
		mmio_setbits_32(hc + HRS09,
				     HRS09_EXTENDED_WR_MODE);
	} else {
		mmio_clrbits_32(hc + HRS09,
				     HRS09_EXTENDED_WR_MODE);
	}

	/* Deassert PHY reset */
	mmio_setbits_32(hc + HRS09,
			     HRS09_PHY_SW_RST);

	while (!(mmio_read_32(hc + HRS09) &
				   HRS09_PHY_INIT_COMPLETE))
		;

	/* Enable SD clock */
	mmio_setbits_32(hc + SRS11,
			     SRS11_SDCE);
}

static int cdns_sdemmc_set_ios(const unsigned int clk, const unsigned int width)
{
	return 0;
}

static int cdns_sdemmc_write(const int lba, const uintptr_t buf, const size_t size)
{
	return 0;
}
