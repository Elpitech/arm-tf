/*
 * Copyright (c) 2020-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DW_I2C_H
#define DW_I2C_H

int i2c_txrx(const uintptr_t base,
	     const unsigned addr,
	     const void *const txbuf,
	     const unsigned txbufsize,
	     void *const rxbuf,
	     const unsigned rxbufsize);

#endif /* DW_I2C_H */
