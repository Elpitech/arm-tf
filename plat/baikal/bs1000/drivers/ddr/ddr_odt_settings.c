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
	uint32_t odt_map;
};

static const struct odt_settings dram_odt_sets[][4] = {
#if defined(ELPITECH)
#if ((BOARD_VER == 11) || (BOARD_VER == 12))
	{ // CH0
		{ 0, 1, 0, 0, 80,   8, 80, 34, 0x0201 },
		{ 0, 3, 1, 0, 79,  13, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
	{ // CH1
		{ 0, 1, 0, 0, 80,   8, 80, 34, 0x0201 },
		{ 0, 3, 1, 0, 79,  11, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
	{ // CH2
		{ 0, 4, 0, 0, 80,  13, 80, 34, 0x0201 },
		{ 0, 3, 1, 4, 83,  24, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
	{ // CH3
		{ 0, 4, 0, 0, 80,  13, 80, 34, 0x0201 },
		{ 0, 0, 5, 0, 84,  26, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
	{ // CH4
		{ 0, 4, 0, 0, 80,  13, 80, 34, 0x0201 },
		{ 0, 0, 5, 0, 84,  26, 80, 34, 0x0102 },
//		{ 0, 3, 1, 4, 84,  17, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
	{ // CH5
		{ 0, 4, 0, 0, 80,  13, 80, 34, 0x0201 },
		{ 0, 0, 5, 0, 84,  26, 80, 34, 0x0102 },
//		{ 0, 3, 1, 4, 79,  11, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
#if (BOARD_VER == 12)
	{ // CH0-chip1
		{ 0, 1, 0, 0, 80,   8, 80, 34, 0x0201 },
		{ 0, 3, 1, 0, 80,  17, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
	{ // CH1-chip1
		{ 0, 1, 0, 0, 80,   8, 80, 34, 0x0201 },
		{ 0, 3, 1, 0, 79,  19, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
	{ // CH2-chip1
		{ 0, 4, 0, 0, 80,  13, 80, 34, 0x0201 },
		{ 0, 3, 1, 0, 79,  19, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
	{ // CH3-chip1
		{ 0, 4, 0, 0, 80,  13, 80, 34, 0x0201 },
		{ 0, 3, 1, 0, 79,  19, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
	{ // CH4-chip1
		{ 0, 4, 0, 0, 80,  13, 80, 34, 0x0201 },
		{ 0, 3, 1, 0, 79,  17, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
	{ // CH5-chip1
		{ 0, 4, 0, 0, 80,  13, 80, 34, 0x0201 },
		{ 0, 3, 1, 0, 79,  17, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
#endif
#elif BOARD_VER == 9
	{ // CH0
		{ 0, 1, 0, 0, 80,   8, 80, 34, 0x0201 },
		{ 0, 0, 1, 0, 80,  16, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
	{ // CH1
		{ 0, 1, 0, 0, 80,   8, 80, 34, 0x0201 },
		{ 0, 3, 1, 0, 80,  18, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
	{ // CH2
		{ 0, 4, 0, 0, 80,  13, 80, 34, 0x0201 },
		{ 0, 2, 6, 0, 80,  16, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
	{ // CH3
		{ 0, 4, 0, 0, 80,  13, 80, 34, 0x0201 },
		{ 0, 0, 1, 0, 81,  15, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
	{ // CH4
		{ 0, 4, 0, 0, 80,  13, 80, 34, 0x0201 },
		{ 0, 0, 1, 0, 80,  16, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
	{ // CH5
		{ 0, 4, 0, 0, 80,  13, 80, 34, 0x0201 },
		{ 0, 3, 6, 0, 80,  11, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
#elif BOARD_VER == 13
	{ // CH0
		{ 0, 1, 0, 0, 80,   8, 80, 34, 0x0201 },
		{ 0, 3, 1, 0, 80,  18, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
	{ // CH1
		{ 0, 1, 0, 0, 80,   8, 80, 34, 0x0201 },
		{ 0, 3, 1, 0, 78,  19, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
	{ // CH2
		{ 0, 4, 0, 0, 80,  13, 80, 34, 0x0201 },
		{ 0, 3, 1, 4, 84,  18, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
	{ // CH3
		{ 0, 4, 0, 0, 80,  13, 80, 34, 0x0201 },
		{ 0, 2, 6, 0, 80,  18, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
	{ // CH4
		{ 0, 4, 0, 0, 80,  13, 80, 34, 0x0201 },
		{ 0, 3, 6, 0, 79,  15, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
	{ // CH5
		{ 0, 4, 0, 0, 80,  13, 80, 34, 0x0201 },
		{ 0, 0, 1, 0, 80,  17, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
#elif BOARD_VER == 14
	{ // CH0
		{ 0, 1, 0, 0, 80,   8, 80, 34, 0x0201 },
		{ 0, 0, 1, 0, 80,  16, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
	{ // CH1
		{ 0, 1, 0, 0, 80,   8, 80, 34, 0x0201 },
		{ 0, 4, 0, 0, 80,  13, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
	{ // CH2
		{ 0, 4, 0, 0, 80,  13, 80, 34, 0x0201 },
		{ 0, 2, 6, 0, 80,  16, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
	{ // CH3
		{ 0, 4, 0, 0, 80,  13, 80, 34, 0x0201 },
		{ 0, 0, 1, 0, 80,  16, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
	{ // CH4
		{ 0, 4, 0, 0, 80,  13, 80, 34, 0x0201 },
		{ 0, 0, 1, 0, 80,  16, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
	{ // CH5
		{ 0, 4, 0, 0, 80,  13, 80, 34, 0x0201 },
		{ 0, 3, 6, 0, 80,  11, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
#else // !(BOARD_VER == {11,12,9,13,14}
	{ // all channels
		{ 0, 1, 0, 0, 82,  14, 80, 34, 0x0 },
		{ 0, 1, 0, 0, 84,  12, 80, 34, 0x0 },
		{ 0, 1, 3, 0, 96,  28, 80, 34, 0x00110044 },
		{ 0, 2, 3, 4, 100, 30, 80, 34, 0x22228888 }
	},
#endif
#else // !defined(ELPITECH)
	{ // all channels
		{ 0, 4, 0, 0, 82,  14, 80, 34, 0x0201 },
		{ 0, 0, 5, 0, 84,  26, 80, 34, 0x0102 },
		{ 0, 1, 3, 7, 100, 28, 80, 34, 0x00010004 },
		{ 0, 2, 7, 4, 102, 30, 80, 34, 0x22228888 }
	},
#endif
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

int ddr_odt_configuration(unsigned int port,
			  struct ddr_configuration *const data)
{
	unsigned int odt_set;
	const struct odt_settings *odt_set_p;

#if defined(ELPITECH)
#if (BOARD_VER == 12)
	if (port >= 12)
#elif (BOARD_VER == 11) || (BOARD_VER == 9) || (BOARD_VER == 13) || (BOARD_VER == 14)
	if (port >= 6)
#endif
#endif
		port = 0; // just to stay within array size
	odt_set_p = &dram_odt_sets[port][0];

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

	data->RTT_PARK	= odt_set_p[odt_set].rtt_park;
	data->RTT_NOM	= odt_set_p[odt_set].rtt_nom;
	data->RTT_WR	= odt_set_p[odt_set].rtt_wr;
	data->DIC	= odt_set_p[odt_set].dic;
	data->HOST_VREF	= odt_set_p[odt_set].host_vref;
	data->DRAM_VREF	= odt_set_p[odt_set].dram_vref;
	data->PHY_ODT	= odt_set_p[odt_set].phy_odt;
	data->PHY_ODI	= odt_set_p[odt_set].phy_odi;
	data->odt_map	= odt_set_p[odt_set].odt_map;

#ifdef BAIKAL_DDRCFG_IN_FLASH
	ddr_odt_flash_values(data);
#endif
	return 0;
}
