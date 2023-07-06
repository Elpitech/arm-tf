/*
 * Copyright (c) 2022, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>

#include <baikal_scp.h>
#include <bs1000_cmu.h>
#include <bs1000_def.h>
#include <bs1000_scp_lcru.h>
#include <bs1000_usb.h>

/* Capability register map */
#define EHC_CAPLENGTH_OFFSET		0x00 /* Capability register length */
#define EHC_HCSPARAMS_OFFSET		0x04 /* Structural parameters */
#define EHC_HCCPARAMS_OFFSET		0x08 /* Capability parameters */
#define EHC_HCSPPORTROUTE_OFFSET	0x0c

/* Operational register map */
#define EHC_USBCMD_OFFSET		0x10 /* USB command register */
#define EHC_USBSTS_OFFSET		0x14 /* USB status register */
#define EHC_USBINTR_OFFSET		0x18 /* USB interrupt enable register */
#define EHC_FRINDEX_OFFSET		0x1c /* USB frame index register */
#define EHC_CTRLDSSEG_OFFSET		0x20 /* Control data structure segment register */
#define EHC_FRAME_BASE_OFFSET		0x24 /* Frame list base address register */
#define EHC_ASYNC_HEAD_OFFSET		0x28 /* Next asynchronous list address register */
#define EHC_CONFIG_FLAG_OFFSET		0x50 /* Configured flag register */
#define EHC_PORT_STAT_OFFSET		0x54 /* Port status/control register */
#define EHC_MICROFRAME_VALUE		0x90 /* Microframe control register */
#define EHC_ULPI_CTL_OFFSET		0x9c /* ULPI control register */
#define EHC_ULPI_PORT_OFFSET		0xa0 /* ULPI port control register */
#define EHC_PHY_CTL_OFFSET		0xa4 /* PHY control register */

void bs1000_usb_init(void)
{
	uint32_t value;

	/* Set USB microframe */

	/* 6.1 */
	scp_lcru_setbits(SCP_GPR_MM_RST_CTL1, SCP_GPR_MM_RST_CTL1_USB_HRST);
	scp_lcru_setbits(SCP_GPR_MM_RST_CTL1, SCP_GPR_MM_RST_CTL1_USB_AUX_WELL_RST);
	scp_lcru_setbits(SCP_GPR_MM_RST_CTL1, SCP_GPR_MM_RST_CTL1_USB_UTMI_PHY_RST);
	scp_lcru_setbits(SCP_GPR_MM_RST_CTL1, SCP_GPR_MM_RST_CTL1_OHCI_0_CLKCKT_RST);
	scp_lcru_setbits(SCP_GPR_MM_RST_CTL1, SCP_GPR_MM_RST_CTL1_OHCI_0_CLKDIV_RST);

	/* 6.2 */
	scp_lcru_clrbits(SCP_GPR_MM_RST_CTL1, SCP_GPR_MM_RST_CTL1_OHCI_0_CLKDIV_RST);

	/* 6.3 */
	cmu_clkch_set_rate(0x4100f0, 100 * 1000 * 1000);
	cmu_clkch_set_rate(0x410100, 48 * 1000 * 1000);
	cmu_clkch_enable(0x4100f0);
	cmu_clkch_enable(0x410100);

	/* 6.4, set USB FLADJ */
	scp_lcru_read(SCP_GPR_USB_CTL, &value);
	value &= ~SCP_GPR_USB_CTL_FLADJ_VALUE_MASK;
	value &= ~SCP_GPR_USB_CTL_FLADJ_ENABLE_MASK;
	value |= 32 << SCP_GPR_USB_CTL_FLADJ_ENABLE_SHIFT;
	value |= 32 << SCP_GPR_USB_CTL_FLADJ_VALUE_SHIFT;
	scp_lcru_write(SCP_GPR_USB_CTL, value);

	/* 6.5 */
	scp_lcru_clrbits(SCP_GPR_MM_RST_CTL1, SCP_GPR_MM_RST_CTL1_OHCI_0_CLKCKT_RST);
	scp_lcru_clrbits(SCP_GPR_MM_RST_CTL1, SCP_GPR_MM_RST_CTL1_USB_UTMI_PHY_RST);
	scp_lcru_clrbits(SCP_GPR_MM_RST_CTL1, SCP_GPR_MM_RST_CTL1_USB_AUX_WELL_RST);
	mdelay(1);
	scp_lcru_clrbits(SCP_GPR_MM_RST_CTL1, SCP_GPR_MM_RST_CTL1_USB_HRST);
}
