/*
 * Copyright (c) 2020-2022, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/bl_common.h>
#include <common/debug.h>
#include <drivers/arm/smmu_v3.h>
#include <drivers/generic_delay_timer.h>
#include <lib/mmio.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <libfdt.h>

#include <baikal_def.h>
#include <baikal_fdt.h>
#include <baikal_gicv3.h>
#include <bs1000_cmu.h>
#include <bs1000_dimm_spd.h>
#include <bs1000_scp_lcru.h>
#include <bs1000_usb.h>
#include <crc.h>
#include <spd.h>

#include "bs1000_pcie.h"

static uint64_t baikal_detect_sdram_capacity(void)
{
	unsigned dimm_idx;
	uint64_t total_capacity = 0;

	for (dimm_idx = 0; ; ++dimm_idx) {
		const uint8_t *buf = baikal_dimm_spd_get(dimm_idx);

		if (buf == NULL) {
			break;
		}

		if (crc16(buf, 126, 0) == spd_get_baseconf_crc(buf)) {
			const uint64_t dimm_capacity = spd_get_baseconf_dimm_capacity(buf);

			INFO("DIMM%u%u: %lu MiB\n", dimm_idx / 2, dimm_idx % 2,
			     dimm_capacity / (1024 * 1024));

			total_capacity += dimm_capacity;
		}
	}
#ifdef BAIKAL_QEMU
	if (total_capacity == 0) {
		total_capacity = (uint64_t)2 * 1024 * 1024 * 1024;
	}
#endif
	return total_capacity;
}

static void baikal_fdt_memory_update(void)
{
	void *fdt = (void *)BAIKAL_SEC_DTB_BASE;
	uint64_t region_descs[4][2];
	unsigned region_num;
	int ret;
	uint64_t total_capacity;

	ret = fdt_open_into(fdt, fdt, BAIKAL_DTB_MAX_SIZE);
	if (ret < 0) {
		ERROR("%s: failed to open FDT @ %p, error %d\n", __func__, fdt, ret);
		return;
	}

	total_capacity = baikal_detect_sdram_capacity();

	region_descs[0][0] = REGION_DRAM0_BASE;
	region_descs[1][0] = REGION_DRAM1_BASE;
	region_descs[2][0] = REGION_DRAM2_BASE;
	region_descs[3][0] = REGION_DRAM3_BASE;

	/* TODO: the code looks unrolled and could be folded */
	if (total_capacity <= REGION_DRAM0_SIZE) {
		region_descs[0][1] = total_capacity;
		region_num = 1;
	} else {
		region_descs[0][1] = REGION_DRAM0_SIZE;
		if (total_capacity <= (REGION_DRAM0_SIZE + REGION_DRAM1_SIZE)) {
			region_descs[1][1] = total_capacity - REGION_DRAM0_SIZE;
			region_num = 2;
		} else {
			region_descs[1][1] = REGION_DRAM1_SIZE;
			if (total_capacity <= (REGION_DRAM0_SIZE + REGION_DRAM1_SIZE + REGION_DRAM2_SIZE)) {
				region_descs[2][1] = total_capacity - (REGION_DRAM0_SIZE + REGION_DRAM1_SIZE);
				region_num = 3;
			} else {
				region_descs[2][1] = REGION_DRAM2_SIZE;
				region_descs[3][1] = total_capacity - (REGION_DRAM0_SIZE + REGION_DRAM1_SIZE + REGION_DRAM2_SIZE);
				region_num = 4;
			}
		}
	}

	fdt_memory_node_set(fdt, region_descs, region_num);

	ret = fdt_pack(fdt);
	if (ret < 0) {
		ERROR("%s: failed to pack FDT @ %p, error %d\n", __func__, fdt, ret);
	}
}

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
	memcpy((void *)BAIKAL_SCMM_SMMU_STRTAB_BASE + 10*SMMU_STE_SIZE, (void *)ste, SMMU_STE_SIZE);
	ste = BAIKAL_SCMM_SMMU_STRTAB_BASE + SMMU_STE_SIZE;
	SMMUV3_STE_BYPASS(ste, 0, SMMU_STE_MTCFG_USE_INCOMING, SMMU_STE_ALLOCCFG_USE_INCOMING,
			  SMMU_STE_SHCFG_USE_INCOMING, SMMU_STE_NSCFG_USE_INCOMING,
			  SMMU_STE_PRIVCFG_USE_INCOMING, SMMU_STE_INSTCFG_USE_INCOMING);
	memcpy((void *)BAIKAL_SCMM_SMMU_STRTAB_BASE + 2*SMMU_STE_SIZE, (void *)ste, SMMU_STE_SIZE);
	memcpy((void *)BAIKAL_SCMM_SMMU_STRTAB_BASE + 3*SMMU_STE_SIZE, (void *)ste, SMMU_STE_SIZE);
	memcpy((void *)BAIKAL_SCMM_SMMU_STRTAB_BASE + 4*SMMU_STE_SIZE, (void *)ste, SMMU_STE_SIZE);
	memcpy((void *)BAIKAL_SCMM_SMMU_STRTAB_BASE + 8*SMMU_STE_SIZE, (void *)ste, SMMU_STE_SIZE);
	memcpy((void *)BAIKAL_SCMM_SMMU_STRTAB_BASE + 12*SMMU_STE_SIZE, (void *)ste, SMMU_STE_SIZE);
	dsb();
	/* Enable SMMU */
	if (smmuv3_enable(SMMU_BASE, true) != 0)
		ERROR("SCMM SMMU enable failed.\n");
}

