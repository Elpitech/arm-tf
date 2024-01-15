/*
 * Copyright (c) 2018-2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <arch.h>
#include <arch_helpers.h>
#include <common/bl_common.h>
#include <common/build_message.h>
#include <drivers/generic_delay_timer.h>
#include <plat/arm/common/plat_arm.h>
#include <plat/common/platform.h>

#include <baikal_bl1_stack.h>
#include <baikal_bootflash.h>
#include <baikal_console.h>
#include <baikal_def.h>
#include <baikal_io_storage.h>
#include <baikal_mshc.h>
#include <bm1000_ccn.h>
#include <bm1000_cmu.h>
#include <bm1000_def.h>
#include <bm1000_private.h>
#include <crc.h>
#include <ddr_main.h>
#include <ddr_spd.h>
#include <memtest.h>
#include <platform_def.h>

#include "bm1000_splash.h"

static uint64_t trusted_mailbox[1 + PLATFORM_CORE_COUNT]
	__section(".trusted_mailbox") __used;
CASSERT(sizeof(trusted_mailbox) == BAIKAL_TRUSTED_MAILBOX_SIZE,
	assert_trusted_mailbox_size);

/* Allocate space in static memory for DDR SPD content */
struct spd_container spd_content __attribute__((aligned(4))) = {0};
CASSERT(sizeof(spd_content) <= BAIKAL_SPD_MAX_SIZE,
	assert_spd_content_size);

#if !DEBUG
static char msg_buf[300];
#endif

void bl1_early_platform_setup(void)
{
	int err;

	baikal_console_boot_init();

	assert(trusted_mailbox == (void *)BAIKAL_TRUSTED_MAILBOX_BASE);
#if DEBUG
	INFO("BL1: crc32:0x%08x size:%lu\n",
		crc32((void *)BL1_RO_BASE, (uintptr_t)__BL1_ROM_END__ - (uintptr_t)BL1_RO_BASE, 0),
		(uintptr_t)__BL1_ROM_END__ - (uintptr_t)BL1_RO_BASE);

	bl1_stack_area_fill();
#endif
	write_cntfrq_el0(plat_get_syscnt_freq2());
	generic_delay_timer_init();

	bootflash_init();
	dram_init();
	ccn_hnf_sam_setup(cmu_pll_is_enabled(MMDDR0_CMU0_BASE) ?  4 : 0,
			  cmu_pll_is_enabled(MMDDR1_CMU0_BASE) ? 14 : 0);

	err  = memtest_rand64(BL1_XLAT_BASE, BL1_XLAT_SIZE, sizeof(uint64_t), read_cntpct_el0());
#if DEBUG
	err |= memtest_rand8(BL1_XLAT_BASE, BL1_XLAT_SIZE, sizeof(uint8_t), read_cntpct_el0());
#endif
	if (err) {
		plat_panic_handler();
	}

	/*
	 * Initialize Interconnect for this cluster during cold boot.
	 * No need for locks as no other CPU is active.
	 */
	plat_arm_interconnect_init();

	/* Enable Interconnect coherency for the primary CPU's cluster */
	plat_arm_interconnect_enter_coherency();

	/* Override A57 XP QoS values */
	ccn_xp_set_qos(0, 1, 0x8);
	ccn_xp_set_qos(4, 1, 0x8);
	ccn_xp_set_qos(5, 1, 0x8);
	ccn_xp_set_qos(9, 1, 0x8);
}

