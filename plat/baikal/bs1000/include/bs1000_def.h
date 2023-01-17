/*
 * Copyright (c) 2021-2022, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BS1000_DEF_H
#define BS1000_DEF_H

#include <lib/utils_def.h>

#define REGION_DRAM0_BASE			ULL(0x80000000)
#define REGION_DRAM0_SIZE			ULL(0x80000000)
#define REGION_DRAM1_BASE			ULL(0x880000000)
#define REGION_DRAM1_SIZE			ULL(0x780000000)
#define REGION_DRAM2_BASE			ULL(0x8800000000)
#define REGION_DRAM2_SIZE			ULL(0x7800000000)
#define REGION_DRAM3_BASE			ULL(0x88000000000)
#define REGION_DRAM3_SIZE			ULL(0x78000000000)

#define MAILBOX_SRAM_BASE			U(0)
#define MAILBOX_SRAM_SIZE			U(0x20000)

#define MAILBOX_IRB_BASE			U(0x200000)
#define MAILBOX_IRB_SIZE			U(0x80000)
#define MAILBOX_IRB0_AP2SCP_STATUS		(MAILBOX_IRB_BASE)
#define MAILBOX_IRB0_AP2SCP_SET			(MAILBOX_IRB_BASE + 0x4)
#define MAILBOX_IRB0_AP2SCP_CLR			(MAILBOX_IRB_BASE + 0x8)
#define MAILBOX_IRB0_SCP2AP_STATUS		(MAILBOX_IRB_BASE + 0x8000)
#define MAILBOX_IRB0_SCP2AP_SET			(MAILBOX_IRB_BASE + 0x8004)
#define MAILBOX_IRB0_SCP2AP_CLR			(MAILBOX_IRB_BASE + 0x8008)

/* NIC-400 GPV address region control */
#define NIC_GPV_REGIONSEC_SECURE		0
#define NIC_GPV_REGIONSEC_NONSECURE		1

#define NIC_SC_CFG_GPV_BASE			U(0x800000)
#define NIC_SC_CFG_GPV_SIZE			U(0x1000)
#define NIC_SC_CFG_CA75_0			(NIC_SC_CFG_GPV_BASE + 0x08)
#define NIC_SC_CFG_CA75_1_5			(NIC_SC_CFG_GPV_BASE + 0x0c)
#define NIC_SC_CFG_CA75_6			(NIC_SC_CFG_GPV_BASE + 0x10)
#define NIC_SC_CFG_CA75_7_11			(NIC_SC_CFG_GPV_BASE + 0x14)
#define NIC_SC_CFG_CORESIGHT_APB		(NIC_SC_CFG_GPV_BASE + 0x34)
#define NIC_SC_CFG_CORESIGHT_CNT		(NIC_SC_CFG_GPV_BASE + 0x30)
#define NIC_SC_CFG_CORESIGHT_STM		(NIC_SC_CFG_GPV_BASE + 0x2c)
#define NIC_SC_CFG_EHCI				(NIC_SC_CFG_GPV_BASE + 0x28)
#define NIC_SC_CFG_GIC				(NIC_SC_CFG_GPV_BASE + 0x1c)
#define NIC_SC_CFG_GMAC0			(NIC_SC_CFG_GPV_BASE + 0x44)
#define NIC_SC_CFG_GMAC1			(NIC_SC_CFG_GPV_BASE + 0x40)
#define NIC_SC_CFG_LSP				(NIC_SC_CFG_GPV_BASE + 0x18)
#define NIC_SC_CFG_OHCI				(NIC_SC_CFG_GPV_BASE + 0x24)
#define NIC_SC_CFG_SCP				(NIC_SC_CFG_GPV_BASE + 0x20)
#define NIC_SC_CFG_SMMU				(NIC_SC_CFG_GPV_BASE + 0x38)

#define SMMU_BASE				U(0x900000)
#define SMMU_SIZE				U(0x100000)

#define OHCI_BASE				U(0xa00000)
#define OHCI_SIZE				U(0x1000)

#define EHCI_BASE				U(0xa10000)
#define EHCI_SIZE				U(0x1000)

#define GMAC0_BASE				U(0xa20000)
#define GMAC0_SIZE				U(0x10000)

#define GMAC1_BASE				U(0xa30000)
#define GMAC1_SIZE				U(0x10000)

#define CORESIGHT_CNT_BASE			U(0xb00000)
#define CORESIGHT_CNT_SIZE			U(0x100000)

#define UART_A1_BASE				U(0xc00000)
#define UART_A1_SIZE				U(0x1000)

#define UART_A2_BASE				U(0xc10000)
#define UART_A2_SIZE				U(0x1000)

#define QSPI1_BASE				U(0xc20000)
#define QSPI1_SIZE				U(0x1000)

#define QSPI2_BASE				U(0xc30000)
#define QSPI2_SIZE				U(0x1000)

#define ESPI_BASE				U(0xc40000)
#define ESPI_SIZE				U(0x1000)

#define GPIO32_BASE				U(0xc50000)
#define GPIO32_SIZE				U(0x1000)

#define GPIO16_BASE				U(0xc60000)
#define GPIO16_SIZE				U(0x1000)

#define GPIO8_1_BASE				U(0xc70000)
#define GPIO8_1_SIZE				U(0x1000)

#define GPIO8_2_BASE				U(0xc80000)
#define GPIO8_2_SIZE				U(0x1000)

