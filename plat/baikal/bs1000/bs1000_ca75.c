/*
 * Copyright (c) 2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <lib/mmio.h>

#include <bs1000_def.h>

#include "bs1000_ca75.h"

#define CA75_GPR_RST_CTL_REG		0x20000
#define CA75_GPR_RST_CTL_CPUPORESET0	BIT(0)
#define CA75_GPR_RST_CTL_CORERESET0	BIT(4)
#define CA75_GPR_RST_CTL_ATRESET	BIT(8)
#define CA75_GPR_RST_CTL_GICRESET	BIT(9)
#define CA75_GPR_RST_CTL_PERIPHRESET	BIT(11)
#define CA75_GPR_RST_CTL_PRESET		BIT(12)
#define CA75_GPR_RST_CTL_SPORESET	BIT(13)
#define CA75_GPR_RST_CTL_SRESET		BIT(14)
#define CA75_GPR_RST_CTL_CRP_ARST	BIT(20)

static const uintptr_t ca75_bases[] = {
	CA75_0_BASE,
	CA75_1_BASE,
	CA75_2_BASE,
	CA75_3_BASE,
	CA75_4_BASE,
	CA75_5_BASE,
	CA75_6_BASE,
	CA75_7_BASE,
	CA75_8_BASE,
	CA75_9_BASE,
	CA75_10_BASE,
	CA75_11_BASE
};
CASSERT(ARRAY_SIZE(ca75_bases) == PLATFORM_CLUSTER_COUNT, assert_ca75_bases_size);

void ca75_enable_core(const u_register_t mpidr)
{
	const unsigned cluster = (mpidr >> MPIDR_AFF2_SHIFT) & MPIDR_AFFLVL_MASK;
	const unsigned core    = (mpidr >> MPIDR_AFF1_SHIFT) & MPIDR_AFFLVL_MASK;

	assert(cluster < PLATFORM_CLUSTER_COUNT);
	assert(core < PLATFORM_MAX_CPUS_PER_CLUSTER);

	const uintptr_t base = ca75_bases[cluster];

	/* TODO: enable CA75 PLL */

	/*
	 * Deassert resets.
	 * N.B. Simultaneous deassertion of cluster/core resets is permitted.
	 */
	mmio_clrbits_32(base + CA75_GPR_RST_CTL_REG,
			       CA75_GPR_RST_CTL_ATRESET     |
			       CA75_GPR_RST_CTL_GICRESET    |
			       CA75_GPR_RST_CTL_PERIPHRESET |
			       CA75_GPR_RST_CTL_PRESET      |
			       CA75_GPR_RST_CTL_SPORESET    |
			       CA75_GPR_RST_CTL_SRESET      |
			       CA75_GPR_RST_CTL_CRP_ARST    |
			      (CA75_GPR_RST_CTL_CORERESET0  |
			       CA75_GPR_RST_CTL_CPUPORESET0) << core);
}
