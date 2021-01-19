/*
 * Copyright (c) 2020, Baikal Electronics JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ARMTF_BAIKAL_I2C_H
#define ARMTF_BAIKAL_I2C_H

unsigned i2c_txrx(const unsigned bus, const unsigned addr, const void *const txbuf, const unsigned txbufsize, void *const rxbuf, const unsigned rxbufsize);

#endif // ARMTF_BAIKAL_I2C_H