void bl1_plat_arch_setup(void)
{
	const mmap_region_t bl_regions[] = {
		MAP_REGION_FLAT(BL_CODE_BASE,
				BL1_CODE_END - BL_CODE_BASE,
				MT_NON_CACHEABLE | MT_RO | MT_SECURE | MT_EXECUTE),

		MAP_REGION_FLAT(BL1_RAM_BASE,
				round_up(BL1_RAM_LIMIT - BL1_RAM_BASE, PAGE_SIZE),
				MT_NON_CACHEABLE | MT_RW | MT_SECURE),
#if USE_COHERENT_MEM
		MAP_REGION_FLAT(BL_COHERENT_RAM_BASE,
				BL_COHERENT_RAM_END - BL_COHERENT_RAM_BASE,
				MT_DEVICE | MT_RW | MT_SECURE),
#endif
		{0}
	};

	const mmap_region_t plat_bm1000_mmap[] = {
		MAP_REGION_FLAT(MAILBOX_SRAM_BASE,
				MAILBOX_SRAM_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MAILBOX_IRB_BASE,
				MAILBOX_IRB_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMAVLSP_LCRU_BASE,
				MMAVLSP_LCRU_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMAVLSP_NIC_CFG_BASE,
				MMAVLSP_NIC_CFG_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMAVLSP_GPIO32_BASE,
				MMAVLSP_GPIO32_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),
#if defined(BAIKAL_DBM10) || defined(BAIKAL_DBM20) || defined(BAIKAL_QEMU_M)
		MAP_REGION_FLAT(MMAVLSP_SPI_BASE,
				MMAVLSP_SPI_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),
#endif
		MAP_REGION_FLAT(MMAVLSP_UART1_BASE,
				MMAVLSP_UART1_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMAVLSP_VDU_BASE,
				MMAVLSP_VDU_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),
#ifdef BAIKAL_MMC_FIP
		MAP_REGION_FLAT(MMAVLSP_EMMC_BASE,
				MMAVLSP_EMMC_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),
#endif
		MAP_REGION_FLAT(MMXGBE_LCRU_BASE,
				MMXGBE_LCRU_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMXGBE_VDU_BASE,
				MMXGBE_VDU_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMXGBE_HDMI_BASE,
				MMXGBE_HDMI_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(SEC_DRAM_BASE,
				SEC_DRAM_SIZE,
				MT_MEMORY | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(FB0_BASE,
				2 * FB_SIZE,
				MT_MEMORY | MT_RW | MT_NS),

		{0}
	};

	setup_page_tables(bl_regions, plat_bm1000_mmap);
	enable_mmu_el3(0);
}

void bl1_platform_setup(void)
{
	generic_delay_timer_init();
	mmavlsp_init();
#if !DEBUG
	snprintf(msg_buf, sizeof(msg_buf), "BE-M1000\nBL1: %s\nBL1: %s\n", build_version_string, build_message);
	lvds_early_splash(msg_buf);
	mmxgbe_init();
	hdmi_early_splash(msg_buf);
#endif

#ifdef BAIKAL_MMC_FIP
	dw_mshc_init();
#endif
	plat_baikal_io_setup();

	/*
	 * To limit amount of low-speed peripheral reads, that can be quite
	 * error prone, once the program read it and configured DRAM, put it
	 * into specific location to access later when required.
	 */
	memcpy((void *)BAIKAL_SEC_SPD_BASE, &spd_content, sizeof(spd_content));
}

/*
 * The function overrides the default ("weak") 'bl1_plat_handle_post_image_load'
 * from the 'plat/common/plat_bl1_common.c'. The weak implementation cannot be
 * used because BL1 RW + BL2 data size exceeds Mailbox SRAM.
 */
int bl1_plat_handle_post_image_load(unsigned int image_id)
{
	static meminfo_t bl2_tzram_layout;
	image_desc_t *image_desc;
	entry_point_info_t *ep_info;

	if (image_id != BL2_IMAGE_ID) {
		return 0;
	}

	/* Get the image descriptor */
	image_desc = bl1_plat_get_image_desc(BL2_IMAGE_ID);
	assert(image_desc != NULL);

	/* Get the entry point info */
	ep_info = &image_desc->ep_info;

	bl2_tzram_layout.total_base = BL2_BASE;
	bl2_tzram_layout.total_size = BL2_SIZE;

	flush_dcache_range((uintptr_t)&bl2_tzram_layout, sizeof(meminfo_t));
	ep_info->args.arg1 = (uintptr_t)&bl2_tzram_layout;
#if DEBUG
	bl1_stack_area_check();
#endif
	VERBOSE("BL1: BL2 memory layout address = %p\n",
		(void *)&bl2_tzram_layout);

	return 0;
}
