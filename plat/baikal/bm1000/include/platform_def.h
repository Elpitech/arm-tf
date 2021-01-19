/*
 * Copyright (c) 2018-2020, Baikal Electronics JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __PLATFORM_DEF_H__
#define __PLATFORM_DEF_H__

#include <arch.h>
#include <plat/common/common_def.h>
#include <common/tbbr/tbbr_img_def.h>

#define FLASH_MAP_FDT		0x40000
#define FLASH_MAP_EFI_VARS	0x50000
#define FLASH_MAP_FIP		0x110000

/* Special value used to verify platform parameters from BL2 to BL3-1 */
#define BAIKAL_BL31_PLAT_PARAM_VAL	0x0f1e2d3c4b5a6978ULL

#define PLATFORM_STACK_SIZE 0x2000

#define PLATFORM_MAX_CPUS_PER_CLUSTER	2
#define PLATFORM_CLUSTER_COUNT		4
#define PLATFORM_CLUSTER0_CORE_COUNT	PLATFORM_MAX_CPUS_PER_CLUSTER
#define PLATFORM_CLUSTER1_CORE_COUNT	PLATFORM_MAX_CPUS_PER_CLUSTER
#define PLATFORM_CLUSTER2_CORE_COUNT	PLATFORM_MAX_CPUS_PER_CLUSTER
#define PLATFORM_CLUSTER3_CORE_COUNT	PLATFORM_MAX_CPUS_PER_CLUSTER
#define PLATFORM_CORE_COUNT		(PLATFORM_CLUSTER0_CORE_COUNT + \
					 PLATFORM_CLUSTER1_CORE_COUNT +	\
					 PLATFORM_CLUSTER2_CORE_COUNT + \
					 PLATFORM_CLUSTER3_CORE_COUNT)

#define BAIKAL_PRIMARY_CPU		0

#define PLAT_NUM_PWR_DOMAINS		(PLATFORM_CLUSTER_COUNT * \
					PLATFORM_CORE_COUNT)
#define PLAT_MAX_PWR_LVL		MPIDR_AFFLVL1

#define PLAT_MAX_RET_STATE		1
#define PLAT_MAX_OFF_STATE		2

/* Local power state for power domains in Run state. */
#define PLAT_LOCAL_STATE_RUN		0
/* Local power state for retention. Valid only for CPU power domains */
#define PLAT_LOCAL_STATE_RET		1
/*
 * Local power state for OFF/power-down. Valid for CPU and cluster power
 * domains.
 */
#define PLAT_LOCAL_STATE_OFF		2

/*
 * Macros used to parse state information from State-ID if it is using the
 * recommended encoding for State-ID.
 */
#define PLAT_LOCAL_PSTATE_WIDTH		4
#define PLAT_LOCAL_PSTATE_MASK		((1 << PLAT_LOCAL_PSTATE_WIDTH) - 1)

/*
 * Some data must be aligned on the biggest cache line size in the platform.
 * This is known only to the platform as it might have a combination of
 * integrated and external caches.
 */
#define CACHE_WRITEBACK_SHIFT		6
#define CACHE_WRITEBACK_GRANULE		(1 << CACHE_WRITEBACK_SHIFT)

/*
 * Partition memory into secure ROM, non-secure DRAM, secure "SRAM",
 * and secure DRAM.
 */
#define MAILBOX_BASE			0x00000000
#define MAILBOX_SIZE			0x00010000

#define NS_DRAM0_BASE			0x80000000
#define NS_DRAM0_SIZE			0x10000000

#define SEC_DRAM0_BASE			0x90000000
#define SEC_DRAM0_SIZE			0x10000000

#define FB_SIZE			(64 * 1024 * 1024) // 64M is sufficient for any available resolution
#define NS_DRAM1_BASE			0xA0000000
#define NS_DRAM1_SIZE			(0x20000000 - 3 * FB_SIZE) // Reserve space for two framebuffers
#define NS_DRAM1_LIMIT			(NS_DRAM1_BASE + NS_DRAM1_SIZE)
#define FB0_BASE			NS_DRAM1_LIMIT
#define FB1_BASE			(FB0_BASE + FB_SIZE)
#define FB2_BASE			(FB1_BASE + FB_SIZE)

