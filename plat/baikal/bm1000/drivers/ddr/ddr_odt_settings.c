/*
 * Copyright (c) 2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

#include "ddr_main.h"

enum {
	ODT_SET_1RANK = 0,
	ODT_SET_2RANK,
	ODT_SET_RDIMM_1RANK,
	ODT_SET_RDIMM_2RANK,
	ODT_SET_2DIMM_1RANK,
	ODT_SET_2DIMM_2RANK,
	ODT_SET_MTA9ASF1G72,
	ODT_SET_MTA18ADF2G72,
	ODT_SET_KVR24N17S8,
	ODT_SET_FL2133D4U15D_8G,
	ODT_SET_DP2400D4U17S_4S01,
	ODT_SET_TSGLH72V6E2,
	ODT_SET_KVR24N17S6,
	ODT_SET_KVR26N19S6
};

static const uint16_t dram_crc_val[] = {
	0x0000,	/* 1-rank */
	0x0000,	/* 2-rank */
	0x0000,	/* RDIMM 1-rank */
	0x0000,	/* RDIMM 2-rank */
	0x0000,	/* 1-rank, 2 DIMMs */
	0x0000,	/* 2-rank, 2 DIMMs */
	0x4688,	/* MTA9ASF1G72 */
	0xb1d3,	/* MTA18ADF2G72 */
	0xbd97,	/* KVR24N17S8/8 */
	0x399d,	/* FL2133D4U15D_8G */
	0xfc67,	/* DP2400D4U17S_4S01 */
	0x9d5d,	/* TSGLH72V6E2 */
	0x7fca, /* KVR24N17S6/4 */
	0xb246  /* KVR26N19S6/4 */
};

struct odt_settings {
	uint32_t phy_odi_pu;
	uint32_t phy_odi_pd;
	uint32_t phy_odt;
	uint32_t dic;
	uint32_t rtt_wr;
	uint32_t rtt_nom;
	uint32_t rtt_park;
};

#if defined(BAIKAL_DBM)
static const struct odt_settings dram_odt_set_0[] = {
	{ 0,  0, 0, 0, 1, 2, 4 }, /* 1-rank (default) */
	{ 0,  0, 0, 0, 1, 2, 4 }, /* 2-rank (default) */
	{ 0,  0, 0, 0, 1, 2, 4 }, /* RDIMM 1-rank */
	{ 0,  0, 0, 0, 1, 2, 4 }, /* RDIMM 2-rank */
	{ 15, 0, 3, 0, 2, 5, 0 }, /* 1-rank, 2 DIMMs */
	{ 15, 0, 3, 0, 2, 7, 0 }, /* 2-rank, 2 DIMMs */
	{ 0,  0, 0, 0, 1, 2, 4 }, /* 1-rank (MTA9ASF1G72) */
	{ 0,  0, 0, 0, 1, 2, 4 }, /* 2-rank (MTA18ADF2G72) */
	{ 0,  0, 0, 0, 0, 6, 0 }, /* KVR24N17S8/8 */
	{ 13, 0, 5, 2, 2, 4, 2 }, /* FL2133D4U15D_8G */
	{ 15, 0, 3, 0, 1, 4, 2 }, /* DP2400D4U17S_4S01 */
	{ 15, 0, 1, 0, 2, 5, 2 }, /* TSGLH72V6E2 */
	{ 0,  0, 0, 0, 1, 2, 4 }, /* KVR24N17S6/4 */
	{ 0,  0, 0, 0, 1, 2, 4 }  /* KVR26N19S6/4 */
};

