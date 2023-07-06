/*
 * Copyright (c) 2022, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BS1000_DIMM_SPD_H
#define BS1000_DIMM_SPD_H

const void *baikal_dimm_spd_get(const unsigned int dimm_idx);
void	    baikal_dimm_spd_read(void);

#endif /* BS1000_DIMM_SPD_H */
