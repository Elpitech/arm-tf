/*
 * Copyright (c) 2020-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/bl_common.h>
#include <common/debug.h>
#include <drivers/generic_delay_timer.h>
#include <lib/mmio.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <libfdt.h>

#include <baikal_def.h>
#include <baikal_fdt.h>
#include <baikal_gicv3.h>
#include <bs1000_cmu.h>
#include <bs1000_scp_lcru.h>
#include <crc.h>
#include <dw_i2c.h>
#include <spd.h>

static uint64_t baikal_detect_sdram_capacity(void)
{
	uintptr_t base = I2C2_BASE;
	uint64_t total_capacity = 0;

#ifdef BAIKAL_QEMU
	return (uint64_t)2 * 1024 * 1024 * 1024;
#endif

	for (;;) {
		unsigned dimm_addr;
		const uint8_t startaddr = 0;

		i2c_txrx(base, 0x36, &startaddr, sizeof(startaddr), NULL, 0);

		for (dimm_addr = 0x50; dimm_addr <= 0x55; ++dimm_addr) {
			uint8_t buf[128];
			const int rxsize = i2c_txrx(base, dimm_addr, &startaddr, sizeof(startaddr),
						    buf, sizeof(buf));

			if (rxsize == sizeof(buf) &&
			    crc16(buf, 126, 0) == ((buf[127] << 8) | buf[126])) {
				const uint64_t dimm_capacity = spd_get_baseconf_dimm_capacity(buf);

				INFO("DIMM[0x%lx@0x%x]: %llu MiB\n",
				     base, dimm_addr, dimm_capacity / (1024 * 1024));

				total_capacity += dimm_capacity;
			}
		}

		if (base == I2C2_BASE) {
			base = I2C3_BASE;
		} else {
			return total_capacity;
		}
	}
}

static void baikal_fdt_memory_update(void)
{
	void *fdt = (void *)PLAT_BAIKAL_SEC_DTB_BASE;
	uint64_t region_descs[4][2];
	unsigned region_num;
	int ret;
	uint64_t total_capacity;

	ret = fdt_open_into(fdt, fdt, PLAT_BAIKAL_DTB_MAX_SIZE);
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

		MAP_REGION_FLAT(NIC_SC_CFG_GPV_BASE,
				NIC_SC_CFG_GPV_SIZE,
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

		MAP_REGION_FLAT(NIC_LSP_CFG_GPV_BASE,
				NIC_LSP_CFG_GPV_SIZE,
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

		MAP_REGION_FLAT(PLAT_BAIKAL_NS_DTB_BASE,
				PLAT_BAIKAL_DTB_MAX_SIZE,
				MT_MEMORY | MT_RW | MT_NS),

		MAP_REGION_FLAT(PLAT_BAIKAL_SEC_DTB_BASE,
				PLAT_BAIKAL_DTB_MAX_SIZE,
				MT_MEMORY | MT_RW | MT_SECURE),

		{0}
	};

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
			 SCP_GPR_MM_RST_CTL3_LSP_I2C2	  |
			 SCP_GPR_MM_RST_CTL3_LSP_I2C2_APB |
			 SCP_GPR_MM_RST_CTL3_LSP_I2C3	  |
			 SCP_GPR_MM_RST_CTL3_LSP_I2C3_APB |
			 SCP_GPR_MM_RST_CTL3_LSP_I2C4	  |
			 SCP_GPR_MM_RST_CTL3_LSP_I2C4_APB);

	baikal_gic_driver_init();
	baikal_gic_init();

	baikal_fdt_memory_update();

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

	memcpy((void *)PLAT_BAIKAL_NS_DTB_BASE,
	       (void *)PLAT_BAIKAL_SEC_DTB_BASE,
	       PLAT_BAIKAL_DTB_MAX_SIZE);
}
