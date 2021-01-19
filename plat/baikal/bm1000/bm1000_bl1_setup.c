/*
 * Copyright (c) 2018-2020, Baikal Electronics JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <arch_helpers.h>
#include <assert.h>
#include <baikal_console.h>
#include <baikal_io_storage.h>
#include <bm1000_private.h>
#include <common/bl_common.h>
#include <plat/arm/common/plat_arm.h>
#include <plat/common/platform.h>
#include <platform_def.h>
#include <spi_dw.h>
#include <tests_common.h>
#include "bm1000_splash.h"

PLAT_BAIKAL_TRUSTED_MAILBOX_DEC;

/* Data structure which holds the extents of the trusted SRAM for BL1 */
static meminfo_t bl2_tzram_layout;

/*
 * Cannot use default weak implementation in bl1_main.c because BL1 RW and BL2
 * data size exceeds Mailbox SRAM.
 */
int bl1_plat_handle_post_image_load(unsigned int image_id)
{
	image_desc_t *image_desc;
	entry_point_info_t *ep_info;

	if (image_id != BL2_IMAGE_ID)
		return 0;

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

#define DDR0_LCRU			0x0E000000
#define DDR1_LCRU			0x22000000
#define MEMCH0				1
#define MEMCH1				2
#define CCN_PERIPH			0x9000000
#define HNF				0x200000

extern uint8_t bl1_logo[];

void init_hnf(uint64_t sbsx_id) {
	uintptr_t pccn = CCN_PERIPH + HNF + 8;
	int	i;
	INFO("DDR update HNFs 0x%llx\n", sbsx_id);
	for (i = 0; i < 8; ++i) {
		mmio_write_64(pccn, sbsx_id);
		pccn += 0x10000;
	}
	dsbish();
	isb();
}

void init_hnf_duo(void) {
	uintptr_t pccn = CCN_PERIPH + HNF + 8;
	int	i;
	INFO("DDR update HNFs duo\n");
	for (i=0; i < 4; ++i) {
		mmio_write_64(pccn, 4);
		pccn += 0x10000;
	}
	for (i=0; i < 4; ++i) {
		mmio_write_64(pccn, 14);
		pccn += 0x10000;
	}
	dsbish();
	isb();
}

void ddr_init(void) {
	uint32_t	memchs = 0;
	uintptr_t pddr0 = DDR0_LCRU;
	uintptr_t pddr1 = DDR1_LCRU;
	INFO("DDR check...\n");
	if(((mmio_read_32(pddr0 + LCRU_CTL0) & LCRU_CTL0_LOCK) == LCRU_CTL0_LOCK) &&
	   ((mmio_read_32(pddr0 + LCRU_CTL6) & LCRU_CTL6_SWEN) == LCRU_CTL6_SWEN)) {
		memchs |= MEMCH0;
	}
	if(((mmio_read_32(pddr1 + LCRU_CTL0) & LCRU_CTL0_LOCK) == LCRU_CTL0_LOCK) &&
	   ((mmio_read_32(pddr1 + LCRU_CTL6) & LCRU_CTL6_SWEN) == LCRU_CTL6_SWEN)) {
		memchs |= MEMCH1;
	}
	INFO("DDR mask 0x%x\n", memchs);
	if(memchs == MEMCH0) {
		init_hnf(4);
	} else if(memchs == MEMCH1) {
		init_hnf(14);
	} else if (memchs == (MEMCH0|MEMCH1)) {
		init_hnf_duo();
	} else {
		ERROR("NO DDRs!\n");
//		while(1) {};
	}
}

void tzc_init(void) {
	INFO("TZC - to do\n");
}

void	xlat_region_check(void) {
	uint32_t	v;
	uint32_t	*pos;
//	uint32_t	i = 1;
	INFO("Memtest %p size %x\n", (void*)BL1_XLAT_BASE, BL1_XLAT_SIZE);
	pos = (uint32_t*)(BL1_XLAT_BASE);
//	while(i) {};
	INFO("Write...\n");
	while(pos < (uint32_t*)(BL1_XLAT_BASE + BL1_XLAT_SIZE)) {
		v = (uint32_t)(((uint64_t)pos) & 0xffffffff);
		*pos = v;
		pos++;
	}
	INFO("Verify...\n");
	pos = (uint32_t*)(BL1_XLAT_BASE);
	while(pos < (uint32_t*)(BL1_XLAT_BASE + BL1_XLAT_SIZE)) {
		v = (uint32_t)(((uint64_t)pos) & 0xffffffff);
		if(*pos != v) {
			ERROR(" [%x] != %x\n", *pos, v);
		}
		pos++;
	}
	INFO("Done...\n");
}

/*******************************************************************************
 * Perform any BL1 specific platform actions.
 ******************************************************************************/
void bl1_early_platform_setup(void)
{
	/* Initialize the console to provide early debug support */
	baikal_console_boot_init();
	ddr_init();
#ifndef BE_VCS
	xlat_region_check();
#endif
	tzc_init();
}

/******************************************************************************
 * Perform the very early platform specific architecture setup.  This only
 * does basic initialization. Later architectural setup (bl1_arch_setup())
 * does not do anything platform specific.
 *****************************************************************************/
void baikal_configure_mmu_bl1(unsigned long total_base,
			   unsigned long total_size,
			   unsigned long ro_start,
			   unsigned long ro_limit,
			   unsigned long coh_start,
			   unsigned long coh_limit);


#if defined(BE_BL1_TST0) || defined(BE_BL1_TST1)
extern bl1_test __BL1_TESTS_DESC_START__;
extern bl1_test __BL1_TESTS_DESC_END__;
// logic to execute tests according to description (1.1... 2.1...)
bl1_test* get_next_test(bl1_test* test_prev) {
	bl1_test*	test_pos = &__BL1_TESTS_DESC_START__;
	bl1_test*	test_end = &__BL1_TESTS_DESC_END__;
	bl1_test*	test_sel = NULL;
	if (test_prev == test_end) return NULL;
	while(test_pos < test_end) {
		if ((test_pos->status & TEST_STATUS_DONE) == 0) {
			if ((test_sel == NULL) ||
			    (strcmp(test_pos->desc, test_sel->desc) < 0)) {
				test_sel = test_pos;
			}
		}
		test_pos++;
	}
	return test_sel;
}

void tests_status_reset(void) {
	bl1_test*	test_pos = &__BL1_TESTS_DESC_START__;
	bl1_test*	test_end = &__BL1_TESTS_DESC_END__;
	while(test_pos < test_end) {
		test_pos->status = 0;
		test_pos++;
	}
}

void tests_run(void) {
	bl1_test*	test_pos = &__BL1_TESTS_DESC_START__;
	while((test_pos = get_next_test(test_pos)) != NULL) {
			TEST_PRINT("# %s\n", test_pos->desc);
			(test_pos->run)();
			test_pos->status = TEST_STATUS_DONE;
	}
}

#endif

void bl1_plat_arch_setup(void)
{

#ifdef BE_BL1_TST0
	TEST_PRINT("=== Test phase 0\n");
	tests_run();
#endif

	baikal_configure_mmu_bl1(BL1_RW_BASE, BL1_RW_LIMIT - BL1_RW_BASE,
				 BL1_RO_BASE, BL1_RO_LIMIT,
				 BL_COHERENT_RAM_BASE, BL_COHERENT_RAM_END);
}

void bl1_platform_setup(void)
{
	/*
	 * Initialize Interconnect for this cluster during cold boot.
	 * No need for locks as no other CPU is active.
	 */
        plat_arm_interconnect_init();
        /*
	 * Enable Interconnect coherency for the primary CPU's cluster.
         */
        plat_arm_interconnect_enter_coherency();

//	write_cntfrq_el0(7330000);
#ifdef BE_BL1_TST1
#ifdef BE_BL1_TST0
	tests_status_reset();
#endif
	TEST_PRINT("=== Test phase 1\n");
	tests_run();
#endif
	plat_baikal_io_setup();

	dw_spi_init(0,0);

	mmxgbe_on();
	hdmi_early_splash(bl1_logo);
}
