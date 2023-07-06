/*
 * Copyright (c) 2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BS1000_SCP_LCRU_H
#define BS1000_SCP_LCRU_H

#include <stdint.h>

int scp_lcru_clrbits(const uint32_t addr, const uint32_t clr);
int scp_lcru_clrsetbits(const uint32_t addr, const uint32_t clr, const uint32_t set);
int scp_lcru_read(const uint32_t addr, uint32_t *const val);
int scp_lcru_setbits(const uint32_t addr, const uint32_t set);
int scp_lcru_write(const uint32_t addr, const uint32_t val);

#endif /* BS1000_SCP_LCRU_H */
