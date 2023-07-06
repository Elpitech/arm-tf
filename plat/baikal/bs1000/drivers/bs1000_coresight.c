/*
 * Copyright (c) 2022, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <lib/mmio.h>

#include <platform_def.h>
#include <bs1000_def.h>
#include <bs1000_cmu.h>
#include <bs1000_scp_lcru.h>
#include <bs1000_coresight.h>

#define CA75_N_BASE(n)			(CA75_BASE + ((n) << 26))
#define CA75_NIC_CFG_GPV_BASE(n)	(CA75_N_BASE(n) + 0x200000)
#define CA75_NIC_CFG_CORESIGHT		0x0c

#define CORESIGHT_TSGEN_TS_BASE		(CORESIGHT_CFG_BASE + 0x20000)
#define TSGEN_CNTCR			0x0
#define TSGEN_CNTCR_EN			BIT(0)

void bs1000_coresight_init(void)
{
	int i;

	/* Enable non-secure access */
	mmio_write_32(NIC_SC_CFG_CORESIGHT_APB,	NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_SC_CFG_CORESIGHT_STM,	NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_SC_CFG_CA75_0,	NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_SC_CFG_CA75_1_5,	NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_SC_CFG_CA75_6,	NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_SC_CFG_CA75_7_11,	NIC_GPV_REGIONSEC_NONSECURE);
	for (i = 0; i < PLATFORM_CLUSTER_COUNT; i++)
		mmio_write_32(CA75_NIC_CFG_GPV_BASE(i) + CA75_NIC_CFG_CORESIGHT,
			      NIC_GPV_REGIONSEC_NONSECURE);

	/* Enable TSGEN_TS */
	mmio_setbits_32(CORESIGHT_TSGEN_TS_BASE + TSGEN_CNTCR, TSGEN_CNTCR_EN);
}
