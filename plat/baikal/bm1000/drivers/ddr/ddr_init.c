/*
 * Copyright (c) 2021-2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>

#include <baikal_def.h>
#include <ndelay.h>

#include "ddr_baikal.h"
#include "ddr_lcru.h"
#include "ddr_master.h"

/* Macros to read/write DDR Memory Controller (uMCTL2) registers */
#define BM_DDRC_READ(p, r)		mmio_read_32(DDR_CTRL(p) + (r))
#define BM_DDRC_WRITE(p, r, v)		mmio_write_32(DDR_CTRL(p) + (r), v)

/* Macros to read/write DDR4 multiPHY Utility Block (PUB) registers */
#define BM_DDR_PUB_READ(p, r)		mmio_read_32(DDR_PHY(p) + (r))
#define BM_DDR_PUB_WRITE(p, r, v)	mmio_write_32(DDR_PHY(p) + (r), v)

#ifdef BAIKAL_DDRCFG_IN_FLASH
#include "ddr_menu.h"

extern struct ddr_flash_config ddr_storage;
#endif

static void baikal_ddrc_set_registers(const unsigned int port,
				      const struct ddr_configuration *data)
{
	uint32_t hif_addr;
	uint32_t reg;
	uint32_t rsl;

	/* Program SAR regs */
	if (data->single_ddr) {
		/* For single DDR Controller in SoC we have normal address regions */
		BM_DDRC_WRITE(port, DDRC_SARBASE(0), 8);    /* region address 2..4 GiB (UMCTL2_SARMINSIZE = 256 MiB) */
		BM_DDRC_WRITE(port, DDRC_SARSIZE(0), 7);    /* region size 2 GiB */
		BM_DDRC_WRITE(port, DDRC_SARBASE(1), 136);  /* region address 34..64 GiB */
		BM_DDRC_WRITE(port, DDRC_SARSIZE(1), 119);  /* region size 30 GiB */
		BM_DDRC_WRITE(port, DDRC_SARBASE(2), 2176); /* region address 544..1024 GiB */
		BM_DDRC_WRITE(port, DDRC_SARSIZE(2), 1919); /* region size 240 GiB */
	} else {
		BM_DDRC_WRITE(port, DDRC_SARBASE(0), 4);
		BM_DDRC_WRITE(port, DDRC_SARSIZE(0), 3);
		BM_DDRC_WRITE(port, DDRC_SARBASE(1), 68);
		BM_DDRC_WRITE(port, DDRC_SARSIZE(1), 59);
		BM_DDRC_WRITE(port, DDRC_SARBASE(2), 1088);
		BM_DDRC_WRITE(port, DDRC_SARSIZE(2), 959);
	}

	/*
	 * Configure MSTR reg
	 * We have two CS signals (two ranks) per DIMM slot on DBM board.
	 * Valid values for Mstr.active_ranks are: 0x1, 0x11, 0x1111
	 */
	reg = 0x40040010;
	if (data->dimms == 1) {
		reg |= GENMASK(27, 24) & (((1 << data->ranks) - 1) << 24);
	} else {
		reg |= GENMASK(27, 24) & (0xf << 24);
		reg |= 1 << 10; /* enable 2T mode */
	}
#ifdef BAIKAL_DDRCFG_IN_FLASH
	if (ddr_storage.flsh_1t2t &&
	    ddr_storage.ddr_sign == BAIKAL_FLASH_USE_STR) {
		reg |= 1 << 10; /* enable 2T mode */
	}
#endif
	if (data->dbus_half) {
		reg |= 1 << 12; /* MSTR_DB_WIDTH::HALF = 1; */
	}
	BM_DDRC_WRITE(port, DDRC_MSTR, reg);

	/* Configure PCCFG reg */
	if (data->dbus_half) {
		BM_DDRC_WRITE(port, DDRC_PCCFG, 1 << 8);
	}

	/* Configure SCHED reg */
	BM_DDRC_WRITE(port, DDRC_SCHED, 0xd00);
	/* Configure DBICTL reg */
	BM_DDRC_WRITE(port, DDRC_DBICTL, 0x1);
	/* Configure INIT0 reg */
	BM_DDRC_WRITE(port, DDRC_INIT0, 0x4002004e);
	/* Configure RANKCTL reg */
	BM_DDRC_WRITE(port, DDRC_RANKCTL, 0x33f);

	/*
	 * We set default mapping (HIF->DRAM) (see Table2-11 BL=8 recommendation in DataBook)
	 * COL5,4,3,2	->HIF A6,A5,A4,A2
	 * BG0		->HIF A3
	 * COL9,8,7,6	->HIF A10,A9,A8,A7
	 * COL11,10	->none
	 * BA1,0	->HIF A12,A11
	 */
	if (data->dbus_half) {
		BM_DDRC_WRITE(port, DDRC_ADDRMAP2, 0x7); /* map COL6,5,4,3 ->HIF A5,A4,A3,A9 */
		BM_DDRC_WRITE(port, DDRC_ADDRMAP3, 0x1f000000); /* map COL9,8,7 ->HIF A8,A7,A6 */
		BM_DDRC_WRITE(port, DDRC_ADDRMAP1, 0x003f0808); /* map BA2->none, BA1->HIF A11, BA0->HIF A10 */
		hif_addr = 12; /* next empty HIF address */
	} else {
		BM_DDRC_WRITE(port, DDRC_ADDRMAP2, 0x01010100);
		BM_DDRC_WRITE(port, DDRC_ADDRMAP3, 0x01010101);
		BM_DDRC_WRITE(port, DDRC_ADDRMAP1, 0x003f0909);
		hif_addr = 13; /* next empty HIF address */
	}
	BM_DDRC_WRITE(port, DDRC_ADDRMAP4, 0x00001f1f);

	reg = 0x1;
	if (data->dbus_half) {
		reg &= ~0x3f; /* map BG0 ->HIF A2 */
	}
	if (data->bank_groups > 2) {
		reg |= GENMASK(13, 8) & ((hif_addr++ - 3) << 8); /* map BG1->HIF */
	} else {
		reg |= 63 << 8; /* map BG1->none */
	}
	BM_DDRC_WRITE(port, DDRC_ADDRMAP8, reg);

	reg  = 0;
	reg |= 0xf & (hif_addr++ - 6); /* map ROW0->HIF */
	reg |= GENMASK(11, 8) & ((hif_addr++ - 7) << 8); /* map ROW1->HIF */
	reg |= GENMASK(19, 16) & ((hif_addr - 8) << 16); /* map ROW10..2->HIF */
	hif_addr += 9;
	reg |= GENMASK(27, 24) & ((hif_addr++ - 17) << 24); /* map ROW11->HIF */
	BM_DDRC_WRITE(port, DDRC_ADDRMAP5, reg);

	reg  = 0;
	reg |= GENMASK(3, 0) & (hif_addr++ - 18); /* map ROW12->HIF */
	reg |= GENMASK(11, 8) & ((hif_addr++ - 19) << 8); /* map ROW13->HIF */
	if ((data->row_address - 1) >= 14) {
		reg |= GENMASK(19, 16) & ((hif_addr++ - 20) << 16); /* ROW14->HIF */
	} else {
		reg |= GENMASK(19, 16) & (15 << 16); /* ROW14->none */
	}
	if ((data->row_address - 1) >= 15) {
		reg |= GENMASK(27, 24) & ((hif_addr++ - 21) << 24); /* ROW15->HIF */
	} else {
		reg |= GENMASK(27, 24) & (15 << 24); /* ROW15->none */
	}
	BM_DDRC_WRITE(port, DDRC_ADDRMAP6, reg);

	reg = 0;
	if ((data->row_address - 1) >= 16) {
		reg |= GENMASK(3, 0) & (hif_addr++ - 22); /* ROW16->HIF */
	}
	if ((data->row_address - 1) == 17) {
		reg |= GENMASK(11, 8) & ((hif_addr++ - 23) << 8); /* ROW17->HIF */
	} else {
		reg |= GENMASK(11, 8) & (15 << 8); /* ROW17->none */
	}
	BM_DDRC_WRITE(port, DDRC_ADDRMAP7, reg);

	/*
	 * DIMM_0 have 00b and 01b rank addresses on DBM board.
	 * DIMM_1 have 10b and 11b rank addresses on DBM board.
	 */
	reg = 0;
	if (data->ranks == 2) {
		reg |= GENMASK(4, 0) & (hif_addr++ - 6); /* CS0->hifXX */
	} else {
		/* CS0->none: map to none for 1-rank DIMMs (that's not external CS0 signal) */
		reg |= GENMASK(4, 0) & 31;
	}
	if (data->dimms == 2) {
		reg |= GENMASK(12, 8) & ((hif_addr++ - 7) << 8); /* CS1->hifXX */
	} else {
		reg |= GENMASK(12, 8) & (31 << 8); /* CS1->none */
	}
	BM_DDRC_WRITE(port, DDRC_ADDRMAP0, reg);

	/* Configure DIMMCTL reg */
	reg = 0x1;
	if (data->registered_dimm) {
		reg |= 1 << 2;
	}
	if (data->mirrored_dimm) {
		reg |= 1 << 1;
	}
	BM_DDRC_WRITE(port, DDRC_DIMMCTL, reg);

	/* Program DQ map */
	BM_DDRC_WRITE(port, DDRC_DQMAP0, data->DQ_map[0]);
	BM_DDRC_WRITE(port, DDRC_DQMAP1, data->DQ_map[1]);
	BM_DDRC_WRITE(port, DDRC_DQMAP2, data->DQ_map[2]);
	BM_DDRC_WRITE(port, DDRC_DQMAP3, data->DQ_map[3]);
	BM_DDRC_WRITE(port, DDRC_DQMAP4, data->DQ_map[4]);
	if (!data->DQ_swap_rank) {
		BM_DDRC_WRITE(port, DDRC_DQMAP5, 0x1);
	}

	/* Program ECC */
	if (data->ecc_on) {
		BM_DDRC_WRITE(port, DDRC_ECCCFG0, 0x4);
	}

	/* Program DFI timings */
	/* See PHY DataBook, Table7-1 (p.648) */
	reg  = 0x00808200;
	reg |= GENMASK(22, 16) & ((data->RL - 4 + !!data->registered_dimm) << 16);
	/* See PHY DataBook, Table7-1 (p.648) */
	reg |= GENMASK(5, 0) & (data->WL - 2 + !!data->registered_dimm);
	/* See PHY DataBook, Table7-1 (p.648) + 2 * DWC_PIPE_DFI2PHY */
	reg |= GENMASK(28, 24) & ((2 + 2 + !!data->registered_dimm) << 24);
	BM_DDRC_WRITE(port, DDRC_DFITMG0, reg);
	BM_DDRC_WRITE(port, DDRC_DFITMG1, 0x00080404);

	/* Program DDRC DRAM timings */
	reg  = 0x00002800;
	reg |= GENMASK(30, 24) & (((data->WL + 4 + data->tWR) / 2) << 24); /* WL + BL/2 + tWR */
	reg |= GENMASK(21, 16) & (DIV_ROUND_UP_2EVAL(data->tFAW, 2) << 16);
	reg |= GENMASK(5, 0) & (data->tRAS / 2);
	reg |= GENMASK(14, 8) & (((data->tRASmax / 1024 - 1) / 2) << 8); /* (tRAS(max) / 1024) - 1) / 2 */
	BM_DDRC_WRITE(port, DDRC_DRAMTMG0, reg);

	reg  = 0;
	reg |= GENMASK(20, 16) & (DIV_ROUND_UP_2EVAL(data->tXP + data->PL, 2) << 16);
	reg |= GENMASK(13, 8) & ((MAX(data->AL + data->tRTP, data->RL + 4 - data->tRP) / 2) << 8);
	/* DDR4: Max of: (AL + tRTP) or (RL + BL/2 - tRP) */
	reg |= GENMASK(6, 0) & DIV_ROUND_UP_2EVAL(data->tRC, 2);
	BM_DDRC_WRITE(port, DDRC_DRAMTMG1, reg);

	reg  = 0;
	reg |= GENMASK(29, 24) & (DIV_ROUND_UP_2EVAL(data->WL + !!data->registered_dimm, 2) << 24);
	reg |= GENMASK(21, 16) & (DIV_ROUND_UP_2EVAL(data->RL + !!data->registered_dimm, 2) << 16);
	/*
	 * DDR4 multiPHY Utility Block (PUB) Databook:
	 * 4_ 9.4 Read Transactions, Table 4-23 Read Command Spacing:
	 * add extra Read System Latency in Read2Write delay
	 */
	rsl = (data->clock_mhz * 2 <= 1600) ? 4 : 6; /* experimental RSL = 4 or 6 clk */
	/* DDR4: RL + BL/2 + 1 + WR_PREAMBLE - WL */
	reg |= GENMASK(13, 8) & (DIV_ROUND_UP_2EVAL(data->RL + 4 + 1 + 1 - data->WL + rsl, 2) << 8);
	/* DDR4: CWL + PL + BL/2 + tWTR_L */
	reg |= GENMASK(5, 0) & DIV_ROUND_UP_2EVAL(data->CWL + data->PL + 4 + data->tWTR_L, 2);
	BM_DDRC_WRITE(port, DDRC_DRAMTMG2, reg);

	reg  = 0;
	/* tMRD. If C/A parity for DDR4 is used, set to tMRD_PAR(tMOD + PL) instead. */
	reg |= GENMASK(17, 12) & (DIV_ROUND_UP_2EVAL(data->tMOD + data->PL, 2) << 12);
	/* tMOD. If C/A parity for DDR4 is used, set to tMOD_PAR(tMOD + PL) instead. */
	reg |= GENMASK(9, 0) & DIV_ROUND_UP_2EVAL(data->tMOD + data->PL + !!data->registered_dimm, 2);
	BM_DDRC_WRITE(port, DDRC_DRAMTMG3, reg);

	reg  = 0;
	reg |= GENMASK(28, 24) & (((data->tRCD > data->AL) ? DIV_ROUND_UP_2EVAL(data->tRCD - data->AL, 2) : 1) << 24); /* tRCD - AL */
	reg |= GENMASK(19, 16) & (DIV_ROUND_UP_2EVAL(data->tCCD_L, 2) << 16); /* DDR4: tCCD_L */
	reg |= GENMASK(11, 8) & (DIV_ROUND_UP_2EVAL(data->tRRD_L, 2) << 8); /* DDR4: tRRD_L */
	reg |= GENMASK(4, 0) & (data->tRP / 2 + 1); /* Note: val=8, but S.Hudchenko example have val=9 */
	BM_DDRC_WRITE(port, DDRC_DRAMTMG4, reg);

	reg  = 0;
	reg |= GENMASK(27, 24) & (DIV_ROUND_UP_2EVAL(data->tCKSRX, 2) << 24); /* DDR4: tCKSRX */
	/* DDR4: max (10 ns, 5 tCK) (+ PL(parity latency)(*)) */
	reg |= GENMASK(19, 16) & (DIV_ROUND_UP_2EVAL(CLOCK_NS(10) + data->PL, 2) << 16);
	/* NOTICE: Only if CRCPARCTL1_ bit<12> = 0, this register should be increased by PL.
	 * Note: val=9, but S_ Hudchenko example have val=6
	 */
	/* DDR4: tCKE + 1 (+ PL(parity latency)(*)) */
	reg |= GENMASK(13, 8) & (DIV_ROUND_UP_2EVAL(data->tCKE + 1 + data->PL, 2) << 8);
	/* NOTICE: Only if CRCPARCTL1_ bit<12> = 0, this register should be increased by PL.
	 * Note: val=6, but S_Hudchenko example have val=4
	 */
	reg |= GENMASK(4, 0) & DIV_ROUND_UP_2EVAL(data->tCKE, 2);
	BM_DDRC_WRITE(port, DDRC_DRAMTMG5, reg);

	reg  = 0;
	/* Note: val=4, but S.Hudchenko example have val=5 */
	reg |= GENMASK(30, 24) & (DIV_ROUND_UP_2EVAL(DIV_ROUND_UP_2EVAL(data->tXS_FAST, 32), 2) << 24);
	/* Note: val=4, but S.Hudchenko example have val=5 */
	reg |= GENMASK(22, 16) & (DIV_ROUND_UP_2EVAL(DIV_ROUND_UP_2EVAL(data->tXS_ABORT, 32), 2) << 16);
	reg |= GENMASK(14, 8) & (DIV_ROUND_UP_2EVAL(DIV_ROUND_UP_2EVAL(data->tXSDLL, 32), 2) << 8);
	/* Note: val=7, but S.Hudchenko example have val=11 */
	reg |= GENMASK(6, 0) & DIV_ROUND_UP_2EVAL(DIV_ROUND_UP_2EVAL(data->tXS, 32), 2);
	BM_DDRC_WRITE(port, DDRC_DRAMTMG8, reg);

	reg  = 0;
	reg |= GENMASK(18, 16) & (DIV_ROUND_UP_2EVAL(data->tCCD_S, 2) << 16);
	reg |= GENMASK(11, 8) & (DIV_ROUND_UP_2EVAL(data->tRRD_S, 2) << 8);
	/* CWL + PL + BL/2 + tWTR_S */
	reg |= GENMASK(5, 0) & DIV_ROUND_UP_2EVAL(data->CWL + data->PL + 4 + data->tWTR_S, 2);
	BM_DDRC_WRITE(port, DDRC_DRAMTMG9, reg);

	reg = 0;
	/* Note: val=19, but S.Hudchenko example have val=21 */
	reg |= GENMASK(30, 24) & (DIV_ROUND_UP_2EVAL(DIV_ROUND_UP_2EVAL(data->tXMPDLL, 32), 2) << 24);
	/* Note: val=7, but S.Hudchenko example have val=9 */
	reg |= GENMASK(20, 16) & ((DIV_ROUND_UP_2EVAL(data->tMPX_LH, 2) + 1) << 16);
	/* Note: val=1, but S.Hudchenko example have val=2 */
	reg |= GENMASK(9, 8) & (DIV_ROUND_UP_2EVAL(data->tMPX_S, 2) << 8);
	reg |= GENMASK(4, 0) & DIV_ROUND_UP_2EVAL(data->tCKMPE, 2);
	BM_DDRC_WRITE(port, DDRC_DRAMTMG11, reg);

	reg = GENMASK(4, 0) & DIV_ROUND_UP_2EVAL(data->tMRD_PDA, 2);
	BM_DDRC_WRITE(port, DDRC_DRAMTMG12, reg);

	/* Set refresh period */
	reg  = 0;
	reg |= GENMASK(27, 16) & (((data->tREFI / 32) / 2) << 16);
	reg |= GENMASK(9, 0) & DIV_ROUND_UP_2EVAL(data->tRFC1, 2); /* tRFC1 for Refresh Mode normal(1x) */
	BM_DDRC_WRITE(port, DDRC_RFSHTMG, reg);

	/* Configure ZQCTL0 reg */
	BM_DDRC_WRITE(port, DDRC_ZQCTL0, 0x01000040);

	/* Configure ODTCFG reg */
	reg = 6 << 8; /* DDR4 BL8: 5 + RD_PREAMBLE */
	if (data->CL > data->CWL) {
		reg |= GENMASK(6, 2) & ((data->CL - data->CWL) << 2);
	}
	reg |= (6 + DDR_CRC_ENABLE) << 24;
	BM_DDRC_WRITE(port, DDRC_ODTCFG, reg);
	/* configure a ODTMAP reg. */
	/*
	 * Board design guide recommends 40 Ohm termination on
	 * the higher rank of the other DIMM. We presume that the rank
	 * makes no differrence (in case of dual rank DIMMS).
	 * Hence the setting should work in all possible cases.
	 */
	BM_DDRC_WRITE(port, DDRC_ODTMAP, 0x11114444);

	/* Configure ODTMAP reg */
	BM_DDRC_WRITE(port, DDRC_ODTMAP, 0x08040201);
	/* Disable DFI updates (for PHY trainings) */
	BM_DDRC_WRITE(port, DDRC_DFIUPD0, 0x80400005);
	/* Enable support for acknowledging PHY-initiated updates */
	BM_DDRC_WRITE(port, DDRC_DFIUPD2, 0x80000000);
}

