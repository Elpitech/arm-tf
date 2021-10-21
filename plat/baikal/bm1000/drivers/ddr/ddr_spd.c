/*
 * Copyright (c) 2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <baikal_def.h>
#include <bm1000_i2c.h>
#include <bm1000_smbus.h>
#include <common/debug.h>
#include <crc16.h>

#include "ddr_spd.h"

#define SPD_DDR0_ADDR_CH0 0x50
#define SPD_DDR0_ADDR_CH1 0x51
#define SPD_DDR1_ADDR_CH0 0x52
#define SPD_DDR1_ADDR_CH1 0x53

static const uint8_t spd_bus_addrs[] = {SPD_DDR0_ADDR_CH0, SPD_DDR0_ADDR_CH1,
					SPD_DDR1_ADDR_CH0, SPD_DDR1_ADDR_CH1};

unsigned ddr_dimm_presence(void)
{
	unsigned dimm_idx;
	unsigned ret = 0;

	for (dimm_idx = 0; dimm_idx < ARRAY_SIZE(spd_bus_addrs); ++dimm_idx) {
		uint8_t spdhdr[3];
		static const uint8_t startaddr = 0;
#if defined(BAIKAL_DIMM_SPD_I2C)
		if (i2c_txrx(BAIKAL_DIMM_SPD_I2C, spd_bus_addrs[dimm_idx], &startaddr,
		    sizeof(startaddr), spdhdr, sizeof(spdhdr)) == sizeof(spdhdr)) {
#elif defined(BAIKAL_DIMM_SPD_SMBUS)
		if (smbus_txrx(BAIKAL_DIMM_SPD_SMBUS, spd_bus_addrs[dimm_idx], &startaddr,
		    sizeof(startaddr), spdhdr, sizeof(spdhdr)) == sizeof(spdhdr)) {
#endif
			if (spdhdr[2] == SPD_MEMTYPE_DDR4) {
				INFO("DIMM%u-%u: DDR4 SDRAM is detected\n", dimm_idx / 2, dimm_idx & 1);
				ret |= 1 << dimm_idx;
			} else {
				ERROR("DIMM%u: unsupported SDRAM type\n", dimm_idx);
			}
		}
	}

	return ret;
}

int ddr_read_spd(const unsigned port, struct ddr4_spd_eeprom *spd)
{
	uint16_t spd_crc;
	static const uint8_t startaddr = 0;
	int rxsize;
	const uint8_t *p = (uint8_t *)spd;

	assert(port < ARRAY_SIZE(spd_bus_addrs));
	assert(spd != NULL);

	memset(spd, 0, sizeof(*spd));

#if defined(BAIKAL_DIMM_SPD_I2C)
	rxsize = i2c_txrx(BAIKAL_DIMM_SPD_I2C, spd_bus_addrs[port * 2], &startaddr,
			  sizeof(startaddr), spd, sizeof(*spd));
#elif defined(BAIKAL_DIMM_SPD_SMBUS)
	rxsize = smbus_txrx(BAIKAL_DIMM_SPD_SMBUS, spd_bus_addrs[port * 2], &startaddr,
			    sizeof(startaddr), spd, sizeof(*spd));
#endif
	if (rxsize != sizeof(*spd)) {
		return -1;
	}

	spd_crc = spd->crc[0] | spd->crc[1] << 8;
	if (spd_crc != crc16(p, 126)) {
		ERROR("DDR#%u SPD CRC checksum fail\n", port);
	}

	spd_crc = spd->mod_section.uc[126] | spd->mod_section.uc[127] << 8;
	if (spd_crc != crc16(p + 128, 126)) {
		ERROR("DDR#%u SPD CRC checksum fail\n", port);
	}
#ifdef BAIKAL_DEBUG
	printf("[spd content]:");
	for (int i = 0; i < rxsize; i++) {
		if ((i % 16) == 0) {
			printf("\n%08x  ", i);
		}
		printf("%02x ", p[i]);
	}
	printf("\n");
#endif
	return 0;
}
