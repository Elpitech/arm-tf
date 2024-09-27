/*
 * Copyright (c) 2020-2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DW_I2C_H
#define DW_I2C_H

int i2c_txrx(uintptr_t base,
	     unsigned int iclk,
	     unsigned int targetaddr,
	     const void *txbuf,
	     unsigned int txbufsize,
	     void *rxbuf,
	     unsigned int rxbufsize);

#endif /* DW_I2C_H */
