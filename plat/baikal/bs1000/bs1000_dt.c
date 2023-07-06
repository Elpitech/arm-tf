/*
 * Copyright (c) 2023, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <libfdt.h>

#include <bs1000_dimm_spd.h>
#include <baikal_def.h>
#include <baikal_fdt.h>
#include <ddr_spd.h>
#include <crc.h>
#include <spd.h>

static uint64_t baikal_detect_sdram_capacity(void)
{
	unsigned int dimm_idx;
	uint64_t total_capacity = 0;

	for (dimm_idx = 0; ; ++dimm_idx) {
		const uint8_t *buf = baikal_dimm_spd_get(dimm_idx);

		if (buf == NULL) {
			break;
		}

		if (crc16(buf, 126, 0) == spd_get_baseconf_crc(buf)) {
			const uint64_t dimm_capacity = spd_get_baseconf_dimm_capacity(buf);

			INFO("DIMM%u%u: %lu MiB\n", dimm_idx / 2, dimm_idx % 2,
			     dimm_capacity / (1024 * 1024));

			total_capacity += dimm_capacity;
		}
	}
#ifdef BAIKAL_QEMU
	if (total_capacity == 0) {
		total_capacity = (uint64_t)2 * 1024 * 1024 * 1024;
	}
#endif
	return total_capacity;
}

static void dt_enable_mc_node(void *fdt, const uintptr_t base)
{
	int node = -1;

	for (;;) {
		const uint32_t *prop;
		int proplen;
		uint64_t reg;

		node = fdt_node_offset_by_compatible(fdt, node, "baikal,bs1000-edac-mc");
		if (node < 0) {
			ERROR("%s: unable to find 'edac-mc' with 'reg' = 0x%lx\n", __func__, base);
			return;
		}

		prop = fdt_getprop(fdt, node, "reg", &proplen);
		if (prop == NULL) {
			ERROR("%s: 'reg' is not found\n", __func__);
			continue;
		} else if (proplen != 16) {
			ERROR("%s: incorrect 'reg' property length\n", __func__);
			continue;
		}

		/* Avoid 'fdt64_to_cpu()' with prop pointer: it could lead to unaligned access */
		reg  = fdt32_to_cpu(prop[0]);
		reg <<= 32;
		reg |= fdt32_to_cpu(prop[1]);

		if (reg == base) {
			int err;

			err = fdt_setprop_string(fdt, node, "status", "okay");
			if (err) {
				ERROR("%s: unable to set 'status' property of node\n", __func__);
			}

			return;
		}
	}
}

void baikal_fdt_memory_update(void)
{
	void *fdt = (void *)BAIKAL_SEC_DTB_BASE;
	uint64_t region_descs[4][2];
	unsigned int region_num;
	int ret;
	uint64_t total_capacity;

	ret = fdt_open_into(fdt, fdt, BAIKAL_DTB_MAX_SIZE);
	if (ret < 0) {
		ERROR("%s: failed to open FDT @ %p, error %d\n", __func__, fdt, ret);
		return;
	}

	total_capacity = baikal_detect_sdram_capacity();

	region_descs[0][0] = REGION_DRAM0_BASE;
	region_descs[1][0] = REGION_DRAM1_BASE;
	region_descs[2][0] = REGION_DRAM2_BASE;
	region_descs[3][0] = REGION_DRAM3_BASE;

	/* TODO: the code looks unrolled and could be folded */
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
			if (total_capacity <= (REGION_DRAM0_SIZE + REGION_DRAM1_SIZE + REGION_DRAM2_SIZE)) {
				region_descs[2][1] = total_capacity - (REGION_DRAM0_SIZE + REGION_DRAM1_SIZE);
				region_num = 3;
			} else {
				region_descs[2][1] = REGION_DRAM2_SIZE;
				region_descs[3][1] = total_capacity - (REGION_DRAM0_SIZE + REGION_DRAM1_SIZE + REGION_DRAM2_SIZE);
				region_num = 4;
			}
		}
	}

	fdt_memory_node_set(fdt, region_descs, region_num);

	ret = fdt_pack(fdt);
	if (ret < 0) {
		ERROR("%s: failed to pack FDT @ %p, error %d\n", __func__, fdt, ret);
	}
}

void baikal_fdt_ddr_node_enable(void)
{
	int ret;
	uintptr_t base;
	struct ddr4_spd_eeprom *spd_content;
	void *fdt = (void *)BAIKAL_SEC_DTB_BASE;

	ret = fdt_open_into(fdt, fdt, BAIKAL_DTB_MAX_SIZE);
	if (ret < 0) {
		ERROR("%s: failed to open FDT @ %p, error %d\n", __func__, fdt, ret);
		return;
	}

	for (int dimm_idx = 0; dimm_idx < 6; ++dimm_idx) {
		spd_content = (struct ddr4_spd_eeprom *)baikal_dimm_spd_get(dimm_idx * 2);
		if (spd_content->mem_type == SPD_MEMTYPE_DDR4) {
			switch (dimm_idx) {
			case 0:
				base = 0x53000000;
				break;
			case 1:
				base = 0x57000000;
				break;
			case 2:
				base = 0x5b000000;
				break;
			case 3:
				base = 0x63000000;
				break;
			case 4:
				base = 0x67000000;
				break;
			case 5:
				base = 0x6b000000;
				break;
			default:
				ERROR("%s: wrong dimm index - %d\n", __func__, dimm_idx);
				break;
			}
			dt_enable_mc_node(fdt, base);
		}
	}
}
