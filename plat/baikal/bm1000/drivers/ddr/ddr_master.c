/*
 * Copyright (c) 2021-2022, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include <common/debug.h>

#include <baikal_def.h>

#include "ddr_baikal.h"
#include "ddr_master.h"
#include "ddr_spd.h"

#define DDR_CRC_ENABLE 0

#define CLOCK_PS(x)	\
	(uint32_t)((((uint64_t)(x) * 1000) / data->tCK + 974) / 1000)
#define CLOCK_NS(x)	CLOCK_PS((uint64_t)(x) * 1000)

#define SPD_TO_PS(mtb, ftb)	((mtb) * 125 + (ftb))

/* BL=8; RD sequential; DLL reset off; set CAS Latency; set Write Recovery; set Read To Precharge */
static inline uint16_t set_mr0(const uint32_t cl, uint32_t wr, const uint32_t rtp)
{
	const uint8_t cl_table[] = {0xd, 0x8, 0xe, 0x9, 0xf, 0xa, 0xc, 0xb};
	uint16_t mr0_reg = 0;
	uint16_t tmp = 0;

	/* The WR value in the mode register must be programmed to be equal or larger than WRmin */
	wr = (wr + 1) & ~0x1U;

	if (rtp <= 10) {
		tmp = rtp - 5;
	} else if (rtp == 11) {
		tmp = 0x7;
	} else if (rtp == 12) {
		tmp = 0x6;
	} else {
		tmp = 0x8;
	}

	mr0_reg |= ((tmp & 0x7) << 9) |
		   ((tmp & 0x8) << 10);

	if (cl <= 16) {
		tmp = cl - 9;
	} else if (cl <= 24) {
		tmp = cl_table[cl - 17];
	} else if (cl <= 32) {
		tmp = cl - 9;
	} else {
		tmp = 0x17;
	}

	mr0_reg |= ((tmp & 0x1) << 2) |
		   ((tmp & 0xe) << 3) |
		   ((tmp & 0x10) << 8);

	return mr0_reg;
}

/*
 * CRC on; ODTwr off; LP ASR manual, normal temperature; set CAS Write Latency
 * Attention: Write Leveling Training require to ODTwr = off
 */
static inline uint16_t set_mr2(const uint32_t cwl, const uint32_t rtt_wr)
{
	uint16_t tmp = (DDR_CRC_ENABLE & 0x1) << 12;

	if (cwl <= 12) {
		tmp |= ((0xff & cwl) - 9) << 3;
	} else {
		tmp |= ((0xff & cwl) / 2 - 3) << 3;
	}

	return tmp | ((rtt_wr & 0x7) << 9);
}

/*
 * Read DBI off; Write DBI off; DM on; CA Parity Persistent Error off; RTT Park off;
 * ODT input buffer PD on; CA Parity Error clear; CRC Error clear; set CA Parity Latency Mode
 */
static inline uint16_t set_mr5(const uint32_t pl, const uint32_t rtt_park)
{
	uint16_t tmp = 1 << 10; /* DM on */

	if (pl) {
		tmp |= pl - 3;
	}

	return tmp | (rtt_park << 6);
}

void ddrc_set_default_vals(struct ctl_content *ddrc)
{
	ddrc->MSTR_	  = 0x4f040010;
	ddrc->RFSHTMG_	  = 0x0092014a;
	ddrc->ECCCFG0_	  = 0x00000000;
	ddrc->ECCCFG1_	  = 0x0;
	ddrc->CRCPARCTL0_ = 0x00000001;
	ddrc->CRCPARCTL1_ = 0x00000091;
	ddrc->INIT0_	  = 0x4002004e;
	ddrc->DIMMCTL_	  = 0x00000003;
	ddrc->RANKCTL_	  = 0x0000033f;
	ddrc->DRAMTMG0_	  = 0x1f0b2813;
	ddrc->DRAMTMG1_	  = 0x00070c1c;
	ddrc->DRAMTMG2_	  = 0x1212031b;
	ddrc->DRAMTMG3_	  = 0x0000f00f;
	ddrc->DRAMTMG4_	  = 0x01030309;
	ddrc->DRAMTMG5_	  = 0x06060403;
	ddrc->DRAMTMG8_	  = 0x05050c0b;
	ddrc->DRAMTMG9_	  = 0x00020418;
	ddrc->DRAMTMG11_  = 0x1509020e;
	ddrc->DRAMTMG12_  = 0x00000008;
	ddrc->ZQCTL0_	  = 0x01000040;
	ddrc->DFITMG0_	  = 0x04a08222;
	ddrc->DFITMG1_	  = 0x00080404;
	ddrc->DFIUPD0_	  = 0x80400005;
	ddrc->DFIUPD2_	  = 0x80000000;
	ddrc->DBICTL_	  = 0x0001;
	ddrc->ADDRMAP1_	  = 0x003f0909;
	ddrc->ADDRMAP2_	  = 0x01010100;
	ddrc->ADDRMAP3_	  = 0x01010101;
	ddrc->ADDRMAP4_	  = 0x00001f1f;
	ddrc->ADDRMAP5_	  = 0x08080808;
	ddrc->ADDRMAP6_	  = 0x08080808;
	ddrc->ADDRMAP7_	  = 0x00000f0f;
	ddrc->ADDRMAP8_	  = 0x00000a01;
	ddrc->ODTCFG_	  = 0x07000600;
	ddrc->ODTMAP_	  = 0x08040201;
	ddrc->SCHED_	  = 0xd00;
	ddrc->SCHED1_	  = 0x40;
	ddrc->DQMAP0_	  = 0x0;
	ddrc->DQMAP1_	  = 0x0;
	ddrc->DQMAP2_	  = 0x0;
	ddrc->DQMAP3_	  = 0x0;
	ddrc->DQMAP4_	  = 0x0;
	ddrc->DQMAP5_	  = 0x1;
	ddrc->PCCFG_	  = 0x0;
	ddrc->SARBASE0_	  = 4;
	ddrc->SARSIZE0_	  = 3;
	ddrc->SARBASE1_	  = 68;
	ddrc->SARSIZE1_	  = 59;
	ddrc->SARBASE2_	  = 1088;
	ddrc->SARSIZE2_	  = 959;
}

void phy_set_default_vals(struct phy_content *phy)
{
	phy->PGCR1_	= 0x020046a0;
	phy->PGCR2_	= 0x40f14780;
	phy->PGCR4_	= 0x40800000;
	phy->PGCR6_	= 0x00013000;
	phy->PTR1_	= 0x3cd2152b;
	phy->PTR3_	= 0x2a1927c0;
	phy->PTR4_	= 0x1003a980;
	phy->PLLCR_	= 0x00038000;
	phy->DCR_	= 0x28000404;
	phy->DTPR0_	= 0x06271009;
	phy->DTPR1_	= 0x281a0008;
	phy->DTPR2_	= 0x000602a1;
	phy->DTPR3_	= 0x02800101;
	phy->DTPR4_	= 0x02950908;
	phy->DTPR5_	= 0x00371009;
	phy->ZQCR_	= 0x04058d00;
	phy->DX8GCR0_	= 0x40000205;
	phy->DSGCR_	= 0x0064443a;
	phy->MR0_	= 0x00000a52;
	phy->MR1_	= 0x00000000;
	phy->MR2_	= 0x00000000;
	phy->MR3_	= 0x00000000;
	phy->MR4_	= 0x00000000;
	phy->MR5_	= 0x00000400;
	phy->MR6_	= 0x00000400;
	phy->RDIMMGCR0_	= 0x3c410000;
	phy->RDIMMGCR1_	= 0x00000c80;
	phy->RDIMMGCR2_	= 0x03ffffbf;
	phy->RDIMMCR0_	= 0x00000000;
	phy->RDIMMCR1_	= 0x00000000;
	phy->RDIMMCR2_	= 0x00000000;
	phy->DTCR0_	= 0x8000b087;
	phy->DTCR1_	= 0x000f0237;
}

