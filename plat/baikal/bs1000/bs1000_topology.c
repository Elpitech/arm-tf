/*
 * Copyright (c) 2020-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/utils_def.h>

#include <bs1000_private.h>
#include <platform_def.h>

static const unsigned char power_domain_tree_desc[] = {
	PLATFORM_CLUSTER_COUNT,
	PLATFORM_CORE_COUNT_PER_CLUSTER,
	PLATFORM_CORE_COUNT_PER_CLUSTER,
	PLATFORM_CORE_COUNT_PER_CLUSTER,
	PLATFORM_CORE_COUNT_PER_CLUSTER,
	PLATFORM_CORE_COUNT_PER_CLUSTER,
	PLATFORM_CORE_COUNT_PER_CLUSTER,
	PLATFORM_CORE_COUNT_PER_CLUSTER,
	PLATFORM_CORE_COUNT_PER_CLUSTER,
	PLATFORM_CORE_COUNT_PER_CLUSTER,
	PLATFORM_CORE_COUNT_PER_CLUSTER,
	PLATFORM_CORE_COUNT_PER_CLUSTER,
	PLATFORM_CORE_COUNT_PER_CLUSTER
};
CASSERT(ARRAY_SIZE(power_domain_tree_desc) == (PLATFORM_CLUSTER_COUNT + 1),
	assert_power_domain_tree_desc_size);

const unsigned char *plat_get_power_domain_tree_desc(void)
{
	return power_domain_tree_desc;
}

/*
 * This function implements a part of the critical interface between the psci
 * generic layer and the platform that allows the former to query the platform
 * to convert an MPIDR to a unique linear index. An error code (-1) is returned
 * in case the MPIDR is invalid.
 */
int plat_core_pos_by_mpidr(u_register_t mpidr)
{
	unsigned int cluster_id, cpu_id, thread_id;

	cluster_id = MPIDR_AFFLVL2_VAL(mpidr);
	cpu_id	   = MPIDR_AFFLVL1_VAL(mpidr);
	thread_id  = MPIDR_AFFLVL0_VAL(mpidr);

	if (cluster_id >= PLATFORM_CLUSTER_COUNT) {
		return -1;
	}

	if (cpu_id >= PLATFORM_MAX_CPUS_PER_CLUSTER) {
		return -1;
	}

	if (thread_id) {
		return -1;
	}

	return plat_baikal_calc_core_pos(mpidr);
}
