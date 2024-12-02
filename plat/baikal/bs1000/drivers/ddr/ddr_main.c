/*
 * Copyright (c) 2022-2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <drivers/console.h>
#include <lib/mmio.h>

#include <baikal_bootflash.h>
#include <baikal_def.h>
#include <bs1000_dimm_spd.h>
#include <spd.h>

#include "ddr_bs1000.h"
#include "ddr_ctrl.h"
#include "ddr_main.h"
#include "ddr_master.h"
#include "ddr_misc.h"
#include "ddr_spd.h"
#include "phy/ddr_phy_main.h"

#if defined(BAIKAL_DDRCFG_IN_FLASH)
int ddr_flash_init_flag[PLATFORM_CHIP_COUNT * DDR_PORT_NUM + 1];
struct ddr_flash_config ddr_storage[PLATFORM_CHIP_COUNT * DDR_PORT_NUM];
#endif

#define BAIKAL_DDRFW_RDIMM_SIZE	55036
#define BAIKAL_DDRFW_UDIMM_SIZE	53468
#define BAIKAL_DDRFW_RDIMM_OFFS	BAIKAL_DDRFW_UDIMM_SIZE

int ddr_init_ecc_memory(int port);
int ddr_odt_configuration(unsigned int port, struct ddr_configuration *data);

uint8_t fw_container[55036];

static uint64_t ddr_get_addr_strip_bit(unsigned int channels, int capacity_gb)
{
	char bytes[3];
	uint64_t mask;

	if (channels == 1) {
		/* set address bit strip mask for CMN to 1 SN-F */
		bytes[0] = 0;
		bytes[1] = 0;
		bytes[2] = 0;
	} else if (channels == 2) {
		/* set address bit strip mask for CMN to 2 SN-F */
		bytes[0] = 9;
		bytes[1] = 0;
		bytes[2] = 0;
	} else if (channels == 4) {
		/* set address bit strip mask for CMN to 4 SN-F */
		bytes[0] = 8;
		bytes[1] = 9;
		bytes[2] = 0;
	} else if (channels == 6) {
		switch (capacity_gb) {
		case 1:
		/*   1 GiB DRAM size per SN-F addressing */
			bytes[0] = 28;
			bytes[1] = 31;
			bytes[2] = 35;
			break;
		case 2:
		/*   2 GiB DRAM size per SN-F addressing */
			bytes[0] = 28;
			bytes[1] = 32;
			bytes[2] = 33;
			break;
		case 4:
		/*   4 GiB DRAM size per SN-F addressing */
			bytes[0] = 28;
			bytes[1] = 33;
			bytes[2] = 34;
			break;
		case 8:
		/*   8 GiB DRAM size per SN-F addressing */
			bytes[0] = 33;
			bytes[1] = 34;
			bytes[2] = 39;
			break;
		case 16:
		/*  16 GiB DRAM size per SN-F addressing */
			bytes[0] = 28;
			bytes[1] = 36;
			bytes[2] = 39;
			break;
		case 32:
		/*  32 GiB DRAM size per SN-F addressing */
			bytes[0] = 28;
			bytes[1] = 36;
			bytes[2] = 37;
			break;
		case 64:
		/*  64 GiB DRAM size per SN-F addressing */
			bytes[0] = 28;
			bytes[1] = 37;
			bytes[2] = 38;
			break;
		case 128:
		/* 128 GiB DRAM size per SN-F addressing */
			bytes[0] = 37;
			bytes[1] = 38;
			bytes[2] = 43;
			break;
		default:
			ERROR("Can't configure CMN - incorrect DRAM size = %d GiB\n", capacity_gb);
			return -1;
		}
	} else {
		ERROR("CMN can't operate with a such DRAM configuration\n");
		return -1;
	}

	mask = (1ULL << bytes[2]) | (1ULL << bytes[1]) | (1ULL << bytes[0]);
	mask &= ~1; /* don't strip bit 0 (bit 0 = none) */
	return mask;
	return 0;
}

#ifdef INTERACTIVE_DDR_CONFIG
static int interactive_ddr_config;

