/*
 * Copyright (c) 2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>

#include <bm1000_def.h>

#include "ddr_lcru.h"
#include "ddr_master.h"

#define DBUS_HALF	0
#define ECC_ENABLE	1

#define TZC_GATEKEEPER_OFS 0x8
#define TZC_RATTRIBUTE_OFS 0x110
#define TZC_REIDACCESS_OFS 0x114

void ddr_odt_configuration(const unsigned port, const uint16_t crc_val, struct ddr_configuration *const data);

static void tzc_set_transparent(const unsigned conf)
{
	if (conf & 0x3) {
		mmio_write_32(MMTZC0_TZC400_BASE + TZC_GATEKEEPER_OFS, 0xf);
		mmio_write_32(MMTZC0_TZC400_BASE + TZC_RATTRIBUTE_OFS, 0xc000000f);
		mmio_write_32(MMTZC0_TZC400_BASE + TZC_REIDACCESS_OFS, 0xffffffff);
	}

	if (conf & 0xc) {
		mmio_write_32(MMTZC1_TZC400_BASE + TZC_GATEKEEPER_OFS, 0xf);
		mmio_write_32(MMTZC1_TZC400_BASE + TZC_RATTRIBUTE_OFS, 0xc000000f);
		mmio_write_32(MMTZC1_TZC400_BASE + TZC_REIDACCESS_OFS, 0xffffffff);
	}
}

static int ddr_port_init(int port, const struct ddr4_spd_eeprom *spd, bool dual_mode, bool double_dimm)
{
	int ret;
	struct ddr_configuration data = {0};

	ret = ddr_config_by_spd(spd, &data);
	if (ret) {
		goto error;
	}

	if (dual_mode) {
		data.single_ddr = 0;
	} else {
		data.single_ddr = 1;
	}

	if (double_dimm) {
		data.dimms = 2;
	}

#if !ECC_ENABLE
	data.ecc_on = false;
#endif

#if DBUS_HALF
	data.dbus_half = true;
#endif

	ddr_odt_configuration(port, ((uint16_t)spd->crc[1] << 8) + spd->crc[0],
			      &data);

	ret = ddr_lcru_initport(port, data.clock_mhz);
	if (ret) {
		goto failed;
	}
	ret = ddr_init(port, dual_mode, &data);
	if (ret) {
		goto failed;
	}

	if (data.ecc_on) {
		ddr_init_ecc_memory(port);
	}

	INFO("DIMM%u: module rate %u MHz, AA-RCD-RP-RAS %u-%u-%u-%u\n", port,
	     data.clock_mhz * 2, data.CL, data.tRCD, data.tRP, data.tRASmax);

	return 0;

failed:
	ddr_lcru_disable(port);
error:
	ERROR("Failed to init DDR port #%d\n", port);
	return -1;
}

int dram_init(void)
{
	int ret = 0;
	unsigned conf = 0;
	const struct ddr4_spd_eeprom *spd[4];

	spd[0] = ddr_read_spd(0);
	if (spd[0] != NULL) {
		if (spd[0]->mem_type == SPD_MEMTYPE_DDR4) {
			INFO("DIMM0: DDR4 SDRAM is detected\n");
			conf |= 0x1;
			spd[1] = ddr_read_spd(1);
			if (spd[1] != NULL && !memcmp(spd[0], spd[1], 128)) {
				INFO("DIMM0-1 is present\n");
				conf |= 0x2;
			}
		} else {
			ERROR("DIMM0: unsupported SDRAM type\n");
		}
	}

	spd[2] = ddr_read_spd(2);
	if (spd[2] != NULL) {
		if (spd[2]->mem_type == SPD_MEMTYPE_DDR4) {
			INFO("DIMM1: DDR4 SDRAM is detected\n");
			conf |= 0x4;
			spd[3] = ddr_read_spd(3);
			if (spd[3] != NULL && !memcmp(spd[2], spd[3], 128)) {
				INFO("DIMM1-1 is present\n");
				conf |= 0x8;
			}
		} else {
			ERROR("DIMM1: unsupported SDRAM type\n");
		}
	}

	if (conf & 0x1) {
		ret = ddr_port_init(0, spd[0], (conf & 0x4), (conf & 0x3) == 0x3);
	} else {
		ddr_lcru_disable(0);
	}

	if (ret < 0) {
		goto error;
	}

	if (conf & 0x4) {
		ret = ddr_port_init(1, spd[2], (conf & 0x1), (conf & 0xc) == 0xc);
		if ((ret < 0) & (conf & 0x1)) {
			ddr_lcru_disable(0);
			goto error;
		}
	}

	tzc_set_transparent(conf);
	return 0;

error:
	ERROR("DDR init failed\n");
	return -1;
}
