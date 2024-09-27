/*
 * Copyright (c) 2022-2024, Baikal Electronics, JSC. All rights reserved.
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
#include <bs1000_sc_lcru.h>
#include <platform_def.h>

#include "bs1000_usb.h"

void bs1000_usb_init(void)
{
	uint32_t value;
	unsigned int chip_idx;

	for (chip_idx = 0; chip_idx < PLATFORM_CHIP_COUNT; ++chip_idx) {
		/* Set USB microframe */

		/* 6.1 */
		sc_lcru_setbits(PLATFORM_ADDR_OUT_CHIP(chip_idx, SC_GPR_MMRST1),
				SC_GPR_MMRST1_USB_AHB);
		sc_lcru_setbits(PLATFORM_ADDR_OUT_CHIP(chip_idx, SC_GPR_MMRST1),
				SC_GPR_MMRST1_USB_AUX_WELL);
		sc_lcru_setbits(PLATFORM_ADDR_OUT_CHIP(chip_idx, SC_GPR_MMRST1),
				SC_GPR_MMRST1_USB_PHY);
		sc_lcru_setbits(PLATFORM_ADDR_OUT_CHIP(chip_idx, SC_GPR_MMRST1),
				SC_GPR_MMRST1_OHCI_0_CLKCKT);
		sc_lcru_setbits(PLATFORM_ADDR_OUT_CHIP(chip_idx, SC_GPR_MMRST1),
				SC_GPR_MMRST1_OHCI_0_CLKDIV);

		/* 6.2 */
		sc_lcru_clrbits(PLATFORM_ADDR_OUT_CHIP(chip_idx, SC_GPR_MMRST1),
				SC_GPR_MMRST1_OHCI_0_CLKDIV);

		/* 6.3 */
		cmu_clkch_set_rate(PLATFORM_ADDR_OUT_CHIP(chip_idx, 0x4100f0),
				   100 * 1000 * 1000);
		cmu_clkch_set_rate(PLATFORM_ADDR_OUT_CHIP(chip_idx, 0x410100),
				   48 * 1000 * 1000);
		cmu_clkch_enable(PLATFORM_ADDR_OUT_CHIP(chip_idx, 0x4100f0));
		cmu_clkch_enable(PLATFORM_ADDR_OUT_CHIP(chip_idx, 0x410100));

		/* 6.4, set USB FLADJ */
		sc_lcru_read(PLATFORM_ADDR_OUT_CHIP(chip_idx, SC_GPR_USB_CTL),
			     &value);
		value &= ~SC_GPR_USB_CTL_FLADJ_VALUE_MASK;
		value &= ~SC_GPR_USB_CTL_FLADJ_ENABLE_MASK;
		value |= 32 << SC_GPR_USB_CTL_FLADJ_ENABLE_SHIFT;
		value |= 32 << SC_GPR_USB_CTL_FLADJ_VALUE_SHIFT;
		sc_lcru_write(PLATFORM_ADDR_OUT_CHIP(chip_idx, SC_GPR_USB_CTL),
			      value);

		/* 6.5 */
		sc_lcru_clrbits(PLATFORM_ADDR_OUT_CHIP(chip_idx, SC_GPR_MMRST1),
				SC_GPR_MMRST1_OHCI_0_CLKCKT);
		sc_lcru_clrbits(PLATFORM_ADDR_OUT_CHIP(chip_idx, SC_GPR_MMRST1),
				SC_GPR_MMRST1_USB_PHY);
		sc_lcru_clrbits(PLATFORM_ADDR_OUT_CHIP(chip_idx, SC_GPR_MMRST1),
				SC_GPR_MMRST1_USB_AUX_WELL);
		mdelay(1);
		sc_lcru_clrbits(PLATFORM_ADDR_OUT_CHIP(chip_idx, SC_GPR_MMRST1),
				SC_GPR_MMRST1_USB_AHB);
	}
}