#define I2C2_BASE				U(0xc90000)
#define I2C2_SIZE				U(0x1000)

#define I2C3_BASE				U(0xca0000)
#define I2C3_SIZE				U(0x1000)

#define I2C4_BASE				U(0xcb0000)
#define I2C4_SIZE				U(0x1000)

#define I2C5_BASE				U(0xcc0000)
#define I2C5_SIZE				U(0x1000)

#define I2C6_BASE				U(0xcd0000)
#define I2C6_SIZE				U(0x1000)

#define WDT_BASE				U(0xce0000)
#define WDT_SIZE				U(0x1000)

#define TIMERS_BASE				U(0xcf0000)
#define TIMERS_SIZE				U(0x1000)

#define NIC_LSP_CFG_GPV_BASE			U(0xd00000)
#define NIC_LSP_CFG_GPV_SIZE			U(0x1000)
#define NIC_LSP_CFG_ESPI			(NIC_LSP_CFG_GPV_BASE + 0x24)
#define NIC_LSP_CFG_GPIO16			(NIC_LSP_CFG_GPV_BASE + 0x34)
#define NIC_LSP_CFG_GPIO32			(NIC_LSP_CFG_GPV_BASE + 0x3c)
#define NIC_LSP_CFG_GPIO8_1			(NIC_LSP_CFG_GPV_BASE + 0x30)
#define NIC_LSP_CFG_GPIO8_2			(NIC_LSP_CFG_GPV_BASE + 0x2c)
#define NIC_LSP_CFG_QSPI1			(NIC_LSP_CFG_GPV_BASE + 0x28)
#define NIC_LSP_CFG_QSPI2			(NIC_LSP_CFG_GPV_BASE + 0x38)
#define NIC_LSP_CFG_I2C2			(NIC_LSP_CFG_GPV_BASE + 0x14)
#define NIC_LSP_CFG_I2C3			(NIC_LSP_CFG_GPV_BASE + 0x10)
#define NIC_LSP_CFG_I2C4			(NIC_LSP_CFG_GPV_BASE + 0x20)
#define NIC_LSP_CFG_I2C5			(NIC_LSP_CFG_GPV_BASE + 0x18)
#define NIC_LSP_CFG_I2C6			(NIC_LSP_CFG_GPV_BASE + 0x08)
#define NIC_LSP_CFG_TIMERS			(NIC_LSP_CFG_GPV_BASE + 0x0c)
#define NIC_LSP_CFG_UART_A1			(NIC_LSP_CFG_GPV_BASE + 0x48)
#define NIC_LSP_CFG_UART_A2			(NIC_LSP_CFG_GPV_BASE + 0x40)
#define NIC_LSP_CFG_UART_S			(NIC_LSP_CFG_GPV_BASE + 0x44)
#define NIC_LSP_CFG_WDT				(NIC_LSP_CFG_GPV_BASE + 0x1c)

#define UART_S_BASE				U(0xe00000)
#define UART_S_SIZE				U(0x1000)

#define GIC_BASE				U(0x1000000)
#define GIC_SIZE				U(0x1000000)
#define GICD_BASE				(GIC_BASE)
#define GICR_BASE				(GIC_BASE + 0x240000)
#define GIC_ITS0_TRANSLATER			(GIC_BASE + 0x050040)
#define GIC_ITS1_TRANSLATER			(GIC_BASE + 0x070040)
#define GIC_ITS2_TRANSLATER			(GIC_BASE + 0x090040)
#define GIC_ITS3_TRANSLATER			(GIC_BASE + 0x0b0040)
#define GIC_ITS4_TRANSLATER			(GIC_BASE + 0x0d0040)
#define GIC_ITS5_TRANSLATER			(GIC_BASE + 0x0f0040)
#define GIC_ITS6_TRANSLATER			(GIC_BASE + 0x110040)
#define GIC_ITS7_TRANSLATER			(GIC_BASE + 0x130040)
#define GIC_ITS8_TRANSLATER			(GIC_BASE + 0x150040)
#define GIC_ITS9_TRANSLATER			(GIC_BASE + 0x170040)
#define GIC_ITS10_TRANSLATER			(GIC_BASE + 0x190040)
#define GIC_ITS11_TRANSLATER			(GIC_BASE + 0x1b0040)
#define GIC_ITS12_TRANSLATER			(GIC_BASE + 0x1d0040)
#define GIC_ITS13_TRANSLATER			(GIC_BASE + 0x1f0040)
#define GIC_ITS14_TRANSLATER			(GIC_BASE + 0x210040)
#define GIC_ITS15_TRANSLATER			(GIC_BASE + 0x230040)

#define CORESIGHT_CFG_BASE			U(0x2000000)
#define CORESIGHT_CFG_SIZE			U(0x1000000)

#define CORESIGHT_STM_BASE			U(0x3000000)
#define CORESIGHT_STM_SIZE			U(0x1000000)

#define CA75_BASE				(CA75_0_BASE)
#define CA75_SIZE				U(0x30000000)

#define CA75_0_BASE				U(0x4000000)
#define CA75_0_CMU0_BASE			(CA75_0_BASE)
#define CA75_0_CMU1_BASE			(CA75_0_BASE + 0x10000)
#define CA75_0_GPR_BASE				(CA75_0_BASE + 0x20000)
#define CA75_0_PVT_BASE				(CA75_0_BASE + 0x30000)
#define CA75_0_GPV_BASE				(CA75_0_BASE + 0x100000)

