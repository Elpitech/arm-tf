/*
 * Copyright (c) 2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef XORSHIFT_H
#define XORSHIFT_H

#define XORSHIFT64(val) do {	\
	val ^= val << 13;	\
	val ^= val >> 7;	\
	val ^= val << 17;	\
	} while (0)

#endif /* XORSHIFT_H */
