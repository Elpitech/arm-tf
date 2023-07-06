/*
 * Copyright (c) 2019-2023, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>

#include <baikal_def.h>
#include <baikal_pvt.h>

static uintptr_t pvt_get_reg_addr(const uintptr_t base, const unsigned int offset)
{
	unsigned int i;
	static const uintptr_t pvt_bases[] = {
#if defined(MMCA57_0_PVT_BASE)
		MMCA57_0_PVT_BASE,
		MMCA57_1_PVT_BASE,
		MMCA57_2_PVT_BASE,
		MMCA57_3_PVT_BASE,
		MMMALI_PVT_BASE
#elif defined(CA75_0_PVT_BASE)
		CA75_0_PVT_BASE,
		CA75_1_PVT_BASE,
		CA75_2_PVT_BASE,
		CA75_3_PVT_BASE,
		CA75_4_PVT_BASE,
		CA75_5_PVT_BASE,
		CA75_6_PVT_BASE,
		CA75_7_PVT_BASE,
		CA75_8_PVT_BASE,
		CA75_9_PVT_BASE,
		CA75_10_PVT_BASE,
		CA75_11_PVT_BASE,
		DDR0_PVT_BASE,
		DDR1_PVT_BASE,
		DDR2_PVT_BASE,
		DDR3_PVT_BASE,
		DDR4_PVT_BASE,
		DDR5_PVT_BASE,
		PCIE0_PVT_BASE,
		PCIE1_PVT_BASE,
		PCIE2_PVT_BASE,
		PCIE3_PVT_BASE,
		PCIE4_PVT_BASE
#endif
	};

	/* Ensure that the offset in PVT region range */
	if (offset > 0x40) {
		return 0;
	}

	for (i = 0; i < ARRAY_SIZE(pvt_bases); ++i) {
		if (pvt_bases[i] == base) {
			return pvt_bases[i] + offset;
		}
	}

	return 0;
}

uint32_t pvt_read_reg(const uintptr_t base, const unsigned int offset)
{
	const uintptr_t reg_addr = pvt_get_reg_addr(base, offset);

	if (reg_addr == 0) {
		return 1;
	}
#ifdef BAIKAL_QEMU
	return 0;
#endif
	return mmio_read_32(reg_addr);
}

uint32_t pvt_write_reg(const uintptr_t base, const unsigned int offset, const uint32_t val)
{
	const uintptr_t reg_addr = pvt_get_reg_addr(base, offset);

	if (reg_addr == 0) {
		return 1;
	}
#ifndef BAIKAL_QEMU
	mmio_write_32(reg_addr, val);
#endif
	return 0;
}