#define CA75_1_BASE				U(0x8000000)
#define CA75_1_CMU0_BASE			(CA75_1_BASE)
#define CA75_1_CMU1_BASE			(CA75_1_BASE + 0x10000)
#define CA75_1_GPR_BASE				(CA75_1_BASE + 0x20000)
#define CA75_1_PVT_BASE				(CA75_1_BASE + 0x30000)
#define CA75_1_GPV_BASE				(CA75_1_BASE + 0x100000)

#define CA75_2_BASE				U(0xc000000)
#define CA75_2_CMU0_BASE			(CA75_2_BASE)
#define CA75_2_CMU1_BASE			(CA75_2_BASE + 0x10000)
#define CA75_2_GPR_BASE				(CA75_2_BASE + 0x20000)
#define CA75_2_PVT_BASE				(CA75_2_BASE + 0x30000)
#define CA75_2_GPV_BASE				(CA75_2_BASE + 0x100000)

#define CA75_3_BASE				U(0x10000000)
#define CA75_3_CMU0_BASE			(CA75_3_BASE)
#define CA75_3_CMU1_BASE			(CA75_3_BASE + 0x10000)
#define CA75_3_GPR_BASE				(CA75_3_BASE + 0x20000)
#define CA75_3_PVT_BASE				(CA75_3_BASE + 0x30000)
#define CA75_3_GPV_BASE				(CA75_3_BASE + 0x100000)

#define CA75_4_BASE				U(0x14000000)
#define CA75_4_CMU0_BASE			(CA75_4_BASE)
#define CA75_4_CMU1_BASE			(CA75_4_BASE + 0x10000)
#define CA75_4_GPR_BASE				(CA75_4_BASE + 0x20000)
#define CA75_4_PVT_BASE				(CA75_4_BASE + 0x30000)
#define CA75_4_GPV_BASE				(CA75_4_BASE + 0x100000)

#define CA75_5_BASE				U(0x18000000)
#define CA75_5_CMU0_BASE			(CA75_5_BASE)
#define CA75_5_CMU1_BASE			(CA75_5_BASE + 0x10000)
#define CA75_5_GPR_BASE				(CA75_5_BASE + 0x20000)
#define CA75_5_PVT_BASE				(CA75_5_BASE + 0x30000)
#define CA75_5_GPV_BASE				(CA75_5_BASE + 0x100000)

#define CA75_6_BASE				U(0x1c000000)
#define CA75_6_CMU0_BASE			(CA75_6_BASE)
#define CA75_6_CMU1_BASE			(CA75_6_BASE + 0x10000)
#define CA75_6_GPR_BASE				(CA75_6_BASE + 0x20000)
#define CA75_6_PVT_BASE				(CA75_6_BASE + 0x30000)
#define CA75_6_GPV_BASE				(CA75_6_BASE + 0x100000)

#define CA75_7_BASE				U(0x20000000)
#define CA75_7_CMU0_BASE			(CA75_7_BASE)
#define CA75_7_CMU1_BASE			(CA75_7_BASE + 0x10000)
#define CA75_7_GPR_BASE				(CA75_7_BASE + 0x20000)
#define CA75_7_PVT_BASE				(CA75_7_BASE + 0x30000)
#define CA75_7_GPV_BASE				(CA75_7_BASE + 0x100000)

#define CA75_8_BASE				U(0x24000000)
#define CA75_8_CMU0_BASE			(CA75_8_BASE)
#define CA75_8_CMU1_BASE			(CA75_8_BASE + 0x10000)
#define CA75_8_GPR_BASE				(CA75_8_BASE + 0x20000)
#define CA75_8_PVT_BASE				(CA75_8_BASE + 0x30000)
#define CA75_8_GPV_BASE				(CA75_8_BASE + 0x100000)

#define CA75_9_BASE				U(0x28000000)
#define CA75_9_CMU0_BASE			(CA75_9_BASE)
#define CA75_9_CMU1_BASE			(CA75_9_BASE + 0x10000)
#define CA75_9_GPR_BASE				(CA75_9_BASE + 0x20000)
#define CA75_9_PVT_BASE				(CA75_9_BASE + 0x30000)
#define CA75_9_GPV_BASE				(CA75_9_BASE + 0x100000)

#define CA75_10_BASE				U(0x2c000000)
#define CA75_10_CMU0_BASE			(CA75_10_BASE)
#define CA75_10_CMU1_BASE			(CA75_10_BASE + 0x10000)
#define CA75_10_GPR_BASE			(CA75_10_BASE + 0x20000)
#define CA75_10_PVT_BASE			(CA75_10_BASE + 0x30000)
#define CA75_10_GPV_BASE			(CA75_10_BASE + 0x100000)

#define CA75_11_BASE				U(0x30000000)
#define CA75_11_CMU0_BASE			(CA75_11_BASE)
#define CA75_11_CMU1_BASE			(CA75_11_BASE + 0x10000)
#define CA75_11_GPR_BASE			(CA75_11_BASE + 0x20000)
#define CA75_11_PVT_BASE			(CA75_11_BASE + 0x30000)
#define CA75_11_GPV_BASE			(CA75_11_BASE + 0x100000)

