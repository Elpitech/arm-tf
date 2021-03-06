/*
 * Copyright (c) 2018-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <arch_helpers.h>
#include <assert.h>
#include <baikal_console.h>
#include <baikal_io_storage.h>
#include <bm1000_cmu.h>
#include <bm1000_def.h>
#include <bm1000_private.h>
#include <common/bl_common.h>
#include <ddr_main.h>
#include <drivers/generic_delay_timer.h>
#include <lib/mmio.h>
#include <memtest.h>
#include <plat/arm/common/plat_arm.h>
#include <plat/common/platform.h>
#include <platform_def.h>
#include <spi_dw.h>
#include "bm1000_splash.h"

static uint64_t trusted_mailbox[1 + PLATFORM_CORE_COUNT]
	__attribute__ ((used, section(".trusted_mailbox")));
CASSERT(sizeof(trusted_mailbox) == PLAT_BAIKAL_TRUSTED_MAILBOX_SIZE,
	assert_trusted_mailbox_size);

/* Data structure which holds the extents of the trusted SRAM for BL1 */
static meminfo_t bl2_tzram_layout;

/*
 * Cannot use default weak implementation in bl1_main.c because BL1 RW and BL2
 * data size exceeds Mailbox SRAM.
 */
int bl1_plat_handle_post_image_load(unsigned image_id)
{
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

	VERBOSE("BL1: BL2 memory layout address = %p\n",
		(void *)&bl2_tzram_layout);

	return 0;
}

static void ccn_hnf_sam_setup(void)
{
	unsigned i;
	uintptr_t pccn = PLAT_ARM_CCN_BASE + CCN_HNF_OFFSET + 8;
	uint64_t snf0 = 0, snf1 = 0;

	INFO("%s...\n", __func__);

	if (cmu_pll_is_enabled(MMDDR0_CMU0_BASE)) {
		snf0 = 4;
	}

	if (cmu_pll_is_enabled(MMDDR1_CMU0_BASE)) {
		snf1 = 14;
	}

	if (!snf0 && !snf1) {
		ERROR("%s: no DDRs\n", __func__);
		return;
	} else if (!snf0) {
		snf0 = snf1;
	} else if (!snf1) {
		snf1 = snf0;
	}

	INFO("%s(%llu, %llu)\n", __func__, snf0, snf1);
	for (i = 0; i < 4; ++i) {
		mmio_write_64(pccn, snf0);
		pccn += 0x10000;
	}

	for (i = 0; i < 4; ++i) {
		mmio_write_64(pccn, snf1);
		pccn += 0x10000;
	}

	dsbish();
	isb();
}

static void ccn_xp_set_qos(int xp, int dev, int qos)
{
	uint64_t val;

	val = mmio_read_64(XP_DEV_QOS_CONTROL(xp, dev));
	val &= ~DEV_QOS_OVERRIDE_MASK;
	val |= (DEV_QOS_OVERRIDE_EN | DEV_QOS_OVERRIDE(qos));
	mmio_write_64(XP_DEV_QOS_CONTROL(xp, dev), val);
}

void bl1_early_platform_setup(void)
{
	int err;

	baikal_console_boot_init();

	assert(trusted_mailbox == (void *)PLAT_BAIKAL_TRUSTED_MAILBOX_BASE);

	write_cntfrq_el0(plat_get_syscnt_freq2());
	generic_delay_timer_init();
	cmu_set_periph_rate2(MMCCN_CMU0_BASE, 25000000, 800000000);

#ifndef BAIKAL_QEMU
	dram_init();
#endif
	ccn_hnf_sam_setup();

	err  = memtest_rand64(BL1_XLAT_BASE, BL1_XLAT_SIZE, sizeof(uint64_t), read_cntpct_el0());
	err |= memtest_rand64(SEC_DRAM0_BASE, SEC_DRAM0_SIZE, 0x1000, read_cntpct_el0());
	err |= memtest_rand8( NS_DRAM0_BASE, MIN(0x10000, NS_DRAM0_SIZE), 1, read_cntpct_el0());
	err |= memtest_rand64(NS_DRAM0_BASE, NS_DRAM0_SIZE, 0x1000, read_cntpct_el0());
	err |= memtest_rand8( NS_DRAM1_BASE, MIN(0x10000, NS_DRAM1_SIZE), 1, read_cntpct_el0());
	err |= memtest_rand64(NS_DRAM1_BASE, NS_DRAM1_SIZE, 0x1000, read_cntpct_el0());
	if (err) {
		ERROR("%s: DRAM error\n", __func__);
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

void baikal_configure_mmu_bl1(unsigned long total_base,
			      unsigned long total_size,
			      unsigned long ro_start,
			      unsigned long ro_limit,
			      unsigned long coh_start,
			      unsigned long coh_limit);

void bl1_plat_arch_setup(void)
{
	baikal_configure_mmu_bl1(BL1_RW_BASE, BL1_RW_LIMIT - BL1_RW_BASE,
				 BL1_RO_BASE, BL1_RO_LIMIT,
				 BL_COHERENT_RAM_BASE, BL_COHERENT_RAM_END);
}

void bl1_platform_setup(void)
{
#if (DEBUG == 0)
	extern uint8_t bl1_logo[];
#endif
	plat_baikal_io_setup();
	dw_spi_init(0);
	mmxgbe_init();
#if (DEBUG == 0)
	hdmi_early_splash(bl1_logo);
#else
	hdmi_early_splash(NULL);
#endif
}
