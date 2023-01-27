/*
 * Copyright (c) 2022, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include <lib/utils_def.h>

#include <bs1000_def.h>
#include <crc.h>
#include <dw_i2c.h>
#include <spd.h>

#define DIMM_NUM	12
#define SPD_MAXSIZE	512

#define SPD_SPA0	0x36
#define SPD_SPA1	0x37

static uint8_t spd_data[DIMM_NUM][SPD_MAXSIZE];

const void* baikal_dimm_spd_get(const unsigned dimm_idx)
{
	if (dimm_idx >= ARRAY_SIZE(spd_data)) {
		return NULL;
	}

	return spd_data[dimm_idx];
}

void baikal_dimm_spd_read(void)
{
	unsigned dimm_idx;

	for (dimm_idx = 0; dimm_idx < ARRAY_SIZE(spd_data); ++dimm_idx) {
		const uintptr_t base = dimm_idx < 6 ? I2C2_BASE : I2C3_BASE;
		uint8_t *buf = spd_data[dimm_idx];
		int rxsize;
		const unsigned spd_addr = 0x50 + dimm_idx % 6;
		uint8_t startaddr = 0;

		memset(buf, 0xff, sizeof(spd_data[0]));

		i2c_txrx(base, SPD_SPA0,
			 &startaddr, sizeof(startaddr),
			 NULL, 0);

		rxsize = i2c_txrx(base, spd_addr,
				  &startaddr, sizeof(startaddr),
				  buf, 128);

		if (rxsize == 128 &&
		    crc16(buf, 126, 0) == spd_get_baseconf_crc(buf)) {
			const unsigned bytes_used = buf[0] & 0xf;

			if (bytes_used > 1 && bytes_used < 5) {
				buf += rxsize;
				startaddr += rxsize;
				rxsize = i2c_txrx(base, spd_addr,
						  &startaddr, sizeof(startaddr),
						  buf, 128);

				if (rxsize == 128 && bytes_used > 2) {
					buf += rxsize;
					startaddr = 0;
					i2c_txrx(base, SPD_SPA1,
						 &startaddr, sizeof(startaddr),
						 NULL, 0);

					i2c_txrx(base, spd_addr,
						 &startaddr, sizeof(startaddr),
						 buf, bytes_used == 3 ? 128 : 256);

					i2c_txrx(base, SPD_SPA0,
						 &startaddr, sizeof(startaddr),
						 NULL, 0);
				}
			}
		}
	}
}