static const struct odt_settings dram_odt_set_1[] = {
	{ 0,  0, 0, 0, 1, 2, 4 }, /* 1-rank (default) */
	{ 0,  0, 0, 0, 1, 2, 4 }, /* 2-rank (default) */
	{ 0,  0, 0, 0, 1, 2, 4 }, /* RDIMM 1-rank */
	{ 0,  0, 0, 0, 1, 2, 4 }, /* RDIMM 2-rank */
	{ 15, 0, 3, 0, 2, 5, 0 }, /* 1-rank, 2 DIMMs */
	{ 15, 0, 3, 0, 2, 7, 0 }, /* 2-rank, 2 DIMMs */
	{ 0,  0, 0, 0, 1, 2, 4 }, /* 1-rank (MTA9ASF1G72) */
	{ 0,  0, 0, 0, 1, 2, 4 }, /* 2-rank (MTA18ADF2G72) */
	{ 0,  0, 0, 0, 0, 6, 0 }, /* KVR24N17S8/8 */
	{ 13, 0, 5, 2, 2, 4, 2 }, /* FL2133D4U15D_8G */
	{ 15, 0, 3, 0, 1, 4, 2 }, /* DP2400D4U17S_4S01 */
	{ 15, 0, 1, 0, 2, 5, 2 }, /* TSGLH72V6E2 */
	{ 0,  0, 0, 0, 1, 2, 4 }, /* KVR24N17S6/4 */
	{ 0,  0, 0, 0, 1, 2, 4 }  /* KVR26N19S6/4 */
};
#elif defined(BAIKAL_MBM)
static const struct odt_settings dram_odt_set_0[] = {
	{ 13, 0, 1, 0, 4, 0, 0 }, /* 1-rank (default) 34,240  34, 80,DIS,DIS */
	{ 15, 0, 1, 0, 2, 0, 2 }, /* 2-rank (default) 30,240  34,120,DIS,120 */
	{ 15, 0, 1, 0, 1, 2, 2 }, /* RDIMM 1-rank     30,240  34,120,120,120 (training by MTA9ADF1G72PZ-3G2) */
	{ 15, 0, 1, 0, 1, 4, 4 }, /* RDIMM 2-rank     30,240  34,120,240,240 (training by MTA18ASF2G72PDZ-2G3) */
	{ 15, 0, 7, 0, 1, 6, 2 }, /* 1-rank, 2 DIMMs */
	{ 15, 0, 3, 0, 0, 3, 0 }, /* 2-rank, 2 DIMMs */
	{ 13, 0, 1, 0, 4, 0, 0 }, /* 1-rank (MTA9ASF1G72) */
	{ 15, 0, 1, 0, 1, 0, 2 }, /* 2-rank (MTA18ADF2G72) */
	{ 0,  0, 0, 0, 0, 6, 0 }, /* KVR24N17S8/8 */
	{ 13, 0, 5, 2, 2, 4, 2 }, /* FL2133D4U15D_8G */
	{ 15, 0, 3, 0, 1, 4, 2 }, /* DP2400D4U17S_4S01 */
	{ 15, 0, 1, 0, 2, 0, 2 }, /* TSGLH72V6E2 */
	{ 13, 0, 3, 0, 4, 6, 7 }, /* KVR24N17S6/4 */
	{  9, 0, 5, 0, 4, 3, 6 }  /* KVR26N19S6/4 */
};

static const struct odt_settings dram_odt_set_1[] = {
	{ 15, 0, 3, 0, 4, 0, 0 }, /* 1-rank (default) 30,120  34, 80,DIS,DIS */
	{ 15, 0, 5, 0, 2, 0, 2 }, /* 2-rank (default) 30, 80  34,240,DIS,120 */
	{ 15, 0, 5, 0, 4, 0, 0 }, /* RDIMM 1-rank     30, 80  34, 80,DIS,DIS (training by MTA9ADF1G72PZ-3G2) */
	{ 15, 0, 3, 0, 1, 2, 4 }, /* RDIMM 2-rank     30,120  34,120,120,240 (training by MTA18ASF2G72PDZ-2G3) */
	{ 15, 0, 7, 0, 1, 6, 2 }, /* 1-rank, 2 DIMMs */
	{ 15, 0, 3, 0, 2, 7, 0 }, /* 2-rank, 2 DIMMs */
	{ 15, 0, 3, 0, 4, 0, 0 }, /* 1-rank (MTA9ASF1G72) */
	{ 15, 0, 5, 0, 2, 0, 2 }, /* 2-rank (MTA18ADF2G72) */
	{ 0,  0, 0, 0, 0, 6, 0 }, /* KVR24N17S8/8 */
	{ 13, 0, 5, 2, 2, 4, 2 }, /* FL2133D4U15D_8G */
	{ 15, 0, 3, 0, 1, 4, 2 }, /* DP2400D4U17S_4S01 */
	{ 15, 0, 1, 0, 2, 0, 2 }, /* TSGLH72V6E2 */
	{ 15, 0, 3, 0, 4, 0, 0 }, /* KVR24N17S6/4 */
	{ 15, 0, 3, 0, 4, 0, 0 }  /* KVR26N19S6/4 */
};
#endif

