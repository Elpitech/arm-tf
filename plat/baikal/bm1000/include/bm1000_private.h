/*
 * Copyright (c) 2018-2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BM1000_PRIVATE_H
#define BM1000_PRIVATE_H

#include <stdint.h>

unsigned int plat_baikal_calc_core_pos(u_register_t mpidr);

int fdt_memory_node_read(uint64_t region_descs[3][2]);
void dt_enable_mc_node(void *fdt, uintptr_t base);

void mmavlsp_init(void);
void mmavlsp_ns_access(void);
void mmcoresight_init(void);
void mmmali_init(void);
void mmpcie_init(void);
void mmusb_init(void);
void mmvdec_init(void);
void mmxgbe_init(void);
void mmxgbe_ns_access(void);

#endif /* BM1000_PRIVATE_H */