int ddrc_config_content(struct ctl_content *ddrc,
			const struct ddr_configuration *data)
{
	uint32_t hif_addr;
	uint32_t rsl;

	if (data->single_ddr) {
		/* for single DDR Controller in SoC we have normal address regions */
		ddrc->SARBASE0_ *= 2; /* region address 2..4 GiB (UMCTL2_SARMINSIZE=256MB) */
		ddrc->SARSIZE0_ = (ddrc->SARSIZE0_ + 1) * 2 - 1; /* region size 2 GiB */
		ddrc->SARBASE1_ *= 2; /* region address 34..64 GiB */
		ddrc->SARSIZE1_ = (ddrc->SARSIZE1_ + 1) * 2 - 1; /* region size 30 GiB */
		ddrc->SARBASE2_ *= 2; /* region address 544..1024 GiB */
		ddrc->SARSIZE2_ = (ddrc->SARSIZE2_ + 1) * 2 - 1; /* region size 240 GiB */
	}

	/*
	 * We have two CS signals (two ranks) per DIMM slot on DBM board.
	 * Valid values for Mstr.active_ranks are: 0x1, 0x11, 0x1111
	 */
	if (data->dimms == 1) {
		ddrc->MSTR_ &= ~GENMASK(27, 24);
		ddrc->MSTR_ |=  GENMASK(27, 24) & (((1 << data->ranks) - 1) << 24);
	} else {
		ddrc->MSTR_ &= ~GENMASK(27, 24);
		ddrc->MSTR_ |=  GENMASK(27, 24) & (0xf << 24);
		ddrc->MSTR_ |=  1 << 10; /* enable 2T mode */
	}

	if (data->dbus_half) {
		ddrc->MSTR_  |= 1 << 12; /* MSTR_DB_WIDTH::HALF = 1; */
		ddrc->PCCFG_ |= 1 << 8;
	}

	/*
	 * We set default mapping (HIF->DRAM) (see Table2-11 BL=8 recommendation in DataBook)
	 * COL5,4,3,2  ->HIF A6,A5,A4,A2
	 * BG0         ->HIF A3
	 * COL9,8,7,6  ->HIF A10,A9,A8,A7
	 * COL11,10    ->none
	 * BA1,0       ->HIF A12,A11
	 */
	hif_addr = 13; /* next empty HIF address */
	if (data->dbus_half) {
		ddrc->ADDRMAP8_ &= ~0x3f; /* map BG0 ->HIF A2 */
		ddrc->ADDRMAP2_ = 0x00000007; /* map COL6,5,4,3 ->HIF A5,A4,A3,A9 */
		ddrc->ADDRMAP3_ = 0x1f000000; /* map COL9,8,7   ->HIF A8,A7,A6 */
		ddrc->ADDRMAP1_ = 0x003f0808; /* map BA2->none, BA1->HIF A11, BA0->HIF A10 */
		hif_addr = 12; /* next empty HIF address */
	}

	ddrc->ADDRMAP8_ &= ~GENMASK(13, 8);
	if (data->bank_groups > 2) {
		ddrc->ADDRMAP8_ |= GENMASK(13, 8) & ((hif_addr++ - 3) << 8); /* map BG1->HIF */
	} else {
		ddrc->ADDRMAP8_ |= 63 << 8; /* map BG1->none */
	}

	ddrc->ADDRMAP5_ &= ~0xf;
	ddrc->ADDRMAP5_ |=  0xf & (hif_addr++ - 6); /* map ROW0->HIF */
	ddrc->ADDRMAP5_ &= ~GENMASK(11, 8);
	ddrc->ADDRMAP5_ |=  GENMASK(11, 8) & ((hif_addr++ - 7) << 8); /* map ROW1->HIF */
	ddrc->ADDRMAP5_ &= ~GENMASK(19, 16);
	ddrc->ADDRMAP5_ |=  GENMASK(19, 16) & ((hif_addr - 8) << 16); /* map ROW10..2->HIF */
	hif_addr += 9;
	ddrc->ADDRMAP5_ &= ~GENMASK(27, 24);
	ddrc->ADDRMAP5_ |=  GENMASK(27, 24) & ((hif_addr++ - 17) << 24); /* map ROW11->HIF */
	ddrc->ADDRMAP6_ &= ~GENMASK(3, 0);
	ddrc->ADDRMAP6_ |=  GENMASK(3, 0) & (hif_addr++ - 18); /* map ROW12->HIF */
	ddrc->ADDRMAP6_ &= ~GENMASK(11, 8);
	ddrc->ADDRMAP6_ |=  GENMASK(11, 8) & ((hif_addr++ - 19) << 8); /* map ROW13->HIF */

	if ((data->row_address - 1) >= 14) {
		ddrc->ADDRMAP6_ &= ~GENMASK(19, 16);
		ddrc->ADDRMAP6_ |=  GENMASK(19, 16) & ((hif_addr++ - 20) << 16); /* ROW14->HIF */
	} else {
		ddrc->ADDRMAP6_ &= ~GENMASK(19, 16);
		ddrc->ADDRMAP6_ |=  GENMASK(19, 16) & (15 << 16); /* ROW14->none */
	}

	if ((data->row_address - 1) >= 15) {
		ddrc->ADDRMAP6_ &= ~GENMASK(27, 24);
		ddrc->ADDRMAP6_ |=  GENMASK(27, 24) & ((hif_addr++ - 21) << 24); /* ROW15->HIF */
	} else {
		ddrc->ADDRMAP6_ &= ~GENMASK(27, 24);
		ddrc->ADDRMAP6_ |=  GENMASK(27, 24) & (15 << 24); /* ROW15->none */
	}

	if ((data->row_address - 1) >= 16) {
		ddrc->ADDRMAP7_ &= ~GENMASK(3, 0);
		ddrc->ADDRMAP7_ |=  GENMASK(3, 0) & (hif_addr++ - 22); /* ROW16->HIF */
	} else {
		ddrc->ADDRMAP7_ &= ~GENMASK(3, 0);
		ddrc->ADDRMAP7_ |=  GENMASK(3, 0) & 15; /* ROW16->none */
	}

	if ((data->row_address - 1) == 17) {
		ddrc->ADDRMAP7_ &= ~GENMASK(11, 8);
		ddrc->ADDRMAP7_ |=  GENMASK(11, 8) & ((hif_addr++ - 23) << 8); /* ROW17->HIF */
	} else {
		ddrc->ADDRMAP7_ &= ~GENMASK(11, 8);
		ddrc->ADDRMAP7_ |=  GENMASK(11, 8) & (15 << 8); /* ROW17->none */
	}

	/*
	 * DIMM_0 have 00b and 01b rank addresses on DBM board.
	 * DIMM_1 have 10b and 11b rank addresses on DBM board.
	 */
	if (data->ranks == 2) {
		ddrc->ADDRMAP0_ |=  GENMASK(4, 0) & (hif_addr++ - 6); /* CS0->hifXX */
	} else {
		/* CS0->none: map to none for 1-rank DIMMs (that's not external CS0 signal) */
		ddrc->ADDRMAP0_ |=  GENMASK(4, 0) & 31;
	}

	if (data->dimms == 2) {
		ddrc->ADDRMAP0_ |=  GENMASK(12, 8) & ((hif_addr++ - 7) << 8); /* CS1->hifXX */
	} else {
		ddrc->ADDRMAP0_ |=  GENMASK(12, 8) & (31 << 8); /* CS1->none */
	}

	if (data->registered_dimm) {
		ddrc->DIMMCTL_ |= 1 << 2;
	} else {
		ddrc->DIMMCTL_ &= ~(1UL << 2);
	}

	if (data->mirrored_dimm) {
		ddrc->DIMMCTL_ |= 1 << 1;
	} else {
		ddrc->DIMMCTL_ &= ~(1UL << 1);
	}

	ddrc->DQMAP0_ = data->DQ_map[0];
	ddrc->DQMAP1_ = data->DQ_map[1];
	ddrc->DQMAP2_ = data->DQ_map[2];
	ddrc->DQMAP3_ = data->DQ_map[3];
	ddrc->DQMAP4_ = data->DQ_map[4];
	if (data->DQ_swap_rank) {
		ddrc->DQMAP5_ &= ~(0x1UL);
	} else {
		ddrc->DQMAP5_ |= 0x1;
	}

	if (data->ecc_on) {
		ddrc->ECCCFG0_ &= ~(0x7UL);
		ddrc->ECCCFG0_ |= 0x4;
	} else {
		ddrc->ECCCFG0_ &= ~(0x7UL);
	}

	/* see PHY DataBook, Table7-1 (p.648) */
	ddrc->DFITMG0_ &= ~GENMASK(22, 16);
	ddrc->DFITMG0_ |=  GENMASK(22, 16) & ((data->RL - 4 + !!data->registered_dimm) << 16);
	ddrc->DFITMG0_ &= ~GENMASK(5, 0);
	/* see PHY DataBook, Table7-1 (p.648) */
	ddrc->DFITMG0_ |=  GENMASK(5, 0) & (data->WL - 2 + !!data->registered_dimm);

	ddrc->DFITMG0_ &= ~GENMASK(28, 24);
	ddrc->DFITMG0_ |=  GENMASK(28, 24) & ((2 + 2 + !!data->registered_dimm) << 24); /* see PHY DataBook, Table7-1 (p.648) + 2*DWC_PIPE_DFI2PHY */
#if DDR_CRC_ENABLE
	ddrc->DRAMTMG0_ &= ~GENMASK(30, 24);
	ddrc->DRAMTMG0_ |=  GENMASK(30, 24) & (((data->WL + 4 + data->tWR_CRC_DM) / 2) << 24); /* WL + BL/2 + WR_CRC_DM (this formula is valid for CRC&DM on) */
#else
	ddrc->DRAMTMG0_ &= ~GENMASK(30, 24);
	ddrc->DRAMTMG0_ |=  GENMASK(30, 24) & (((data->WL + 4 + data->tWR) / 2) << 24); /* WL + BL/2 + tWR */
#endif
	ddrc->DRAMTMG0_ &= ~GENMASK(21, 16);
	ddrc->DRAMTMG0_ |=  GENMASK(21, 16) & (DIV_ROUND_UP_2EVAL(data->tFAW, 2) << 16);
	ddrc->DRAMTMG0_ &= ~GENMASK(5, 0);
	ddrc->DRAMTMG0_ |=  GENMASK(5, 0) & (data->tRAS / 2);
	ddrc->DRAMTMG0_ &= ~GENMASK(14, 8);
	ddrc->DRAMTMG0_ |=  GENMASK(14, 8) & (((data->tRASmax / 1024 - 1) / 2) << 8); /* (tRAS(max)/1024)-1)/2 */
	ddrc->DRAMTMG1_ &= ~GENMASK(20, 16);
	ddrc->DRAMTMG1_ |=  GENMASK(20, 16) & (DIV_ROUND_UP_2EVAL(data->tXP + data->PL, 2) << 16);
	ddrc->DRAMTMG1_ &= ~GENMASK(13, 8);
	ddrc->DRAMTMG1_ |=  GENMASK(13, 8) & ((MAX(data->AL + data->tRTP, data->RL + 4 - data->tRP) / 2) << 8);
	/* DDR4: Max of: (AL + tRTP) or (RL + BL/2 - tRP) */
	ddrc->DRAMTMG1_ &= ~GENMASK(6, 0);
	ddrc->DRAMTMG1_ |=  GENMASK(6, 0) & DIV_ROUND_UP_2EVAL(data->tRC, 2);
	ddrc->DRAMTMG2_ &= ~GENMASK(29, 24);
	ddrc->DRAMTMG2_ |=  GENMASK(29, 24) & (DIV_ROUND_UP_2EVAL(data->WL + !!data->registered_dimm, 2) << 24);
	ddrc->DRAMTMG2_ &= ~GENMASK(21, 16);
	ddrc->DRAMTMG2_ |=  GENMASK(21, 16) & (DIV_ROUND_UP_2EVAL(data->RL + !!data->registered_dimm, 2) << 16);

	/*
	 * DDR4 multiPHY Utility Block (PUB) Databook:
	 * 4_ 9.4 Read Transactions, Table 4-23 Read Command Spacing:
	 * add extra Read System Latency in Read2Write delay
	 */
	rsl = (data->clock_mhz * 2 <= 1600) ? 4 : 6; /* experimental RSL = 4 or 6 clk */
	ddrc->DRAMTMG2_ &= ~GENMASK(13, 8);
	ddrc->DRAMTMG2_ |=  GENMASK(13, 8) & (DIV_ROUND_UP_2EVAL(data->RL + 4 + 1 + 1 - data->WL + rsl, 2) << 8); /* DDR4: RL + BL/2 + 1 + WR_PREAMBLE - WL */
#if DDR_CRC_ENABLE
	ddrc->DRAMTMG2_ &= ~GENMASK(5, 0);
	ddrc->DRAMTMG2_ |=  GENMASK(5, 0) & DIV_ROUND_UP_2EVAL(data->CWL + data->PL + 4 + data->tWTR_L_CRC_DM, 2); /* DDR4: CWL + PL + BL/2 + tWTR_L_CRC_DM(tWTR_L+5) (this formula is valid for CRC&DM on) */
#else
	ddrc->DRAMTMG2_ &= ~GENMASK(5, 0);
	ddrc->DRAMTMG2_ |=  GENMASK(5, 0) & DIV_ROUND_UP_2EVAL(data->CWL + data->PL + 4 + data->tWTR_L, 2); /* DDR4: CWL + PL + BL/2 + tWTR_L */
#endif
	ddrc->DRAMTMG3_ &= ~GENMASK(17, 12);
	ddrc->DRAMTMG3_ |=  GENMASK(17, 12) & (DIV_ROUND_UP_2EVAL(data->tMOD + data->PL, 2) << 12); /* tMRD. If C/A parity for DDR4 is used, set to tMRD_PAR(tMOD+PL) instead. */
	ddrc->DRAMTMG3_ &= ~GENMASK(9, 0);
	ddrc->DRAMTMG3_ |=  GENMASK(9, 0) & DIV_ROUND_UP_2EVAL(data->tMOD + data->PL + !!data->registered_dimm, 2); /* tMOD. If C/A parity for DDR4 is used, set to tMOD_PAR(tMOD+PL) instead. */
	ddrc->DRAMTMG4_ &= ~GENMASK(28, 24);
	ddrc->DRAMTMG4_ |=  GENMASK(28, 24) & (((data->tRCD > data->AL) ? DIV_ROUND_UP_2EVAL(data->tRCD - data->AL, 2) : 1) << 24); /* tRCD - AL */
	ddrc->DRAMTMG4_ &= ~GENMASK(19, 16);
	ddrc->DRAMTMG4_ |=  GENMASK(19, 16) & (DIV_ROUND_UP_2EVAL(data->tCCD_L, 2) << 16); /* DDR4: tCCD_L */
	ddrc->DRAMTMG4_ &= ~GENMASK(11, 8);
	ddrc->DRAMTMG4_ |=  GENMASK(11, 8) & (DIV_ROUND_UP_2EVAL(data->tRRD_L, 2) << 8); /* DDR4: tRRD_L */
	ddrc->DRAMTMG4_ &= ~GENMASK(4, 0);
	ddrc->DRAMTMG4_ |=  GENMASK(4, 0) & (data->tRP / 2 + 1); /* Note: val=8, but S.Hudchenko example have val=9 */
	ddrc->DRAMTMG5_ &= ~GENMASK(27, 24);
	ddrc->DRAMTMG5_ |=  GENMASK(27, 24) & (DIV_ROUND_UP_2EVAL(data->tCKSRX, 2) << 24); /* DDR4: tCKSRX */
	ddrc->DRAMTMG5_ &= ~GENMASK(19, 16);
	ddrc->DRAMTMG5_ |=  GENMASK(19, 16) & (DIV_ROUND_UP_2EVAL(CLOCK_NS(10) + data->PL, 2) << 16); /* DDR4: max (10 ns, 5 tCK) (+ PL(parity latency)(*)) */
	/* NOTICE: Only if CRCPARCTL1_ bit<12> = 0, this register should be increased by PL.
	 * Note: val=9, but S_ Hudchenko example have val=6 */
	ddrc->DRAMTMG5_ &= ~GENMASK(13, 8);
	ddrc->DRAMTMG5_ |=  GENMASK(13, 8) & (DIV_ROUND_UP_2EVAL(data->tCKE + 1 + data->PL, 2) << 8); /* DDR4: tCKE + 1 (+ PL(parity latency)(*)) */
	/* NOTICE: Only if CRCPARCTL1_ bit<12> = 0, this register should be increased by PL.
	 * Note: val=6, but S_Hudchenko example have val=4 */
	ddrc->DRAMTMG5_ &= ~GENMASK(4, 0);
	ddrc->DRAMTMG5_ |=  GENMASK(4, 0) & DIV_ROUND_UP_2EVAL(data->tCKE, 2);
	ddrc->DRAMTMG8_ &= ~GENMASK(30, 24);
	ddrc->DRAMTMG8_ |=  GENMASK(30, 24) & (DIV_ROUND_UP_2EVAL(DIV_ROUND_UP_2EVAL(data->tXS_FAST, 32), 2) << 24); /* Note: val=4, but S.Hudchenko example have val=5 */
	ddrc->DRAMTMG8_ &= ~GENMASK(22, 16);
	ddrc->DRAMTMG8_ |=  GENMASK(22, 16) & (DIV_ROUND_UP_2EVAL(DIV_ROUND_UP_2EVAL(data->tXS_ABORT, 32), 2) << 16); /* Note: val=4, but S.Hudchenko example have val=5 */
	ddrc->DRAMTMG8_ &= ~GENMASK(14, 8);
	ddrc->DRAMTMG8_ |=  GENMASK(14, 8) & (DIV_ROUND_UP_2EVAL(DIV_ROUND_UP_2EVAL(data->tXSDLL, 32), 2) << 8);
	ddrc->DRAMTMG8_ &= ~GENMASK(6, 0);
	ddrc->DRAMTMG8_ |=  GENMASK(6, 0) & DIV_ROUND_UP_2EVAL(DIV_ROUND_UP_2EVAL(data->tXS, 32), 2); /* Note: val=7, but S.Hudchenko example have val=11 */
	ddrc->DRAMTMG9_ &= ~(1UL << 30); /* Write Preamble 1clk */
	ddrc->DRAMTMG9_ &= ~GENMASK(18, 16);
	ddrc->DRAMTMG9_ |=  GENMASK(18, 16) & (DIV_ROUND_UP_2EVAL(data->tCCD_S, 2) << 16);
	ddrc->DRAMTMG9_ &= ~GENMASK(11, 8);
	ddrc->DRAMTMG9_ |=  GENMASK(11, 8) & (DIV_ROUND_UP_2EVAL(data->tRRD_S, 2) << 8);
#if DDR_CRC_ENABLE
	ddrc->DRAMTMG9_ &= ~GENMASK(5, 0);
	ddrc->DRAMTMG9_ |=  GENMASK(5, 0) & DIV_ROUND_UP_2EVAL(data->CWL + data->PL + 4 + data->tWTR_S_CRC_DM, 2); /* CWL + PL + BL/2 + tWTR_S_CRC _DM (this formula is valid for CRC&DM on) */
#else
	ddrc->DRAMTMG9_ &= ~GENMASK(5, 0);
	ddrc->DRAMTMG9_ |=  GENMASK(5, 0) & DIV_ROUND_UP_2EVAL(data->CWL + data->PL + 4 + data->tWTR_S, 2); /* CWL + PL + BL/2 + tWTR_S */
#endif
	ddrc->DRAMTMG11_ &= ~GENMASK(30, 24);
	ddrc->DRAMTMG11_ |=  GENMASK(30, 24) & (DIV_ROUND_UP_2EVAL(DIV_ROUND_UP_2EVAL(data->tXMPDLL, 32), 2) << 24); /* Note: val=19, but S.Hudchenko example have val=21 */
	ddrc->DRAMTMG11_ &= ~GENMASK(20, 16);
	ddrc->DRAMTMG11_ |=  GENMASK(20, 16) & ((DIV_ROUND_UP_2EVAL(data->tMPX_LH, 2) + 1) << 16); /* Note: val=7, but S.Hudchenko example have val=9 */
	ddrc->DRAMTMG11_ &= ~GENMASK(9, 8);
	ddrc->DRAMTMG11_ |=  GENMASK(9, 8) & (DIV_ROUND_UP_2EVAL(data->tMPX_S, 2) << 8); /* Note: val=1, but S.Hudchenko example have val=2 */
	ddrc->DRAMTMG11_ &= ~GENMASK(4, 0);
	ddrc->DRAMTMG11_ |=  GENMASK(4, 0) & DIV_ROUND_UP_2EVAL(data->tCKMPE, 2);
	ddrc->DRAMTMG12_ &= ~GENMASK(4, 0);
	ddrc->DRAMTMG12_ |=  GENMASK(4, 0) & DIV_ROUND_UP_2EVAL(data->tMRD_PDA, 2);

	ddrc->RFSHTMG_ &= ~GENMASK(27, 16);
	ddrc->RFSHTMG_ |=  GENMASK(27, 16) & (((data->tREFI / 32) / 2) << 16);
	ddrc->RFSHTMG_ &= ~GENMASK(9, 0);
	ddrc->RFSHTMG_ |=  GENMASK(9, 0) & DIV_ROUND_UP_2EVAL(data->tRFC1, 2); /* tRFC1 for Refresh Mode normal(1x) */

	ddrc->ODTCFG_ &= ~GENMASK(11, 8);
	ddrc->ODTCFG_ |= 6 << 8; /* DDR4 BL8: 5 + RD_PREAMBLE */

	ddrc->ODTCFG_ &= ~GENMASK(6, 2);
	if (data->CL > data->CWL) {
		ddrc->ODTCFG_ |= GENMASK(6, 2) & ((data->CL - data->CWL) << 2);
	}

	ddrc->ODTCFG_ &= ~GENMASK(27, 24);
	ddrc->ODTCFG_ |= (6 + DDR_CRC_ENABLE) << 24;
	ddrc->ODTCFG_ &= ~GENMASK(20, 16);
	ddrc->ODTCFG_ |= ((ddrc->DFITMG1_ >> 28) << 16);
#if DDR_CRC_ENABLE
	ddrc->CRCPARCTL1_ |= 1 << 4;
#else
	ddrc->CRCPARCTL1_ &= ~(1UL << 4);
#endif
#ifdef CRC_ERROR_TEST
	/* set CRC off in controller, but set CRC on in DRAM (inject CRC errors in any SDRAM write) */
	ddrc->CRCPARCTL1_ &= ~(1UL << 4);
#endif
	return 0;
}

