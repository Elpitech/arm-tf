/*
 * Copyright (c) 2021-2022, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <string.h>

#include <common/debug.h>

#include <baikal_def.h>
#include <bm1000_smbus.h>
#include <crc.h>
#include <dw_i2c.h>
#include <ndelay.h>

#include "ddr_spd.h"

#define DIMM0_SPD_ADDR	0x50
#define DIMM1_SPD_ADDR	0x52

#define SPD_SPA0	0x36
#define SPD_PAGE_SIZE	256
#define SPD_FULL_SIZE	512

#ifdef BAIKAL_DIMM_SPD_STATIC
static const uint8_t spd_static[] = {
	0x23, 0x11, 0x0c, 0x02, 0x84, 0x19, 0x00, 0x08, 0x00, 0x00, 0x00, 0x03, 0x01, 0x03, 0x00, 0x00,
	0x00, 0x00, 0x07, 0x0d, 0xf8, 0x7f, 0x00, 0x00, 0x6e, 0x6e, 0x6e, 0x11, 0x00, 0x6e, 0x20, 0x08,
	0x00, 0x05, 0x70, 0x03, 0x00, 0xa8, 0x1b, 0x28, 0x28, 0x00, 0x78, 0x00, 0x14, 0x3c, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0x36, 0x16, 0x36,
	0x16, 0x36, 0x16, 0x36, 0x00, 0x00, 0x16, 0x36, 0x16, 0x36, 0x16, 0x36, 0x16, 0x36, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9d, 0xb5, 0x00, 0x00, 0x00, 0x00, 0xe7, 0xd6, 0x59, 0x18,
	0x11, 0x11, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0xa0
};
#else
const uint8_t spd_bus_addrs[] = {DIMM0_SPD_ADDR, DIMM1_SPD_ADDR};
#endif

void* ddr_read_spd(const unsigned dimm_idx)
{
	uint8_t *p = (uint8_t *)&spd_content.content[dimm_idx];
#ifndef BAIKAL_DIMM_SPD_STATIC
	unsigned page;
	int rxsize = 0;
	uint8_t startaddr = 0;
#ifdef BAIKAL_DUAL_CHANNEL_MODE
	uint8_t channel_bytes[SPD_PAGE_SIZE / 2];
#endif

	assert(dimm_idx < ARRAY_SIZE(spd_bus_addrs));

	for (page = 0; page < SPD_FULL_SIZE / SPD_PAGE_SIZE; ++page) {
		int rxpsize;
		BAIKAL_SPD_BUS_HANDLER(BAIKAL_DIMM_SPD_DEV, SPD_SPA0 + page,
			   &startaddr, sizeof(startaddr), NULL, 0);

		rxpsize = BAIKAL_SPD_BUS_HANDLER(BAIKAL_DIMM_SPD_DEV, spd_bus_addrs[dimm_idx],
				     &startaddr, sizeof(startaddr), p + rxsize, SPD_PAGE_SIZE);
		if (rxpsize != SPD_PAGE_SIZE) {
			break;
		}

		rxsize += rxpsize;
	}

	BAIKAL_SPD_BUS_HANDLER(BAIKAL_DIMM_SPD_DEV, SPD_SPA0, &startaddr, sizeof(startaddr), NULL, 0);

	if (rxsize != SPD_FULL_SIZE) {
		return NULL;
	}
#else /* defined(BAIKAL_DIMM_SPD_STATIC) */
	memcpy(p, spd_static, sizeof(spd_static));
#endif
	if (crc16(p +   0, 126, 0) != ((p[127] << 8) | p[126]) ||
	    crc16(p + 128, 126, 0) != ((p[255] << 8) | p[254])) {
		ERROR("DIMM%u: SPD CRC checksum fail\n", dimm_idx);
		return NULL;
	}

#ifdef BAIKAL_DUAL_CHANNEL_MODE
	/*
	 * Check if 2 DIMMs are inserted into the slots related to a single port
	 */
	rxsize = BAIKAL_SPD_BUS_HANDLER(BAIKAL_DIMM_SPD_DEV, spd_bus_addrs[dimm_idx] + 1,
			     &startaddr, sizeof(startaddr), &channel_bytes, SPD_PAGE_SIZE / 2);
	if (rxsize != SPD_PAGE_SIZE / 2) {
		goto _exit;
	}

	if ((channel_bytes[126] != p[126]) || (channel_bytes[127] != p[127])) {
		ERROR("DDR PORT%d: can't work in 2-channel mode with DIMMs that differ\n", dimm_idx);
		return NULL;
	}

	spd_content.dual_channel[dimm_idx] = 'y';

	/*
	 * Read serial and part numbers from second DIMM
	 */
	BAIKAL_SPD_BUS_HANDLER(BAIKAL_DIMM_SPD_DEV, SPD_SPA0 + 1, &startaddr, sizeof(startaddr), NULL, 0);
	startaddr = 69;
	rxsize = BAIKAL_SPD_BUS_HANDLER(BAIKAL_DIMM_SPD_DEV, spd_bus_addrs[dimm_idx] + 1,
			     &startaddr, sizeof(startaddr), &channel_bytes, 24);
	if (rxsize == 24) {
		spd_content.extra[dimm_idx].serial_number = *(uint32_t *)channel_bytes;
		memcpy(spd_content.extra[dimm_idx].part_number, &channel_bytes[4], 20);
	}
	BAIKAL_SPD_BUS_HANDLER(BAIKAL_DIMM_SPD_DEV, SPD_SPA0, &startaddr, sizeof(startaddr), NULL, 0);
_exit:
#endif
	return p;
}

void ddr_write_conf(unsigned dimm_idx, void *buf, int size)
{
	uint8_t local_buf[17];
	int txsize = 0, len;

	local_buf[0] = 0;
	BAIKAL_SPD_BUS_HANDLER(BAIKAL_DIMM_SPD_DEV, SPD_SPA0 + 1, local_buf, 1, NULL, 0);

	while (txsize < size) {
		len = MIN(16, size - txsize);
		local_buf[0] = 128 + txsize;
		memcpy(local_buf + 1, buf + txsize, len);
		if (BAIKAL_SPD_BUS_HANDLER(BAIKAL_DIMM_SPD_DEV,
				 spd_bus_addrs[dimm_idx], local_buf, len + 1, NULL, 0) < 0) {
			if (txsize > 0) { /* EEPROM may be busy with preceding write */
				ndelay(5000000);
				if (BAIKAL_SPD_BUS_HANDLER(BAIKAL_DIMM_SPD_DEV, spd_bus_addrs[dimm_idx],
						local_buf, len + 1, NULL, 0) < 0)
					break;
			} else {
				break;
			}
		}
		txsize += len;
	}

	if (txsize != size) {
		ERROR("Can't write into SPD page 1 - size %d, txsize %d (dimm_idx %d)\n",
			size, txsize, dimm_idx);
	}

	local_buf[0] = 0;
	BAIKAL_SPD_BUS_HANDLER(BAIKAL_DIMM_SPD_DEV, SPD_SPA0, local_buf, 1, NULL, 0);
}

