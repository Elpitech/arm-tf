/*
 * Copyright (c) 2018-2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/bl_common.h>
#include <drivers/generic_delay_timer.h>
#include <lib/mmio.h>
#include <libfdt.h>
#include <plat/arm/common/plat_arm.h>
#include <plat/common/platform.h>

#include <baikal_def.h>
#include <baikal_fdt.h>
#include <baikal_gicv3.h>
#include <bm1000_cmu.h>
#include <bm1000_def.h>
#include <bm1000_private.h>
#include <ddr_spd.h>
#include <platform_def.h>
#include <spd.h>

#include "bm1000_mmca57.h"
#include "bm1000_splash.h"

#define XSTR(x) STR(x)
#define STR(x) #x

static void baikal_fdt_memory_update(void)
{
	unsigned int dimm_idx;
	void *fdt = (void *)(uintptr_t)BAIKAL_SEC_DTB_BASE;
	int ret;
	struct spd_container *const spd_ptr = (void *)BAIKAL_SEC_SPD_BASE;
	unsigned long long total_capacity = 0;

	/*
	 * DBM has 4 DIMM slots with the following SPD addresses:
	 * 0x50, 0x51, 0x52, 0x53. But only 0x50 and 0x52 are used by SCP-FW.
	 */
	const uint8_t dimm_spd_addrs[] = {0x50, 0x52};

	ret = fdt_open_into(fdt, fdt, BAIKAL_DTB_MAX_SIZE);
	if (ret < 0) {
		ERROR("%s: failed to open FDT @ %p, error %d\n", __func__, fdt, ret);
		return;
	}

	/* Read DDR4 DIMM SPD EEPROMs for base configuration and DRAM parameters */
	for (dimm_idx = 0; dimm_idx < ARRAY_SIZE(dimm_spd_addrs); ++dimm_idx) {
		if (spd_ptr->content[dimm_idx].mem_type == SPD_MEMTYPE_DDR4) {
			unsigned int half_bus_shift = 0;

			if (dimm_idx) {
				if (mmio_read_32(MMDDR1_CTRL_BASE) & BIT(12)) {
					half_bus_shift = 1;
				}
			} else {
				if (mmio_read_32(MMDDR0_CTRL_BASE) & BIT(12)) {
					half_bus_shift = 1;
				}
			}

			const unsigned long long dimm_capacity =
				spd_get_baseconf_dimm_capacity(&spd_ptr->content[dimm_idx])
					>> half_bus_shift;

			if (dimm_capacity > 0) {
#ifdef BAIKAL_DUAL_CHANNEL_MODE
				if (spd_ptr->dual_channel[dimm_idx] == 'y') {
					INFO("DIMM%u: capacity is %lld MiB\n", dimm_idx,
								dimm_capacity * 2 / (1024 * 1024));
					total_capacity += dimm_capacity * 2;
				} else {
					INFO("DIMM%u: capacity is %lld MiB\n", dimm_idx,
								dimm_capacity / (1024 * 1024));
					total_capacity += dimm_capacity;
				}
#else
				INFO("DIMM%u: capacity is %lld MiB\n", dimm_idx, dimm_capacity / (1024 * 1024));
				total_capacity += dimm_capacity;
#endif
				dt_enable_mc_node(fdt, dimm_idx == 0 ? MMDDR0_CTRL_BASE : MMDDR1_CTRL_BASE);
			} else {
				ERROR("DIMM%u: capacity is unidentified\n", dimm_idx);
			}
		}
	}

	if (total_capacity > 0) {
		uint64_t region_descs[3][2];
		unsigned int region_num;

		region_descs[0][0] = REGION_DRAM0_BASE;
		region_descs[1][0] = REGION_DRAM1_BASE;
		region_descs[2][0] = REGION_DRAM2_BASE;

		if (total_capacity <= REGION_DRAM0_SIZE) {
			region_descs[0][1] = total_capacity;
			region_num = 1;
		} else {
			region_descs[0][1] = REGION_DRAM0_SIZE;
			if (total_capacity <= (REGION_DRAM0_SIZE + REGION_DRAM1_SIZE)) {
				region_descs[1][1] = total_capacity - REGION_DRAM0_SIZE;
				region_num = 2;
			} else {
				region_descs[1][1] = REGION_DRAM1_SIZE;
				region_descs[2][1] = total_capacity - (REGION_DRAM0_SIZE + REGION_DRAM1_SIZE);
				region_num = 3;
			}
		}

		fdt_memory_node_set(fdt, region_descs, region_num);
	}

	ret = fdt_pack(fdt);
	if (ret < 0) {
		ERROR("%s: failed to pack FDT @ %p, error %d\n", __func__, fdt, ret);
	}

	flush_dcache_range((uintptr_t)fdt, BAIKAL_DTB_MAX_SIZE);
}