int phy_config_content(struct phy_content *phy,
		       const struct ddr_configuration *data)
{
	if (data->mirrored_dimm) {
		/* address mirror */
		phy->DCR_ |= 1 << 27;
		phy->DCR_ |= 1 << 29;
	} else {
		phy->DCR_ &= ~(1 << 27);
		phy->DCR_ &= ~(1 << 29);
	}

	if (data->registered_dimm) {
		phy->DCR_ |= 1 << 27;
	}

	if (data->bank_groups < 4) {
		phy->DCR_ |= 1 << 30; /* set BG1 signal unused */
	}

	if (!data->ecc_on) {
		phy->DX8GCR0_ &= ~(1 << 0);
	}

	phy->dbus_half = data->dbus_half;

	phy->PGCR2_ &= ~GENMASK(17, 0);
	phy->PGCR2_ |=  GENMASK(17, 0) & (data->tREFI - data->tRFC1 - 400);
	if (data->registered_dimm) {
		phy->PGCR2_ &= ~GENMASK(31, 29);
		phy->PGCR2_ |= 1 << 29;
	}

	if (data->clock_mhz / 2 > 465) {
		phy->PLLCR_ &= ~(0x3 << 19); /* PHY ref clock 440..600 MHz */
	} else {
		phy->PLLCR_ &= ~(0x3 << 19);
		phy->PLLCR_ |= 0x1 << 19; /* PHY ref clock 225..490 MHz */
	}

	/* PLL Reset, 9 usec + 16 clocks; in PHY clocks = DRAM clocks / 2 */
	phy->PTR1_ &= ~GENMASK(12, 0);
	phy->PTR1_ |=  GENMASK(12, 0) & (CLOCK_NS(9000) / 2 + 16);
	/* PLL Lock, 26 usec (see tLOCK in PHY Databook); in PHY clocks = DRAM clocks / 2 */
	phy->PTR1_ &= ~GENMASK(31, 16);
	phy->PTR1_ |=  GENMASK(31, 16) & ((CLOCK_NS(26000) / 2) << 16);
	/* DRAM initialization, CKE low time with power and clock stable (500 us), in DRAM clock; */
	phy->PTR3_ &= ~GENMASK(19, 0);
	phy->PTR3_ |=  GENMASK(19, 0) & CLOCK_NS(500000);
	/* DRAM initialization, CKE high time to first command (tRFC + 10 ns); */
	phy->PTR3_ &= ~GENMASK(29, 20);
	phy->PTR3_ |= (data->tRFC1 + CLOCK_NS(10)) << 20;
	/* Affected: Fine Granularity Refresh Mode normal(1x) */
	/* DRAM initialization, Reset low time (200 us on power-up or 100 ns after power-up) */
	phy->PTR4_ &= ~GENMASK(17, 0);
	phy->PTR4_ |=  GENMASK(17, 0) & CLOCK_NS(200000);

	if (data->registered_dimm) {
		phy->RDIMMGCR0_ |= 0x1;
		phy->RDIMMGCR0_ &= ~(1 << 28);
		phy->RDIMMGCR0_ &= ~(1 << 29);
		phy->RDIMMGCR1_ &= ~GENMASK(13, 0);
		phy->RDIMMGCR1_ |=  GENMASK(13, 0) & CLOCK_NS(5000);
		phy->RDIMMGCR2_ = 0x0004a438;

		/* RC03 Command_Address and CS Driver Control */
		uint8_t ca_drv = (data->registered_ca_stren >> 4) & 0x3;
		uint8_t cs_drv = (data->registered_ca_stren >> 6) & 0x3;
		phy->RDIMMGCR0_ &= ~GENMASK(15, 12);
		phy->RDIMMGCR0_ |=  GENMASK(15, 12) & (((cs_drv << 2) | ca_drv) << 12);

		/* RC04 ODT and CKE Driver Control */
		uint8_t cke_drv = (data->registered_ca_stren) & 0x3;
		uint8_t odt_drv = (data->registered_ca_stren >> 2) & 0x3;
		phy->RDIMMGCR0_ &= ~GENMASK(19, 16);
		phy->RDIMMGCR0_ |=  GENMASK(19, 16) & (((cke_drv << 2) | odt_drv) << 16);

		/* RC05 Clock Driver Control */
		uint8_t y0y2_drv = (data->registered_clk_stren) & 0x3;
		uint8_t y1y3_drv = (data->registered_clk_stren >> 2) & 0x3;
		phy->RDIMMGCR0_ &= ~GENMASK(23, 20);
		phy->RDIMMGCR0_ |=  GENMASK(23, 20) & (((y0y2_drv << 2) | y1y3_drv) << 20);

		switch (data->clock_mhz * 2) {
		/* RC10: RDIMM Operating Speed Control Word (CR1)*/
		/* RC3x: Fine Granularity Operating Speed Control (CR2) */
		case 1600:
			phy->RDIMMGCR1_ &= ~GENMASK(11, 8);
			phy->RDIMMGCR2_ &= ~GENMASK(23, 16);
			phy->RDIMMGCR2_ |=  GENMASK(23, 16) & 0x11 << 16;
			break;
		case 1866:
			phy->RDIMMGCR1_ &= ~GENMASK(11, 8);
			phy->RDIMMGCR1_ |=  GENMASK(11, 8) & (1 << 8);
			phy->RDIMMGCR2_ &= ~GENMASK(23, 16);
			phy->RDIMMGCR2_ |=  GENMASK(23, 16) & 0x1f << 16;
			break;
		case 2132:
			phy->RDIMMGCR1_ &= ~GENMASK(11, 8);
			phy->RDIMMGCR1_ |= (GENMASK(11, 8) & (2 << 8));
			phy->RDIMMGCR2_ &= ~GENMASK(23, 16);
			phy->RDIMMGCR2_ |= (GENMASK(23, 16) & 0x2c << 16);
			break;
		case 2400:
			phy->RDIMMGCR1_ &= ~GENMASK(11, 8);
			phy->RDIMMGCR1_ |=  GENMASK(11, 8) & (3 << 8);
			phy->RDIMMGCR2_ &= ~GENMASK(23, 16);
			phy->RDIMMGCR2_ |=  GENMASK(23, 16) & 0x39 << 16;
			break;
		case 2666:
			phy->RDIMMGCR1_ &= ~GENMASK(11, 8);
			phy->RDIMMGCR1_ |=  GENMASK(11, 8) & (4 << 8);
			phy->RDIMMGCR2_ &= ~GENMASK(23, 16);
			phy->RDIMMGCR2_ |=  GENMASK(23, 16) & 0x47 << 16;
			break;
		default:
			break;
		}

		if (data->mirrored_dimm) {
			phy->RDIMMGCR1_ &= ~GENMASK(23, 20);
			phy->RDIMMGCR1_ |=  GENMASK(23, 20) & 0xc << 8;
		} else {
			phy->RDIMMGCR1_ &= ~GENMASK(23, 20);
			phy->RDIMMGCR1_ |=  GENMASK(23, 20) & 0x4 << 8;
		}
	}

	/* DRAM timings */
	phy->DTPR0_ &= ~GENMASK(3, 0);
	phy->DTPR0_ |=  GENMASK(3, 0) & data->tRTP; /* READtoPRE */
	phy->DTPR0_ &= ~GENMASK(14, 8);
	phy->DTPR0_ |=  GENMASK(14, 8) & (data->tRP << 8); /* PREperiod */
	phy->DTPR0_ &= ~GENMASK(22, 16);
	phy->DTPR0_ |=  GENMASK(22, 16) & (data->tRAS << 16); /* ACTtoPRE(min) */
	phy->DTPR0_ &= ~GENMASK(29, 24);
	phy->DTPR0_ |=  GENMASK(29, 24) & (data->tRRD_L << 24); /* ACTtoACT */
	phy->DTPR1_ &= ~GENMASK(23, 16);
	phy->DTPR1_ |=  GENMASK(23, 16) & (data->tFAW << 16);
	phy->DTPR2_ &= ~GENMASK(9, 0);
	phy->DTPR2_ |=  GENMASK(9, 0) & data->tXS;
	phy->DTPR2_ &= ~GENMASK(19, 16);
	phy->DTPR2_ |=  GENMASK(19, 16) & (data->tCKE << 16); /* CKE width */
	phy->DTPR3_ &= ~GENMASK(25, 16);
	phy->DTPR3_ |=  GENMASK(25, 16) & ((data->tDLLK - 128) << 16); /* DLL lock */
	phy->DTPR4_ &= ~GENMASK(4, 0);
	phy->DTPR4_ |=  GENMASK(4, 0) & data->tXP; /* PDexit */
	/* REFCMDperiod; Affected: Fine Granularity Refresh Mode normal(1x) */
	phy->DTPR4_ &= ~GENMASK(25, 16);
	phy->DTPR4_ |=  GENMASK(25, 16) & (data->tRFC1 << 16);
	phy->DTPR5_ &= ~GENMASK(4, 0);
	phy->DTPR5_ |=  GENMASK(4, 0) & data->tWTR_L; /* WRITEtoREAD */
	phy->DTPR5_ &= ~GENMASK(14, 8);
	phy->DTPR5_ |=  GENMASK(14, 8) & (data->tRCD << 8); /* ACTtoREAD/WRITE */
	phy->DTPR5_ &= ~GENMASK(23, 16);
	phy->DTPR5_ |=  GENMASK(23, 16) & (data->tRC << 16); /* ACTtoACT */

	phy->MR0_ = set_mr0(data->CL, data->tWR, data->tRTP);
	/* DLL on; DIC RZQ/7 (33 ohm); AL=CL-1; WL off; ODT RTT_NOM off; TDQS off; Qoff normal */
	phy->MR1_ = 0x9 | (data->DIC & 0x1) << 1 | (data->RTT_NOM & 0x7) << 8;
	phy->MR2_ = set_mr2(data->CWL, data->RTT_WR);

	/*
	 * MPR read format serial; Fine Granularity Refresh Mode normal(1x); Temperature Sensor Readout off;
	 * Per DRAM Addressability off; Geardown Mode 1/2; MPR operation normal; MPR Page 0;
	 * Set Write Command Latency
	 */
	phy->MR3_ = ((uint8_t)data->WCL - 4) << 9;

	/*
	 * Write Preamble 1clk; Read Preable 1 clk; Read Preable Training off; Self Refresh Abort off;
	 * Command Address Latency off; Vref Monitor off; Temp Controlled Refresh off;
	 * Max Power Down mode off
	 */
	phy->MR4_ = 0x0;
	phy->MR5_ = set_mr5(data->PL, data->RTT_PARK);
#ifdef BAIKAL_DUAL_CHANNEL_MODE
	phy->MR6_ = (data->tCCD_L - 4) << 10 | data->PHY_DRAM_VREF;
#else
	phy->MR6_ = (data->tCCD_L - 4) << 10;
#endif

	/* data training configuration */
	phy->DTCR0_ |= 1 << 6; /* use MPR for DQS training (DDR4) */
	phy->DTCR0_ &= ~GENMASK(31, 28);
	phy->DTCR0_ |= 1 << 28; /* set refresh burst count to 1 in PUB mode */
	if (data->registered_dimm) {
		phy->DTCR0_ &= ~GENMASK(15, 14);
	}

	phy->DTCR1_ &= ~GENMASK(31, 16);
	phy->DTCR1_ |=  GENMASK(31, 16) & ((1 << data->ranks) - 1) << 16;
	if (data->dimms == 2) {
		phy->DCR_ |= 1 << 28; /* enable 2T mode */

		/* We have two CS signals (two ranks) per DIMM slot on DBM board */
		phy->DTCR1_ &= ~GENMASK(31, 16);
		if (data->ranks == 1) {
			phy->DTCR1_ |= GENMASK(31, 16) & 0x5 << 16;
		} else {
			phy->DTCR1_ |= GENMASK(31, 16) & 0xf << 16;
		}
	}

	if (data->clock_mhz / 2 > 467) {
		phy->ZQCR_ |= 0x7 << 8;
	}

	return 0;
}

