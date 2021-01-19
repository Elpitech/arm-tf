/*
 * Copyright (c) 2015-2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <baikal_console.h>
#include <baikal_gicv3.h>
#include <bm1000_cmu.h>
#include <bm1000_private.h>
#include <common/bl_common.h>
#include <drivers/arm/gicv3.h>
#include <lib/mmio.h>
#include <plat/arm/common/plat_arm.h>
#include <plat/common/platform.h>
#include <platform_def.h>
#include "bm1000_splash.h"
#include "bm1000_vdu.h"

#define MALI_PVT_CCH_ON(addr, divider) CLKCH_ON(addr + PVT_CCH_OFFSET, divider)
#define CORTEX_PVT_CCH_ON(addr, divider) CLKCH_ON(addr + PVT_CCH_OFFSET, divider)

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

modeline_t fdt_lvds_video_mode;

/*
 * Placeholder variables for copying the arguments that have been passed to
 * BL3-1 from BL2.
 */
static entry_point_info_t bl32_image_ep_info;
static entry_point_info_t bl33_image_ep_info;

/*******************************************************************************
 * Perform any BL3-1 early platform setup.  Here is an opportunity to copy
 * parameters passed by the calling EL (S-EL1 in BL2 & S-EL3 in BL1) before
 * they are lost (potentially). This needs to be done before the MMU is
 * initialized so that the memory layout can be used while creating page
 * tables. BL2 has flushed this information to memory, so we are guaranteed
 * to pick up good data.
 ******************************************************************************/
void bl31_early_platform_setup2(u_register_t arg0, u_register_t arg1,
                                u_register_t arg2, u_register_t arg3)
{
	baikal_console_boot_init();

	/*
	 * Check params passed from BL2
	 */
	bl_params_t *params_from_bl2 = (bl_params_t *) arg0;

	assert(params_from_bl2);
	assert(params_from_bl2->h.type == PARAM_BL_PARAMS);
	assert(params_from_bl2->h.version >= VERSION_2);

	bl_params_node_t *bl_params = params_from_bl2->head;

	/*
	 * Copy BL33 and BL32 (if present), entry point information.
	 * They are stored in Secure RAM, in BL2's address space.
	 */
	while (bl_params) {
		if (bl_params->image_id == BL32_IMAGE_ID)
			bl32_image_ep_info = *bl_params->ep_info;

		if (bl_params->image_id == BL33_IMAGE_ID)
			bl33_image_ep_info = *bl_params->ep_info;

		bl_params = bl_params->next_params_info;
	}

	if (!bl33_image_ep_info.pc)
		panic();
}

void bl31_plat_arch_setup(void)
{
        plat_arm_interconnect_init();
        plat_arm_interconnect_enter_coherency();

	baikal_configure_mmu_el3(BL31_RO_BASE, (BL31_END - BL31_RO_BASE),
			      BL31_RO_BASE, BL31_RO_LIMIT,
			      BL_COHERENT_RAM_BASE, BL_COHERENT_RAM_END);

}

void init_peripherals(void)
{
	int fb_cpp;
	int w1, h1;
	int w2, h2;

	// Prepare hardware
	INFO("Initialize...\n");
	INFO("LSP\n");
	mmlsp_on();
	mmlsp_toNSW();

#ifndef BE_QEMU
	INFO("Cortex PVT\n");
	CORTEX_PVT_CCH_ON(A57_0_PVTCC_ADDR, PVT_DIV);
	CORTEX_PVT_CCH_ON(A57_1_PVTCC_ADDR, PVT_DIV);
	CORTEX_PVT_CCH_ON(A57_2_PVTCC_ADDR, PVT_DIV);
	CORTEX_PVT_CCH_ON(A57_3_PVTCC_ADDR, PVT_DIV);
#endif

	INFO("XGBE\n");
	fb_cpp = bmp_to_fb((uintptr_t) FB2_BASE, &hdmi_video_mode, bl31_logo, 0, 0, 1);
	if (fb_cpp) {
		if (fb_cpp > 2) {
			// Put SDK version just behind the logo, aligned to its right edge
			bmp_get_dimensions(bl31_logo, &w1, &h1);
			bmp_get_dimensions(bl31_sdk_version_logo, &w2, &h2);
			bmp_to_fb((uintptr_t) FB2_BASE,
					&hdmi_video_mode, bl31_sdk_version_logo,
					(w1 - w2) / 2, (h1 + h2) / 2, 0);
		}
		wait_for_vblank(MMXGBE_VDU_BASE);
		vdu_set_fb(MMXGBE_VDU_BASE, (uintptr_t) FB2_BASE,
				&hdmi_video_mode, fb_cpp);
	}
	mmxgbe_toNSW();

	INFO("USB\n");
	mmusb_on();
	mmusb_toNSW();
	mmusb_chc();
	mmusb_initSATA();

	INFO("Mali\n");
	mmmali_on();
	mmmali_toNSW();

#ifndef BE_QEMU
	INFO("Mali PVT\n");
	MALI_PVT_CCH_ON(MALI_PVTCC_ADDR, PVT_DIV);
#endif


	INFO("PCI\n");
	mmpci_on();
	mmpci_toNSW();

	INFO("VDEC\n");
	mmvdec_on();
	mmvdec_toNSW();
	INFO("...done\n");
}