static void baikal_ddrphy_set_registers(const unsigned int port,
					const struct ddr_configuration *data)
{
	uint32_t reg;

	/* Configure DCR reg */
	reg = 0x00000404;
	if (data->mirrored_dimm) {
		/* Address mirror */
		reg |= 1 << 27;
		reg |= 1 << 29;
	}
	if (data->registered_dimm) {
		reg |= 1 << 27;
	}
	if (data->bank_groups < 4) {
		reg |= 1 << 30; /* set BG1 signal unused */
	}
	if (data->dimms == 2) {
		reg |= 1 << 28; /* enable 2T mode */
	}
#ifdef BAIKAL_DDRCFG_IN_FLASH
	if (ddr_storage.flsh_1t2t &&
	    ddr_storage.ddr_sign == BAIKAL_FLASH_USE_STR) {
		reg |= 1 << 28; /* enable 2T mode */
	}
#endif
	BM_DDR_PUB_WRITE(port, DDR_PUB_DCR, reg);

	/* Set DX8 byte line usage (for ECC) */
	reg = 0x40000205;
	if (!data->ecc_on) {
		reg &= ~(1 << 0);
	}
	BM_DDR_PUB_WRITE(port, DDR_PUB_DX8GCR0, reg);

	/* Set refresh period */
	reg  = 0x00f00000;
	reg |= GENMASK(17, 0) & (data->tREFI - data->tRFC1 - 400);
	if (data->registered_dimm) {
		reg |= 1 << 29;
	}
	BM_DDR_PUB_WRITE(port, DDR_PUB_PGCR2, reg);

	/* Set user BIST pattern */
	BM_DDR_PUB_WRITE(port, DDR_PUB_PGCR4, 0x40800000);
	/* Set VT compensation settings */
	BM_DDR_PUB_WRITE(port, DDR_PUB_PGCR6, 0x00013000);

	/* Set PLL referece freq range */
	reg = 0x00038000;
	if (!(data->clock_mhz / 2 > 465)) {
		reg |= 0x1 << 19; /* PHY ref clock 225..490 MHz */
	}
	BM_DDR_PUB_WRITE(port, DDR_PUB_PLLCR, reg);

	/* Program PHY timings */
	/* PLL Reset, 9 usec + 16 clocks; in PHY clocks = DRAM clocks / 2 */
	reg  = GENMASK(12, 0) & (CLOCK_NS(9000) / 2 + 16);
	/* PLL Lock, 26 usec (see tLOCK in PHY Databook); in PHY clocks = DRAM clocks / 2 */
	reg |= GENMASK(31, 16) & ((CLOCK_NS(26000) / 2) << 16);
	BM_DDR_PUB_WRITE(port, DDR_PUB_PTR1, reg);

	/* DRAM initialization, CKE low time with power and clock stable (500 us), in DRAM clock; */
	reg  = GENMASK(19, 0) & CLOCK_NS(500000);
	/* DRAM initialization, CKE high time to first command (tRFC + 10 ns); */
	reg |= (data->tRFC1 + CLOCK_NS(10)) << 20;
	BM_DDR_PUB_WRITE(port, DDR_PUB_PTR3, reg);

	/* Affected: Fine Granularity Refresh Mode normal(1x) */
	/* DRAM initialization, Reset low time (200 us on power-up or 100 ns after power-up) */
	reg  = 0x10000000;
	reg |= GENMASK(17, 0) & CLOCK_NS(200000);
	BM_DDR_PUB_WRITE(port, DDR_PUB_PTR4, reg);

	if (data->registered_dimm) {
		/* RC03 Command_Address and CS Driver Control */
		uint8_t ca_drv = (data->registered_ca_stren >> 4) & 0x3;
		uint8_t cs_drv = (data->registered_ca_stren >> 6) & 0x3;
		/* RC04 ODT and CKE Driver Control */
		uint8_t cke_drv = (data->registered_ca_stren) & 0x3;
		uint8_t odt_drv = (data->registered_ca_stren >> 2) & 0x3;
		/* RC05 Clock Driver Control */
		uint8_t y0y2_drv = (data->registered_clk_stren) & 0x3;
		uint8_t y1y3_drv = (data->registered_clk_stren >> 2) & 0x3;

		uint32_t rdimmgcr2 = 0x0000a438;

		reg  = 0x0c000001;
		reg |= GENMASK(15, 12) & (((cs_drv << 2) | ca_drv) << 12);
		reg |= GENMASK(19, 16) & (((cke_drv << 2) | odt_drv) << 16);
		reg |= GENMASK(23, 20) & (((y0y2_drv << 2) | y1y3_drv) << 20);
		BM_DDR_PUB_WRITE(port, DDR_PUB_RDIMMGCR0, reg);

		reg  =  GENMASK(13, 0) & CLOCK_NS(5000);
		reg &= ~GENMASK(11, 8);

		switch (data->clock_mhz * 2) {
		/* RC10: RDIMM Operating Speed Control Word (CR1) */
		/* RC3x: Fine Granularity Operating Speed Control (CR2) */
		case 1600:
			rdimmgcr2 |= GENMASK(23, 16) & 0x11 << 16;
			break;
		case 1866:
			reg |= GENMASK(11, 8) & (1 << 8);
			rdimmgcr2 |= GENMASK(23, 16) & 0x1f << 16;
			break;
		case 2132:
			reg |= GENMASK(11, 8) & (2 << 8);
			rdimmgcr2 |= GENMASK(23, 16) & 0x2c << 16;
			break;
		case 2400:
			reg |= GENMASK(11, 8) & (3 << 8);
			rdimmgcr2 |= GENMASK(23, 16) & 0x39 << 16;
			break;
		case 2666:
			reg |= GENMASK(11, 8) & (4 << 8);
			rdimmgcr2 |= GENMASK(23, 16) & 0x47 << 16;
			break;
		default:
			break;
		}

		if (data->mirrored_dimm) {
			reg |= GENMASK(23, 20) & 0xc << 8;
		} else {
			reg |= GENMASK(23, 20) & 0x4 << 8;
		}

		BM_DDR_PUB_WRITE(port, DDR_PUB_RDIMMGCR1, reg);
		BM_DDR_PUB_WRITE(port, DDR_PUB_RDIMMGCR2, rdimmgcr2);
	}

	/* Program DRAM timings */
	reg  = GENMASK(3, 0) & data->tRTP; /* READtoPRE */
	reg |= GENMASK(14, 8) & (data->tRP << 8); /* PREperiod */
	reg |= GENMASK(22, 16) & (data->tRAS << 16); /* ACTtoPRE(min) */
	reg |= GENMASK(29, 24) & (data->tRRD_L << 24); /* ACTtoACT */
	BM_DDR_PUB_WRITE(port, DDR_PUB_DTPR0, reg);

	reg  = 0x28000008;
	reg |= GENMASK(23, 16) & (data->tFAW << 16);
	BM_DDR_PUB_WRITE(port, DDR_PUB_DTPR1, reg);

	reg  = GENMASK(9, 0) & data->tXS;
	reg |= GENMASK(19, 16) & (data->tCKE << 16); /* CKE width */
	BM_DDR_PUB_WRITE(port, DDR_PUB_DTPR2, reg);

	reg  = 0x00000101;
	reg |= GENMASK(25, 16) & ((data->tDLLK - 128) << 16); /* DLL lock */
	BM_DDR_PUB_WRITE(port, DDR_PUB_DTPR3, reg);

	reg  = 0x00000900;
	reg |= GENMASK(4, 0) & data->tXP; /* PDexit */
	/* REFCMDperiod; Affected: Fine Granularity Refresh Mode normal(1x) */
	reg |= GENMASK(25, 16) & (data->tRFC1 << 16);
	BM_DDR_PUB_WRITE(port, DDR_PUB_DTPR4, reg);

	reg  = GENMASK(4, 0) & data->tWTR_L; /* WRITEtoREAD */
	reg |= GENMASK(14, 8) & (data->tRCD << 8); /* ACTtoREAD/WRITE */
	reg |= GENMASK(23, 16) & (data->tRC << 16); /* ACTtoACT */
	BM_DDR_PUB_WRITE(port, DDR_PUB_DTPR5, reg);

	/* Program DRAM mode registers */
	reg = set_mr0(data->CL, data->tWR, data->tRTP);
	BM_DDR_PUB_WRITE(port, DDR_PUB_MR0, reg);

	/* DLL on; DIC RZQ/7 (33 ohm); AL=0; WL off; ODT RTT_NOM off; TDQS off; Qoff normal */
	reg = 0x1 | (data->DIC & 0x1) << 1 | (data->RTT_NOM & 0x7) << 8;
	if (data->AL)
		reg |= (data->CL - data->AL) << 3;
	BM_DDR_PUB_WRITE(port, DDR_PUB_MR1, reg);

	reg = set_mr2(data->CWL, data->RTT_WR);
	BM_DDR_PUB_WRITE(port, DDR_PUB_MR2, reg);

	/*
	 * MPR read format serial; Fine Granularity Refresh Mode normal(1x); Temperature Sensor Readout off;
	 * Per DRAM Addressability off; Geardown Mode 1/2; MPR operation normal; MPR Page 0;
	 * Set Write Command Latency
	 */
	reg = ((uint8_t)data->WCL - 4) << 9;
	BM_DDR_PUB_WRITE(port, DDR_PUB_MR3, reg);

	/*
	 * Write Preamble 1clk; Read Preable 1 clk; Read Preable Training off; Self Refresh Abort off;
	 * Command Address Latency off; Vref Monitor off; Temp Controlled Refresh off;
	 * Max Power Down mode off
	 */
	BM_DDR_PUB_WRITE(port, DDR_PUB_MR4, 0);

	reg = set_mr5(data->PL, data->RTT_PARK);
	BM_DDR_PUB_WRITE(port, DDR_PUB_MR5, reg);
#if defined(BAIKAL_DUAL_CHANNEL_MODE) || defined(BAIKAL_DDRCFG_IN_FLASH)
	if ((data->dimms == 2)
#ifdef BAIKAL_DDRCFG_IN_FLASH
		|| (ddr_storage.ddr_sign == BAIKAL_FLASH_USE_STR &&
		    ddr_storage.flsh_vref_use)
#endif
	) {
		reg = (data->tCCD_L - 4) << 10 | data->PHY_DRAM_VREF;
	} else {
		reg = (data->tCCD_L - 4) << 10;
	}
#else
	reg = (data->tCCD_L - 4) << 10;
#endif
	BM_DDR_PUB_WRITE(port, DDR_PUB_MR6, reg);

	/* Data training configuration */
	reg  = 0x0000b087;
	reg |= 1 << 6; /* use MPR for DQS training (DDR4) */
	reg |= 1 << 28; /* set refresh burst count to 1 in PUB mode */
	if (data->registered_dimm) {
		reg &= ~GENMASK(15, 14);
	}
	BM_DDR_PUB_WRITE(port, DDR_PUB_DTCR0, reg);

	reg  = 0x00000237;
	reg |= ((1 << data->ranks) - 1) << 16;
	if (data->dimms == 2) {
		/* We have two CS signals (two ranks) per DIMM slot on DBM board */
		reg &= ~GENMASK(31, 16);
		if (data->ranks == 1) {
			reg |= 0x5 << 16;
		} else {
			reg |= 0xf << 16;
		}
	}
	BM_DDR_PUB_WRITE(port, DDR_PUB_DTCR1, reg);

	BM_DDR_PUB_WRITE(port, DDR_PUB_DSGCR, 0x0064443a);

	if (data->clock_mhz / 2 > 467) {
		BM_DDR_PUB_WRITE(port, DDR_PUB_ZQCR, 0x04058f00);
	} else {
		BM_DDR_PUB_WRITE(port, DDR_PUB_ZQCR, 0x04058d00);
	}

	reg = 0x0007bb00 | (data->PHY_ODT << 4) | data->PHY_ODI_PU;
	BM_DDR_PUB_WRITE(port, DDR_PUB_ZQ0PR, reg);
	BM_DDR_PUB_WRITE(port, DDR_PUB_ZQ1PR, reg);

	if (data->dbus_half) {
		reg  = BM_DDR_PUB_READ(port, DDR_PUB_DX4GCR0);
		reg &= ~0x1U;
		BM_DDR_PUB_WRITE(port, DDR_PUB_DX4GCR0, reg);
		BM_DDR_PUB_WRITE(port, DDR_PUB_DX5GCR0, reg);
		BM_DDR_PUB_WRITE(port, DDR_PUB_DX6GCR0, reg);
		BM_DDR_PUB_WRITE(port, DDR_PUB_DX7GCR0, reg);
	}
}

