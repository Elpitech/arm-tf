/*
 * Copyright (c) 2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

#include <common/debug.h>
#include <drivers/delay_timer.h>

#include <baikal_def.h>
#include <dbm_bmc.h>
#include <dw_i2c.h>

#define DBM_BMC_I2C_BUS			MMAVLSP_I2C1_BASE
#define DBM_BMC_I2C_ADDR		0x23

#define DBM_BMC_CMD_RST			0x1
#define DBM_BMC_CMD_EMU_PWR_OFF		0x2
#define DBM_BMC_CMD_PWR_OFF		0x3
#define DBM_BMC_CMD_WAKE_SRC		0x4

#define DBM_BMC_STATUS_EMPTY		0x80

static uint8_t dbm_bmc_get_status(void)
{
	uint8_t request;

	i2c_txrx(DBM_BMC_I2C_BUS, BAIKAL_I2C_ICLK_FREQ,
		 DBM_BMC_I2C_ADDR, NULL, 0, &request, sizeof(request));

	return request;
}

int dbm_bmc_pwr_get_wake_src(void)
{
	uint8_t request[5];
	uint64_t timeout;

	timeout = timeout_init_us(100000);
	for (;;) {
		if (dbm_bmc_get_status() == DBM_BMC_STATUS_EMPTY) {
			request[0] = DBM_BMC_CMD_WAKE_SRC;
			i2c_txrx(DBM_BMC_I2C_BUS, BAIKAL_I2C_ICLK_FREQ,
				 DBM_BMC_I2C_ADDR, &request, sizeof(request[0]), NULL, 0);
			break;
		}

		if (timeout_elapsed(timeout)) {
			return -1;
		}
	}

	timeout = timeout_init_us(100000);
	for (;;) {
		i2c_txrx(DBM_BMC_I2C_BUS, BAIKAL_I2C_ICLK_FREQ,
			 DBM_BMC_I2C_ADDR, NULL, 0, &request, sizeof(request));
		if (request[0] == 4) {
			return request[1]	|
			       request[2] << 8	|
			       request[3] << 16	|
			       request[4] << 24;
		}

		if (timeout_elapsed(timeout)) {
			return -1;
		}
	}
}

static void dbm_bmc_send_request(const uint8_t cmd, const uint32_t ms, const char * const str)
{
	const uint8_t request[] = {
		cmd,
		ms & 0xff,
		(ms >> 8) & 0xff,
		(ms >> 16) & 0xff,
		(ms >> 24) & 0xff
	};
	uint64_t timeout;

	INFO("BMC: %s\n", str);
	timeout = timeout_init_us(100000);
	for (;;) {
		if (dbm_bmc_get_status() == DBM_BMC_STATUS_EMPTY) {
			i2c_txrx(DBM_BMC_I2C_BUS, BAIKAL_I2C_ICLK_FREQ,
				 DBM_BMC_I2C_ADDR, &request, sizeof(request), NULL, 0);

			mdelay(3000);
		}

		if (timeout_elapsed(timeout)) {
			break;
		}
	}

	ERROR("BMC: %s failed\n", str);
	panic();
}

void dbm_bmc_emu_pwr_off(const uint32_t ms)
{
	dbm_bmc_send_request(DBM_BMC_CMD_EMU_PWR_OFF, ms, "emulated power off");
}

void dbm_bmc_pwr_off(const uint32_t ms)
{
	dbm_bmc_send_request(DBM_BMC_CMD_PWR_OFF, ms, "power off");
}

void dbm_bmc_pwr_rst(const uint32_t ms)
{
	dbm_bmc_send_request(DBM_BMC_CMD_RST, ms, "power reset");
}