void bl31_platform_setup(void)
{
	// initialize macromodules and prepare for NS
	init_peripherals();
	baikal_gic_driver_init();
	baikal_gic_init();
}

void bl31_plat_enable_mmu(uint32_t flags)
{
//        plat_arm_interconnect_init();
        plat_arm_interconnect_enter_coherency();

	enable_mmu_el3(flags);
}


void bl31_plat_runtime_setup(void)
{
	extern struct baikal_clk pclk_list[];
	struct baikal_clk *lvds_vdu_cmu = NULL;
	int LVDS_VDU_CMU_HW_ID = 0x20010000;
	int fb_cpp;
	int w1, h1;
	int w2, h2;

	/* Set current_frequencies from device tree */
	int i = 0;
	while(pclk_list[i].name) {
		if (pclk_list[i].hw_id == LVDS_VDU_CMU_HW_ID)
			lvds_vdu_cmu = &pclk_list[i];
		else if (pclk_list[i].is_cpu == false &&
		    pclk_list[i].deny_reconfig_pll == false) {
			/* Might be a useful hack, to set NR to parent frequency/10^6 */
			cmu_reconfig_nr(pclk_list[i].hw_id);
		} else {
			if (pclk_list[i].is_cpu == true)
				cmu_corepll_set_clken_gen(pclk_list[i].hw_id, CA57_SCLKEN, 2);
			cmu_pll_set_rate(pclk_list[i].hw_id, pclk_list[i].parent_freq, pclk_list[i].current_freq);
		}
		i++;
	}

	fb_cpp = bmp_to_fb((uintptr_t) FB0_BASE, lvds_video_mode, bl31_logo, 0, 0, 1);
	if (fb_cpp) {
		vdu_set_fb(MMAVLSP_VDU_BASE, (uintptr_t) FB0_BASE,
				lvds_video_mode, fb_cpp);
		if (fb_cpp > 2) {
			// Put SDK version just behind the logo, aligned to its right edge
			bmp_get_dimensions(bl31_logo, &w1, &h1);
			bmp_get_dimensions(bl31_sdk_version_logo, &w2, &h2);
			bmp_to_fb((uintptr_t) FB0_BASE,
					lvds_video_mode, bl31_sdk_version_logo,
					(w1 - w2) / 2, (h1 + h2) / 2, 0);
		}
		if (!fdt_get_panel(&fdt_lvds_video_mode)) {
			lvds_video_mode = &fdt_lvds_video_mode;
		}
		if (lvds_vdu_cmu)
			cmu_pll_set_rate(LVDS_VDU_CMU_HW_ID, lvds_vdu_cmu->parent_freq, lvds_video_mode->clock * 7);
		vdu_init(MMAVLSP_VDU_BASE, (uintptr_t) FB0_BASE, lvds_video_mode);
	}
}

unsigned int plat_get_syscnt_freq2(void)
{
	return SYS_COUNTER_FREQ_IN_TICKS;
}

/*******************************************************************************
 * Return a pointer to the 'entry_point_info' structure of the next image
 * for the security state specified. BL3-3 corresponds to the non-secure
 * image type while BL3-2 corresponds to the secure image type. A NULL
 * pointer is returned if the image does not exist.
 ******************************************************************************/
entry_point_info_t *bl31_plat_get_next_image_ep_info(uint32_t type)
{
	entry_point_info_t *next_image_info;

	assert(sec_state_is_valid(type));
	next_image_info = (type == NON_SECURE)
			? &bl33_image_ep_info : &bl32_image_ep_info;
	/*
	 * None of the images on the ARM development platforms can have 0x0
	 * as the entrypoint
	 */
	if (next_image_info->pc)
		return next_image_info;
	else
		return NULL;
}
