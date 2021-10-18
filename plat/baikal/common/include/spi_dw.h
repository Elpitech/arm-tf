/*
 * Copyright (c) 2018-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SPI_DW_H
#define SPI_DW_H

#include <stddef.h>
#include <stdint.h>

#define SPI_NOR_MAX_ID_LEN	3

struct flash_info {
	uint8_t id[SPI_NOR_MAX_ID_LEN];
	uint8_t sector_log2size;
	uint8_t sector_log2count;
};

void dw_spi_init (int line);
int  dw_spi_read (int line, uint32_t adr, void *data, size_t size);
int  dw_spi_erase(int line, uint32_t adr, size_t size, size_t sector_size);
int  dw_spi_write(int line, uint32_t adr, void *data, size_t size);
int  dw_spi_4byte(int line);
int  dw_spi_3byte(int line);
const struct flash_info *dw_spi_get_info(int line);

#endif /* SPI_DW_H */
