/*
 * Copyright (c) 2021-2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

#include <baikal_scp.h>
#include <bs1000_sc_lcru.h>
#include <platform_def.h>

int sc_lcru_clrbits(const uintptr_t addr, const uint32_t clr)
{
	int err;
	uint32_t val;

	err = sc_lcru_read(addr, &val);
	if (err) {
		return err;
	}

	return sc_lcru_write(addr, val & ~clr);
}

int sc_lcru_clrsetbits(const uintptr_t addr, const uint32_t clr, const uint32_t set)
{
	int err;
	uint32_t val;

	err = sc_lcru_read(addr, &val);
	if (err) {
		return err;
	}

	return sc_lcru_write(addr, (val & ~clr) | set);
}

int sc_lcru_read(const uintptr_t addr, uint32_t *const val)
{
	unsigned int chip = PLATFORM_ADDR_CHIP(addr);
	int err = scp_cmd(chip, 'R', PLATFORM_ADDR_IN_CHIP(addr), 0);

	*val = *(uint32_t *)scp_buf(chip);
	return err;
}

int sc_lcru_setbits(const uintptr_t addr, const uint32_t set)
{
	int err;
	uint32_t val;

	err = sc_lcru_read(addr, &val);
	if (err) {
		return err;
	}

	return sc_lcru_write(addr, val | set);
}

int sc_lcru_write(const uintptr_t addr, const uint32_t val)
{
	unsigned int chip = PLATFORM_ADDR_CHIP(addr);

	return scp_cmd(chip, 'W', PLATFORM_ADDR_IN_CHIP(addr), val);
}