static int console_getc_nonblocked(void)
{
	int err = ERROR_NO_VALID_CONSOLE;
	console_t *console;

	for (console = console_list; console != NULL;
	     console = console->next) {
		if ((console->flags & CONSOLE_FLAG_BOOT) && (console->getc != NULL)) {
			int ret = console->getc(console);
			if (ret >= 0)
				return ret;
			if (err != ERROR_NO_PENDING_CHAR)
				err = ret;
		}
	}

        return err;
}

static void get_num(uint32_t *ret)
{
	int val = 0, key;

	printf(" - Enter number\n");
	while (1) {
		key = console_getc();
		if (key >= '0' && key <= '9') {
			console_putc(key);
			val = val * 10 + key - '0';
		} else if (key == '\n' || key == '\r') {
			*ret = val;
			break;
		} else {
			break;
		}
	}
	console_putc('\n');
	return;
}

static void get_hex_num(uint32_t *ret)
{
	int val = 0, key;

	printf(" - Enter hex number\n");
	while (1) {
		key = console_getc();
		if (key >= '0' && key <= '9') {
			console_putc(key);
			val = val * 16 + key - '0';
		} else if (key > 'a' && key <= 'f') {
			console_putc(key);
			val = val * 16 + key - 'a' + 0xa;
		} else if (key == '\n' || key == '\r') {
			*ret = val;
			break;
		} else {
			break;
		}
	}
	console_putc('\n');
	return;
}

static int ddr_update_config(unsigned int port, struct ddr4_spd_eeprom *spd,
			     struct ddr_configuration *data)
{
	int key;
	uint32_t odt, freq, old_clock;

	old_clock = data->clock_mhz;
	while (1) {
		odt = (data->RTT_WR << 8) + (data->RTT_NOM << 4) + data->RTT_PARK;
		freq = data->clock_mhz * 2;
		printf("Port %d: Enter number to change value or any other key to continue:\n"
			"1. DDR_FREQ  (%d)\n"
			"2. HOST_VREF (%d)\n"
			"3. DRAM_VREF (%d)\n"
			"4. ODT       (%x)\n"
			"5. ODT MAP   (%08x)\n"
			"*. Continue\n", port, freq, data->HOST_VREF, data->DRAM_VREF,
			odt, data->odt_map);
		key = console_getc();
		if (key == '1') {
			get_num(&freq);
			data->clock_mhz = freq / 2;
		} else if (key == '2') {
			get_num(&data->HOST_VREF);
		} else if (key == '3') {
			get_num(&data->DRAM_VREF);
		} else if (key == '4') {
			get_hex_num(&odt);
			data->RTT_WR = (odt >> 8) & 0x7;
			data->RTT_NOM = (odt >> 4) & 0x7;
			data->RTT_PARK = odt & 0x7;
		} else if (key == '5') {
			get_hex_num(&data->odt_map);
		} else {
			break;
		}
	}

	if (data->clock_mhz != old_clock)
		ddr_config_by_spd(port, spd, data);

	return 0;
}
#endif