//#define SEC_SRAM_BASE			0x80000000
//#define SEC_SRAM_SIZE			0x00010000


/* Load pageable part of OP-TEE at end of secure DRAM */
#define BAIKAL_OPTEE_PAGEABLE_LOAD_BASE	(SEC_DRAM0_BASE + SEC_DRAM0_SIZE - \
					 BAIKAL_OPTEE_PAGEABLE_LOAD_SIZE)
#define BAIKAL_OPTEE_PAGEABLE_LOAD_SIZE	0x00400000

/*
 * ARM-TF lives in SRAM, partition it here
 */
#define SHARED_SRAM_BASE			0x0//(BL1_RW_BASE - 0x10)
#define SHARED_SRAM_SIZE			0x10000//(BL1_RW_LIMIT - SHARED_RAM_BASE)
//#define SHARED_RAM_BASE			0x00000000//(NS_DRAM0_BASE + NS_DRAM0_SIZE)
//#define SHARED_RAM_SIZE			0x00010000

#define PLAT_BAIKAL_TRUSTED_MAILBOX_BASE	(BL1_RW_BASE)
#define PLAT_BAIKAL_TRUSTED_MAILBOX_SIZE	(8 + PLAT_BAIKAL_HOLD_SIZE)
#define PLAT_BAIKAL_HOLD_BASE		(PLAT_BAIKAL_TRUSTED_MAILBOX_BASE + 8)
#define PLAT_BAIKAL_HOLD_SIZE		(PLATFORM_CORE_COUNT * PLAT_BAIKAL_HOLD_ENTRY_SIZE)
#define PLAT_BAIKAL_HOLD_ENTRY_SIZE	8
#define PLAT_BAIKAL_HOLD_STATE_WAIT	0
#define PLAT_BAIKAL_HOLD_STATE_GO	1
#define PLAT_BAIKAL_TRUSTED_MAILBOX_DEC \
	uint64_t cpus_states[PLATFORM_CORE_COUNT + 1] __section("cpus_hold_state")  = { 0 }

/*
 * DT related constants
 */
#define PLAT_BAIKAL_DT_BASE		NS_DRAM0_BASE
#define PLAT_BAIKAL_DT_MAX_SIZE		0x10000
#define PLAT_BAIKAL_DT_LIMIT		(PLAT_BAIKAL_DT_BASE + PLAT_BAIKAL_DT_MAX_SIZE)
#define PLAT_BAIKAL_NS_IMAGE_OFFSET	NS_DRAM1_BASE

/*
 * BL1 specific defines.
 *
 * BL1 RW data is relocated from ROM to RAM at runtime so we need 2 sets of
 * addresses.
 * Put BL1 RW at the top of the Secure SRAM. BL1_RW_BASE is calculated using
 * the current BL1 RW debug size plus a little space for growth.
 */
#define BL1_XLAT_BASE			(SEC_DRAM0_BASE)
#define BL1_XLAT_SIZE			(0x10000)

#define PLAT_SEC_FIP_BASE		(SEC_DRAM0_BASE + BL1_XLAT_SIZE) /* Assuming lower 16K will be taken by XLAT tables */
#define PLAT_SEC_FIP_MAX_SIZE		0x600000
#define PLAT_SEC_FIP_LIMIT		(PLAT_SEC_FIP_BASE + PLAT_SEC_FIP_MAX_SIZE)

#define BL1_RO_BASE			MAILBOX_BASE
#define BL1_RO_SIZE			0xC000
#define BL1_RO_LIMIT			(MAILBOX_BASE + BL1_RO_SIZE)
#define BL1_RW_BASE			(MAILBOX_BASE + BL1_RO_SIZE)
#define SCP_SERVICE_BASE		0xEFF0
#define BL1_RW_LIMIT			0xF000

#define PLAT_BAIKAL_SPD_BASE		BL1_RW_LIMIT
#define PLAT_BAIKAL_SPD_SIZE		256

/* boot flash */
#define SPI_BASE			0x20210000
#define SPI_OFFSET			0x00001000
#define GPIO_BASE			0x20200000

/*
 * BL2 specific defines.
 *
 * Put BL2 just below BL3-1. BL2_BASE is calculated using the current BL2 debug
 * size plus a little space for growth.
 */
