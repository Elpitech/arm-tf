/*
 * Copyright (c) 2020, Baikal Electronics JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <drivers/arm/gicv3.h>
#include <plat/common/platform.h>
#include <platform_def.h>

/* The GICv3 driver only needs to be initialized in EL3 */
static uintptr_t baikal_rdistif_base_addrs[PLATFORM_CORE_COUNT];

static const interrupt_prop_t baikal_interrupt_props[] = {
	INTR_PROP_DESC(BAIKAL_IRQ_SEC_SGI_0, GIC_HIGHEST_SEC_PRIORITY,
		       INTR_GROUP1S, GIC_INTR_CFG_EDGE),
	INTR_PROP_DESC(BAIKAL_IRQ_SEC_SGI_1, GIC_HIGHEST_SEC_PRIORITY,
		       INTR_GROUP1S, GIC_INTR_CFG_EDGE),
	INTR_PROP_DESC(BAIKAL_IRQ_SEC_SGI_2, GIC_HIGHEST_SEC_PRIORITY,
		       INTR_GROUP1S, GIC_INTR_CFG_EDGE),
	INTR_PROP_DESC(BAIKAL_IRQ_SEC_SGI_3, GIC_HIGHEST_SEC_PRIORITY,
		       INTR_GROUP1S, GIC_INTR_CFG_EDGE),
	INTR_PROP_DESC(BAIKAL_IRQ_SEC_SGI_4, GIC_HIGHEST_SEC_PRIORITY,
		       INTR_GROUP1S, GIC_INTR_CFG_EDGE),
	INTR_PROP_DESC(BAIKAL_IRQ_SEC_SGI_5, GIC_HIGHEST_SEC_PRIORITY,
		       INTR_GROUP1S, GIC_INTR_CFG_EDGE),
	INTR_PROP_DESC(BAIKAL_IRQ_SEC_SGI_6, GIC_HIGHEST_SEC_PRIORITY,
		       INTR_GROUP1S, GIC_INTR_CFG_EDGE),
	INTR_PROP_DESC(BAIKAL_IRQ_SEC_SGI_7, GIC_HIGHEST_SEC_PRIORITY,
		       INTR_GROUP1S, GIC_INTR_CFG_EDGE)
};

static unsigned int baikal_mpidr_to_core_pos(u_register_t mpidr)
{
	return plat_core_pos_by_mpidr(mpidr);
}

static const struct gicv3_driver_data baikal_gic_data = {
	.gicd_base = GICD_BASE,
	.gicr_base = GICR_BASE,
	.interrupt_props = baikal_interrupt_props,
	.interrupt_props_num = ARRAY_SIZE(baikal_interrupt_props),
	.rdistif_num = PLATFORM_CORE_COUNT,
	.rdistif_base_addrs = baikal_rdistif_base_addrs,
	.mpidr_to_core_pos = baikal_mpidr_to_core_pos
};

void baikal_gic_driver_init(void)
{
	gicv3_driver_init(&baikal_gic_data);
}

void baikal_gic_init(void)
{
	gicv3_distif_init();
	gicv3_rdistif_init(plat_my_core_pos());
	gicv3_cpuif_enable(plat_my_core_pos());
}
