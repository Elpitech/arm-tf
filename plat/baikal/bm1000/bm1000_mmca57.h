/*
 * Copyright (c) 2018-2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BM1000_MMCA57_H
#define BM1000_MMCA57_H

#include <stdint.h>

unsigned int mmca57_get_sclk_div(uintptr_t base);
void	     mmca57_enable_core(u_register_t mpidr);
void	     mmca57_reconf_sclken(uintptr_t base, unsigned int div);

#endif /* BM1000_MMCA57_H */
