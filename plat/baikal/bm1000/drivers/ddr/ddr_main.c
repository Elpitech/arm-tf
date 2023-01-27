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

int ddr_odt_configuration(const unsigned port, const uint16_t crc_val, struct ddr_configuration *const data);

static void tzc_set_transparent(const unsigned conf)
{
	if (conf & 0x1) {
		mmio_write_32(MMTZC0_TZC400_BASE + TZC_GATEKEEPER_OFS, 0xf);
		mmio_write_32(MMTZC0_TZC400_BASE + TZC_RATTRIBUTE_OFS, 0xc000000f);
		mmio_write_32(MMTZC0_TZC400_BASE + TZC_REIDACCESS_OFS, 0xffffffff);
	}

	if (conf & 0x2) {
		mmio_write_32(MMTZC1_TZC400_BASE + TZC_GATEKEEPER_OFS, 0xf);
		mmio_write_32(MMTZC1_TZC400_BASE + TZC_RATTRIBUTE_OFS, 0xc000000f);
		mmio_write_32(MMTZC1_TZC400_BASE + TZC_REIDACCESS_OFS, 0xffffffff);
	}
}

static int ddr_port_init(int port, struct spd_container *ctx, bool dual_mode)
{
	struct ddr_configuration data = {0};
	const uint16_t spd_crc = ctx->content[port].crc[1] << 8 |
				 ctx->content[port].crc[0];

	if (ddr_config_by_spd(port, &data)) {
		goto error;
	}

	if (dual_mode) {
		data.single_ddr = 0;
	} else {
		data.single_ddr = 1;
	}

#if !ECC_ENABLE
	data.ecc_on = false;
#endif

#if DBUS_HALF
	data.dbus_half = true;
#endif

	if (ddr_odt_configuration(port, spd_crc, &data)) {
		goto error;
	}

	if (ddr_lcru_initport(port, data.clock_mhz)) {
		goto failed;
	}
	if (ddr_init(port, dual_mode, &data)) {
		goto failed;
	}

	if (data.ecc_on) {
		ddr_init_ecc_memory(port);
	}

	ctx->speed_mts[port] = data.clock_mhz * 2;

	INFO("DIMM%u: module rate %u MHz, AA-RCD-RP-RAS %u-%u-%u-%u\n", port,
	     data.clock_mhz * 2, data.CL, data.tRCD, data.tRP, data.tRAS);

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
	extern struct spd_container spd_content;

	if (ddr_read_spd(0) != NULL) {
		if (spd_content.content[0].mem_type == SPD_MEMTYPE_DDR4) {
			INFO("DIMM0: DDR4 SDRAM is detected\n");
			conf |= 0x1;
		} else {
			ERROR("DIMM0: unsupported SDRAM type\n");
		}
	}

	if (ddr_read_spd(1) != NULL) {
		if (spd_content.content[1].mem_type == SPD_MEMTYPE_DDR4) {
			INFO("DIMM1: DDR4 SDRAM is detected\n");
			conf |= 0x2;
		} else {
			ERROR("DIMM1: unsupported SDRAM type\n");
		}
	}

	if (conf & 0x1) {
		ret = ddr_port_init(0, &spd_content, (conf & 0x2));
	} else {
		ddr_lcru_disable(0);
	}

	if (ret) {
		goto error;
	}

	if (conf & 0x2) {
		ret = ddr_port_init(1, &spd_content, (conf & 0x1));
	}

	if (ret) {
		goto error;
	}

	tzc_set_transparent(conf);
	return 0;

error:
	ERROR("DDR init failed\n");
	return -1;
}
