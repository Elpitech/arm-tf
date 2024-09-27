/*
 * Copyright (c) 2023-2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DDR_MENU_H
#define DDR_MENU_H

#include <baikal_def.h>

#define FLASH_SPEEDBIN_3200		0xefff
#define FLASH_SPEEDBIN_2933		0xcfff
#define FLASH_SPEEDBIN_2666		0xcff7
#define FLASH_SPEEDBIN_2400		0x3f77
#define FLASH_SPEEDBIN_2133		0x3e33
#define FLASH_SPEEDBIN_1866		0x1622
#define FLASH_SPEEDBIN_1600		0x1600

#define BAIKAL_FLASH_USE_SPD		0xd182
#define BAIKAL_FLASH_USE_STR		0xf1a3

#define BAIKAL_DIC_RZQ_DIV_7		0x0fff
#define BAIKAL_DIC_RZQ_DIV_5		0x0f0f

#define BAIKAL_RTTWR_DYN_OFF		0x7fff
#define BAIKAL_RTTWR_RZQ_DIV_4		0x07ff
#define BAIKAL_RTTWR_RZQ_DIV_2		0x03ff
#define BAIKAL_RTTWR_HI_Z		0x01ff
#define BAIKAL_RTTWR_RZQ_DIV_3		0x00ff

#define BAIKAL_RTTNOM_RZQ_DIS		0x7fff
#define BAIKAL_RTTNOM_RZQ_DIV_4		0x07ff
#define BAIKAL_RTTNOM_RZQ_DIV_2		0x03ff
#define BAIKAL_RTTNOM_RZQ_DIV_6		0x01ff
#define BAIKAL_RTTNOM_RZQ_DIV_1		0x00ff
#define BAIKAL_RTTNOM_RZQ_DIV_5		0x007f
#define BAIKAL_RTTNOM_RZQ_DIV_3		0x003f
#define BAIKAL_RTTNOM_RZQ_DIV_7		0x0037

#define BAIKAL_RTTPARK_RZQ_DIS		0x7fff
#define BAIKAL_RTTPARK_RZQ_DIV_4	0x07ff
#define BAIKAL_RTTPARK_RZQ_DIV_2	0x03ff
#define BAIKAL_RTTPARK_RZQ_DIV_6	0x01ff
#define BAIKAL_RTTPARK_RZQ_DIV_1	0x00ff
#define BAIKAL_RTTPARK_RZQ_DIV_5	0x007f
#define BAIKAL_RTTPARK_RZQ_DIV_3	0x003f
#define BAIKAL_RTTPARK_RZQ_DIV_7	0x0037

#define BAIKAL_ECC_DISABLE		0x00de
#define BAIKAL_CRC_ENABLE		0x00ea
#define BAIKAL_PARITY_EN		0x00ea

struct ddr_flash_config {
	uint16_t speedbin;
	uint16_t ddr_sign;
	uint16_t flsh_dic;
	uint16_t flsh_rtt_wr;
	uint16_t flsh_rtt_nom;
	uint16_t flsh_rtt_park;
	uint16_t flsh_cl;
	uint16_t flsh_trcd;
	uint16_t flsh_trp;
	uint16_t flsh_tras;
	uint16_t flsh_tfaw;
	uint16_t flsh_1t2t;
	uint16_t flsh_host_v;
	uint16_t flsh_dram_v;
	uint16_t flsh_vref_use;
	uint16_t flsh_spd_crc;
	uint16_t flsh_ecc_dis;
	uint16_t flsh_crc_en;
	uint16_t flsh_par_en;
	uint16_t reserved[13];
};

#endif /* DDR_MENU_H */