static void dcu_load_mode_command(const int port, const int mr_num, const int mr_value)
{
	uint64_t cmu_cmd; /* supposed content of cache */
	uint32_t reg;
	uint64_t timeout;
	uint32_t tmrd = BM_DDR_PUB_READ(port, DDR_PUB_DTPR1) & 0x1f;

	cmu_cmd = ((uint64_t)tmrd << DCU_DFIELD_DTP) |
		  (1ULL << DCU_DFIELD_TAG) |
		  (1ULL << DCU_DFIELD_CMD) |
		  (mr_num << DCU_DFIELD_BANK) |
		  (mr_value << DCU_DFIELD_ADDR);

	/* Initiate write to cache by writing to DCUAR & DCUDR */
	BM_DDR_PUB_WRITE(port, DDR_PUB_DCUAR, 0);
	BM_DDR_PUB_WRITE(port, DDR_PUB_DCUDR, (uint32_t)cmu_cmd);
	/* Manually increment cache slice address */
	BM_DDR_PUB_WRITE(port, DDR_PUB_DCUAR, 1 << 4);
	BM_DDR_PUB_WRITE(port, DDR_PUB_DCUDR, (uint32_t)(cmu_cmd >> 32));
	/* Run the first command in the cache */
	BM_DDR_PUB_WRITE(port, DDR_PUB_DCURR, 1 << 0);

	for (timeout = timeout_init_us(10000);;) {
		reg = BM_DDR_PUB_READ(port, DDR_PUB_DCUSR0);
		if (reg & 0x1) {
			break;
		} else if (timeout_elapsed(timeout)) {
			ERROR("%s: failed to load mode register, 0x%x\n", __func__, reg);
			break;
		}
	}
}
#if defined(BAIKAL_DUAL_CHANNEL_MODE) || defined(BAIKAL_DDRCFG_IN_FLASH)
static void run_dram_vref_training(const int port)
{
	uint32_t reg = BM_DDR_PUB_READ(port, DDR_PUB_MR6);

	/* Run vrefdq training */
	dcu_load_mode_command(port, 6, reg | (1 << 7));
	/* Disable vrefdq training */
	dcu_load_mode_command(port, 6, reg);
}
#endif
static int baikal_ddrphy_pir_training(const unsigned int port, const uint32_t mode)
{
	uint32_t reg;
	int ret = 0;
	uint64_t timeout;

	/*
	 * Trigger PHY initialization
	 *
	 * DDR PHY DataBook, p.84, PIR[0]:
	 * It is recommended that this bit be set 1b1 in a separate config write step
	 * after other bits in this register have been programmed to avoid any race condition.
	 */
	BM_DDR_PUB_WRITE(port, DDR_PUB_PIR, mode & ~0x1UL);
	BM_DDR_PUB_WRITE(port, DDR_PUB_PIR, mode |  0x1);

	/*
	 * Wait for DDR PHY initialization done (IDONE)
	 *
	 * DDR PHY DataBook, p.84:
	 * Note that PGSR0[IDONE] is not cleared immediately after PIR[INIT] is set,
	 * and therefore software must wait a minimum of 10 configuration clock cycles
	 * from when PIR[INIT] is set to when it starts polling for PGSR0[IDONE]
	 */
	ndelay(40);

	for (timeout = timeout_init_us(10000);;) {
		reg = BM_DDR_PUB_READ(port, DDR_PUB_PGSR0);
		if (reg & 0x1) {
			if (reg & 0x0ff80000) {
				ret = reg;
				ERROR("%s: completed with errors, 0x%x (0x%08x)\n",
				      __func__, ret, BM_DDR_PUB_READ(port, DDR_PUB_PGSR1));
			}
			break;
		} else if (timeout_elapsed(timeout)) {
			ret = reg;
			ERROR("%s: failed, 0x%x\n", __func__, ret);
			break;
		}
	}

	udelay(1);

	return ret;
}

