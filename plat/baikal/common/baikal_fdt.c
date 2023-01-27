/*
 * Copyright (c) 2021-2022, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <common/debug.h>
#include <libfdt.h>

#include <baikal_fdt.h>

bool fdt_device_is_enabled(const void *fdt, const int nodeoffset)
{
	int statlen;
	const char *status;

	/*
	 * Lack of a status property should be treated as if the property
	 * existed with the value of "okay" (see "Devicetree Specification").
	 * If the status property is present, check whether it is set to "ok"
	 * or "okay". Anything else is treated as "disabled".
	 */
	status = fdt_getprop(fdt, nodeoffset, "status", &statlen);
	if (!status) {
		return true;
	}

	if (statlen > 0) {
		if (!strcmp(status, "ok") || !strcmp(status, "okay")) {
			return true;
		}
	}

	return false;
}

void fdt_memory_node_set(void *fdt,
			 const uint64_t region_descs[][2],
			 const unsigned region_num)
{
	int err;
	uint64_t memregs[4][2];
	int node;
	unsigned region;

	assert(fdt != NULL);
	assert(region_num >= 1 && region_num <= ARRAY_SIZE(memregs));

	node = fdt_path_offset(fdt, "/memory@80000000");
	if (node == -FDT_ERR_NOTFOUND) {
		node = fdt_add_subnode(fdt, 0, "memory@80000000");
		if (node < 0) {
			ERROR("%s: unable to add memory subnode\n", __func__);
			return;
		}

		err = fdt_setprop_string(fdt, node, "device_type", "memory");
		if (err) {
			ERROR("%s: unable to set 'device_type' of memory subnode\n", __func__);
			return;
		}
	}

	for (region = 0; region < region_num; ++region) {
		memregs[region][0] = cpu_to_fdt64(region_descs[region][0]);
		memregs[region][1] = cpu_to_fdt64(region_descs[region][1]);
	}

	err = fdt_setprop(fdt, node, "reg", memregs, sizeof(memregs[0]) * region_num);
	if (err) {
		ERROR("%s: unable to set 'reg' property of memory subnode\n", __func__);
		return;
	}
}
