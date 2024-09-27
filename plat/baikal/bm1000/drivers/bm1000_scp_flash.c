/*
 * Copyright (c) 2018-2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <common/debug.h>
#include <lib/utils_def.h>

#include <baikal_scp.h>
#include <bm1000_scp_flash.h>

#define SCP_FLASH_CHUNK_MAX_SIZE	UL(1024)

/* Assume 3-byte address mode by default */
static unsigned int addr_nbytes = 3;

static int scp_flash_requires_en4b(const uint32_t addr, const size_t size)
{
	return (addr >= 0x1000000 || addr + size > 0x1000000) && addr_nbytes != 4;
}

static int scp_flash_switch_addr_mode(const unsigned int nbytes)
{
	int err;

	assert(nbytes == 3 || nbytes == 4);

	INFO("SCP Flash: switch to %u-byte address mode\n", nbytes);
	err = scp_cmd(nbytes == 4 ? 'F' : 'f', 0, 0);
	if (err) {
		return err;
	}

	addr_nbytes = nbytes;

	return 0;
}

int scp_flash_erase(uint32_t addr, size_t size)
{
	int err;

	VERBOSE("SCP Flash: erase 0x%x / 0x%lx\n", addr, size);

	while (size) {
		size_t chunk = MIN(size, SCP_FLASH_CHUNK_MAX_SIZE);

		if (scp_flash_requires_en4b(addr, chunk)) {
			err = scp_flash_switch_addr_mode(4);
			if (err) {
				return err;
			}
		}

		err = scp_cmd('E', addr, chunk);
		if (err) {
			return err;
		}

		addr += chunk;
		size -= chunk;
	}

	return 0;
}

int scp_flash_lock(int lock)
{
	return scp_cmd('L', lock, 0);
}

int scp_flash_read(uint32_t addr, void *buf, size_t size)
{
	int err;
	uint8_t *pbuf = buf;

	VERBOSE("SCP Flash: read 0x%x / 0x%lx\n", addr, size);

	while (size) {
		size_t chunk = MIN(size, SCP_FLASH_CHUNK_MAX_SIZE);

		if (scp_flash_requires_en4b(addr, chunk)) {
			err = scp_flash_switch_addr_mode(4);
			if (err) {
				return err;
			}
		}

		err = scp_cmd('R', addr, chunk);
		if (err) {
			return -1;
		}

		memcpy(pbuf, scp_buf(), chunk);
		addr += chunk;
		pbuf += chunk;
		size -= chunk;
	}

	return 0;
}

int scp_flash_write(uint32_t addr, void *data, size_t size)
{
	int err;
	uint8_t *pdata = data;

	VERBOSE("SCP Flash: write 0x%x / 0x%lx\n", addr, size);

	while (size) {
		size_t chunk = MIN(size, SCP_FLASH_CHUNK_MAX_SIZE);

		if (scp_flash_requires_en4b(addr, chunk)) {
			err = scp_flash_switch_addr_mode(4);
			if (err) {
				return err;
			}
		}

		memcpy(scp_buf(), pdata, chunk);
		err = scp_cmd('W', addr, chunk);
		if (err) {
			return err;
		}

		addr  += chunk;
		pdata += chunk;
		size  -= chunk;
	}

	return 0;
}
