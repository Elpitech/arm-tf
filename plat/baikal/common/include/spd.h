/*
 * Copyright (c) 2020-2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SPD_H
#define SPD_H

unsigned int	   spd_get_baseconf_crc(const void *baseconf);
unsigned long long spd_get_baseconf_dimm_capacity(const void *baseconf);

#endif /* SPD_H */
