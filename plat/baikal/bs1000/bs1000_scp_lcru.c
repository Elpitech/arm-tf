/*
 * Copyright (c) 2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

#include <baikal_scp.h>
#include <bs1000_scp_lcru.h>

int scp_lcru_clrbits(const uint32_t addr, const uint32_t clr)
{
	int err;
	uint32_t val;

	err = scp_lcru_read(addr, &val);
	if (err) {
		return err;
	}

	return scp_lcru_write(addr, val & ~clr);
}

int scp_lcru_clrsetbits(const uint32_t addr, const uint32_t clr, const uint32_t set)
{
	int err;
	uint32_t val;

	err = scp_lcru_read(addr, &val);
	if (err) {
		return err;
	}

	return scp_lcru_write(addr, (val & ~clr) | set);
}

int scp_lcru_read(const uint32_t addr, uint32_t *const val)
{
	int err = scp_cmd('R', addr, 0);
	*val = *(uint32_t *)scp_buf();
	return err;
}

int scp_lcru_setbits(const uint32_t addr, const uint32_t set)
{
	int err;
	uint32_t val;

	err = scp_lcru_read(addr, &val);
	if (err) {
		return err;
	}

	return scp_lcru_write(addr, val | set);
}

int scp_lcru_write(const uint32_t addr, const uint32_t val)
{
	return scp_cmd('W', addr, val);
}
