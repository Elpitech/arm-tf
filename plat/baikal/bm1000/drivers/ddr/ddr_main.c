/*
 * Copyright (c) 2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <bm1000_def.h>
#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>

#include "ddr_lcru.h"
#include "ddr_master.h"

#define DBUS_HALF	0
#define ECC_ENABLE	0

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

static int ddr_port_init(int port, bool dual_mode, bool double_dimm)
{
	int ret;
	struct ddr4_spd_eeprom spd;
	struct ddr_configuration data = {0};

	ret = ddr_read_spd(port, &spd);
	if (ret) {
		goto error;
	}

	ret = ddr_config_by_spd(&spd, &data);
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

	if (data.ecc_on) {
		data.ecc_on = ECC_ENABLE;
	}
	if (!data.dbus_half) {
		data.dbus_half = DBUS_HALF;
	}

	ddr_odt_configuration(port, ((uint16_t)spd.crc[1] << 8) + spd.crc[0],
			      &data);

	ret = ddr_lcru_initport(port, data.clock_mhz);
	if (ret) {
		goto failed;
	}
	ret = ddr_init(port, dual_mode, &data);
	if (ret) {
		goto failed;
	}

	INFO("DDR port #%d: module speed: %u MHz\n", port, data.clock_mhz * 2);
	INFO("DDR port #%d: AA-RCD-RP-RAS (cycles): %u-%u-%u-%u\n", port,
	     data.CL, data.tRCD, data.tRP, data.tRASmax);

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
	const unsigned conf = ddr_dimm_presence();

	if (conf & 0x3) {
		ret = ddr_port_init(0, !!(conf & 0xc), (conf & 0x3) == 0x3);
	} else {
		ddr_lcru_disable(0);
	}

	if (ret < 0) {
		goto error;
	}

	if (conf & 0xc) {
		ret = ddr_port_init(1, !!(conf & 0x3), (conf & 0xc) == 0xc);
		if ((ret < 0) && (conf & 0x1)) {
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
