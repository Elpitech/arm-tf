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

static int ddr_port_init(int port, const struct ddr4_spd_eeprom *spd,
			 struct ddr_local_conf *cfg, bool dual_mode)
{
	int ret;
	struct ddr_configuration data = {0};

	ret = ddr_config_by_spd(spd, &data, cfg);
	if (ret) {
		goto error;
	}

	if (dual_mode) {
		data.single_ddr = 0;
	} else {
		data.single_ddr = 1;
	}

	data.dimms = cfg->dimms;

#if !ECC_ENABLE
	data.ecc_on = false;
#endif

#if DBUS_HALF
	data.dbus_half = true;
#endif

	ddr_odt_configuration(port, ((uint16_t)spd->crc[1] << 8) + spd->crc[0],
			      &data);

	if (cfg->rtt_wr)
		data.RTT_WR = cfg->rtt_wr - 1;
	if (cfg->rtt_nom)
		data.RTT_NOM = cfg->rtt_nom - 1;
	if (cfg->rtt_park)
		data.RTT_PARK = cfg->rtt_park - 1;
	if (cfg->phy_odt)
		data.PHY_ODT = cfg->phy_odt - 1;
	if (cfg->phy_odi_pu)
		data.PHY_ODI_PU = cfg->phy_odi_pu - 1;
	if (cfg->odi)
		data.DIC = (cfg->odi - 1) * 2; // only 0 and 2 valid

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
	struct ddr_local_conf *conf0 = NULL, *conf1 = NULL;

	spd[0] = ddr_read_spd(0);
	if (spd[0] != NULL) {
		if (spd[0]->mem_type == SPD_MEMTYPE_DDR4) {
			INFO("DIMM0: DDR4 SDRAM is detected\n");
			conf |= DIMM0_PRESENT;
			conf0 = (struct ddr_local_conf *)((uint8_t *)spd[0] + 384); /* SPD user area */
			spd[1] = ddr_read_spd(1);
			if (spd[1] != NULL && !memcmp(spd[0], spd[1], 128)) {
				INFO("DIMM0-1 is present\n");
				conf |= DIMM1_PRESENT;
			}
		} else {
			ERROR("DIMM0: unsupported SDRAM type\n");
		}
	}

	spd[2] = ddr_read_spd(2);
	if (spd[2] != NULL) {
		if (spd[2]->mem_type == SPD_MEMTYPE_DDR4) {
			INFO("DIMM1: DDR4 SDRAM is detected\n");
			conf |= DIMM2_PRESENT;
			conf1 = (struct ddr_local_conf *)((uint8_t *)spd[2] + 384); /* SPD user area */
			spd[3] = ddr_read_spd(3);
			if (spd[3] != NULL && !memcmp(spd[2], spd[3], 128)) {
				INFO("DIMM1-1 is present\n");
				conf |= DIMM3_PRESENT;
			}
		} else {
			ERROR("DIMM1: unsupported SDRAM type\n");
		}
	}

	ddr_conf(conf0, conf1, conf);

	if (conf & DIMM0_PRESENT) {
		ret = ddr_port_init(0, spd[0], conf0, (conf & DIMM2_PRESENT));
	} else {
		ddr_lcru_disable(0);
	}

	if (ret < 0) {
		goto error;
	}

	if (conf & DIMM2_PRESENT) {
		ret = ddr_port_init(1, spd[2], conf1, (conf & DIMM0_PRESENT));
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
