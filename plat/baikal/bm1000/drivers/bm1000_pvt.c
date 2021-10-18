/*
 * Copyright (c) 2019-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <bm1000_def.h>
#include <bm1000_pvt.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>

static uintptr_t pvt_get_addr(uint32_t pvt_id, uint32_t offset)
{
	static const uintptr_t pvt_bases[] = {
		MMCA57_0_PVT_BASE,
		MMCA57_1_PVT_BASE,
		MMCA57_2_PVT_BASE,
		MMCA57_3_PVT_BASE,
		MMMALI_PVT_BASE
	};

	/* Ensure that pvt_id is correct and the offset in PVT address range */
	if (pvt_id < ARRAY_SIZE(pvt_bases) && offset < 0x10000) {
		return pvt_bases[pvt_id] + offset;
	}

	return 0;
}

uint32_t pvt_read_reg(uint32_t pvt_id, uint32_t offset)
{
	uintptr_t addr;

	addr = pvt_get_addr(pvt_id, offset);
	if (!addr) {
		return 1;
	}

	return mmio_read_32(addr);
}

uint32_t pvt_write_reg(uint32_t pvt_id, uint32_t offset, uint32_t val)
{
	uintptr_t addr;

	addr = pvt_get_addr(pvt_id, offset);
	if (!addr) {
		return 1;
	}

	mmio_write_32(addr, val);
	return 0;
}
