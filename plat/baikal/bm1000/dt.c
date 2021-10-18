/*
 * Copyright (c) 2018-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <bm1000_private.h>
#include <common/debug.h>
#include <libfdt.h>
#include <platform_def.h>

int fdt_update_memory(void *fdt,
		      const uint64_t region_descs[][2],
		      const unsigned region_num)
{
	int err;
	int memoff;
	uint64_t memregs[3][2];
	unsigned region;

	if (region_num < 1 || region_num > ARRAY_SIZE(memregs)) {
		ERROR("%s: invalid number of memory regions\n", __func__);
		return -1;
	}

	memoff = fdt_path_offset(fdt, "/memory@80000000");
	if (memoff == -FDT_ERR_NOTFOUND) {
		memoff = fdt_add_subnode(fdt, 0, "memory@80000000");
		if (memoff < 0) {
			ERROR("%s: unable to add a memory subnode\n", __func__);
			return memoff;
		}

		err = fdt_setprop_string(fdt, memoff, "device_type", "memory");
		if (err) {
			ERROR("%s: unable to set device_type of a memory subnode\n",
			      __func__);
			return err;
		}
	}

	for (region = 0; region < region_num; ++region) {
		memregs[region][0] = cpu_to_fdt64(region_descs[region][0]);
		memregs[region][1] = cpu_to_fdt64(region_descs[region][1]);
	}

	err = fdt_setprop(fdt, memoff, "reg", memregs,
			  sizeof(memregs[0]) * region_num);
	if (err) {
		ERROR("FDT: unable to set reg property of a memory subnode\n");
		return err;
	}

	return 0;
}

int fdt_memory_node_read(uint64_t region_descs[3][2])
{
	void *fdt = (void *)(uintptr_t)PLAT_BAIKAL_DT_BASE;
	int memoff;
	unsigned region;
	const uint64_t *prop;
	int proplen;
	int ret;

	for (region = 0; region < 3; ++region) {
		region_descs[region][0] = 0;
		region_descs[region][1] = 0;
	}

	ret = fdt_open_into(fdt, fdt, PLAT_BAIKAL_DT_MAX_SIZE);
	if (ret < 0) {
		ERROR("%s: unable to open FDT (%d)\n", __func__, ret);
		return ret;
	}

	memoff = fdt_path_offset(fdt, "/memory@80000000");
	if (memoff == -FDT_ERR_NOTFOUND) {
		ERROR("%s: node is not found\n", __func__);
		return memoff;
	}

	prop = fdt_getprop(fdt, memoff, "reg", &proplen);
	if (prop == NULL) {
		ERROR("%s: reg is not found\n", __func__);
		return -1;
	} else if (!proplen || (proplen % 16) || proplen > 48) {
		ERROR("%s: incorrect 'reg' property length\n", __func__);
		return -1;
	}

	for (region = 0; region < proplen / 16; ++region) {
		region_descs[region][0] = fdt64_to_cpu(prop[2 * region + 0]);
		region_descs[region][1] = fdt64_to_cpu(prop[2 * region + 1]);
	}

	return 0;
}
