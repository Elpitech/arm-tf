/*
 * Copyright (c) 2018-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <baikal_scp.h>
#include <common/debug.h>
#include <errno.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>
#include <platform_def.h>
#include <spi_dw.h>
#include <string.h>

#define SPI_MAX_READ	UL(1024)

static int eeprom_read(const uint32_t adr, uint8_t *rx, const size_t size)
{
	if (scp_cmd('R', adr, size)) {
		return -1;
	}

	memcpy(rx, scp_buf(), size);
	return 0;
}

static int eeprom_erase(const uint32_t adr, const size_t size)
{
	return scp_cmd('E', adr, size);
}

static int eeprom_write(const uint32_t adr, uint8_t *wx, const size_t size)
{
	memcpy(scp_buf(), wx, size);
	return scp_cmd('W', adr, size);
}

/* Wrappers */
int dw_spi_read(int line, uint32_t adr, void *data, size_t size)
{
	int err;
	int part;
	uint8_t *pdata = data;

	while (size) {
		part = MIN(size, SPI_MAX_READ);
		err = eeprom_read(adr, pdata, part);

		if (err) {
			return err;
		}

		adr   += part;
		pdata += part;
		size  -= part;
	}

	return 0;
}

int dw_spi_erase(int line, uint32_t adr, size_t size, size_t sector_size)
{
	int err;
	int part;

	while (size) {
		part = MIN(size, SPI_MAX_READ);
		err = eeprom_erase(adr, part);

		if (err) {
			return err;
		}

		adr   += part;
		size  -= part;
	}

	return 0;
}

int dw_spi_write(int line, uint32_t adr, void *data, size_t size)
{
	int err;
	int part;
	uint8_t *pdata = data;

	while (size) {
		part = MIN(size, SPI_MAX_READ);
		err = eeprom_write(adr, pdata, part);

		if (err) {
			return err;
		}

		adr   += part;
		pdata += part;
		size  -= part;
	}

	return 0;
}

int dw_spi_4byte(int line)
{
	return 0;
}

int dw_spi_3byte(int line)
{
	return 0;
}

const struct flash_info *dw_spi_get_info(int line)
{
	static const struct flash_info tmp = {
	    .sector_log2size  = 16,
	    .sector_log2count = 8
	};

	return &tmp;
}

void dw_spi_init(int line)
{
	scp_cmd('T', 0, 0); /* disable trace */
}
