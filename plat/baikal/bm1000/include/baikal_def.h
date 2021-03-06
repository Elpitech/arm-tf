/*
 * Copyright (c) 2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BAIKAL_DEF_H
#define BAIKAL_DEF_H

#include <bm1000_def.h>

#define BAIKAL_BL31_PLAT_PARAM_VAL	ULL(0x0f1e2d3c4b5a6978)
#define BAIKAL_PRIMARY_CPU		U(0)

#define BAIKAL_GICD_BASE		MMUSB_GICD_BASE
#define BAIKAL_GICR_BASE		MMUSB_GICR_BASE

#define BAIKAL_GPIO32_BASE		MMAVLSP_GPIO32_BASE

#define BAIKAL_IRQ_SEC_SGI_0		8
#define BAIKAL_IRQ_SEC_SGI_1		9
#define BAIKAL_IRQ_SEC_SGI_2		10
#define BAIKAL_IRQ_SEC_SGI_3		11
#define BAIKAL_IRQ_SEC_SGI_4		12
#define BAIKAL_IRQ_SEC_SGI_5		13
#define BAIKAL_IRQ_SEC_SGI_6		14
#define BAIKAL_IRQ_SEC_SGI_7		15

#define BAIKAL_CONSOLE_16550
#define BAIKAL_UART_BASE		MMAVLSP_UART1_BASE
#define BAIKAL_UART_CLK_IN_HZ		7372800
#define BAIKAL_UART_BAUDRATE		115200

#define BAIKAL_SPI_BASE			MMAVLSP_SPI_BASE

#define BAIKAL_FLASH_MAP_FDT		U(0x40000)
#define BAIKAL_FLASH_MAP_SCP		U(0x80000)
#define BAIKAL_FLASH_MAP_FIP		U(0x110000)

#define SYS_COUNTER_FREQ_IN_TICKS	ULL(50000000)

#if defined(BAIKAL_DBM)
# define BAIKAL_BOOT_SPI_CS_GPIO_PIN	24
# define BAIKAL_DDR_CUSTOM_CLOCK_FREQ	800
# define BAIKAL_DIMM_SPD_I2C		MMAVLSP_I2C2_BASE
#elif defined(BAIKAL_MBM)
# define BAIKAL_DDR_CUSTOM_CLOCK_FREQ	1200
# define BAIKAL_DIMM_SPD_SMBUS		MMAVLSP_SMBUS1_BASE
# if BOARD_VER == 2
#  define BAIKAL_LVDS_BKLT_EN_GPIO_PIN	16
#  define BAIKAL_FLASH_4BYTEADR
# endif
# if BOARD_VER == 0
#  define BAIKAL_LVDS_CLKEN_GPIO_PIN	17
#  define BAIKAL_HDMI_CLKEN_GPIO_PIN	18
# endif
#elif defined(BAIKAL_QEMU)
# define BAIKAL_BOOT_SPI_CS_GPIO_PIN	24
# define BAIKAL_FLASH_4BYTEADR
# undef  SYS_COUNTER_FREQ_IN_TICKS
# define SYS_COUNTER_FREQ_IN_TICKS	ULL((1000 * 1000 * 1000) / 16)
#endif

#endif /* BAIKAL_DEF_H */