static int baikal_ddrphy_vref_training(const unsigned int port, const unsigned int clock_mhz)
{
	uint32_t dtcr0_stored;
	uint32_t reg;
	int ret;

	reg  = BM_DDR_PUB_READ(port, DDR_PUB_VTCR0);
	reg &= ~GENMASK(31, 29);
	reg |=	GENMASK(31, 29) & ((3 * clock_mhz / 640) << 29); /* Number of ctl_clk supposed to be (> 150ns) during DRAM VREF training */
	reg &= ~GENMASK(5, 0);
	reg |= 0x8; /* DRAM Vref initial value (this one is for DDR0 channel speedbin DDR4-2133) */
#if defined(BAIKAL_DUAL_CHANNEL_MODE) || defined(BAIKAL_DDRCFG_IN_FLASH)
	uint32_t dram_vref_val = BM_DDR_PUB_READ(port, DDR_PUB_MR6) & 0x3f;

	if (dram_vref_val) {
		reg &= ~GENMASK(5, 0);
		reg |= dram_vref_val;
	}
#endif
	reg |= 1 << 27; /* DRAM Vref PDA mode */
	BM_DDR_PUB_WRITE(port, DDR_PUB_VTCR0, reg);

	/* D4MV I/Os with PVREF_DAC settings (PUB DataBook p.353) */
	reg  = BM_DDR_PUB_READ(port, DDR_PUB_VTCR1);
	reg &= ~GENMASK(7, 5);
	reg |=	GENMASK(7, 5) & ((clock_mhz / 160) << 5); /* Number of ctl_clk supposed to be (> 200ns) during Host IO VREF training */
	reg &= ~(0x1U << 8);
	BM_DDR_PUB_WRITE(port, DDR_PUB_VTCR1, reg);

	/* 4.4.8.6 Steps to Run VREF Training (PUB DataBook p.363) */
	reg  = dtcr0_stored = BM_DDR_PUB_READ(port, DDR_PUB_DTCR0);
	reg &= ~GENMASK(31, 28); /* Disable refresh during training */
	BM_DDR_PUB_WRITE(port, DDR_PUB_DTCR0, reg);

	/* Program number of LCDL eye points for which VREF training is repeated (ENUM) by writing VTCR1[2] */
	reg = BM_DDR_PUB_READ(port, DDR_PUB_VTCR1);
	reg |= 1 << 2; /* Allow to program number of LCDL eye points for which VREF training is repeated */
	reg &= ~GENMASK(31, 28);
	reg |= 0x1 << 28; /* Program HOST VREF step size used during VREF training (HVSS) VTCR1[31:28] = 0x1 */
	reg |= 0xf << 12; /* Program VREF word count (VWCR) VTCR1[15:12] = 0xf */
	BM_DDR_PUB_WRITE(port, DDR_PUB_VTCR1, reg);

	/* Run vref training itself */
	ret = baikal_ddrphy_pir_training(port, DDR_PUB_PIR_VREF);

	/* Restore DTCR0 value */
	BM_DDR_PUB_WRITE(port, DDR_PUB_DTCR0, dtcr0_stored);

	return ret;
}