#define CMN_BASE				U(0x34000000)
#define CMN_SIZE				U(0x4000000)

#define PCIE_BASE				(PCIE0_BASE)
#define PCIE_SIZE				U(0x18000000)

#define PCIE0_BASE				U(0x38000000)
#define PCIE0_CMU0_BASE				(PCIE0_BASE)
#define PCIE0_GPR_BASE				(PCIE0_BASE + 0x20000)
#define PCIE0_GPR_SS_MODE_CTL			(PCIE0_GPR_BASE + 0x10)
#define PCIE0_GPR_PWRUP_RST_CTL			(PCIE0_GPR_BASE + 0x18)
#define PCIE0_GPR_ITS0_TGTADDR_CTL		(PCIE0_GPR_BASE + 0x88)
#define PCIE0_GPR_ITS1_TGTADDR_CTL		(PCIE0_GPR_BASE + 0x90)
#define PCIE0_PVT_BASE				(PCIE0_BASE + 0x30000)
#define PCIE0_NIC_CFG_GPV			(PCIE0_BASE + 0x200000)
#define PCIE0_NIC_CFG_P0			(PCIE0_NIC_CFG_GPV + 0x08)
#define PCIE0_NIC_CFG_P1			(PCIE0_NIC_CFG_GPV + 0x0c)
#define PCIE0_NIC_CFG_PHY			(PCIE0_NIC_CFG_GPV + 0x18)
#define PCIE0_NIC_SLV_GPV			(PCIE0_BASE + 0x300000)
#define PCIE0_NIC_SLV_P0			(PCIE0_NIC_SLV_GPV + 0x08)
#define PCIE0_NIC_SLV_P1			(PCIE0_NIC_SLV_GPV + 0x0c)
#define PCIE0_P0_DBI_BASE			(PCIE0_BASE + 0x1000000)
#define PCIE0_P1_DBI_BASE			(PCIE0_BASE + 0x1400000)

#define PCIE1_BASE				U(0x3c000000)
#define PCIE1_CMU0_BASE				(PCIE1_BASE)
#define PCIE1_GPR_BASE				(PCIE1_BASE + 0x20000)
#define PCIE1_GPR_SS_MODE_CTL			(PCIE1_GPR_BASE + 0x10)
#define PCIE1_GPR_PWRUP_RST_CTL			(PCIE1_GPR_BASE + 0x18)
#define PCIE1_GPR_ITS0_TGTADDR_CTL		(PCIE1_GPR_BASE + 0x88)
#define PCIE1_GPR_ITS1_TGTADDR_CTL		(PCIE1_GPR_BASE + 0x90)
#define PCIE1_PVT_BASE				(PCIE1_BASE + 0x30000)
#define PCIE1_NIC_CFG_GPV			(PCIE1_BASE + 0x200000)
#define PCIE1_NIC_CFG_P0			(PCIE1_NIC_CFG_GPV + 0x08)
#define PCIE1_NIC_CFG_P1			(PCIE1_NIC_CFG_GPV + 0x0c)
#define PCIE1_NIC_CFG_PHY			(PCIE1_NIC_CFG_GPV + 0x18)
#define PCIE1_NIC_SLV_GPV			(PCIE1_BASE + 0x300000)
#define PCIE1_NIC_SLV_P0			(PCIE1_NIC_SLV_GPV + 0x08)
#define PCIE1_NIC_SLV_P1			(PCIE1_NIC_SLV_GPV + 0x0c)
#define PCIE1_P0_DBI_BASE			(PCIE1_BASE + 0x1000000)
#define PCIE1_P1_DBI_BASE			(PCIE1_BASE + 0x1400000)

#define PCIE2_BASE				U(0x44000000)
#define PCIE2_CMU0_BASE				(PCIE2_BASE)
#define PCIE2_GPR_BASE				(PCIE2_BASE + 0x20000)
#define PCIE2_GPR_SS_MODE_CTL			(PCIE2_GPR_BASE + 0x10)
#define PCIE2_GPR_PWRUP_RST_CTL			(PCIE2_GPR_BASE + 0x18)
#define PCIE2_GPR_ITS0_TGTADDR_CTL		(PCIE2_GPR_BASE + 0x88)
#define PCIE2_GPR_ITS1_TGTADDR_CTL		(PCIE2_GPR_BASE + 0x90)
#define PCIE2_PVT_BASE				(PCIE2_BASE + 0x30000)
#define PCIE2_NIC_CFG_GPV			(PCIE2_BASE + 0x200000)
#define PCIE2_NIC_CFG_P0			(PCIE2_NIC_CFG_GPV + 0x08)
#define PCIE2_NIC_CFG_P1			(PCIE2_NIC_CFG_GPV + 0x0c)
#define PCIE2_NIC_CFG_PHY			(PCIE2_NIC_CFG_GPV + 0x18)
#define PCIE2_NIC_SLV_GPV			(PCIE2_BASE + 0x300000)
#define PCIE2_NIC_SLV_P0			(PCIE2_NIC_SLV_GPV + 0x08)
#define PCIE2_NIC_SLV_P1			(PCIE2_NIC_SLV_GPV + 0x0c)
#define PCIE2_P0_DBI_BASE			(PCIE2_BASE + 0x1000000)
#define PCIE2_P1_DBI_BASE			(PCIE2_BASE + 0x1400000)

