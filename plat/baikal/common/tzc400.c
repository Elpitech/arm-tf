/*
 * Copyright (c) 2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/mmio.h>
#include <lib/utils_def.h>

#include <tzc400.h>

#define TZC_400_GATE_KEEPER_OFFSET		U(0x008)
#define TZC_400_REGION_ATTR_0_OFFSET		U(0x110)
#define TZC_400_REGION_ID_ACCESS_0_OFFSET	U(0x114)

void tzc400_set_transparent(const uintptr_t base)
{
	mmio_write_32(base + TZC_400_GATE_KEEPER_OFFSET, 0xf);
	mmio_write_32(base + TZC_400_REGION_ATTR_0_OFFSET, 0xc000000f);
	mmio_write_32(base + TZC_400_REGION_ID_ACCESS_0_OFFSET, 0xffffffff);
}
