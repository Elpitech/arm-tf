/*
 * Copyright (c) 2018-2020, Baikal Electronics JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <assert.h>
#include <baikal_scp.h>
#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <errno.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>
#include <spi_dw.h>
#include <string.h>

static int eeprom_read (int port, int line, uint32_t adr, uint8_t *rx, size_t size) {
	volatile struct ScpService *s_scp = (volatile struct ScpService *)SCP_SERVICE_BASE;
	int	ok = scp_cmd('R', adr, size);
	if(ok == 0 && s_scp->st == 'G') {
		memcpy(rx, (uint32_t*)&s_scp->buff[0], size);
	}
	return ok;
}

int dw_spi_read (int port, int line, uint32_t adr, void* data, size_t size)
{
    int err;
    int part;
    uint8_t *pdata = data;
    while (size) {

        part = (size > SPI_MAX_READ)? SPI_MAX_READ : size;

        err = eeprom_read (port, line, adr, pdata, part);
        if(err){
            return err;
        }
        adr   += part;
        pdata += part;
        size  -= part;
    }
    return 0;
}

static int eeprom_erase (int port, int line, uint32_t adr, size_t size) {
	volatile struct ScpService *s_scp = (volatile struct ScpService *)SCP_SERVICE_BASE;
	int	ok = scp_cmd('E', adr, size);
	if(ok == 0 && s_scp->st == 'G') {
		return 0;
	}
	return ok;
}

int  dw_spi_erase   (int port, int line, uint32_t adr, size_t size, size_t sectore_size)
{
    int err;
    int part;

    while (size) {

        part = (size > SPI_MAX_READ)? SPI_MAX_READ : size;

        err = eeprom_erase (port, line, adr, part);
        if(err){
            return err;
        }
        adr   += part;
        size  -= part;
    }
    return 0;
}

static int eeprom_write (int port, int line, uint32_t adr, uint8_t *wx, size_t size) {
	volatile struct ScpService *s_scp = (volatile struct ScpService *)SCP_SERVICE_BASE;
	int	ok;
	memcpy((uint32_t*)&s_scp->buff[0], wx, size);
	ok = scp_cmd('W', adr, size);
	if(ok == 0 && s_scp->st == 'G') {
		return 0;
	}
	return ok;
}

int  dw_spi_write   (int port, int line, uint32_t adr, void *data, size_t size)
{
    int err;
    int part;
    uint8_t *pdata = data;
    while (size) {

        part = (size > SPI_MAX_READ)? SPI_MAX_READ : size;

        err = eeprom_write (port, line, adr, pdata, part);
        if(err){
            return err;
        }
        adr   += part;
        pdata += part;
        size  -= part;
    }
    return 0;
}

int dw_spi_4byte (int port, int line)
{
	return 0;
}

int dw_spi_3byte (int port, int line)
{
	return 0;
}

const struct flash_info tmp = {.sector_size = 64, .n_sectors = 512};
const struct flash_info *dw_get_info (int port, int line)
{
    return &tmp;
}

void dw_spi_init (int port, int line)
{
	scp_cmd('T', 0, 0); // disable trace
}
