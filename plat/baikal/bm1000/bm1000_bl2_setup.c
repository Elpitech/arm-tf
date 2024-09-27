/*
 * Copyright (c) 2018-2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <common/bl_common.h>
#include <common/desc_image_load.h>
#include <drivers/generic_delay_timer.h>
#include <lib/xlat_tables/xlat_tables_v2.h>

#include <baikal_console.h>
#include <baikal_def.h>
#include <baikal_io_storage.h>
#include <baikal_mshc.h>
#include <platform_def.h>

#ifdef SPD_opteed
#include <optee_utils.h>
#endif

/* Data structure which holds the extents of the trusted SRAM for BL2 */
static meminfo_t bl2_tzram_layout __aligned(CACHE_WRITEBACK_GRANULE);

void bl2_early_platform_setup2(u_register_t arg0,
			       u_register_t arg1,
			       u_register_t arg2,
			       u_register_t arg3)
{
	meminfo_t *mem_layout = (meminfo_t *)arg1;

	baikal_console_boot_init();
	generic_delay_timer_init();

	/* Setup the BL2 memory layout */
	bl2_tzram_layout = *mem_layout;
#ifdef BAIKAL_MMC_FIP
	dw_mshc_init();
#endif
	plat_baikal_io_setup();
}

void bl2_plat_arch_setup(void)
{
	const mmap_region_t bl_regions[] = {
		MAP_REGION_FLAT(bl2_tzram_layout.total_base,
				bl2_tzram_layout.total_size,
				MT_MEMORY | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(BL_CODE_BASE,
				BL_CODE_END - BL_CODE_BASE,
				MT_CODE | MT_SECURE),
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
#ifdef BAIKAL_MMC_FIP
		MAP_REGION_FLAT(MMAVLSP_EMMC_BASE,
				MMAVLSP_EMMC_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),
#endif
		MAP_REGION_FLAT(SEC_DRAM_BASE,
				SEC_DRAM_SIZE,
				MT_MEMORY | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(NS_DRAM1_BASE,
				NS_DRAM1_SIZE,
				MT_MEMORY | MT_RW | MT_NS),

		{0}
	};

	setup_page_tables(bl_regions, plat_bm1000_mmap);
	enable_mmu_el1(0);
}

void bl2_platform_setup(void)
{
}

int bl2_plat_handle_post_image_load(unsigned int image_id)
{
	int err = 0;
	unsigned int mode;
	bl_mem_params_node_t *bl_mem_params = get_bl_mem_params_node(image_id);
#ifdef SPD_opteed
	bl_mem_params_node_t *pager_mem_params = NULL;
	bl_mem_params_node_t *paged_mem_params = NULL;
#endif
	assert(bl_mem_params);

	switch (image_id) {
#ifdef __aarch64__
	case BL32_IMAGE_ID:
#ifdef SPD_opteed
		pager_mem_params = get_bl_mem_params_node(BL32_EXTRA1_IMAGE_ID);
		assert(pager_mem_params);

		paged_mem_params = get_bl_mem_params_node(BL32_EXTRA2_IMAGE_ID);
		assert(paged_mem_params);

		err = parse_optee_header(&bl_mem_params->ep_info,
					 &pager_mem_params->image_info,
					 &paged_mem_params->image_info);
		if (err != 0) {
			WARN("OPTEE header parse error.\n");
		}

		/*
		 * OP-TEE expect to receive DTB address in x2.
		 * This will be copied into x2 by dispatcher.
		 */
		bl_mem_params->ep_info.args.arg3 = BAIKAL_SEC_DTB_BASE;
#endif
		/*
		 * The Secure Payload Dispatcher service is responsible for
		 * setting the SPSR prior to entry into the BL3-2 image.
		 */
		bl_mem_params->ep_info.spsr = 0;
		break;
#endif
	case BL33_IMAGE_ID:
		/* Figure out what mode we enter the non-secure world in */
		mode = (el_implemented(2) != EL_IMPL_NONE) ? MODE_EL2 : MODE_EL1;

		/* BL33 expects to receive the primary CPU MPID (through r0) */
		bl_mem_params->ep_info.args.arg0 = 0xffff & read_mpidr();
		bl_mem_params->ep_info.spsr = SPSR_64(mode, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);
		break;
	}

	return err;
}