static int baikal_ddrphy_dram_init(const unsigned int port, const unsigned int clock_mhz)
{
	uint32_t reg;
	int ret;
	uint64_t timeout;

	/* Disable PHY IO ODT and DQS PU/PD for normal operation */
	BM_DDR_PUB_WRITE(port, DDR_PUB_DXCCR, 0x20401004);

	/* Disable DDR Controller auto-refreshes during trainings */
	BM_DDRC_WRITE(port, DDRC_RFSHCTL3, 0x1);
	ndelay(40);
	if (BM_DDRC_READ(port, DDRC_RFSHCTL3) != 0x1) {
		return 1;
	}

	if (BM_DDR_PUB_READ(port, DDR_PUB_RDIMMGCR0) & 0x1) {
		ret = baikal_ddrphy_pir_training(port, DDR_PUB_PIR_DRAM_STEP0 | DDR_PUB_PIR_RDIMM);
	} else {
		ret = baikal_ddrphy_pir_training(port, DDR_PUB_PIR_DRAM_STEP0);
	}

	if (ret) {
		return ret;
	}
#if defined(BAIKAL_DUAL_CHANNEL_MODE) || defined(BAIKAL_DDRCFG_IN_FLASH)
	uint32_t dram_vref_val = BM_DDR_PUB_READ(port, DDR_PUB_MR6) & 0x3f;

	if (dram_vref_val) {
		run_dram_vref_training(port);
	}
#endif
	/*
	 * BE-M1000 PG. 7.2 DDR PHY:
	 * MR2.RTTWR must be zeroed during DDR4 write leveling training.
	 */
	reg  = BM_DDR_PUB_READ(port, DDR_PUB_MR2);
	reg &= 0xffff;
	if (reg & GENMASK(11, 9)) {
		dcu_load_mode_command(port, 2, reg & ~GENMASK(11, 9));
	}
	ret = baikal_ddrphy_pir_training(port, DDR_PUB_PIR_WL);
	if (reg & GENMASK(11, 9)) {
		/* Restore initial value in MR2 */
		dcu_load_mode_command(port, 2, reg);
	}
	if (ret) {
		return ret;
	}

	/* Disable QS counter during DQS Gate training */
	BM_DDR_PUB_WRITE(port, DDR_PUB_DXCCR, BM_DDR_PUB_READ(port, DDR_PUB_DXCCR) & ~(1 << 22));
	ret = baikal_ddrphy_pir_training(port, DDR_PUB_PIR_DQSGATE);
	BM_DDR_PUB_WRITE(port, DDR_PUB_DXCCR, BM_DDR_PUB_READ(port, DDR_PUB_DXCCR) | (1 << 22));
	if (ret) {
		return ret;
	}

	ret = baikal_ddrphy_pir_training(port, DDR_PUB_PIR_DRAM_STEP2);
	if (ret) {
		return ret;
	}

	ret = baikal_ddrphy_vref_training(port, clock_mhz);
	if (ret) {
		return ret;
	}

	/* Enable DDR Controller auto-refreshes after training */
	BM_DDRC_WRITE(port, DDRC_RFSHCTL3, 0);
	ndelay(40);
	if (BM_DDRC_READ(port, DDRC_RFSHCTL3) != 0) {
		return 2;
	}

	/* Enable DFI update request */
	reg  = BM_DDR_PUB_READ(port, DDR_PUB_DSGCR);
	reg |= DDR_PUB_DSGCR_PUREN;
	BM_DDR_PUB_WRITE(port, DDR_PUB_DSGCR, reg);

	/* Enable AXI port */
	BM_DDRC_WRITE(port, DDRC_PCTRL(0), 0x1);
	ndelay(40);
	if (BM_DDRC_READ(port, DDRC_PCTRL(0)) != 0x1) {
		return 3;
	}

	for (timeout = timeout_init_us(10000);;) {
		reg = BM_DDRC_READ(port, DDRC_STAT);
		if ((reg & 0x3) == 0x1) {
			break;
		} else if (timeout_elapsed(timeout)) {
			ret = ~reg;
			ERROR("%s: failed, 0x%x\n", __func__, ret);
			break;
		}
	}

	ndelay(40);

	return ret;
}

