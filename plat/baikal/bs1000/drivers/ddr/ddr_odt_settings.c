/*
 * Copyright (c) 2022, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <baikal_def.h>
#include "ddr_main.h"

enum {
	ODT_SET_1RANK_SDIMM = 0,
	ODT_SET_2RANK_SDIMM,
	ODT_SET_1RANK_DDIMM,
	ODT_SET_2RANK_DDIMM,
};

struct odt_settings {
	uint32_t dic;
	uint32_t rtt_wr;
	uint32_t rtt_nom;
	uint32_t rtt_park;
	uint32_t host_vref;
	uint32_t dram_vref;
	uint32_t phy_odt;
	uint32_t phy_odi;
};

static const struct odt_settings dram_odt_set[] = {
	{ 0, 1, 0, 0, 82,  14, 80, 34 },
	{ 0, 1, 3, 0, 84,  26, 80, 34 },
	{ 0, 1, 3, 0, 100, 28, 80, 34 },
	{ 0, 1, 3, 0, 102, 30, 80, 34 }
};

#ifdef BAIKAL_DDRCFG_IN_FLASH
#include <common/debug.h>

static void ddr_odt_flash_values(struct ddr_configuration *const data)
{
	int err_flag = 0;
	struct ddr_flash_config *ddr_port_config = data->ddr_port_config;

	if (ddr_port_config->ddr_sign != BAIKAL_FLASH_USE_STR) {
		return;
	}

	if (ddr_port_config->flsh_dic) {
		switch (ddr_port_config->flsh_dic) {
		case BAIKAL_DIC_RZQ_DIV_7:
			data->DIC = 0;
			break;
		case BAIKAL_DIC_RZQ_DIV_5:
			data->DIC = 1;
			break;
		default:
			err_flag = 0xf0f0;
			break;
		}
	}
	if (ddr_port_config->flsh_rtt_wr) {
		switch (ddr_port_config->flsh_rtt_wr) {
		case BAIKAL_RTTWR_DYN_OFF:
			data->RTT_WR = 0;
			break;
		case BAIKAL_RTTWR_RZQ_DIV_4:
			data->RTT_WR = 1;
			break;
		case BAIKAL_RTTWR_RZQ_DIV_2:
			data->RTT_WR = 2;
			break;
		case BAIKAL_RTTWR_HI_Z:
			data->RTT_WR = 3;
			break;
		case BAIKAL_RTTWR_RZQ_DIV_3:
			data->RTT_WR = 4;
			break;
		default:
			err_flag = 0xf0f0;
			break;
		}
	}
	if (ddr_port_config->flsh_rtt_nom) {
		switch (ddr_port_config->flsh_rtt_nom) {
		case BAIKAL_RTTNOM_RZQ_DIS:
			data->RTT_NOM = 0;
			break;
		case BAIKAL_RTTNOM_RZQ_DIV_4:
			data->RTT_NOM = 1;
			break;
		case BAIKAL_RTTNOM_RZQ_DIV_2:
			data->RTT_NOM = 2;
			break;
		case BAIKAL_RTTNOM_RZQ_DIV_6:
			data->RTT_NOM = 3;
			break;
		case BAIKAL_RTTNOM_RZQ_DIV_1:
			data->RTT_NOM = 4;
			break;
		case BAIKAL_RTTNOM_RZQ_DIV_5:
			data->RTT_NOM = 5;
			break;
		case BAIKAL_RTTNOM_RZQ_DIV_3:
			data->RTT_NOM = 6;
			break;
		case BAIKAL_RTTNOM_RZQ_DIV_7:
			data->RTT_NOM = 7;
			break;
		default:
			err_flag = 0xf0f0;
			break;
		}
	}
	if (ddr_port_config->flsh_rtt_park) {
		switch (ddr_port_config->flsh_rtt_park) {
		case BAIKAL_RTTPARK_RZQ_DIS:
			data->RTT_PARK = 0;
			break;
		case BAIKAL_RTTPARK_RZQ_DIV_4:
			data->RTT_PARK = 1;
			break;
		case BAIKAL_RTTPARK_RZQ_DIV_2:
			data->RTT_PARK = 2;
			break;
		case BAIKAL_RTTPARK_RZQ_DIV_6:
			data->RTT_PARK = 3;
			break;
		case BAIKAL_RTTPARK_RZQ_DIV_1:
			data->RTT_PARK = 4;
			break;
		case BAIKAL_RTTPARK_RZQ_DIV_5:
			data->RTT_PARK = 5;
			break;
		case BAIKAL_RTTPARK_RZQ_DIV_3:
			data->RTT_PARK = 6;
			break;
		case BAIKAL_RTTPARK_RZQ_DIV_7:
			data->RTT_PARK = 7;
			break;
		default:
			err_flag = 0xf0f0;
			break;
		}
	}
	if (ddr_port_config->flsh_vref_use) {
		data->HOST_VREF = ddr_port_config->flsh_host_v;
		data->DRAM_VREF = ddr_port_config->flsh_dram_v;
	}
	if (err_flag) {
		ERROR("Failed to configure some of the ODT values from flash\n");
	}
}
#endif

int ddr_odt_configuration(const unsigned int port,
			  struct ddr_configuration *const data)
{
	unsigned int odt_set;

	if (data->dimms == 1) {
		if (data->ranks == 1) {
			odt_set = ODT_SET_1RANK_SDIMM;
		} else {
			odt_set = ODT_SET_2RANK_SDIMM;
		}
	} else {
		if (data->ranks == 1) {
			odt_set = ODT_SET_1RANK_DDIMM;
		} else {
			odt_set = ODT_SET_2RANK_DDIMM;
		}
	}

	data->RTT_PARK	= dram_odt_set[odt_set].rtt_park;
	data->RTT_NOM	= dram_odt_set[odt_set].rtt_nom;
	data->RTT_WR	= dram_odt_set[odt_set].rtt_wr;
	data->DIC	= dram_odt_set[odt_set].dic;
	data->HOST_VREF	= dram_odt_set[odt_set].host_vref;
	data->DRAM_VREF	= dram_odt_set[odt_set].dram_vref;
	data->PHY_ODT	= dram_odt_set[odt_set].phy_odt;
	data->PHY_ODI	= dram_odt_set[odt_set].phy_odi;

#ifdef BAIKAL_DDRCFG_IN_FLASH
	ddr_odt_flash_values(data);
#endif
	return 0;
}
