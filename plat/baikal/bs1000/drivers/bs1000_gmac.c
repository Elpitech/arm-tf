/*
 * Copyright (c) 2023, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <drivers/arm/smmu_v3.h>
#include <common/debug.h>
#include <lib/mmio.h>

#include <bs1000_def.h>
#include <bs1000_gmac.h>

#define SMMUV3_STE_BYPASS(ste, ma, mt, alloc, sh, ns, priv, inst)				\
	do {											\
		uint32_t *pe = (uint32_t *)(ste);						\
												\
		pe[0] =  SMMU_STE_0_V;								\
		pe[0] |= SMMU_STE_CONFIG_BYPASS_BYPASS << SMMU_STE_0_CONFIG_SHIFT;		\
		pe[3] =  ((ma) << SMMU_STE_3_MEMATTR_SHIFT) & SMMU_STE_3_MEMATTR_MASK;		\
		if ((mt) != 0)									\
			pe[3] |= SMMU_STE_3_MTCFG;						\
		pe[3] |= ((alloc) << SMMU_STE_3_ALLOCCFG_SHIFT) & SMMU_STE_3_ALLOCCFG_MASK;	\
		pe[3] |= ((sh) << SMMU_STE_3_SHCFG_SHIFT) & SMMU_STE_3_SHCFG_MASK;		\
		pe[3] |= ((ns) << SMMU_STE_3_NSCFG_SHIFT) & SMMU_STE_3_NSCFG_MASK;		\
		pe[3] |= ((priv) << SMMU_STE_3_PRIVCFG_SHIFT) & SMMU_STE_3_PRIVCFG_MASK;	\
		pe[3] |= ((inst) << SMMU_STE_3_INSTCFG_SHIFT) & SMMU_STE_3_INSTCFG_MASK;	\
	} while (false)

static void baikal_gmac_memattrs_smmu_fix(void)
{
	uint64_t base_val;
	uintptr_t ste;

	/* Allocate Command queue */
	base_val = BAIKAL_SCMM_SMMU_CMDQ_BASE | BAIKAL_SCMM_SMMU_CMDQ_LOG2SIZE;
	mmio_write_64(SMMU_BASE + SMMU_S_CMDQ_BASE, base_val);
	mmio_write_32(SMMU_BASE + SMMU_S_CMDQ_PROD, 0);
	mmio_write_32(SMMU_BASE + SMMU_S_CMDQ_CONS, 0);
	/* Allocate Event queue */
	base_val = BAIKAL_SCMM_SMMU_EVENTQ_BASE | BAIKAL_SCMM_SMMU_EVENTQ_LOG2SIZE;
	mmio_write_64(SMMU_BASE + SMMU_S_EVENTQ_BASE, base_val);
	mmio_write_32(SMMU_BASE + SMMU_S_EVENTQ_PROD, 0);
	mmio_write_32(SMMU_BASE + SMMU_S_EVENTQ_CONS, 0);
	/* Configure Stream table */
	mmio_write_32(SMMU_BASE + SMMU_S_STRTAB_BASE_CFG, BAIKAL_SCMM_SMMU_STRTAB_LOG2SIZE);
	mmio_write_64(SMMU_BASE + SMMU_S_STRTAB_BASE, BAIKAL_SCMM_SMMU_STRTAB_BASE);
	memset((void *)BAIKAL_SCMM_SMMU_STRTAB_BASE, 0, BAIKAL_SCMM_SMMU_STRTAB_SIZE);
	dsb();
	/* Initialize Command and Event queues */
	if (smmuv3_enable_queues(SMMU_BASE, true) != 0) {
		ERROR("SCMM SMMU initialization failed.\n");
		return;
	}
	/* Invalidate TLB's and configuration caches */
	if (smmuv3_invalidate_all(SMMU_BASE) != 0) {
		ERROR("SCMM SMMU invalidation failed.\n");
		return;
	}
	/* Create Stream Table Entries */
	ste = BAIKAL_SCMM_SMMU_STRTAB_BASE;
	SMMUV3_STE_BYPASS(ste, SMMU_STE_MEMATTR_IWB_OWB, SMMU_STE_MTCFG_REPLACE_MEMATTR,
			  SMMU_STE_ALLOCCFG_RW, SMMU_STE_SHCFG_OUTER, SMMU_STE_NSCFG_NSEC,
			  SMMU_STE_PRIVCFG_USE_INCOMING, SMMU_STE_INSTCFG_USE_INCOMING);
	memcpy((void *)BAIKAL_SCMM_SMMU_STRTAB_BASE + 10 * SMMU_STE_SIZE, (void *)ste, SMMU_STE_SIZE);
	ste = BAIKAL_SCMM_SMMU_STRTAB_BASE + SMMU_STE_SIZE;
	SMMUV3_STE_BYPASS(ste, 0, SMMU_STE_MTCFG_USE_INCOMING, SMMU_STE_ALLOCCFG_USE_INCOMING,
			  SMMU_STE_SHCFG_USE_INCOMING, SMMU_STE_NSCFG_USE_INCOMING,
			  SMMU_STE_PRIVCFG_USE_INCOMING, SMMU_STE_INSTCFG_USE_INCOMING);
	memcpy((void *)BAIKAL_SCMM_SMMU_STRTAB_BASE + 2 * SMMU_STE_SIZE, (void *)ste, SMMU_STE_SIZE);
	memcpy((void *)BAIKAL_SCMM_SMMU_STRTAB_BASE + 3 * SMMU_STE_SIZE, (void *)ste, SMMU_STE_SIZE);
	memcpy((void *)BAIKAL_SCMM_SMMU_STRTAB_BASE + 4 * SMMU_STE_SIZE, (void *)ste, SMMU_STE_SIZE);
	memcpy((void *)BAIKAL_SCMM_SMMU_STRTAB_BASE + 8 * SMMU_STE_SIZE, (void *)ste, SMMU_STE_SIZE);
	memcpy((void *)BAIKAL_SCMM_SMMU_STRTAB_BASE + 12 * SMMU_STE_SIZE, (void *)ste, SMMU_STE_SIZE);
	dsb();
	/* Enable SMMU */
	if (smmuv3_enable(SMMU_BASE, true) != 0) {
		ERROR("SCMM SMMU enable failed.\n");
	}
}

void bs1000_gmac_init(void)
{
	/*
	 * Due to hardware bug in BS-1000, GMAC's have wrong AxCACHE and AxPROT
	 * AXI bus transactions attributes. This results in GMAC's DMA operations
	 * are not cache-coherent. We use SMMU to set these attributes to the
	 * proper values.
	 */
	baikal_gmac_memattrs_smmu_fix();
}
