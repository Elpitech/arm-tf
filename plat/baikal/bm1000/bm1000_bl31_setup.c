/*
 * Copyright (c) 2018-2022, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/bl_common.h>
#include <common/debug.h>
#include <drivers/generic_delay_timer.h>
#include <plat/arm/common/plat_arm.h>
#include <plat/common/platform.h>

#include <baikal_gicv3.h>
#include <bm1000_cmu.h>
#include <bm1000_def.h>
#include <bm1000_private.h>
#include <platform_def.h>

#include "bm1000_mmca57.h"
#include "bm1000_splash.h"
#include "bm1000_vdu.h"

/*
 * The next 3 constants identify the extents of the code, RO data region and the
 * limit of the BL3-1 image.  These addresses are used by the MMU setup code and
 * therefore they must be page-aligned.  It is the responsibility of the linker
 * script to ensure that __RO_START__, __RO_END__ & __BL31_END__ linker symbols
 * refer to page-aligned addresses.
 */
#define BL31_RO_BASE (unsigned long)(&__RO_START__)
#define BL31_RO_LIMIT (unsigned long)(&__RO_END__)
#define BL31_END (unsigned long)(&__BL31_END__)

extern uint8_t bl31_logo[];
extern uint8_t bl31_sdk_version_logo[];

void bl31_plat_arch_setup(void)
{
	plat_arm_interconnect_init();
	plat_arm_interconnect_enter_coherency();

	baikal_configure_mmu_el3(BL31_RO_BASE, (BL31_END - BL31_RO_BASE),
				 BL31_RO_BASE, BL31_RO_LIMIT,
				 BL_COHERENT_RAM_BASE, BL_COHERENT_RAM_END);
}

void bl31_platform_setup(void)
{
	int fb_cpp;

	generic_delay_timer_init();

	INFO("Init AVLSP...\n");
	mmavlsp_init();

#ifndef BAIKAL_QEMU
	INFO("Init CA57 PVTs...\n");
	cmu_clkch_enable_by_base(MMCA57_0_PVT_CLKCHCTL, PVT_CLKCH_DIV);
	cmu_clkch_enable_by_base(MMCA57_1_PVT_CLKCHCTL, PVT_CLKCH_DIV);
	cmu_clkch_enable_by_base(MMCA57_2_PVT_CLKCHCTL, PVT_CLKCH_DIV);
	cmu_clkch_enable_by_base(MMCA57_3_PVT_CLKCHCTL, PVT_CLKCH_DIV);
#endif

	INFO("Init XGbE...\n");
	fb_cpp = bmp_to_fb((uintptr_t)FB2_BASE, &hdmi_video_mode, bl31_logo, 0, 0, 1);
	if (fb_cpp > 2) { /* we do not support 16-bit and lower color resolutions here */
		int h1, h2, w1, w2;

		/* Put SDK version just behind the logo, aligned to its right edge */
		bmp_get_dimensions(bl31_logo, &w1, &h1);
		bmp_get_dimensions(bl31_sdk_version_logo, &w2, &h2);
		bmp_to_fb((uintptr_t)FB2_BASE,
			  &hdmi_video_mode,
			  bl31_sdk_version_logo,
			  (w1 - w2) / 2,
			  (h1 + h2) / 2,
			  0);
#ifndef BAIKAL_QEMU
		wait_for_vblank(MMXGBE_VDU_BASE);
#endif
		vdu_set_fb(MMXGBE_VDU_BASE,
			   (uintptr_t)FB2_BASE,
			   &hdmi_video_mode,
			   fb_cpp);
	}

	mmxgbe_ns_access();

	INFO("Init USB...\n");
	mmusb_init();

	INFO("Init Mali...\n");
	mmmali_init();

	INFO("Init Mali PVT...\n");
	cmu_clkch_enable_by_base(MMMALI_PVT_CLKCHCTL, PVT_CLKCH_DIV);

	INFO("Init PCIe...\n");
	mmpcie_init();

	INFO("Init VDec...\n");
	mmvdec_init();

	baikal_gic_driver_init();
	baikal_gic_init();
}

void bl31_plat_enable_mmu(uint32_t flags)
{
	enable_mmu_el3(flags);
}

void bl31_plat_runtime_setup(void)
{
	int fb_cpp;
	unsigned idx;
	struct cmu_desc *lvds_vdu_cmu = NULL;

	/* Set frequencies from device tree */
	for (idx = 0; ; ++idx) {
		struct cmu_desc *const cmu = cmu_desc_get_by_idx(idx);
		if (cmu == NULL || !cmu->base) {
			break;
		}

		INFO("%s: base=0x%08lx name=%s rate=%lu\n", __func__, cmu->base, cmu->name, cmu->fpllreq);

		if (cmu->base == MMAVLSP_CMU1_BASE) {
			lvds_vdu_cmu = cmu;
			continue;
		}

		cmu_pll_set_rate(cmu->base, cmu->frefclk, cmu->fpllreq);
	}

	if (!fdt_get_panel(&lvds_video_mode)) {
		fb_cpp = bmp_to_fb((uintptr_t)FB0_BASE, &lvds_video_mode,
				   bl31_logo, 0, 0, 1);
		if (fb_cpp > 2) { /* we do not support 16-bit and lower color resolutions here */
			int h1, h2, w1, w2;

			vdu_set_fb(MMAVLSP_VDU_BASE,
				   (uintptr_t)FB0_BASE,
				   &lvds_video_mode,
				   fb_cpp);

			/*
			 * Put SDK version just behind the logo,
			 * aligned to its right edge
			 */
			bmp_get_dimensions(bl31_logo, &w1, &h1);
			bmp_get_dimensions(bl31_sdk_version_logo, &w2, &h2);
			bmp_to_fb((uintptr_t)FB0_BASE,
				  &lvds_video_mode,
				  bl31_sdk_version_logo,
				  (w1 - w2) / 2,
				  (h1 + h2) / 2,
				  0);

			if (lvds_vdu_cmu != NULL) {
				cmu_pll_set_rate(lvds_vdu_cmu->base,
						 lvds_vdu_cmu->frefclk,
						 lvds_video_mode.clock * 7);
			}

			vdu_init(MMAVLSP_VDU_BASE, (uintptr_t)FB0_BASE,
				 &lvds_video_mode);
		}
	}
}