void ddr_odt_configuration(const unsigned port,
			   const uint16_t crc_val,
			   struct ddr_configuration *const data)
{
	unsigned odt_set;

	if (crc_val == dram_crc_val[ODT_SET_MTA9ASF1G72]) {
		odt_set = ODT_SET_MTA9ASF1G72;
	} else if (crc_val == dram_crc_val[ODT_SET_KVR26N19S6]) {
		odt_set = ODT_SET_KVR26N19S6;
	} else if (crc_val == dram_crc_val[ODT_SET_KVR24N17S6]) {
		odt_set = ODT_SET_KVR24N17S6;
	} else if (crc_val == dram_crc_val[ODT_SET_MTA18ADF2G72]) {
		odt_set = ODT_SET_MTA18ADF2G72;
	} else if (crc_val == dram_crc_val[ODT_SET_KVR24N17S8]) {
		odt_set = ODT_SET_KVR24N17S8;
	} else if (crc_val == dram_crc_val[ODT_SET_FL2133D4U15D_8G]) {
		odt_set = ODT_SET_FL2133D4U15D_8G;
	} else if (crc_val == dram_crc_val[ODT_SET_DP2400D4U17S_4S01]) {
		odt_set = ODT_SET_DP2400D4U17S_4S01;
	} else if (crc_val == dram_crc_val[ODT_SET_TSGLH72V6E2]) {
		odt_set = ODT_SET_TSGLH72V6E2;
	} else if (data->dimms == 2 && data->ranks == 2) {
		odt_set = ODT_SET_2DIMM_2RANK;
	} else if (data->dimms == 2 && data->ranks == 1) {
		odt_set = ODT_SET_2DIMM_1RANK;
	} else if (data->registered_dimm && data->ranks == 2) {
		odt_set = ODT_SET_RDIMM_2RANK;
	} else if (data->registered_dimm && data->ranks == 1) {
		odt_set = ODT_SET_RDIMM_1RANK;
	} else if (data->ranks == 2) {
		odt_set = ODT_SET_2RANK;
	} else {
		odt_set = ODT_SET_1RANK;
	}

	if (!port) {
		data->RTT_PARK	 = dram_odt_set_0[odt_set].rtt_park;
		data->RTT_NOM	 = dram_odt_set_0[odt_set].rtt_nom;
		data->RTT_WR	 = dram_odt_set_0[odt_set].rtt_wr;
		data->DIC	 = dram_odt_set_0[odt_set].dic;
		data->PHY_ODI_PU = dram_odt_set_0[odt_set].phy_odi_pu;
		data->PHY_ODI_PD = dram_odt_set_0[odt_set].phy_odi_pd;
		data->PHY_ODT	 = dram_odt_set_0[odt_set].phy_odt;
	} else {
		data->RTT_PARK	 = dram_odt_set_1[odt_set].rtt_park;
		data->RTT_NOM	 = dram_odt_set_1[odt_set].rtt_nom;
		data->RTT_WR	 = dram_odt_set_1[odt_set].rtt_wr;
		data->DIC	 = dram_odt_set_1[odt_set].dic;
		data->PHY_ODI_PU = dram_odt_set_1[odt_set].phy_odi_pu;
		data->PHY_ODI_PD = dram_odt_set_1[odt_set].phy_odi_pd;
		data->PHY_ODT	 = dram_odt_set_1[odt_set].phy_odt;
	}
}
