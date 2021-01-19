/*
 * Copyright (c) 2020, Baikal Electronics JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <baikal_sip_svc_flash.h>
#include <common/debug.h>
#include <spi_dw.h>

static uint8_t flash_buf[1024] __aligned(8);
static unsigned int flash_buf_idx;
static struct flash_sector_info flash_sector_info;
static bool flash_sector_info_valid;

int baikal_smc_flash_handler(uint32_t smc_fid,
			     u_register_t x1,
			     u_register_t x2,
			     u_register_t x3,
			     u_register_t x4)
{
	switch (smc_fid) {
	case BAIKAL_SMC_FLASH_WRITE:
		return dw_spi_write(0, 0, x1, flash_buf, x2);
	case BAIKAL_SMC_FLASH_READ:
		return dw_spi_read(0, 0, x1, flash_buf, x2);
	case BAIKAL_SMC_FLASH_PUSH:
	{
		uint64_t *ptr;

		if (flash_buf_idx > (sizeof (flash_buf) - 4 * sizeof (u_register_t))) {
			return -1;
		}

		ptr = (uint64_t *)&flash_buf[flash_buf_idx];
		*ptr++ = x1;
		*ptr++ = x2;
		*ptr++ = x3;
		*ptr   = x4;
		flash_buf_idx += 4 * sizeof (u_register_t);
		return 0;
	}
	case BAIKAL_SMC_FLASH_POSITION:
		if (x1 > (sizeof (flash_buf) - 4 * sizeof (u_register_t))) {
			return -1;
		}

		flash_buf_idx = x1;
		return 0;
	case BAIKAL_SMC_FLASH_ERASE:
	case BAIKAL_SMC_FLASH_INFO:
		if (!flash_sector_info_valid) {
			const struct flash_info* info;
			info = dw_get_info(0, 0);
			if (info == NULL) {
				return -1;
			}

			flash_sector_info.sector_size = info->sector_size;
			flash_sector_info.sector_num  = info->n_sectors;
			flash_sector_info_valid = true;
		}

		if (smc_fid == BAIKAL_SMC_FLASH_ERASE) {
			return dw_spi_erase(0, 0, x1, x2, flash_sector_info.sector_size);
		}

		return 0;
	default:
		ERROR("%s: unknown smc_fid 0x%x\n", __func__, smc_fid);
		return -1;
	}
}

const void* baikal_smc_flash_get_next_data(void)
{
	if (flash_buf_idx <= (sizeof (flash_buf) - 4 * sizeof (u_register_t))) {
		const void* ptr = &flash_buf[flash_buf_idx];
		flash_buf_idx += 4 * sizeof (u_register_t);
		return ptr;
	}

	return NULL;
}

const struct flash_sector_info* baikal_smc_flash_get_sector_info(void)
{
	if (flash_sector_info_valid) {
		return &flash_sector_info;
	}

	return NULL;
}
