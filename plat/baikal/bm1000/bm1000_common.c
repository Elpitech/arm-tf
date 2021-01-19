/*
 * Copyright (c) 2018-2020, Baikal Electronics JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <assert.h>
#include <bm1000_private.h>
#include <common/bl_common.h>
#include <lib/mmio.h>
#include <lib/xlat_tables/xlat_tables_compat.h>
#include <platform_def.h>

#define MAP_DEVICE0	MAP_REGION_FLAT(DEVICE0_BASE,			\
					DEVICE0_SIZE,			\
					MT_DEVICE | MT_RW | MT_SECURE)

#ifdef DEVICE1_BASE
#define MAP_DEVICE1	MAP_REGION_FLAT(DEVICE1_BASE,			\
					DEVICE1_SIZE,			\
					MT_DEVICE | MT_RW | MT_SECURE)
#endif

#ifdef DEVICE2_BASE
#define MAP_DEVICE2	MAP_REGION_FLAT(DEVICE2_BASE,			\
					DEVICE2_SIZE,			\
					MT_DEVICE | MT_RW | MT_SECURE)
#endif

#define MAP_SHARED_RAM	MAP_REGION_FLAT(SHARED_SRAM_BASE,		\
					SHARED_SRAM_SIZE,		\
					MT_DEVICE  | MT_RW | MT_SECURE)

#define MAP_BL32_MEM	MAP_REGION_FLAT(BL32_MEM_BASE, BL32_MEM_SIZE,	\
					MT_MEMORY | MT_RW | MT_SECURE)

#define MAP_NS_DRAM0	MAP_REGION_FLAT(NS_DRAM0_BASE, NS_DRAM0_SIZE,	\
					MT_MEMORY | MT_RW | MT_NS)

#define MAP_NS_DRAM1	MAP_REGION_FLAT(NS_DRAM1_BASE, NS_DRAM1_SIZE,	\
					MT_MEMORY | MT_RW | MT_NS)

#define MAP_FRAMEBUFFER	MAP_REGION_FLAT(FB0_BASE, (3 * FB_SIZE),		\
					MT_MEMORY | MT_RW | MT_NS)

#define MAP_SEC_DRAM0	MAP_REGION_FLAT(SEC_DRAM0_BASE, SEC_DRAM0_SIZE,	\
					MT_MEMORY | MT_RW | MT_SECURE)

/*
 * Table of regions for various BL stages to map using the MMU.
 * This doesn't include TZRAM as the 'mem_layout' argument passed to
 * arm_configure_mmu_elx() will give the available subset of that,
 */
#ifdef IMAGE_BL1
static const mmap_region_t plat_baikal_mmap[] = {
	MAP_NS_DRAM0,
	MAP_SEC_DRAM0,
	MAP_NS_DRAM1,
	MAP_FRAMEBUFFER,
	MAP_SHARED_RAM,
	MAP_DEVICE0,
#ifdef MAP_DEVICE1
	MAP_DEVICE1,
#endif
#ifdef MAP_DEVICE2
	MAP_DEVICE2,
#endif
	{0}
};
void baikal_configure_mmu_bl1(unsigned long total_base,
			   unsigned long total_size,
			   unsigned long ro_start,
			   unsigned long ro_limit,
			   unsigned long coh_start,
			   unsigned long coh_limit)
	{
		mmap_add_region(total_base, total_base,
				total_size,
				MT_NON_CACHEABLE | MT_RW | MT_SECURE);
		mmap_add_region(ro_start, ro_start,
				ro_limit - ro_start,
				MT_NON_CACHEABLE | MT_RO | MT_EXECUTE |  MT_SECURE);
		mmap_add_region(coh_start, coh_start,
				coh_limit - coh_start,
				MT_MEMORY | MT_RW | MT_SECURE);
		mmap_add(plat_baikal_mmap);
		init_xlat_tables();
		enable_mmu_el3(0);
	}
#endif
#ifdef IMAGE_BL2
static const mmap_region_t plat_baikal_mmap[] = {
	MAP_NS_DRAM0,
	MAP_NS_DRAM1,
	MAP_SEC_DRAM0,
	MAP_DEVICE0,
#ifdef MAP_DEVICE1
	MAP_DEVICE1,
#endif
#ifdef MAP_DEVICE2
	MAP_DEVICE2,
#endif
	{0}
};
#endif
#ifdef IMAGE_BL31
static const mmap_region_t plat_baikal_mmap[] = {
	MAP_NS_DRAM0,
	MAP_FRAMEBUFFER,
	MAP_SHARED_RAM,
	MAP_DEVICE0,
#ifdef MAP_DEVICE1
	MAP_DEVICE1,
#endif
#ifdef MAP_DEVICE2
	MAP_DEVICE2,
#endif
	MAP_BL32_MEM,
	{0}
};
#endif

/*******************************************************************************
 * Macro generating the code for the function setting up the pagetables as per
 * the platform memory map & initialize the mmu, for the given exception level
 ******************************************************************************/