#define PCIE3_BASE				U(0x48000000)
#define PCIE3_CMU0_BASE				(PCIE3_BASE)
#define PCIE3_GPR_BASE				(PCIE3_BASE + 0x20000)
#define PCIE3_GPR_SS_MODE_CTL			(PCIE3_GPR_BASE + 0x10)
#define PCIE3_GPR_PWRUP_RST_CTL			(PCIE3_GPR_BASE + 0x18)
#define PCIE3_GPR_ITS0_TGTADDR_CTL		(PCIE3_GPR_BASE + 0x120)
#define PCIE3_GPR_ITS1_TGTADDR_CTL		(PCIE3_GPR_BASE + 0x128)
#define PCIE3_GPR_ITS2_TGTADDR_CTL		(PCIE3_GPR_BASE + 0x130)
#define PCIE3_GPR_ITS3_TGTADDR_CTL		(PCIE3_GPR_BASE + 0x138)
#define PCIE3_GPR_ITS4_TGTADDR_CTL		(PCIE3_GPR_BASE + 0x140)
#define PCIE3_PVT_BASE				(PCIE3_BASE + 0x30000)
#define PCIE3_NIC_CFG_GPV			(PCIE3_BASE + 0x200000)
#define PCIE3_NIC_CFG_P0			(PCIE3_NIC_CFG_GPV + 0x08)
#define PCIE3_NIC_CFG_P1			(PCIE3_NIC_CFG_GPV + 0x0c)
#define PCIE3_NIC_CFG_P2			(PCIE3_NIC_CFG_GPV + 0x10)
#define PCIE3_NIC_CFG_P3			(PCIE3_NIC_CFG_GPV + 0x14)
#define PCIE3_NIC_CFG_PHY			(PCIE3_NIC_CFG_GPV + 0x24)
#define PCIE3_NIC_SLV_GPV			(PCIE3_BASE + 0x300000)
#define PCIE3_NIC_SLV_P0			(PCIE3_NIC_SLV_GPV + 0x08)
#define PCIE3_NIC_SLV_P1			(PCIE3_NIC_SLV_GPV + 0x0c)
#define PCIE3_NIC_SLV_P2			(PCIE3_NIC_SLV_GPV + 0x10)
#define PCIE3_NIC_SLV_P3			(PCIE3_NIC_SLV_GPV + 0x14)
#define PCIE3_P0_DBI_BASE			(PCIE3_BASE + 0x1000000)
#define PCIE3_P1_DBI_BASE			(PCIE3_BASE + 0x1400000)
#define PCIE3_P2_DBI_BASE			(PCIE3_BASE + 0x1800000)
#define PCIE3_P3_DBI_BASE			(PCIE3_BASE + 0x1c00000)

#define PCIE4_BASE				U(0x4c000000)
#define PCIE4_CMU0_BASE				(PCIE4_BASE)
#define PCIE4_GPR_BASE				(PCIE4_BASE + 0x20000)
#define PCIE4_GPR_SS_MODE_CTL			(PCIE4_GPR_BASE + 0x10)
#define PCIE4_GPR_PWRUP_RST_CTL			(PCIE4_GPR_BASE + 0x18)
#define PCIE4_GPR_ITS0_TGTADDR_CTL		(PCIE4_GPR_BASE + 0x120)
#define PCIE4_GPR_ITS1_TGTADDR_CTL		(PCIE4_GPR_BASE + 0x128)
#define PCIE4_GPR_ITS2_TGTADDR_CTL		(PCIE4_GPR_BASE + 0x130)
#define PCIE4_GPR_ITS3_TGTADDR_CTL		(PCIE4_GPR_BASE + 0x138)
#define PCIE4_GPR_ITS4_TGTADDR_CTL		(PCIE4_GPR_BASE + 0x140)
#define PCIE4_PVT_BASE				(PCIE4_BASE + 0x30000)
#define PCIE4_NIC_CFG_GPV			(PCIE4_BASE + 0x200000)
#define PCIE4_NIC_CFG_P0			(PCIE4_NIC_CFG_GPV + 0x08)
#define PCIE4_NIC_CFG_P1			(PCIE4_NIC_CFG_GPV + 0x0c)
#define PCIE4_NIC_CFG_P2			(PCIE4_NIC_CFG_GPV + 0x10)
#define PCIE4_NIC_CFG_P3			(PCIE4_NIC_CFG_GPV + 0x14)
#define PCIE4_NIC_CFG_PHY			(PCIE4_NIC_CFG_GPV + 0x24)
#define PCIE4_NIC_SLV_GPV			(PCIE4_BASE + 0x300000)
#define PCIE4_NIC_SLV_P0			(PCIE4_NIC_SLV_GPV + 0x08)
#define PCIE4_NIC_SLV_P1			(PCIE4_NIC_SLV_GPV + 0x0c)
#define PCIE4_NIC_SLV_P2			(PCIE4_NIC_SLV_GPV + 0x10)
#define PCIE4_NIC_SLV_P3			(PCIE4_NIC_SLV_GPV + 0x14)
#define PCIE4_P0_DBI_BASE			(PCIE4_BASE + 0x1000000)
#define PCIE4_P1_DBI_BASE			(PCIE4_BASE + 0x1400000)
#define PCIE4_P2_DBI_BASE			(PCIE4_BASE + 0x1800000)
#define PCIE4_P3_DBI_BASE			(PCIE4_BASE + 0x1c00000)