int ddr_config_by_spd(int port, struct ddr_configuration *data)
{
	uint32_t tmp;
	extern struct spd_container spd_content;
	struct ddr4_spd_eeprom *const spd = &spd_content.content[port];

	data->dimms = 1;
#ifdef BAIKAL_DUAL_CHANNEL_MODE
	if (spd_content.dual_channel[port] == 'y') {
		data->dimms = 2;
	}
#endif
	/*
	 * The SPD spec has not the burst length byte, but DDR4 spec has
	 * nature BL8 and BC4, BL8 -bit3, BC4 -bit2.
	 */
	data->burst_lengths_bitmask = 0xc;

	/* DIMM organization parameters */
	data->ranks = ((spd->organization >> 3) & 0x7) + 1;
	if (data->ranks > 2) {
		ERROR("%d-rank memory is not supported\n", data->ranks);
		return -1;
	}

	data->device_width = 4 << (spd->organization & 0x7);
	if (data->device_width > 16 || data->device_width < 8) {
		ERROR("current ddr column width (%d bits) isn't supported\n", data->device_width);
		return -1;
	}

	/* SDRAM device parameters */
	data->bank_groups = ((spd->density_banks >> 6) & 0x3) * 2;
	if (data->bank_groups > 4 || data->bank_groups < 2) {
		ERROR("ddrc supports DIMMs with 2 or 4 bank groups only\n");
		return -1;
	}

	data->row_address = ((spd->addressing >> 3) & 0x7) + 12;
	if (data->row_address > 18 || data->row_address < 14) {
		ERROR("unsupported row address width (%d)\n", data->row_address);
		return -1;
	}

	switch (spd->module_type & DDR4_SPD_MODULETYPE_MASK) {
	case DDR4_SPD_MODULETYPE_RDIMM:
	case DDR4_SPD_MODULETYPE_72B_SO_RDIMM:
		data->registered_dimm	   = 1;
		data->mirrored_dimm	   = spd->mod_section.registered.reg_map & 0x1;
		data->registered_ca_stren  = spd->mod_section.registered.ca_stren;
		data->registered_clk_stren = spd->mod_section.registered.clk_stren;
		break;

	case DDR4_SPD_MODULETYPE_UDIMM:
	case DDR4_SPD_MODULETYPE_SO_DIMM:
	case DDR4_SPD_MODULETYPE_72B_SO_UDIMM:
		data->mirrored_dimm	   = spd->mod_section.unbuffered.addr_mapping & 0x1;
		data->registered_dimm	   = 0;
		data->registered_ca_stren  = 0;
		data->registered_clk_stren = 0;
		break;

	default:
		ERROR("%s: SDRAM memory module type 0x%02x is not supported\n",
		      __func__, spd->module_type);
		return -1;
	}

	/*
	 * MTB - medium timebase. The MTB in the SPD spec is 125 ps.
	 * FTB - fine timebase. The FTB in the SPD spec is 1 ps.
	 */
	if (spd->timebases) {
		ERROR("%s: unknown timebases 0x%02x\n", __func__, spd->timebases);
		return -1;
	}

	/* Get Connector to SDRAM Bit Mapping */
	memset(data->DQ_map, 0, sizeof(data->DQ_map));
	memcpy(((uint8_t *)data->DQ_map + 0), spd->mapping + 0, 8);
	memcpy(((uint8_t *)data->DQ_map + 8), spd->mapping + 10, 8);
	memcpy(((uint8_t *)data->DQ_map + 16), spd->mapping + 8, 2);
	data->DQ_swap_rank = ((spd->mapping[0] >> 6) & 0x3) == 0 ? 1 : 0;

	/* The DIMM has ECC capability when the extension bus exist */
	data->ecc_on = (spd->bus_width >> 3) & 0x1;
	if ((spd->bus_width & 0x7) == 0x2) {
		data->dbus_half = 1;
	}

	/* Get Cycle Time (tCK) */
	data->tCK = SPD_TO_PS(spd->tck_min, spd->fine_tck_min);
	data->clock_mhz = 1000000 / data->tCK;
	if ((data->clock_mhz <= 800) || ((data->clock_mhz > 800) && (data->clock_mhz < 900))) {
		data->clock_mhz = 800;
	} else if ((data->clock_mhz <= 933) || ((data->clock_mhz > 933) && (data->clock_mhz < 1050))) {
		data->clock_mhz = 933;
	} else if ((data->clock_mhz <= 1066) || ((data->clock_mhz > 1066) && (data->clock_mhz < 1150))) {
		data->clock_mhz = 1066;
	} else if ((data->clock_mhz <= 1200) || ((data->clock_mhz > 1200) && (data->clock_mhz < 1300))) {
		data->clock_mhz = 1200;
	} else {
		data->clock_mhz = 1333;
	}
#ifdef BAIKAL_DDR_CUSTOM_CLOCK_FREQ
	if (data->clock_mhz > BAIKAL_DDR_CUSTOM_CLOCK_FREQ) {
		data->clock_mhz = BAIKAL_DDR_CUSTOM_CLOCK_FREQ;
		data->tCK = 1000000 / data->clock_mhz;
	}
#endif

#ifdef BAIKAL_DUAL_CHANNEL_MODE
#ifdef BAIKAL_DBM20
	if (spd_content.dual_channel[port] == 'y') {
		data->clock_mhz = 800; /* 1600 MHz in double rate */
		data->tCK = 1000000 / data->clock_mhz;
	}
#endif
#endif /* BAIKAL_DUAL_CHANNEL_MODE */

	/* Compute CAS Latency (CL) */
	uint64_t lat_mask = ((uint64_t)spd->caslat_b1 << 7)  |
			    ((uint64_t)spd->caslat_b2 << 15) |
			    ((uint64_t)spd->caslat_b3 << 23) |
			    (((uint64_t)spd->caslat_b4 & 0x3f) << 31);

	if (spd->caslat_b4 & 0x80) {
		lat_mask <<= 16;
	}

	tmp = SPD_TO_PS(spd->taa_min, spd->fine_taa_min);
	tmp = CLOCK_PS(tmp);
	while (tmp < 64 && (lat_mask & (1ULL << tmp)) == 0) {
		++tmp;
	}

	if (tmp * data->tCK > 18000) {
		ERROR("%s: the choosen CAS Latency %u is too large\n", __func__, tmp);
	}

	data->CL = tmp;

	/*
	 * Compute CAS Write Latency (CWL). JEDEC 79-4, page 16, table 6
	 * (we set 1-tCK preamble mode in configuration code)
	 */
	switch (data->clock_mhz * 2) {
	case 1600:
		lat_mask = BIT(9) | BIT(11);
		break;
	case 1866:
		lat_mask = BIT(10) | BIT(12);
		break;
	case 2132:
		lat_mask = BIT(11) | BIT(14);
		break;
	case 2400:
		lat_mask = BIT(12) | BIT(16);
		break;
	case 2666:
		lat_mask = BIT(14) | BIT(18);
		break;
	case 2932:
	case 3200:
		lat_mask = BIT(16) | BIT(20);
		break;
	default:
		lat_mask = BIT(18);
		break;
	}

	tmp = data->CL;
	while (tmp > 0 && (lat_mask & (1ULL << tmp)) == 0) {
		--tmp;
	}

	data->CWL = tmp;

	/*
	 * Compute Write Command Latency when CRC and DM are both enabled (WCL)
	 * JEDEC 79-4 page 17 table 8
	 */
	data->WCL = 4;
	if (data->clock_mhz * 2 > 1600) {
		++data->WCL;
	}
	if (data->clock_mhz * 2 > 2666) {
		++data->WCL;
	}
	if (data->clock_mhz * 2 > 3200) {
		++data->WCL;
	}

	/* Compute C/A Parity Latency (PL): JEDEC 79-4, page 21, table 12 */
	if (data->clock_mhz * 2 < 1600) {
		data->PL = 0;
	} else if (data->clock_mhz * 2 <= 2132) {
		data->PL = 4;
	} else if (data->clock_mhz * 2 <= 2666) {
		data->PL = 5;
	} else if (data->clock_mhz * 2 <= 3200) {
		data->PL = 6;
	} else {
		data->PL = 8;
	}

	/* Compute Additive Latency (AL). Note: we set AL to (CL-1) like S.Hudchenko example */
	data->AL = data->CL - 1;

	/* Compute Read Latency (RL) */
	data->RL = data->AL + data->CL + data->PL;

	/* Compute Write Latency (WL) */
	data->WL = data->AL + data->CWL + data->PL;

	/* Get Write Recovery Time (tWR) */
	tmp = SPD_TO_PS(((spd->twr_min_ext & 0xf) << 8) | spd->twr_min_lsb, 0);
	data->tWR = CLOCK_PS(MAX(tmp, 15000U)); /* if WR left empty in spd */

	/* Get Active to Precharge Delay Time (tRAS) */
	tmp = SPD_TO_PS(((spd->tras_trc_ext & 0xf) << 8) | spd->tras_min_lsb, 0);
	data->tRAS = CLOCK_PS(tmp);

	/* Get Row Precharge Delay Time (tRP) */
	tmp = SPD_TO_PS(spd->trp_min, spd->fine_trp_min);
	data->tRP = CLOCK_PS(tmp);

	/* Get Active to Active/Refresh Delay Time (tRC) */
	tmp = SPD_TO_PS(((spd->tras_trc_ext & 0xf0) << 4) | spd->trc_min_lsb, spd->fine_trc_min);
	data->tRC = CLOCK_PS(tmp);

	/* Get Write to Read Time (tWTR_L), same bank group */
	tmp = SPD_TO_PS(((spd->twtr_min_ext & 0xf0) << 4) | spd->twtrl_min_lsb, 0);
	data->tWTR_L = CLOCK_PS(MAX(tmp, 7500U));

	/* Get Write to Read Time (tWTR_S), different bank group */
	tmp = SPD_TO_PS(((spd->twtr_min_ext & 0xf) << 8) | spd->twtrs_min_lsb, 0);
	data->tWTR_S = CLOCK_PS(MAX(tmp, 2500U));

	/* Get RAS to CAS Delay Time (tRCD) */
	tmp = SPD_TO_PS(spd->trcd_min, spd->fine_trcd_min);
	data->tRCD = CLOCK_PS(tmp);

	/* Get Activate to Activate Delay Time (tRRD_L), same bank group */
	tmp = SPD_TO_PS(spd->trrdl_min, spd->fine_trrdl_min);
	data->tRRD_L = MAX(4U, CLOCK_PS(tmp));

	/* Get Activate to Activate Delay Time (tRRD_S), different bank group */
	tmp = SPD_TO_PS(spd->trrds_min, spd->fine_trrds_min);
	data->tRRD_S = MAX(4U, CLOCK_PS(tmp));

	/* Get Refresh Recovery Delay Time, 1x mode (tRFC1) */
	tmp = SPD_TO_PS((spd->trfc1_min_msb << 8) | spd->trfc1_min_lsb, 0);
	data->tRFC1 = CLOCK_PS(tmp);

	/* Get Refresh Recovery Delay Time, 4x mode (tRFC4) */
	tmp = SPD_TO_PS((spd->trfc4_min_msb << 8) | spd->trfc4_min_lsb, 0);
	data->tRFC4 = CLOCK_PS(tmp);

	/* Get Four Activate Window Delay Time (tFAW) */
	tmp = SPD_TO_PS(((spd->tfaw_msb & 0xf) << 8) | spd->tfaw_min, 0);
	switch (data->device_width) {
	case 16:
		data->tFAW = MAX(28U, CLOCK_PS(tmp));
		break;
	case 8:
		data->tFAW = MAX(20U, CLOCK_PS(tmp));
		break;
	default:
		data->tFAW = MAX(16U, CLOCK_PS(tmp));
		break;
	}

	/* Get Average Periodic Refresh Interval (tREFI) */
	data->tREFI = CLOCK_NS(7800);

	/* Compute DLL Locking time (tDLLK): DLL lock. JEDEC 79-4, page 22, table 13 */
	if (data->clock_mhz * 2 <= 1866) {
		data->tDLLK = 597;
	} else if (data->clock_mhz * 2 <= 2400) {
		data->tDLLK = 768;
	} else if (data->clock_mhz * 2 <= 2666) {
		data->tDLLK = 854;
	} else if (data->clock_mhz * 2 <= 2932) {
		data->tDLLK = 940;
	} else {
		data->tDLLK = 1024;
	}

	/* Compute Exit Power Down with DLL on (tXP) */
	data->tXP = MAX(4U, CLOCK_NS(6));

	/* Compute Read to Precharge for autoprecharge (tRTP) */
	data->tRTP = MAX(4U, CLOCK_PS(7500));

	/*
	 * Compute Write Recovery time when CRC and DM are enabled (tWR_CRC_DM)
	 *
	 * Compute Write to Read time for different bank group with both CRC and
	 * DM enabled (tWTR_S_CRC_DM)
	 *
	 * Compute Write to Read time for same bank group with both CRC and DM
	 * enabled (tWTR_L_CRC_DM)
	 */
	if (data->clock_mhz * 2 <= 1600) {
		tmp = MAX(4U, CLOCK_PS(3750));
	} else {
		tmp = MAX(5U, CLOCK_PS(3750));
	}
	data->tWR_CRC_DM = data->tWR + tmp;
	data->tWTR_S_CRC_DM = data->tWTR_S + tmp;
	data->tWTR_L_CRC_DM = data->tWTR_L + tmp;

	/* Compute Mode Register Set time (tMOD) */
	data->tMOD = MAX(24U, CLOCK_NS(15));

	/*
	 * Compute Valid Clock Requirement before Self Refresh Exit or
	 * Power-Down Exit or Reset Exit (tCKSRX)
	 */
	data->tCKSRX = MAX(5U, CLOCK_NS(10));

	/* Compute CKE minimum pulse width (tCKE) */
	data->tCKE = MAX(3U, CLOCK_NS(5));

	/* Compute Mode Register Set command cycle time in PDA mode (tMRD_PDA) */
	data->tMRD_PDA = MAX(16U, CLOCK_NS(10));

	/* Get CAS to CAS Delay Time (tCCD_L), same bank group */
	tmp = SPD_TO_PS(spd->tccdl_min, spd->fine_tccdl_min);
	data->tCCD_L = MAX(5U, CLOCK_PS(tmp));

	/* CAS_n to CAS_n command Delay (different BG). Constant for all speedbins */
	data->tCCD_S = 4;
	/* CS setup time to CKE. Get this parameter from SDRAM chip datasheet */
	data->tMPX_S = 1;
	/* CS_n Low hold time to CKE rising edge. JEDEC 79-4, page 134 */
	data->tMPX_LH = CLOCK_NS(12);
	/* Command pass disable delay. JEDEC 79-4, page 190, table 101,102 */
	data->tCPDED = 4;

	/* Compute Exit Self Refresh to commands not requiring a locked DLL (tXS) */
	data->tXS = data->tRFC1 + CLOCK_NS(10);
	/* Compute Exit Self Refresh to ZQCL,ZQCS and MRS (tXS_FAST) */
	data->tXS_FAST = data->tRFC4 + CLOCK_NS(10);

	/* Compute SRX to commands not requiring a locked DLL in Self Refresh ABORT (tXS_ABORT) */
	data->tXS_ABORT = data->tRFC4 + CLOCK_NS(10);

	/* Compute Exit Self Refresh to tXSDLL commands requiring a locked DLL (tXSDLL) */
	data->tXSDLL = data->tDLLK;
	/* Compute Exit MPSM to commands not requiring a locked DLL (tXMP) */
	data->tXMP = data->tXS;
	/* Compute Exit MPSM to commands requiring a locked DLL (tXMPDLL) */
	data->tXMPDLL = data->tXMP + data->tXSDLL;
	/* Compute Valid clock requirement after MPSM entry (tCKMPE) */
	data->tCKMPE = data->tMOD + data->tCPDED;
	/* Compute Maximum ACT to PRE command period (tRASmax) */
	data->tRASmax = data->tREFI * 9;

	return 0;
}