static int ddr_port_init(unsigned int port, struct ddr4_spd_eeprom *spd,
			 unsigned int channels, bool dual_channel_mode)
{
	int err = 0;
	static int fw_read_flag; /* {2,1} if fw for {r,u}dimm is in place */
	struct ddr_configuration data = {0};
	int capacity_gb = spd_get_baseconf_dimm_capacity(spd) / 1024 / 1024 / 1024;
	uint64_t t_start, t_stop, t_diff;

	data.dimms = dual_channel_mode ? 2 : 1;

#ifdef BAIKAL_DDRCFG_IN_FLASH
	data.ddr_port_config = &ddr_storage[port];

	if ((ddr_storage[port].speedbin == 0) &&
		(ddr_storage[port].ddr_sign == 0)) {
		ddr_flash_init_flag[port + 1] = 0xf;
		ddr_flash_init_flag[0] = 0xf;
		goto skip_flash_crc_check;
	}
	if (ddr_storage[port].flsh_spd_crc !=
		((spd->crc[1] << 8) | (spd->crc[0]))) {
		ddr_flash_init_flag[port + 1] = 0xf;
		ddr_flash_init_flag[0] = 0xf;
		memset(&ddr_storage[port], 0, sizeof(struct ddr_flash_config));
	}
skip_flash_crc_check:
#endif

	if (ddr_config_by_spd(port, spd, &data)) {
		goto error;
	}

#ifdef BAIKAL_DDRCFG_IN_FLASH
	if ((ddr_storage[port].flsh_ecc_dis == BAIKAL_ECC_DISABLE) &&
		(ddr_storage[port].ddr_sign == BAIKAL_FLASH_USE_STR)) {
		/* disable ECC */
		data.ecc_on = 0;
	}
	if ((ddr_storage[port].flsh_crc_en == BAIKAL_CRC_ENABLE) &&
		(ddr_storage[port].ddr_sign == BAIKAL_FLASH_USE_STR)) {
		/* enable DRAM CRC */
		data.crc_on = 1;
	}
	if ((ddr_storage[port].flsh_par_en == BAIKAL_PARITY_EN) &&
		(ddr_storage[port].ddr_sign == BAIKAL_FLASH_USE_STR)) {
		/* enable CA PARITY */
		data.par_on = 1;
	}
#else
	/* disable CA PARITY */
	data.par_on = 0;
	/* disable DRAM CRC */
	data.crc_on = 0;
	/* disable DRAM PHY Equalization */
	data.phy_eql = 0;
#endif

	data.addr_strip_bit = ddr_get_addr_strip_bit(channels,
				capacity_gb * (dual_channel_mode ? 2 : 1));
	if ((int64_t)data.addr_strip_bit < 0) {
		goto error;
	}

	if (ddr_odt_configuration(port, &data)) {
		goto error;
	}

#ifdef INTERACTIVE_DDR_CONFIG
	if (interactive_ddr_config & (1 << port)) {
retry:
		ddr_update_config(port, spd, &data); 
	}
#endif

	/* enable PHY 2D training */
	if (data.clock_mhz >= 1200) {
		data.phy_training_2d = 1;
	}

	if (data.clock_mhz != 800) {
		if (ddr_lcpcmd_set_speedbin(port, data.clock_mhz)) {
			ERROR("Set speedbin failed (%dMHz)\n", data.clock_mhz);
			goto error;
		}
	}

	ddrlcru_apb_reset_off(port);

	if (ctrl_init(port, &data)) {
		ERROR("Failed to init controller\n");
		goto error;
	}

	ddrlcru_core_reset_off(port);

	err = ctrl_prepare_phy_init(port);
	if (err) {
		ERROR("Failed to prepare PHY init\n");
		goto error;
	}

	if (data.registered_dimm) {
		if (fw_read_flag != 2) {
			err = bootflash_read(BAIKAL_SPI_DDRFW_NVBASE + BAIKAL_DDRFW_RDIMM_OFFS,
					     fw_container, BAIKAL_DDRFW_RDIMM_SIZE);
			fw_read_flag = 2;
		}
	} else {
		if (fw_read_flag != 1) {
			err = bootflash_read(BAIKAL_SPI_DDRFW_NVBASE,
					     fw_container, BAIKAL_DDRFW_UDIMM_SIZE);
			fw_read_flag = 1;
		}
	}

	if (err) {
		goto error;
	}

	t_start = read_cntpct_el0();

	phy_main(port, &data);

	err = ctrl_complete_phy_init(port, &data);
	t_stop = read_cntpct_el0();
	t_diff = (t_stop -t_start) / (SYS_COUNTER_FREQ_IN_TICKS / 1000000);

	if (err) {
		printf("phy init failed in %d us\n", (int)t_diff);
#ifdef INTERACTIVE_DDR_CONFIG
		goto retry;
#else
		goto error;
#endif
	}

	if (data.registered_dimm) {
		/* this is experimental workaround code
		 * for some RDIMMs (like MTA18ASF2G72PZ-3G2, MTA36ASF4G72PZ-3G2)
		 */
		umctl2_enter_SR(port); /* enter DRAM self-refresh mode */
		udelay(10);
		umctl2_exit_SR(port); /* exit DRAM self-refresh mode */
	}

#if defined(BAIKAL_DDRCFG_IN_FLASH)
	if (ddr_flash_init_flag[port + 1]) {
		ddr_flash_save_conf(&data, ((spd->crc[1] << 8) | (spd->crc[0])));
		ddr_flash_init_flag[port + 1] = 0x0;
	}
#endif

	if (data.ecc_on) {
		ddr_init_ecc_memory(port);
	}

	INFO("DIMM%u: module rate %u MHz, AA-RCD-RP-RAS %u-%u-%u-%u\n", port,
	     data.clock_mhz * 2, data.CL, data.tRCD, data.tRP, data.tRAS);

#ifdef INTERACTIVE_DDR_CONFIG
	if (interactive_ddr_config & (1 << port)) {
		int key;
		printf("phy init completed in %d us\n", (int)t_diff);
		t_stop = read_cntpct_el0();
		t_diff = (t_stop -t_start) / (SYS_COUNTER_FREQ_IN_TICKS / 1000000);
		printf("port %d init OK (%d us). Reconfig or Continue [R/C]? (C)", port, (int)t_diff);
		key = console_getc();
		printf("\n");
		if (key == 'r' || key == 'R')
			goto retry;
	}
#endif

	return 0;

error:
	ERROR("Failed to init DDR port #%u\n", port);
	return -1;
}

