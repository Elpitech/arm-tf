/*
 * Copyright (c) 2020, Baikal Electronics JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BAIKAL_SMBUS_H
#define BAIKAL_SMBUS_H

unsigned smbus_txrx(const unsigned bus, const unsigned addr, const void *const txbuf, const unsigned txbufsize, void *const rxbuf, const unsigned rxbufsize);

#endif // BAIKAL_SMBUS_H