#define BL2_BASE			PLAT_SEC_FIP_LIMIT
#define BL2_SIZE			0x22000
#define BL2_LIMIT			(BL2_BASE + BL2_SIZE)//BL31_BASE


#define BL_RAM_BASE			BL2_BASE//BL1_RW_BASE//NS_DRAM0_BASE//0x0//(BL1_RW_BASE )//(MAILBOX_BASE + MAILBOX_SIZE / 2)//(SHARED_RAM_BASE + SHARED_RAM_SIZE)
#define BL_RAM_SIZE			(BL2_LIMIT - BL2_BASE)//BL1_RW_SIZE//0x10000//(BL1_RW_LIMIT - BL_RAM_BASE)//0x10000//(MAILBOX_SIZE / 2)//(NS_DRAM0_SIZE - SHARED_RAM_SIZE)


/*
 * BL3-1 specific defines.
 *
 * Put BL3-1 at the top of the Trusted SRAM. BL31_BASE is calculated using the
 * current BL3-1 debug size plus a little space for growth.
 */
#define BL31_BASE			BL2_LIMIT//(BL31_LIMIT - 0x20000)
#define BL31_SIZE			0x100000
#define BL31_LIMIT			(BL31_BASE + BL31_SIZE)
#define BL31_PROGBITS_LIMIT		(BL31_LIMIT + 0x12000)//(BL1_RW_LIMIT + 0x12000)


/*
 * BL3-2 specific defines.
 *
 * BL3-2 can execute from Secure SRAM, or Secure DRAM.
 */
#define BL32_SRAM_BASE			BL_RAM_BASE
#define BL32_SRAM_LIMIT			BL31_BASE
#define BL32_DRAM_BASE			BL31_PROGBITS_LIMIT
#define BL32_DRAM_LIMIT			(SEC_DRAM0_SIZE + SEC_DRAM0_BASE)

#define BL32_MEM_BASE			SEC_DRAM0_BASE
#define BL32_MEM_SIZE			SEC_DRAM0_SIZE
#define BL32_BASE			BL32_DRAM_BASE
#define BL32_LIMIT			BL32_DRAM_LIMIT


#define PLAT_PHY_ADDR_SPACE_SIZE	(1ull << 36)
#define PLAT_VIRT_ADDR_SPACE_SIZE	(1ull << 36)
#define MAX_MMAP_REGIONS		16
#define MAX_XLAT_TABLES			8
#define MAX_IO_DEVICES			3
#define MAX_IO_HANDLES			6

/*
 * PL011 related constants
 */
#define UART0_BASE			0x20230000
#define UART1_BASE			0x20240000
#define UART0_CLK_IN_HZ			7372800
#define UART1_CLK_IN_HZ			7372800

#define PLAT_BAIKAL_BOOT_UART_BASE	UART0_BASE
#define PLAT_BAIKAL_BOOT_UART_CLK_IN_HZ	UART0_CLK_IN_HZ

#define PLAT_BAIKAL_CRASH_UART_BASE	UART0_BASE
#define PLAT_BAIKAL_CRASH_UART_CLK_IN_HZ	UART0_CLK_IN_HZ

#define PLAT_BAIKAL_CONSOLE_BAUDRATE	115200


#define BAIKAL_SPI0_BASE		0x20210000
#define BAIKAL_SPI1_BASE		0x20220000
#define BAIKAL_SPI0_SIZE		0x10000

#define DEVICE0_BASE			0x02000000
#define DEVICE0_SIZE			0x12000000

#define DEVICE1_BASE			0x20000000
#define DEVICE1_SIZE			0x20100000

#define DEVICE2_BASE			0x00200000
#define DEVICE2_SIZE			0x00200000

#define SPI_MAX_READ			1024
#define MBOX_SBASE			(0x200000)
#define AP2SCP_ADDR(rg)			(MBOX_SBASE + rg * 0x10000)
#define AP2SCP_STATUS_R(rg)		(AP2SCP_ADDR(rg))
#define AP2SCP_SET_R(rg)		(AP2SCP_ADDR(rg) + 4)
#define AP2SCP_CLEAR_R(rg)		(AP2SCP_ADDR(rg) + 8)
#define SCP2AP_ADDR(rg)			(MBOX_SBASE + rg * 0x10000 + 0x8000)
#define SCP2AP_STATUS_R(rg)		(SCP2AP_ADDR(rg))
#define SCP2AP_SET_R(rg)		(SCP2AP_ADDR(rg) + 4)
#define SCP2AP_CLEAR_R(rg)		(SCP2AP_ADDR(rg) + 8)