void bl31_plat_arch_setup(void)
{
	const mmap_region_t bl_regions[] = {
		MAP_REGION_FLAT(BL31_START,
				BL31_END - BL31_START,
				MT_MEMORY | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(BL_CODE_BASE,
				BL_CODE_END - BL_CODE_BASE,
				MT_CODE | MT_SECURE),

		{0}
	};

	const mmap_region_t plat_bs1000_mmap[] = {
		MAP_REGION_FLAT(MAILBOX_SRAM_BASE,
				MAILBOX_SRAM_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(MAILBOX_IRB_BASE,
				MAILBOX_IRB_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(SMMU_BASE,
				SMMU_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(UART_A1_BASE,
				UART_A1_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(QSPI1_BASE,
				QSPI1_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(I2C2_BASE,
				I2C2_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(I2C3_BASE,
				I2C3_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(GIC_BASE,
				GIC_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(CA75_BASE,
				CA75_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(PCIE_BASE,
				PCIE_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(DDR_BASE,
				DDR_SIZE,
				MT_DEVICE | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(BAIKAL_NS_DTB_BASE,
				BAIKAL_DTB_MAX_SIZE,
				MT_MEMORY | MT_RW | MT_NS),

		MAP_REGION_FLAT(BAIKAL_SEC_DTB_BASE,
				BAIKAL_DTB_MAX_SIZE,
				MT_MEMORY | MT_RW | MT_SECURE),

		MAP_REGION_FLAT(BAIKAL_SCMM_SMMU_BASE,
				BAIKAL_SCMM_SMMU_SIZE,
				MT_NON_CACHEABLE | MT_RW | MT_SECURE),

		{0}
	};

	/* Enable non-secure access */
	mmio_write_32(NIC_SC_CFG_EHCI,		NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_SC_CFG_GIC,		NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_SC_CFG_GMAC0,		NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_SC_CFG_GMAC1,		NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_SC_CFG_LSP,		NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_SC_CFG_OHCI,		NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_SC_CFG_SCP,		NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_SC_CFG_SMMU,		NIC_GPV_REGIONSEC_NONSECURE);

	mmio_write_32(NIC_LSP_CFG_ESPI,		NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_LSP_CFG_GPIO16,	NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_LSP_CFG_GPIO32,	NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_LSP_CFG_GPIO8_1,	NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_LSP_CFG_GPIO8_2,	NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_LSP_CFG_QSPI1,	NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_LSP_CFG_QSPI2,	NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_LSP_CFG_I2C2,		NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_LSP_CFG_I2C3,		NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_LSP_CFG_I2C4,		NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_LSP_CFG_I2C5,		NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_LSP_CFG_I2C6,		NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_LSP_CFG_TIMERS,	NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_LSP_CFG_UART_A1,	NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_LSP_CFG_UART_A2,	NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_LSP_CFG_UART_S,	NIC_GPV_REGIONSEC_NONSECURE);
	mmio_write_32(NIC_LSP_CFG_WDT,		NIC_GPV_REGIONSEC_NONSECURE);

	pcie_init();

	setup_page_tables(bl_regions, plat_bs1000_mmap);
	enable_mmu_el3(0);
}

void bl31_platform_setup(void)
{
	generic_delay_timer_init();

	/* Deassert resets */
	scp_lcru_clrbits(SCP_GPR_MM_RST_CTL2,
			 SCP_GPR_MM_RST_CTL2_LSP_SS_APB	     |
			 SCP_GPR_MM_RST_CTL2_LSP_GPIO_DIV    |
			 SCP_GPR_MM_RST_CTL2_LSP_GPIO32	     |
			 SCP_GPR_MM_RST_CTL2_LSP_GPIO32_APB  |
			 SCP_GPR_MM_RST_CTL2_LSP_GPIO16	     |
			 SCP_GPR_MM_RST_CTL2_LSP_GPIO16_APB  |
			 SCP_GPR_MM_RST_CTL2_LSP_GPIO8_1     |
			 SCP_GPR_MM_RST_CTL2_LSP_GPIO8_1_APB |
			 SCP_GPR_MM_RST_CTL2_LSP_GPIO8_2     |
			 SCP_GPR_MM_RST_CTL2_LSP_GPIO8_2_APB |
			 SCP_GPR_MM_RST_CTL2_LSP_TIMER1	     |
			 SCP_GPR_MM_RST_CTL2_LSP_TIMER2	     |
			 SCP_GPR_MM_RST_CTL2_LSP_TIMER3	     |
			 SCP_GPR_MM_RST_CTL2_LSP_TIMER4	     |
			 SCP_GPR_MM_RST_CTL2_LSP_TIMERS_APB  |
			 SCP_GPR_MM_RST_CTL2_LSP_WDT	     |
			 SCP_GPR_MM_RST_CTL2_LSP_WDT_APB     |
			 SCP_GPR_MM_RST_CTL2_LSP_UART_S	     |
			 SCP_GPR_MM_RST_CTL2_LSP_UART_S_APB  |
			 SCP_GPR_MM_RST_CTL2_LSP_UART_A1     |
			 SCP_GPR_MM_RST_CTL2_LSP_UART_A1_APB |
			 SCP_GPR_MM_RST_CTL2_LSP_UART_A2     |
			 SCP_GPR_MM_RST_CTL2_LSP_UART_A2_APB |
			 SCP_GPR_MM_RST_CTL2_LSP_WDT_DIV);

	scp_lcru_clrbits(SCP_GPR_MM_RST_CTL3,
			 SCP_GPR_MM_RST_CTL3_LSP_SPI1	  |
			 SCP_GPR_MM_RST_CTL3_LSP_SPI1_APB |
			 SCP_GPR_MM_RST_CTL3_LSP_SPI2	  |
			 SCP_GPR_MM_RST_CTL3_LSP_SPI2_APB |
			 SCP_GPR_MM_RST_CTL3_LSP_I2C2	  |
			 SCP_GPR_MM_RST_CTL3_LSP_I2C2_APB |
			 SCP_GPR_MM_RST_CTL3_LSP_I2C3	  |
			 SCP_GPR_MM_RST_CTL3_LSP_I2C3_APB |
			 SCP_GPR_MM_RST_CTL3_LSP_I2C4	  |
			 SCP_GPR_MM_RST_CTL3_LSP_I2C4_APB |
			 SCP_GPR_MM_RST_CTL3_LSP_I2C5	  |
			 SCP_GPR_MM_RST_CTL3_LSP_I2C5_APB |
			 SCP_GPR_MM_RST_CTL3_LSP_I2C6	  |
			 SCP_GPR_MM_RST_CTL3_LSP_I2C6_APB |
			 SCP_GPR_MM_RST_CTL3_LSP_ESPI	  |
			 SCP_GPR_MM_RST_CTL3_LSP_ESPI_RST);

	baikal_gic_driver_init();
	baikal_gic_init();
	bs1000_usb_init();

	baikal_dimm_spd_read();
	baikal_fdt_memory_update();

	/*
	 * Due to hardware bug in BS-1000, GMAC's have wrong AxCACHE and AxPROT
	 * AXI bus transactions attributes. This results in GMAC's DMA operations
	 * are not cache-coherent. We use SMMU to set these attributes to the
	 * proper values.
	 */
	baikal_gmac_memattrs_smmu_fix();

	memcpy((void *)BAIKAL_NS_DTB_BASE,
	       (void *)BAIKAL_SEC_DTB_BASE,
	       BAIKAL_DTB_MAX_SIZE);
}
