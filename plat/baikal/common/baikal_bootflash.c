/*
 * Copyright (c) 2022-2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <stdint.h>

#include <baikal_bootflash.h>
# include <baikal_def.h>
#if defined(BAIKAL_SCP_FLASH)
# include <bm1000_scp_flash.h>
#else
# include <spi_nor_flash.h>
#endif

int bootflash_erase(const uint32_t addr, const size_t size)
{
#if defined(BAIKAL_SCP_FLASH)
	return scp_flash_erase(addr, size);
#else
	return spi_nor_flash_erase(BAIKAL_BOOTFLASH_SPI_SS_LINE, addr, size);
#endif
}

int bootflash_init(void)
{
#if defined(BAIKAL_SCP_FLASH)
	return 0;
#else
	return spi_nor_flash_init(BAIKAL_BOOTFLASH_SPI_SS_LINE);
#endif
}

int bootflash_lock(const int lock)
{
#if defined(BAIKAL_SCP_FLASH)
	return scp_flash_lock(lock);
#else
	return 0;
#endif
}

int bootflash_read(const uint32_t addr, void *const buf, const size_t size)
{
#if defined(BAIKAL_SCP_FLASH)
	return scp_flash_read(addr, buf, size);
#else
	return spi_nor_flash_read(BAIKAL_BOOTFLASH_SPI_SS_LINE, addr, buf, size);
#endif
}

int bootflash_write(const uint32_t addr, void *const data, const size_t size)
{
#if defined(BAIKAL_SCP_FLASH)
	return scp_flash_write(addr, data, size);
#else
	return spi_nor_flash_write(BAIKAL_BOOTFLASH_SPI_SS_LINE, addr, data, size);
#endif
}
