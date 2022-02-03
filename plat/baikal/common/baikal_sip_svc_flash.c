/*
 * Copyright (c) 2020-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include <cdefs.h>
#include <common/debug.h>

#include <baikal_def.h>
#include <baikal_sip_svc_flash.h>
#include <spi_dw.h>

static int baikal_smc_flash_info(uint64_t *const data);
static int baikal_smc_flash_position(const unsigned pos);
static int baikal_smc_flash_pull(uint64_t *const data);
static int baikal_smc_flash_push(const uint64_t data0,
				 const uint64_t data1,
				 const uint64_t data2,
				 const uint64_t data3);

static uint8_t	flash_buf[1024] __aligned(8);
static unsigned	flash_buf_idx;
static unsigned	sector_count;
static unsigned	sector_size;

int baikal_smc_flash_handler(const uint32_t smc_fid,
			     const uint64_t x1,
			     const uint64_t x2,
			     const uint64_t x3,
			     const uint64_t x4,
			     uint64_t *data)
{
	switch (smc_fid) {
	case BAIKAL_SMC_FLASH_WRITE:
		return dw_spi_write(0, x1, flash_buf, x2);
	case BAIKAL_SMC_FLASH_READ:
		return dw_spi_read(0, x1, flash_buf, x2);
	case BAIKAL_SMC_FLASH_PUSH:
		return baikal_smc_flash_push(x1, x2, x3, x4);
	case BAIKAL_SMC_FLASH_PULL:
		return baikal_smc_flash_pull(data);
	case BAIKAL_SMC_FLASH_POSITION:
		return baikal_smc_flash_position(x1);
	case BAIKAL_SMC_FLASH_ERASE:
		return dw_spi_erase(0, x1, x2, sector_size);
	case BAIKAL_SMC_FLASH_INFO:
		return baikal_smc_flash_info(data);
	default:
		ERROR("%s: unknown smc_fid 0x%x\n", __func__, smc_fid);
		return -1;
	}
}

static int baikal_smc_flash_info(uint64_t *const data)
{
	if (!sector_count || !sector_size) {
		const struct flash_info *info;

		info = dw_spi_get_info(0);
		if (info == NULL) {
			return -1;
		}

		dw_spi_3byte(0);

#ifdef BAIKAL_FLASH_4BYTEADR
		if ((1 << info->sector_log2size) *
		    (1 << info->sector_log2count) > 16 * 1024 * 1024) {
			dw_spi_4byte(0);
		}
#endif
		sector_size  = 1 << info->sector_log2size;
		sector_count = 1 << info->sector_log2count;
	}

	data[0] = sector_size;
	data[1] = sector_count;
	return 0;
}

static int baikal_smc_flash_position(const unsigned pos)
{
	if (pos > sizeof(flash_buf) - 4 * sizeof(uint64_t)) {
		return -1;
	}

	flash_buf_idx = pos;
	return 0;
}

static int baikal_smc_flash_pull(uint64_t *const data)
{
	memcpy(data, &flash_buf[flash_buf_idx], 4 * sizeof(uint64_t));
	flash_buf_idx += 4 * sizeof(uint64_t);
	return 0;
}

static int baikal_smc_flash_push(const uint64_t data0,
				 const uint64_t data1,
				 const uint64_t data2,
				 const uint64_t data3)
{
	uint64_t *const buf = (void *)&flash_buf[flash_buf_idx];

	buf[0] = data0;
	buf[1] = data1;
	buf[2] = data2;
	buf[3] = data3;
	flash_buf_idx += 4 * sizeof(uint64_t);
	return 0;
}