extern uint8_t bl31_logo[];

static void bl31_splash(void)
{
	modeline_t old_lvds_mode, new_lvds_mode;
	int do_lvds_init;

#ifdef SDK_VERSION
	snprintf(sdk_version, SDK_VERSION_SIZE, "== v%s ==", XSTR(SDK_VERSION));
#endif
	display_logo_and_version(MMXGBE_VDU_BASE, FB1_BASE, &hdmi_video_mode, bl31_logo);
#if DEBUG
	vdu_init(MMXGBE_VDU_BASE, (uintptr_t)FB1_BASE, &hdmi_video_mode);
#endif
	memcpy(&old_lvds_mode, &lvds_video_mode, sizeof(lvds_video_mode));
	memcpy(&new_lvds_mode, &lvds_video_mode, sizeof(lvds_video_mode));
	if (!fdt_get_panel(&new_lvds_mode)) {
		display_logo_and_version(MMAVLSP_VDU_BASE, FB0_BASE, &new_lvds_mode, bl31_logo);
#if !DEBUG
		do_lvds_init = memcmp(&old_lvds_mode, &new_lvds_mode, sizeof(old_lvds_mode));
#else
		do_lvds_init = 1;
#endif
		if (do_lvds_init) {
			vdu_init(MMAVLSP_VDU_BASE, (uintptr_t)FB0_BASE,
				 &new_lvds_mode);
		}
	}
}