#define CA57_LCRU_0			0x28000000
#define CA57_LCRU_1			0x0C000000
#define CA57_LCRU_2			0x0A000000
#define CA57_LCRU_3			0x26000000
#define CA57_LCRU_SIZE			0x10000
#define	VDEC_LCRU			0x24000000
#define MMVDEC_GPR(ind)			(VDEC_LCRU + LCRU_GPR + ind)
#define	USB_LCRU			0x2c000000
#define MALI_LCRU			0x2a000000
#define PCI_LCRU			0x02000000
#define AVLSP_LCRU			0x20000000

/* GIC related constants */
#define GICD_BASE			0x2d000000
#define GICC_BASE			0x2d000000
#define GICR_BASE			0x2d100000

#define BAIKAL_IRQ_SEC_SGI_0		8
#define BAIKAL_IRQ_SEC_SGI_1		9
#define BAIKAL_IRQ_SEC_SGI_2		10
#define BAIKAL_IRQ_SEC_SGI_3		11
#define BAIKAL_IRQ_SEC_SGI_4		12
#define BAIKAL_IRQ_SEC_SGI_5		13
#define BAIKAL_IRQ_SEC_SGI_6		14
#define BAIKAL_IRQ_SEC_SGI_7		15

/*
 * System counter
 */
#if BE_QEMU
#define SYS_COUNTER_FREQ_IN_TICKS	((1000 * 1000 * 1000) / 16)
#elif BE_HAPS
#define SYS_COUNTER_FREQ_IN_TICKS	(7333000)
#else
#define SYS_COUNTER_FREQ_IN_TICKS	(50000000)
#endif

/* CCN related constants */
#define PLAT_ARM_CCN_BASE               0x9000000
#define PLAT_ARM_CLUSTER_TO_CCN_ID_MAP  1, 9, 11, 19
#define PLAT_ARM_CLUSTER_COUNT		4

