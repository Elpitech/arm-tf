/*
 * Copyright (c) 2019-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BM1000_PVT_H
#define BM1000_PVT_H

#include <stdint.h>

#define PVT_READ	0
#define PVT_WRITE	1

uint32_t pvt_read_reg(uint32_t pvt_id, uint32_t offset);
uint32_t pvt_write_reg(uint32_t pvt_id, uint32_t offset, uint32_t val);

#endif /* BM1000_PVT_H */