#define DEFINE_CONFIGURE_MMU_EL(_el)					\
	void baikal_configure_mmu_el##_el(unsigned long total_base,	\
				   unsigned long total_size,		\
				   unsigned long ro_start,		\
				   unsigned long ro_limit,		\
				   unsigned long coh_start,		\
				   unsigned long coh_limit)		\
	{								\
		mmap_add_region(total_base, total_base,			\
				total_size,				\
				MT_MEMORY | MT_RW | MT_SECURE);		\
		mmap_add_region(ro_start, ro_start,			\
				ro_limit - ro_start,			\
				MT_NON_CACHEABLE | MT_RO | MT_EXECUTE |  MT_SECURE);	 	\
		mmap_add_region(coh_start, coh_start,			\
				coh_limit - coh_start,			\
				MT_NON_CACHEABLE | MT_RW | MT_SECURE);		\
		mmap_add(plat_baikal_mmap);				\
		init_xlat_tables();					\
									\
		enable_mmu_el##_el(0);			\
	}

//		MT_MEMORY | MT_RW | MT_SECURE);
//
//		enable_mmu_el##_el(DISABLE_DCACHE);

/* Define EL1 and EL3 variants of the function initialising the MMU */
DEFINE_CONFIGURE_MMU_EL(1)
DEFINE_CONFIGURE_MMU_EL(3)

void pll_on(uintptr_t cmu_base, const PllCtlInitValues *const pllinit, const char *err_msg)
{
	uint32_t ctl0 = 0x60000000, ctl6 = 0x2, ctl4;

	if (((mmio_read_32(cmu_base + LCRU_CTL0) & LCRU_CTL0_LOCK) == LCRU_CTL0_LOCK) &&
	    ((mmio_read_32(cmu_base + LCRU_CTL6) & LCRU_CTL6_SWEN) == LCRU_CTL6_SWEN)) {
		WARN("PLL 0x%lx is already enabled, skipped\n", cmu_base);
		return;
	}

	ctl6 = mmio_read_32(cmu_base + LCRU_CTL6);
	ctl6 &= ~LCRU_CTL6_SWEN;			// Disable
	mmio_write_32(cmu_base + LCRU_CTL6, ctl6);

	ctl0 = mmio_read_32(cmu_base + LCRU_CTL0);
	ctl0 |= LCRU_CTL0_CTRL_EN;
	mmio_write_32(cmu_base + LCRU_CTL0, ctl0);
	ctl0 &= ~LCRU_CTL0_BYPASS;
	mmio_write_32(cmu_base + LCRU_CTL0, ctl0);
	ctl0 &= ~LCRU_CTL0_CLKOD_MASK;
	ctl0 |= LCRU_CTL0_CLKOD_SET(pllinit->ClkOD);
	ctl0 &= ~LCRU_CTL0_CLKR_MASK;
	ctl0 |= LCRU_CTL0_CLKR_SET(pllinit->ClkR);
	mmio_write_32(cmu_base + LCRU_CTL0, ctl0);

	mmio_write_32(cmu_base + LCRU_CTL1, pllinit->ClkFLo);
	mmio_write_32(cmu_base + LCRU_CTL2, pllinit->ClkFHi);

	ctl4 = mmio_read_32(cmu_base + LCRU_CTL4);
	ctl4 &=  ~(LCRU_CTL4_IIGAIN_LGMLT_MASK |
		   LCRU_CTL4_IPGAIN_LGMLT_MASK |
		   LCRU_CTL4_IGAIN_LGMLT_MASK  |
		   LCRU_CTL4_PGAIN_LGMLT_MASK) ;
	ctl4 |= LCRU_CTL4_IIGAIN_LGMLT_SET(pllinit->IIGain) |
		LCRU_CTL4_IPGAIN_LGMLT_SET(pllinit->IPGain) |
		LCRU_CTL4_IGAIN_LGMLT_SET(pllinit->IGain) |
		LCRU_CTL4_PGAIN_LGMLT_SET(pllinit->PGain);
	mmio_write_32(cmu_base + LCRU_CTL4, ctl4);

	ctl0 |= LCRU_CTL0_RST;
	mmio_write_32(cmu_base + LCRU_CTL0, ctl0);	// under reset

	// Wait ready
	WAIT_DELAY(
		((mmio_read_32(cmu_base + LCRU_CTL0) & LCRU_CTL0_LOCK) == 0),
		10000000, // 10 ms
		ERROR("%s\n", err_msg)
		);

//	while((mmio_read_32(cmu_base + LCRU_CTL0) & LCRU_CTL0_LOCK) == 0) {};

	ctl6 |= LCRU_CTL6_SWEN;
	mmio_write_32(cmu_base + LCRU_CTL6, ctl6);	// enable
	ctl6 &= ~LCRU_CTL6_SWRST;
	mmio_write_32(cmu_base + LCRU_CTL6, ctl6);	// remove reset
}
