/*
 * Copyright (c) 2015-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <arch_helpers.h>
#include <assert.h>
#include <baikal_console.h>
#include <baikal_io_storage.h>
#include <bm1000_i2c.h>
#include <bm1000_private.h>
#include <bm1000_smbus.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <common/desc_image_load.h>
#include <crc16.h>
#include <lib/utils.h>
#include <libfdt.h>
#ifdef SPD_opteed
#include <optee_utils.h>
#endif
#include <platform_def.h>
#include <spd.h>
#include <string.h>

/*
 * The next 2 constants identify the extents of the code & RO data region.
 * These addresses are used by the MMU setup code and therefore they must be
 * page-aligned.  It is the responsibility of the linker script to ensure that
 * __RO_START__ and __RO_END__ linker symbols refer to page-aligned addresses.
 */
#define BL2_RO_BASE (unsigned long)(&__RO_START__)
#define BL2_RO_LIMIT (unsigned long)(&__RO_END__)

/* Data structure which holds the extents of the trusted SRAM for BL2 */
static meminfo_t bl2_tzram_layout __aligned(CACHE_WRITEBACK_GRANULE);

void bl2_early_platform_setup2(u_register_t arg0, u_register_t arg1,
                               u_register_t arg2, u_register_t arg3)
{
	meminfo_t *mem_layout = (meminfo_t *) arg1;
	baikal_console_boot_init();

	/* Setup the BL2 memory layout */
	bl2_tzram_layout = *mem_layout;
	plat_baikal_io_setup();
}

static void security_setup(void)
{
	/*
	 * This is where a TrustZone address space controller and other
	 * security related peripherals, would be configured.
	 */
}

static void dt_add_psci(void *fdt)
{
	if (dt_add_psci_node(fdt)) {
		ERROR("BL2: failed to add PSCI node to FDT\n");
		return;
	}

	if (dt_add_psci_cpu_enable_methods(fdt)) {
		ERROR("BL2: failed to add PSCI cpu enable methods to FDT\n");
		return;
	}
}

void bl2_platform_setup(void)
{
	void *fdt = (void *)(uintptr_t)PLAT_BAIKAL_DT_BASE;
	int ret;

	security_setup();

	ret = fdt_open_into(fdt, fdt, PLAT_BAIKAL_DT_MAX_SIZE);
	if (ret >= 0) {
#if defined(BE_DBM) || defined(BE_MITX)
		unsigned dimm_idx;
#if defined(BE_DBM)
		// DBM has 4 DIMM slots with the following SPD addresses: 0x50, 0x51, 0x52, 0x53.
		// But only 0x50 and 0x52 are used by SCP.
		static const uint8_t dimm_spd_addrs[] = {0x50, 0x52};
#elif defined (BE_MITX)
		static const uint8_t dimm_spd_addrs[] = {0x50, 0x52};
#endif
		unsigned long long total_capacity = 0;

		// Read DDR4 DIMM SPD EEPROMs for base configuration and DRAM parameters
		for (dimm_idx = 0; dimm_idx < sizeof dimm_spd_addrs / sizeof dimm_spd_addrs[0]; ++dimm_idx) {
			uint8_t spd[128];
			unsigned spd_rx_size;
			static const uint8_t startaddr = 0;
#if defined(BE_DBM)
			spd_rx_size = i2c_txrx(1, dimm_spd_addrs[dimm_idx], &startaddr, sizeof startaddr, spd, sizeof spd);
#elif defined (BE_MITX)
			spd_rx_size = smbus_txrx(0, dimm_spd_addrs[dimm_idx], &startaddr, sizeof startaddr, spd, sizeof spd);
#endif
			if (spd_rx_size == sizeof spd) {
				if (spd_get_baseconf_crc(spd) == crc16(spd, sizeof spd - 2)) {
					const unsigned long long dimm_capacity = spd_get_baseconf_dimm_capacity(spd);
					if (dimm_capacity > 0) {
						INFO("BL2: DDR4 DIMM%d capacity is %lld MiB\n", dimm_idx, dimm_capacity / (1024 * 1024));
						total_capacity += dimm_capacity;
					} else {
						ERROR("BL2: DDR4 DIMM%d capacity is unidentified\n", dimm_idx);
					}
				} else {
					ERROR("BL2: DDR4 DIMM%d SPD has invalid CRC\n", dimm_idx);
				}
			} else {
				INFO("BL2: DDR4 DIMM%d SPD is not detected\n", dimm_idx);
			}
		}

		if (total_capacity > 0) {
			unsigned long long rangedescs[3][2];
			unsigned rangenum;

			rangedescs[0][0] = 0x80000000;
			rangedescs[1][0] = 0x880000000;
			rangedescs[2][0] = 0x8800000000;

			if (total_capacity <= 2ull * 1024 * 1024 * 1024) {
				rangedescs[0][1] = total_capacity;
				rangenum = 1;
			} else {
				rangedescs[0][1] = 2ull * 1024 * 1024 * 1024;
				if (total_capacity <= 32ull * 1024 * 1024 * 1024) {
					rangedescs[1][1] = total_capacity - 2ull * 1024 * 1024 * 1024;
					rangenum = 2;
				} else {
					rangedescs[1][1] = 30ull * 1024 * 1024 * 1024;
					rangedescs[2][1] = total_capacity - 32ull * 1024 * 1024 * 1024;
					rangenum = 3;
				}
			}

			dt_update_memory(fdt, rangedescs, rangenum);
		}
#endif
		dt_add_psci(fdt);
		ret = fdt_pack(fdt);
		if (ret < 0) {
			ERROR("BL2: failed to pack FDT at %p, error %d\n", fdt, ret);
		}

		flush_dcache_range((unsigned long)fdt, PLAT_BAIKAL_DT_MAX_SIZE);
	} else {
		ERROR("BL2: invalid FDT at %p, error %d\n", fdt, ret);
	}

	/* TODO Initialize timer */
}

