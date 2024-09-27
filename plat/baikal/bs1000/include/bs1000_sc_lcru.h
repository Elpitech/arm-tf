/*
 * Copyright (c) 2021-2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BS1000_SC_LCRU_H
#define BS1000_SC_LCRU_H

#include <stdint.h>

int sc_lcru_clrbits(uintptr_t addr, uint32_t clr);
int sc_lcru_clrsetbits(uintptr_t addr, uint32_t clr, uint32_t set);
int sc_lcru_read(uintptr_t addr, uint32_t *val);
int sc_lcru_setbits(uintptr_t addr, uint32_t set);
int sc_lcru_write(uintptr_t addr, uint32_t val);

#endif /* BS1000_SC_LCRU_H */