int dram_init(void)
{
	int ret = 0;
	uint64_t conf = 0;
	unsigned int channels[PLATFORM_CHIP_COUNT];
	struct ddr4_spd_eeprom *spd_content;
	unsigned int chip_idx, port_idx, slot_idx;
#ifdef INTERACTIVE_DDR_CONFIG
	uint64_t timeout;
	int key = 0;
#endif

#ifdef INTERACTIVE_DDR_CONFIG
	printf("Press S for interactive DDR configuration...");
	timeout = timeout_init_us(1000000);
	while (!timeout_elapsed(timeout)) {
		key = console_getc_nonblocked();
		if (key > 0)
			break;
	}
	printf("\n");
	if (key == 's' || key == 'S') {
		printf("Select starting port (0..5)...");
		key = console_getc();
		interactive_ddr_config = 0x3f;
		if (key > '0' && key <= '5') {
			interactive_ddr_config <<= (key - '0');
			interactive_ddr_config &= 0x3f;
		}
	}
#else
	udelay(100000);
#endif

	for (chip_idx = 0, slot_idx = 0; chip_idx < PLATFORM_CHIP_COUNT; ++chip_idx) {
		channels[chip_idx] = 0;
		for (port_idx = 0; port_idx < DDR_PORT_NUM; ++port_idx) {
			spd_content = (struct ddr4_spd_eeprom *)baikal_dimm_spd_get(chip_idx, port_idx * 2);
			if (spd_content->mem_type == SPD_MEMTYPE_DDR4) {
				INFO("DIMM%u: DDR4 SDRAM is detected\n", chip_idx * DDR_PORT_NUM + port_idx);
				channels[chip_idx]++;
				conf |= 1 << slot_idx;
			}
			slot_idx++;
			spd_content = (struct ddr4_spd_eeprom *)baikal_dimm_spd_get(chip_idx, port_idx * 2 + 1);
			if (spd_content->mem_type == SPD_MEMTYPE_DDR4) {
				conf |= 1 << slot_idx;
			}
			slot_idx++;
		}
	}

	bootflash_init();

#if defined(BAIKAL_DDRCFG_IN_FLASH)
	ddr_flash_conf_load((void *)ddr_storage);
#endif

	for (chip_idx = 0, slot_idx = 0; chip_idx < PLATFORM_CHIP_COUNT; ++chip_idx) {
		for (port_idx = 0; port_idx < DDR_PORT_NUM; ++port_idx, slot_idx += 2) {
			if (conf & (1 << slot_idx)) {
				bool dual_channel_mode = (conf & (1 << (slot_idx + 1))) ? true : false;

				spd_content = (struct ddr4_spd_eeprom *)baikal_dimm_spd_get(chip_idx, port_idx * 2);
				ret = ddr_port_init(chip_idx * DDR_PORT_NUM + port_idx,
						    spd_content,
						    channels[chip_idx],
						    dual_channel_mode);
				if (ret) {
					goto error;
				}
			}
		}
	}

#if defined(BAIKAL_DDRCFG_IN_FLASH)
	if (ddr_flash_init_flag[0]) {
		ret = ddr_flash_conf_store((void *)ddr_storage);
		if (!ret) {
			ddr_flash_init_flag[0] = 0;
		}
	}
#endif

	return 0;
error:
	ERROR("DDR init failed\n");
	return -1;
}
