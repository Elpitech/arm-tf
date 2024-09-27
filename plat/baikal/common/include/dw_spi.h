/*
 * Copyright (c) 2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DW_SPI_H
#define DW_SPI_H

#include <stddef.h>

#define DW_SPI_MAX_READ		UL(0x10000)

int dw_spi_eepromread(uintptr_t base,
		      unsigned int baudr,
		      unsigned int line,
		      void *txdata, size_t txsize,
		      void *rxbuff, size_t rxsize);

int dw_spi_tx(uintptr_t base,
	      unsigned int baudr,
	      unsigned int line,
	      const void *tx0data, size_t tx0size,
	      const void *tx1data, size_t tx1size);

#endif /* DW_SPI_H */
