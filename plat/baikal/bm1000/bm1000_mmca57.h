/*
 * Copyright (c) 2018-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BM1000_MMCA57_H
#define BM1000_MMCA57_H

#include <stdint.h>

void mmca57_enable_core(const u_register_t mpidr);
void mmca57_reconf_sclken(const uintptr_t base, const unsigned div);

#endif /* BM1000_MMCA57_H */