int ddr_init(const int port, const bool dual_mode, struct ddr_configuration *info)
{
<<<<<<< HEAD
	int err;
	uint32_t reg;
=======
	int ret = 0;
	unsigned int i;
>>>>>>> 45d03642d (BM1000: Merge DDR driver with Elpitech version)

	ddr_lcru_clkch_rst(port, LCRU_DDR_CMU_CLKCH0_ACLK, 1);
	ddr_lcru_clkch_rst(port, LCRU_DDR_CMU_CLKCH1_CORE, 1);

	if (dual_mode) {
		ddr_lcru_dual_mode(port, DDR_DUAL_MODE);
	}

	ndelay(40);

	baikal_ddrc_set_registers(port, info);
	ndelay(40);

	/* Clear LCU reset. (DDR CTRL & DDR AXI clocks on) */
	ddr_lcru_clkch_rst(port, LCRU_DDR_CMU_CLKCH0_ACLK, 0);
	ddr_lcru_clkch_rst(port, LCRU_DDR_CMU_CLKCH1_CORE, 0);
	ndelay(40);

	baikal_ddrphy_set_registers(port, info);
<<<<<<< HEAD
=======

	/* set ODTCRn according to ODTMAP */
	for (i = 0; i < 4; i++) {
		BM_DDR_PUB_WRITE(port, DDR_PUB_RANKIDR, i);
		if (i < 2)
			BM_DDR_PUB_WRITE(port, DDR_PUB_ODTCR, 0x00040004);
		else
			BM_DDR_PUB_WRITE(port, DDR_PUB_ODTCR, 0x00010001);
	}

>>>>>>> 45d03642d (BM1000: Merge DDR driver with Elpitech version)
	if (!info->ecc_on) {
		/* Disable unused 9-th data byte */
		reg  = BM_DDR_PUB_READ(port, DDR_PUB_DX8GCR0);
		reg &= ~(1 << 0);
		BM_DDR_PUB_WRITE(port, DDR_PUB_DX8GCR0, reg);
	}

	err = baikal_ddrphy_pir_training(port, DDR_PUB_PIR_PHY_INIT);
	if (err) {
		return err;
	}
#if defined(BAIKAL_DUAL_CHANNEL_MODE) || defined(BAIKAL_DDRCFG_IN_FLASH)
	if (info->dimms == 2
#ifdef BAIKAL_DDRCFG_IN_FLASH
		|| (ddr_storage.ddr_sign == BAIKAL_FLASH_USE_STR &&
		    ddr_storage.flsh_vref_use)
#endif
	) {
<<<<<<< HEAD
		unsigned int i;

		/* Read internal Vref values */
		reg = (info->PHY_HOST_VREF << 24) |
		      (info->PHY_HOST_VREF << 16) |
		      (info->PHY_HOST_VREF <<  8) |
		      (info->PHY_HOST_VREF);

		/* Set internal Vref values */
		for (i = 0; i <= 8; i++) {
			/* All registers lie at constant offsets from one another */
			BM_DDR_PUB_WRITE(port,
					 DDR_PUB_DX0GCR5 +
					 (DDR_PUB_DX1GCR5 - DDR_PUB_DX0GCR5) * i,
					 reg);
=======
		/* set internal Vref values */
		uint32_t DXnGCR5_val = (info->PHY_HOST_VREF << 24)
					| (info->PHY_HOST_VREF << 16)
					| (info->PHY_HOST_VREF << 8)
					| (info->PHY_HOST_VREF);
		for (i = 0; i <= 8; i++) {
			/* all registers lie at constant offsets from one another: */
			BM_DDR_PUB_WRITE(port, DDR_PUB_DX0GCR5 +
					(DDR_PUB_DX1GCR5 - DDR_PUB_DX0GCR5) * i,
						 DXnGCR5_val);
>>>>>>> 45d03642d (BM1000: Merge DDR driver with Elpitech version)
		}
	}
#endif
	return baikal_ddrphy_dram_init(port, info->clock_mhz);
}

int ddr_init_ecc_memory(const int port)
{
	uint32_t reg;
	uint64_t timeout;

	/* Disable AXI port */
	BM_DDRC_WRITE(port, DDRC_PCTRL(0), 0);

	/* Zero scrub_interval value and set scrub_mode to perform writes */
	reg  = BM_DDRC_READ(port, DDRC_SBRCTL);
	reg &= ~GENMASK(20, 8);
	reg |= 1 << 2;
	BM_DDRC_WRITE(port, DDRC_SBRCTL, reg);

	BM_DDRC_WRITE(port, DDRC_SBRWDATA0, 0);
	BM_DDRC_WRITE(port, DDRC_SBRWDATA1, 0);

	/* Enable scrub in a separate step to avoid race conditions */
	BM_DDRC_WRITE(port, DDRC_SBRCTL, reg | 0x1);

	/* It may take up to 3 seconds to init 32 GiB of memory */
	for (timeout = timeout_init_us(3000000);;) {
		reg = BM_DDRC_READ(port, DDRC_SBRSTAT);
		if (reg & 0x2) {
			break;
		} else if (timeout_elapsed(timeout)) {
			ERROR("%s: failed to init memory (undone)\n", __func__);
			goto exit;
		}
	}

	for (timeout = timeout_init_us(10000);;) {
		reg = BM_DDRC_READ(port, DDRC_SBRSTAT);
		if (!(reg & 0x1)) {
			break;
		} else if (timeout_elapsed(timeout)) {
			ERROR("%s: failed to init memory (busy)\n", __func__);
			goto exit;
		}
	}

exit:
	/* Set maximum scrub_interval value and set scrub_mode to perform reads */
	reg  = BM_DDRC_READ(port, DDRC_SBRCTL);
	reg |= 0x1fff << 8;
	reg &= ~0x5;
	BM_DDRC_WRITE(port, DDRC_SBRCTL, reg);

	/* Enable scrub in a separate step to avoid race conditions */
	BM_DDRC_WRITE(port, DDRC_SBRCTL, reg | 0x1);
	/* Enable AXI port */
	BM_DDRC_WRITE(port, DDRC_PCTRL(0), 0x1);

	return 0;
}
