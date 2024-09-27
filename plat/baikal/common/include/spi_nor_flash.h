/*
 * Copyright (c) 2018-2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SPI_NOR_FLASH_H
#define SPI_NOR_FLASH_H

#include <stddef.h>
#include <stdint.h>

int spi_nor_flash_init(int line);
int spi_nor_flash_erase(int line, uint32_t addr, size_t size);
int spi_nor_flash_read(int line, uint32_t addr, void *data, size_t size);
int spi_nor_flash_write(int line, uint32_t addr, void *data, size_t size);

#endif /* SPI_NOR_FLASH_H */