#define PCIE_GPR_SS_MODE_CTL_SSMODE_MASK	GENMASK(1, 0)
#define PCIE_GPR_SS_MODE_CTL_DEVTYPE_MASK	GENMASK(7, 4)

#define PCIE_GPR_PWRUP_RST_CTL_PWRUP_RST	BIT(0)
#define PCIE_GPR_PWRUP_RST_CTL_P0_PERST_EN	BIT(4)
#define PCIE_GPR_PWRUP_RST_CTL_P1_PERST_EN	BIT(5)
#define PCIE_GPR_PWRUP_RST_CTL_P2_PERST_EN	BIT(6)
#define PCIE_GPR_PWRUP_RST_CTL_P3_PERST_EN	BIT(7)
#define PCIE_GPR_PWRUP_RST_CTL_P0_PERST		BIT(8)
#define PCIE_GPR_PWRUP_RST_CTL_P1_PERST		BIT(9)
#define PCIE_GPR_PWRUP_RST_CTL_P2_PERST		BIT(10)
#define PCIE_GPR_PWRUP_RST_CTL_P3_PERST		BIT(11)

#define DDR_BASE				(DDR0_BASE)
#define DDR_SIZE				U(0x1c000000)

#define DDR0_BASE				U(0x50000000)
#define DDR0_CMU0_BASE				(DDR0_BASE)
#define DDR0_GPR_BASE				(DDR0_BASE + 0x20000)
#define DDR0_PVT_BASE				(DDR0_BASE + 0x30000)
#define DDR0_CTRL_BASE				(DDR0_BASE + 0x3000000)

#define DDR1_BASE				U(0x54000000)
#define DDR1_CMU0_BASE				(DDR1_BASE)
#define DDR1_GPR_BASE				(DDR1_BASE + 0x20000)
#define DDR1_PVT_BASE				(DDR1_BASE + 0x30000)
#define DDR1_CTRL_BASE				(DDR1_BASE + 0x3000000)

#define DDR2_BASE				U(0x58000000)
#define DDR2_CMU0_BASE				(DDR2_BASE)
#define DDR2_GPR_BASE				(DDR2_BASE + 0x20000)
#define DDR2_PVT_BASE				(DDR2_BASE + 0x30000)
#define DDR2_CTRL_BASE				(DDR2_BASE + 0x3000000)

#define DDR3_BASE				U(0x60000000)
#define DDR3_CMU0_BASE				(DDR3_BASE)
#define DDR3_GPR_BASE				(DDR3_BASE + 0x20000)
#define DDR3_PVT_BASE				(DDR3_BASE + 0x30000)
#define DDR3_CTRL_BASE				(DDR3_BASE + 0x3000000)

#define DDR4_BASE				U(0x64000000)
#define DDR4_CMU0_BASE				(DDR4_BASE)
#define DDR4_GPR_BASE				(DDR4_BASE + 0x20000)
#define DDR4_PVT_BASE				(DDR4_BASE + 0x30000)
#define DDR4_CTRL_BASE				(DDR4_BASE + 0x3000000)

#define DDR5_BASE				U(0x68000000)
#define DDR5_CMU0_BASE				(DDR5_BASE)
#define DDR5_GPR_BASE				(DDR5_BASE + 0x20000)
#define DDR5_PVT_BASE				(DDR5_BASE + 0x30000)
#define DDR5_CTRL_BASE				(DDR5_BASE + 0x3000000)

/*******************************************************************************
 * Address map from SCP. This region is not directly accessible by Cortex-A75
 * Use these definitions as arguments in AP/SCP API
 ******************************************************************************/
#define SCP_LCRU_BASE				U(0x400000)
#define SCP_LCRU_SIZE				U(0x200000)

#define SCP_CMU0_BASE				(SCP_LCRU_BASE)
#define SCP_CMU0_CLKCHCTL_CMN			(SCP_CMU0_BASE + CMU_CLKCHCTL_OFFSET(0))
#define SCP_CMU0_CLKCHCTL_CMN_CS		(SCP_CMU0_BASE + CMU_CLKCHCTL_OFFSET(1))