void bl2_plat_arch_setup(void)
{
	baikal_configure_mmu_el1(bl2_tzram_layout.total_base,
			      bl2_tzram_layout.total_size,
			      BL2_RO_BASE, BL2_RO_LIMIT,
			      BL_COHERENT_RAM_BASE, BL_COHERENT_RAM_END);
}

/*******************************************************************************
 * Gets SPSR for BL32 entry
 ******************************************************************************/
static uint32_t baikal_get_spsr_for_bl32_entry(void)
{
	/*
	 * The Secure Payload Dispatcher service is responsible for
	 * setting the SPSR prior to entry into the BL3-2 image.
	 */
	return 0;
}

/*******************************************************************************
 * Gets SPSR for BL33 entry
 ******************************************************************************/
static uint32_t baikal_get_spsr_for_bl33_entry(void)
{
	unsigned int mode;
	uint32_t spsr;

	/* Figure out what mode we enter the non-secure world in */
	mode = (el_implemented(2) != EL_IMPL_NONE) ? MODE_EL2 : MODE_EL1;

	/*
	 * TODO: Consider the possibility of specifying the SPSR in
	 * the FIP ToC and allowing the platform to have a say as
	 * well.
	 */
	spsr = SPSR_64(mode, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);
	return spsr;
}

static int baikal_bl2_handle_post_image_load(unsigned int image_id)
{
	int err = 0;
	bl_mem_params_node_t *bl_mem_params = get_bl_mem_params_node(image_id);
#ifdef SPD_opteed
	bl_mem_params_node_t *pager_mem_params = NULL;
	bl_mem_params_node_t *paged_mem_params = NULL;
#endif

	assert(bl_mem_params);

	switch (image_id) {
# ifdef AARCH64
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
		bl_mem_params->ep_info.args.arg3 = PLAT_BAIKAL_DT_BASE;
#endif
		bl_mem_params->ep_info.spsr = baikal_get_spsr_for_bl32_entry();
		break;
# endif
	case BL33_IMAGE_ID:
		/* BL33 expects to receive the primary CPU MPID (through r0) */
		bl_mem_params->ep_info.args.arg0 = 0xffff & read_mpidr();
		bl_mem_params->ep_info.spsr = baikal_get_spsr_for_bl33_entry();
		break;
	}

	return err;
}

/*******************************************************************************
 * This function can be used by the platforms to update/use image
 * information for given `image_id`.
 ******************************************************************************/
int bl2_plat_handle_post_image_load(unsigned int image_id)
{
	return baikal_bl2_handle_post_image_load(image_id);
}

unsigned long plat_get_ns_image_entrypoint(void)
{
	return PLAT_BAIKAL_NS_IMAGE_OFFSET;
}
