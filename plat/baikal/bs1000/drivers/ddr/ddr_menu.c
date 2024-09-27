/*
 * Copyright (c) 2023-2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>

#include <baikal_bootflash.h>
#include "ddr_menu.h"
#include "ddr_main.h"
#include "ddr_bs1000.h"

int ddr_flash_conf_load(void *ddr_storage)
{
	int err = bootflash_read(BAIKAL_DDRCFG_NVBASE,
				 ddr_storage,
				 sizeof(struct ddr_flash_config) *
				 DDR_PORT_NUM * PLATFORM_CHIP_COUNT);
	if (err) {
		ERROR("Failed to read flash for DDR frequency setup\n");
		return -1;
	}
	return 0;
}

int ddr_flash_save_conf(struct ddr_configuration *data, uint16_t spd_crc)
{
	struct ddr_flash_config *ddr_port_config = data->ddr_port_config;

	switch (data->clock_mhz) {
	case 1600:
		ddr_port_config->speedbin = FLASH_SPEEDBIN_3200;
		break;
	case 1433:
		ddr_port_config->speedbin = FLASH_SPEEDBIN_2933;
		break;
	case 1333:
		ddr_port_config->speedbin = FLASH_SPEEDBIN_2666;
		break;
	case 1200:
		ddr_port_config->speedbin = FLASH_SPEEDBIN_2400;
		break;
	case 1066:
		ddr_port_config->speedbin = FLASH_SPEEDBIN_2133;
		break;
	case 933:
		ddr_port_config->speedbin = FLASH_SPEEDBIN_1866;
		break;
	case 800:
		ddr_port_config->speedbin = FLASH_SPEEDBIN_1600;
		break;
	default:
		/* Improbable situation */
		break;
	}

	switch (data->DIC) {
	case 0:
		ddr_port_config->flsh_dic = BAIKAL_DIC_RZQ_DIV_7;
		break;
	case 1:
		ddr_port_config->flsh_dic = BAIKAL_DIC_RZQ_DIV_5;
		break;
	default:
		/* Improbable situation */
		break;
	}

	switch (data->RTT_WR) {
	case 0:
		ddr_port_config->flsh_rtt_wr = BAIKAL_RTTWR_DYN_OFF;
		break;
	case 1:
		ddr_port_config->flsh_rtt_wr = BAIKAL_RTTWR_RZQ_DIV_4;
		break;
	case 2:
		ddr_port_config->flsh_rtt_wr = BAIKAL_RTTWR_RZQ_DIV_2;
		break;
	case 3:
		ddr_port_config->flsh_rtt_wr = BAIKAL_RTTWR_HI_Z;
		break;
	case 4:
		ddr_port_config->flsh_rtt_wr = BAIKAL_RTTWR_RZQ_DIV_3;
		break;
	default:
		/* Improbable situation */
		break;
	}

	switch (data->RTT_NOM) {
	case 0:
		ddr_port_config->flsh_rtt_nom = BAIKAL_RTTNOM_RZQ_DIS;
		break;
	case 1:
		ddr_port_config->flsh_rtt_nom = BAIKAL_RTTNOM_RZQ_DIV_4;
		break;
	case 2:
		ddr_port_config->flsh_rtt_nom = BAIKAL_RTTNOM_RZQ_DIV_2;
		break;
	case 3:
		ddr_port_config->flsh_rtt_nom = BAIKAL_RTTNOM_RZQ_DIV_6;
		break;
	case 4:
		ddr_port_config->flsh_rtt_nom = BAIKAL_RTTNOM_RZQ_DIV_1;
		break;
	case 5:
		ddr_port_config->flsh_rtt_nom = BAIKAL_RTTNOM_RZQ_DIV_5;
		break;
	case 6:
		ddr_port_config->flsh_rtt_nom = BAIKAL_RTTNOM_RZQ_DIV_3;
		break;
	case 7:
		ddr_port_config->flsh_rtt_nom = BAIKAL_RTTNOM_RZQ_DIV_7;
		break;
	default:
		/* Improbable situation */
		break;
	}

	switch (data->RTT_PARK) {
	case 0:
		ddr_port_config->flsh_rtt_park = BAIKAL_RTTPARK_RZQ_DIS;
		break;
	case 1:
		ddr_port_config->flsh_rtt_park = BAIKAL_RTTPARK_RZQ_DIV_4;
		break;
	case 2:
		ddr_port_config->flsh_rtt_park = BAIKAL_RTTPARK_RZQ_DIV_2;
		break;
	case 3:
		ddr_port_config->flsh_rtt_park = BAIKAL_RTTPARK_RZQ_DIV_6;
		break;
	case 4:
		ddr_port_config->flsh_rtt_park = BAIKAL_RTTPARK_RZQ_DIV_1;
		break;
	case 5:
		ddr_port_config->flsh_rtt_park = BAIKAL_RTTPARK_RZQ_DIV_5;
		break;
	case 6:
		ddr_port_config->flsh_rtt_park = BAIKAL_RTTPARK_RZQ_DIV_3;
		break;
	case 7:
		ddr_port_config->flsh_rtt_park = BAIKAL_RTTPARK_RZQ_DIV_7;
		break;
	default:
		/* Improbable situation */
		break;
	}

	ddr_port_config->flsh_cl = data->CL;
	ddr_port_config->flsh_trcd = data->tRCD;
	ddr_port_config->flsh_trp = data->tRP;
	ddr_port_config->flsh_tras = data->tRAS;
	ddr_port_config->flsh_tfaw = data->tFAW;

	ddr_port_config->flsh_1t2t = data->timing_2t;
	ddr_port_config->flsh_host_v = data->HOST_VREF;
	ddr_port_config->flsh_dram_v = data->DRAM_VREF;
	ddr_port_config->flsh_vref_use = false;
	ddr_port_config->flsh_spd_crc = spd_crc;

	ddr_port_config->ddr_sign = BAIKAL_FLASH_USE_SPD;

	return 0;
}

int ddr_flash_conf_store(void *ddr_storage)
{
	int err = bootflash_erase(BAIKAL_DDRCFG_NVBASE,
				  BAIKAL_BOOTFLASH_SUBSECTOR);
	if (err) {
		ERROR("Failed to erase flash\n");
		return -2;
	}


	err = bootflash_write(BAIKAL_DDRCFG_NVBASE,
			      ddr_storage,
			      sizeof(struct ddr_flash_config) *
			      DDR_PORT_NUM * PLATFORM_CHIP_COUNT);

	if (err) {
		ERROR("Failed to write default DDR structure to flash\n");
		return -1;
	}

	return 0;
}