void bl31_plat_arch_setup(void)
{
	const mmap_region_t bl_regions[] = {
		MAP_REGION_FLAT(BL31_START,
				BL31_END - BL31_START,
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

		MAP_REGION_FLAT(MMPCIE_LCRU_BASE,
				MMPCIE_LCRU_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMPCIE_NIC_CFG_BASE,
				MMPCIE_NIC_CFG_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMCORESIGHT_LCRU_BASE,
				MMCORESIGHT_LCRU_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMCORESIGHT_NIC_CFG_BASE,
				MMCORESIGHT_NIC_CFG_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMCORESIGHT_CORESIGHT_BASE,
				MMCORESIGHT_CORESIGHT_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMCCN_LCRU_BASE,
				MMCCN_LCRU_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMCCN_CCN_BASE,
				MMCCN_CCN_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMDDR0_LCRU_BASE,
				MMDDR0_LCRU_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMDDR0_CTRL_BASE,
				MMDDR0_CTRL_SIZE,
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

		MAP_REGION_FLAT(MMAVLSP_I2C1_BASE,
				MMAVLSP_I2C1_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMAVLSP_VDU_BASE,
				MMAVLSP_VDU_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMDDR1_LCRU_BASE,
				MMDDR1_LCRU_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMDDR1_CTRL_BASE,
				MMDDR1_CTRL_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMPCIE_NIC_BASE,
				MMPCIE_NIC_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMVDEC_LCRU_BASE,
				MMVDEC_LCRU_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMVDEC_SMMU_BASE,
				MMVDEC_SMMU_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMVDEC_NIC_CFG_BASE,
				MMVDEC_NIC_CFG_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMCA57_0_LCRU_BASE,
				MMCA57_0_LCRU_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMCA57_0_PVT_BASE,
				MMCA57_0_PVT_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMCA57_1_LCRU_BASE,
				MMCA57_1_LCRU_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMCA57_1_PVT_BASE,
				MMCA57_1_PVT_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMCA57_2_LCRU_BASE,
				MMCA57_2_LCRU_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMCA57_2_PVT_BASE,
				MMCA57_2_PVT_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMCA57_3_LCRU_BASE,
				MMCA57_3_LCRU_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMCA57_3_PVT_BASE,
				MMCA57_3_PVT_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMMALI_LCRU_BASE,
				MMMALI_LCRU_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMMALI_NIC_CFG_BASE,
				MMMALI_NIC_CFG_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMUSB_LCRU_BASE,
				MMUSB_LCRU_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMUSB_NIC_CFG_BASE,
				MMUSB_NIC_CFG_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMUSB_GIC_BASE,
				MMUSB_GIC_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMXGBE_LCRU_BASE,
				MMXGBE_LCRU_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMXGBE_NIC_CFG_BASE,
				MMXGBE_NIC_CFG_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMXGBE_VDU_BASE,
				MMXGBE_VDU_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MMXGBE_HDMI_BASE,
				MMXGBE_HDMI_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(NS_DRAM0_BASE,
				NS_DRAM0_SIZE,
				MT_MEMORY | MT_RW | MT_NS),

		MAP_REGION_FLAT(SEC_DRAM_BASE,
				SEC_DRAM_SIZE,
				MT_MEMORY | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(FB0_BASE,
				2 * FB_SIZE,
				MT_MEMORY | MT_RW | MT_NS),

		{0}
	};

	plat_arm_interconnect_init();
	plat_arm_interconnect_enter_coherency();

	setup_page_tables(bl_regions, plat_bm1000_mmap);
	enable_mmu_el3(0);
}

void bl31_platform_setup(void)
{
	generic_delay_timer_init();

	/* Init MM-CA57 PVTs */
	cmu_clkch_enable_by_base(MMCA57_0_PVT_CLKCHCTL, PVT_CLKCH_DIV);
	cmu_clkch_enable_by_base(MMCA57_1_PVT_CLKCHCTL, PVT_CLKCH_DIV);
	cmu_clkch_enable_by_base(MMCA57_2_PVT_CLKCHCTL, PVT_CLKCH_DIV);
	cmu_clkch_enable_by_base(MMCA57_3_PVT_CLKCHCTL, PVT_CLKCH_DIV);

	/* Init MM-CoreSight */
	mmcoresight_init();

	/* Init MM-Mali */
	mmmali_init();

	/* Init MM-Mali PVT */
	cmu_clkch_enable_by_base(MMMALI_PVT_CLKCHCTL, PVT_CLKCH_DIV);

	/* Init MM-PCIe */
	mmpcie_init();

	/* Init MM-USB */
	mmusb_init();

	/* Init MM-VDec */
	mmvdec_init();
#if DEBUG
	/* Init MM-XGbE */
	mmxgbe_init();
	hdmi_tx_init();
#endif
	bl31_splash();

	baikal_gic_driver_init();
	baikal_gic_init();

	baikal_fdt_memory_update();

	mmavlsp_ns_access();
	mmxgbe_ns_access();

	memcpy((void *)BAIKAL_NS_DTB_BASE,
	       (void *)BAIKAL_SEC_DTB_BASE,
	       BAIKAL_DTB_MAX_SIZE);

	flush_dcache_range((uintptr_t)BAIKAL_NS_DTB_BASE, BAIKAL_DTB_MAX_SIZE);

	memcpy((void *)BAIKAL_NS_SPD_BASE,
	       (void *)BAIKAL_SEC_SPD_BASE,
	       sizeof(struct spd_container));

	flush_dcache_range((uintptr_t)BAIKAL_NS_SPD_BASE, sizeof(struct spd_container));
}
