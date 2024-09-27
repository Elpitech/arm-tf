/*
 * Copyright (c) 2020-2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BM1000_SMBUS_H
#define BM1000_SMBUS_H

enum smbus_sht {
	SMBUS_SHT_400KHZ = 0,
	SMBUS_SHT_100KHZ = 1
};

unsigned int smbus_txrx(uintptr_t base,
			unsigned int iclk,
			enum smbus_sht sht,
			unsigned int sclclk,
			unsigned int targetaddr,
			const void *txbuf,
			unsigned int txbufsize,
			void *rxbuf,
			unsigned int rxbufsize);

#endif /* BM1000_SMBUS_H */
