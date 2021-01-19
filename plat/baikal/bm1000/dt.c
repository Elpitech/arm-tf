/*
 * Copyright (c) 2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <bm1000_private.h>
#include <common/debug.h>
#include <drivers/console.h>
#include <lib/psci/psci.h>
#include <libfdt.h>
#include <string.h>

static int append_psci_compatible(void *fdt, int offs, const char *str)
{
	return fdt_appendprop(fdt, offs, "compatible", str, strlen(str) + 1);
}

int dt_add_psci_node(void *fdt)
{
	int offs;

	if (fdt_path_offset(fdt, "/psci") >= 0) {
		WARN("PSCI Device Tree node already exists!\n");
		return 0;
	}

	offs = fdt_path_offset(fdt, "/");
	if (offs < 0)
		return -1;
	offs = fdt_add_subnode(fdt, offs, "psci");
	if (offs < 0)
		return -1;
	if (append_psci_compatible(fdt, offs, "arm,psci-1.0"))
		return -1;
	if (append_psci_compatible(fdt, offs, "arm,psci-0.2"))
		return -1;
	if (append_psci_compatible(fdt, offs, "arm,psci"))
		return -1;
	if (fdt_setprop_string(fdt, offs, "method", "smc"))
		return -1;
	if (fdt_setprop_u32(fdt, offs, "cpu_suspend", PSCI_CPU_SUSPEND_AARCH64))
		return -1;
	if (fdt_setprop_u32(fdt, offs, "cpu_off", PSCI_CPU_OFF))
		return -1;
	if (fdt_setprop_u32(fdt, offs, "cpu_on", PSCI_CPU_ON_AARCH64))
		return -1;
	if (fdt_setprop_u32(fdt, offs, "sys_poweroff", PSCI_SYSTEM_OFF))
		return -1;
	if (fdt_setprop_u32(fdt, offs, "sys_reset", PSCI_SYSTEM_RESET))
		return -1;
	return 0;
}

static int check_node_compat_prefix(void *fdt, int offs, const char *prefix)
{
	const size_t prefix_len = strlen(prefix);
	size_t l;
	int plen;
	const char *prop;

	prop = fdt_getprop(fdt, offs, "compatible", &plen);
	if (!prop)
		return -1;

	while (plen > 0) {
		if (memcmp(prop, prefix, prefix_len) == 0)
			return 0; /* match */

		l = strlen(prop) + 1;
		prop += l;
		plen -= l;
	}

	return -1;
}

int dt_add_psci_cpu_enable_methods(void *fdt)
{
	int offs = 0;

	while (1) {
		offs = fdt_next_node(fdt, offs, NULL);
		if (offs < 0)
			break;
		if (fdt_getprop(fdt, offs, "enable-method", NULL))
			continue; /* already set */
		if (check_node_compat_prefix(fdt, offs, "arm,cortex-a"))
			continue; /* no compatible */
		if (fdt_setprop_string(fdt, offs, "enable-method", "psci"))
			return -1;
		/* Need to restart scanning as offsets may have changed */
		offs = 0;
	}
	return 0;
}

int dt_update_memory(void *fdt, const unsigned long long rangedescs[][2], const unsigned rangenum)
{
	int err;
	int memoff;
	uint64_t memregs[3][2];
	unsigned range;

	if (rangenum < 1 || rangenum > sizeof (memregs) / sizeof (memregs[0])) {
		ERROR("FDT: invalid number of memory ranges\n");
		return -1;
	}

	memoff = fdt_path_offset(fdt, "/memory@80000000");
	if (memoff == -FDT_ERR_NOTFOUND) {
		memoff = fdt_add_subnode(fdt, 0, "memory@80000000");
		if (memoff < 0) {
			ERROR("FDT: unable to add a memory subnode\n");
			return memoff;
		}

		err = fdt_setprop_string(fdt, memoff, "device_type", "memory");
		if (err) {
			ERROR("FDT: unable to set device_type of a memory subnode\n");
			return err;
		}
	}

	for (range = 0; range < rangenum; ++range) {
		memregs[range][0] = cpu_to_fdt64(rangedescs[range][0]);
		memregs[range][1] = cpu_to_fdt64(rangedescs[range][1]);
	}

	err = fdt_setprop(fdt, memoff, "reg", memregs, sizeof (memregs[0]) * rangenum);
	if (err) {
		ERROR("FDT: unable to set reg property of a memory subnode\n");
		return err;
	}

	return 0;
}
