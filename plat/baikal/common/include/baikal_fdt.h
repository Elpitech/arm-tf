/*
 * Copyright (c) 2021-2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BAIKAL_FDT_H
#define BAIKAL_FDT_H

bool fdt_node_is_enabled(const void *fdt, int nodeoffset);
void fdt_memory_node_set(void *fdt,
			 const uint64_t region_descs[][2],
			 unsigned int region_num);

#endif /* BAIKAL_FDT_H */