#define SCP_CMU1_BASE				(SCP_LCRU_BASE + 0x10000)
#define SCP_CMU1_CLKCHCTL_APB_CFG		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(0))
#define SCP_CMU1_CLKCHCTL_SCP_EFUSE		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(1))
#define SCP_CMU1_CLKCHCTL_SCP_EXT_CFG		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(2))
#define SCP_CMU1_CLKCHCTL_SCP_APB_SPI		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(3))
#define SCP_CMU1_CLKCHCTL_SCP_UART		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(4))
#define SCP_CMU1_CLKCHCTL_SCP_GPIO		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(5))
#define SCP_CMU1_CLKCHCTL_SCP_SPI		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(6))
#define SCP_CMU1_CLKCHCTL_SCP_I2C1		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(7))
#define SCP_CMU1_CLKCHCTL_SCP_I2C2		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(8))
#define SCP_CMU1_CLKCHCTL_GMAC0_APB		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(9))
#define SCP_CMU1_CLKCHCTL_GMAC0_AXI		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(10))
#define SCP_CMU1_CLKCHCTL_GMAC1_APB		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(11))
#define SCP_CMU1_CLKCHCTL_GMAC1_AXI		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(12))
#define SCP_CMU1_CLKCHCTL_USB_H			(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(13))
#define SCP_CMU1_CLKCHCTL_USB_OHCI_48		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(14))
#define SCP_CMU1_CLKCHCTL_LSP_APB		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(15))
#define SCP_CMU1_CLKCHCTL_LSP_GPIO		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(16))
#define SCP_CMU1_CLKCHCTL_LSP_UART_S		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(17))
#define SCP_CMU1_CLKCHCTL_LSP_UART_A1		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(18))
#define SCP_CMU1_CLKCHCTL_LSP_UART_A2		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(19))
#define SCP_CMU1_CLKCHCTL_LSP_QSPI1		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(20))
#define SCP_CMU1_CLKCHCTL_LSP_QSPI2		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(21))
#define SCP_CMU1_CLKCHCTL_LSP_ESPI		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(22))
#define SCP_CMU1_CLKCHCTL_LSP_I2C2		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(23))
#define SCP_CMU1_CLKCHCTL_LSP_I2C3		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(24))
#define SCP_CMU1_CLKCHCTL_LSP_I2C4		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(25))
#define SCP_CMU1_CLKCHCTL_LSP_I2C5		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(26))
#define SCP_CMU1_CLKCHCTL_LSP_I2C6		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(27))
#define SCP_CMU1_CLKCHCTL_LSP_TIMER1		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(28))
#define SCP_CMU1_CLKCHCTL_LSP_TIMER2		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(29))
#define SCP_CMU1_CLKCHCTL_LSP_TIMER3		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(30))
#define SCP_CMU1_CLKCHCTL_LSP_TIMER4		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(31))
#define SCP_CMU1_CLKCHCTL_LSP_WDT		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(32))
#define SCP_CMU1_CLKCHCTL_TCU			(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(33))
#define SCP_CMU1_CLKCHCTL_SC_AXI_CFG		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(34))
#define SCP_CMU1_CLKCHCTL_CA75_AHB_CFG		(SCP_CMU1_BASE + CMU_CLKCHCTL_OFFSET(35))

#define SCP_CMU2_BASE				(SCP_LCRU_BASE + 0x20000)
#define SCP_CMU2_CLKCHCTL_SCP			(SCP_CMU2_BASE + CMU_CLKCHCTL_OFFSET(0))
#define SCP_CMU2_CLKCHCTL_GMAC0_PTP		(SCP_CMU2_BASE + CMU_CLKCHCTL_OFFSET(1))
#define SCP_CMU2_CLKCHCTL_GMAC0_TXX2		(SCP_CMU2_BASE + CMU_CLKCHCTL_OFFSET(2))
#define SCP_CMU2_CLKCHCTL_GMAC1_PTP		(SCP_CMU2_BASE + CMU_CLKCHCTL_OFFSET(3))
#define SCP_CMU2_CLKCHCTL_GMAC1_TXX2		(SCP_CMU2_BASE + CMU_CLKCHCTL_OFFSET(4))
#define SCP_CMU2_CLKCHCTL_GIC_DISTR		(SCP_CMU2_BASE + CMU_CLKCHCTL_OFFSET(5))
#define SCP_CMU2_CLKCHCTL_CORESIGHT_DBG		(SCP_CMU2_BASE + CMU_CLKCHCTL_OFFSET(6))
#define SCP_CMU2_CLKCHCTL_CORESIGHT_CNT		(SCP_CMU2_BASE + CMU_CLKCHCTL_OFFSET(7))
#define SCP_CMU2_CLKCHCTL_CORESIGHT_TS		(SCP_CMU2_BASE + CMU_CLKCHCTL_OFFSET(8))
#define SCP_CMU2_CLKCHCTL_CORESIGHT_LPD		(SCP_CMU2_BASE + CMU_CLKCHCTL_OFFSET(9))
#define SCP_CMU2_CLKCHCTL_TBU			(SCP_CMU2_BASE + CMU_CLKCHCTL_OFFSET(10))
#define SCP_CMU2_CLKCHCTL_HS_CFG		(SCP_CMU2_BASE + CMU_CLKCHCTL_OFFSET(11))
#define SCP_CMU2_CLKCHCTL_SCP_BISR		(SCP_CMU2_BASE + CMU_CLKCHCTL_OFFSET(12))