/* Baikal-M Network interconnect config(NIC) addresses */
#define BAIKAL_AVLSP_GPV		0x20100000
#define BAIKAL_VDEC_GPV			0x24100000
#define BAIKAL_MALI_GPV			0x2A100000
#define BAIKAL_PCIE_CFG_GPV		0x02100000
#define BAIKAL_PCIE_GPV			0x40000000
#define BAIKAL_USB_GPV			0x2C100000
#define BAIKAL_XGB_GPV			0x30100000
/* NIC AVLSP CFG */
#define BAIKAL_NIC_AVLSP_TCU		(BAIKAL_AVLSP_GPV + 0x18)
#define BAIKAL_NIC_AVLSP_HDA		(BAIKAL_AVLSP_GPV + 0x38)
#define BAIKAL_NIC_AVLSP_VDU		(BAIKAL_AVLSP_GPV + 0x44)
#define BAIKAL_NIC_AVLSP_SDC		(BAIKAL_AVLSP_GPV + 0x3C)
#define BAIKAL_NIC_AVLSP_TIMERS		(BAIKAL_AVLSP_GPV + 0x40)
#define BAIKAL_NIC_AVLSP_GPIO		(BAIKAL_AVLSP_GPV + 0x08)
#define BAIKAL_NIC_AVLSP_UART_1		(BAIKAL_AVLSP_GPV + 0x2C)
#define BAIKAL_NIC_AVLSP_UART_2		(BAIKAL_AVLSP_GPV + 0x28)
#define BAIKAL_NIC_AVLSP_SPI		(BAIKAL_AVLSP_GPV + 0x20)
#define BAIKAL_NIC_AVLSP_ESPI		(BAIKAL_AVLSP_GPV + 0x34)
#define BAIKAL_NIC_AVLSP_I2S		(BAIKAL_AVLSP_GPV + 0x14)
#define BAIKAL_NIC_AVLSP_I2C_1		(BAIKAL_AVLSP_GPV + 0x24)
#define BAIKAL_NIC_AVLSP_I2C_2		(BAIKAL_AVLSP_GPV + 0x30)
#define BAIKAL_NIC_AVLSP_SMBUS_1	(BAIKAL_AVLSP_GPV + 0x0C)
#define BAIKAL_NIC_AVLSP_SMBUS_2	(BAIKAL_AVLSP_GPV + 0x10)
/* NIC VDEC CFG */
#define BAIKAL_NIC_VDEC_TCU		(BAIKAL_VDEC_GPV + 0x24)
#define BAIKAL_NIC_VDEC_CTL		(BAIKAL_VDEC_GPV + 0x08)
/* NIC MALI CFG */
#define BAIKAL_NIC_MALI_TCU		(BAIKAL_MALI_GPV + 0x0C)
#define BAIKAL_NIC_MALI			(BAIKAL_MALI_GPV + 0x08)
/* NIC PCIE CFG */
#define BAIKAL_NIC_PCIE_CFG_TCU		(BAIKAL_PCIE_CFG_GPV + 0x18)
#define BAIKAL_NIC_PCIE_CFG_X4_0	(BAIKAL_PCIE_CFG_GPV + 0x0C)
#define BAIKAL_NIC_PCIE_CFG_X4_1	(BAIKAL_PCIE_CFG_GPV + 0x10)
#define BAIKAL_NIC_PCIE_CFG_X8		(BAIKAL_PCIE_CFG_GPV + 0x14)
/* NIC PCIE */
#define BAIKAL_NIC_PCIE_X4_0		(BAIKAL_PCIE_GPV + 0x08)
#define BAIKAL_NIC_PCIE_X4_1		(BAIKAL_PCIE_GPV + 0x0C)
#define BAIKAL_NIC_PCIE_X8		(BAIKAL_PCIE_GPV + 0x10)
/* NIC USB CFG */
#define BAIKAL_NIC_USB_TCU		(BAIKAL_USB_GPV + 0x18)
#define BAIKAL_NIC_USB_0		(BAIKAL_USB_GPV + 0x08)
#define BAIKAL_NIC_USB_1		(BAIKAL_USB_GPV + 0x0C)
#define BAIKAL_NIC_SATA_0		(BAIKAL_USB_GPV + 0x10)
#define BAIKAL_NIC_SATA_1		(BAIKAL_USB_GPV + 0x14)
#define BAIKAL_NIC_USB_GIC		(BAIKAL_USB_GPV + 0x1C)
#define BAIKAL_NIC_USB_DMA330_0		(BAIKAL_USB_GPV + 0x20)
#define BAIKAL_NIC_USB_DMA330_1		(BAIKAL_USB_GPV + 0x24)
/* NIC XGB CFG */
#define BAIKAL_NIC_XGB_TCU		(BAIKAL_XGB_GPV + 0x24)
#define BAIKAL_NIC_XGB_VDU		(BAIKAL_XGB_GPV + 0x08)
#define BAIKAL_NIC_XGB_HDMI		(BAIKAL_XGB_GPV + 0x28)
#define BAIKAL_NIC_XGB0_CNT		(BAIKAL_XGB_GPV + 0x0C)
#define BAIKAL_NIC_XGB0_PHY		(BAIKAL_XGB_GPV + 0x10)
#define BAIKAL_NIC_XGB1_CNT		(BAIKAL_XGB_GPV + 0x14)
#define BAIKAL_NIC_XGB1_PHY		(BAIKAL_XGB_GPV + 0x18)
#define BAIKAL_NIC_1GB_0		(BAIKAL_XGB_GPV + 0x1C)
#define BAIKAL_NIC_1GB_1		(BAIKAL_XGB_GPV + 0x20)

/* PVT CLock channels offsets */
#define MALI_PVTCC_ADDR		0x2A060000
#define A57_0_PVTCC_ADDR	0x28200000
#define A57_1_PVTCC_ADDR	0x0C200000
#define A57_2_PVTCC_ADDR	0x0A200000
#define A57_3_PVTCC_ADDR	0x26200000
#define PVT_AREA_MASK		0xFFFF
#define PVT_CCH_OFFSET		0x40
#define PVT_DIV			21 /* Input 25 MHz, but recommended freq is 1.19 MHz => 21 is div */

#endif /* __PLATFORM_DEF_H__ */