#define SCP_GPR_BASE				(SCP_LCRU_BASE + 0x30000)
#define SCP_GPR_SS_RST_CTL			(SCP_GPR_BASE)
#define SCP_GPR_MM_RST_CTL1			(SCP_GPR_BASE + 0x8)
#define SCP_GPR_MM_RST_CTL1_USB_HRST		BIT(10)
#define SCP_GPR_MM_RST_CTL1_USB_AUX_WELL_RST	BIT(11)
#define SCP_GPR_MM_RST_CTL1_OHCI_0_CLKCKT_RST	BIT(12)
#define SCP_GPR_MM_RST_CTL1_OHCI_0_CLKDIV_RST	BIT(13)
#define SCP_GPR_MM_RST_CTL1_USB_UTMI_PHY_RST	BIT(14)
#define SCP_GPR_MM_RST_CTL2			(SCP_GPR_BASE + 0x10)
#define SCP_GPR_MM_RST_CTL2_LSP_SS_APB		BIT(0)
#define SCP_GPR_MM_RST_CTL2_LSP_GPIO_DIV	BIT(1)
#define SCP_GPR_MM_RST_CTL2_LSP_GPIO32		BIT(2)
#define SCP_GPR_MM_RST_CTL2_LSP_GPIO32_APB	BIT(3)
#define SCP_GPR_MM_RST_CTL2_LSP_GPIO16		BIT(4)
#define SCP_GPR_MM_RST_CTL2_LSP_GPIO16_APB	BIT(5)
#define SCP_GPR_MM_RST_CTL2_LSP_GPIO8_1		BIT(6)
#define SCP_GPR_MM_RST_CTL2_LSP_GPIO8_1_APB	BIT(7)
#define SCP_GPR_MM_RST_CTL2_LSP_GPIO8_2		BIT(8)
#define SCP_GPR_MM_RST_CTL2_LSP_GPIO8_2_APB	BIT(9)
#define SCP_GPR_MM_RST_CTL2_LSP_TIMER1		BIT(10)
#define SCP_GPR_MM_RST_CTL2_LSP_TIMER2		BIT(11)
#define SCP_GPR_MM_RST_CTL2_LSP_TIMER3		BIT(12)
#define SCP_GPR_MM_RST_CTL2_LSP_TIMER4		BIT(13)
#define SCP_GPR_MM_RST_CTL2_LSP_TIMERS_APB	BIT(14)
#define SCP_GPR_MM_RST_CTL2_LSP_WDT		BIT(15)
#define SCP_GPR_MM_RST_CTL2_LSP_WDT_APB		BIT(16)
#define SCP_GPR_MM_RST_CTL2_LSP_UART_S		BIT(17)
#define SCP_GPR_MM_RST_CTL2_LSP_UART_S_APB	BIT(18)
#define SCP_GPR_MM_RST_CTL2_LSP_UART_A1		BIT(19)
#define SCP_GPR_MM_RST_CTL2_LSP_UART_A1_APB	BIT(20)
#define SCP_GPR_MM_RST_CTL2_LSP_UART_A2		BIT(21)
#define SCP_GPR_MM_RST_CTL2_LSP_UART_A2_APB	BIT(22)
#define SCP_GPR_MM_RST_CTL2_LSP_WDT_DIV		BIT(23)

#define SCP_GPR_MM_RST_CTL3			(SCP_GPR_BASE + 0x18)
#define SCP_GPR_MM_RST_CTL3_LSP_SPI1		BIT(0)
#define SCP_GPR_MM_RST_CTL3_LSP_SPI1_APB	BIT(1)
#define SCP_GPR_MM_RST_CTL3_LSP_SPI2		BIT(2)
#define SCP_GPR_MM_RST_CTL3_LSP_SPI2_APB	BIT(3)
#define SCP_GPR_MM_RST_CTL3_LSP_I2C2		BIT(4)
#define SCP_GPR_MM_RST_CTL3_LSP_I2C2_APB	BIT(5)
#define SCP_GPR_MM_RST_CTL3_LSP_I2C3		BIT(6)
#define SCP_GPR_MM_RST_CTL3_LSP_I2C3_APB	BIT(7)
#define SCP_GPR_MM_RST_CTL3_LSP_I2C4		BIT(8)
#define SCP_GPR_MM_RST_CTL3_LSP_I2C4_APB	BIT(9)
#define SCP_GPR_MM_RST_CTL3_LSP_I2C5		BIT(10)
#define SCP_GPR_MM_RST_CTL3_LSP_I2C5_APB	BIT(11)
#define SCP_GPR_MM_RST_CTL3_LSP_I2C6		BIT(12)
#define SCP_GPR_MM_RST_CTL3_LSP_I2C6_APB	BIT(13)
#define SCP_GPR_MM_RST_CTL3_LSP_ESPI		BIT(14)
#define SCP_GPR_MM_RST_CTL3_LSP_ESPI_RST	BIT(15)

#define SCP_GPR_LSP_CTL				(SCP_GPR_BASE + 0x50)
#define SCP_GPR_LSP_CTL_SEL_PERIPH_MASK		GENMASK(10, 8)
#define SCP_GPR_LSP_CTL_SEL_PERIPH_SHIFT	8

#define SCP_GPR_USB_CTL				(SCP_GPR_BASE + 0x58)
#define SCP_GPR_USB_CTL_FLADJ_VALUE_MASK	GENMASK(14, 9)
#define SCP_GPR_USB_CTL_FLADJ_ENABLE_MASK	GENMASK(20, 15)
#define SCP_GPR_USB_CTL_FLADJ_VALUE_SHIFT	9
#define SCP_GPR_USB_CTL_FLADJ_ENABLE_SHIFT	15

#define SCP_GPR_GMAC_DIV_CTL			(SCP_GPR_BASE + 0x128)
#define SCP_GPR_GMAC_DIV_CTL_GMAC0_DIV2_EN	BIT(0)
#define SCP_GPR_GMAC_DIV_CTL_GMAC1_DIV2_EN	BIT(4)

#define SCP_GPR_BOOT_INFO_STS			(SCP_GPR_BASE + 0x184)

#define SCP_PVT_BASE				(SCP_LCRU_BASE + 0x40000)
#define SCP_GPV_BASE				(SCP_LCRU_BASE + 0x100000)

#define CMU_CLKCHCTL_OFFSET(clkch)		(0x20 + (clkch) * 0x10)

#endif /* BS1000_DEF_H */
