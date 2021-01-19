/*
 * Copyright (c) 2018-2020, Baikal Electronics JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BM1000_CMU_H
#define BM1000_CMU_H

#include <stdbool.h>
#include <stdint.h>

typedef struct baikal_clk {
	uint32_t	hw_id;
	uint32_t	reg;
	const char	*name;
	unsigned int	min, max;
	uint32_t	current_freq;
	uint32_t	parent_freq;
	bool		is_cpu;
	bool		deny_reconfig_pll;
	int		cmu_clkch_mshc;
} baikal_clk_t;

#define DEFAULT_PARENT_RATE		25000000
#define DEFAULT_FREQUENCY		255000000

/* mshc */
#define CMU_MSHC_DIV_EN			0
#define CMU_MSHC_DIV_DIS		1
#define CMU_MSHC_DIV			16

/* cmd */
#define CMU_PLL_SET_RATE		0
#define CMU_PLL_GET_RATE		1
#define CMU_PLL_ENABLE			2
#define CMU_PLL_DISABLE			3
#define CMU_PLL_ROUND_RATE		4
#define CMU_PLL_IS_ENABLED		5
#define CMU_CLK_CH_SET_RATE		6
#define CMU_CLK_CH_GET_RATE		7
#define CMU_CLK_CH_ENABLE		8
#define CMU_CLK_CH_DISABLE		9
#define CMU_CLK_CH_ROUND_RATE		10
#define CMU_CLK_CH_IS_ENABLED		11
#define CMU_CLK_MSHC_SPEED		12

/* reg */
#define CMU_PLL_CTL0			0x0000
#define CMU_PLL_CTL1			0x0004
#define CMU_PLL_CTL2			0x0008
#define CMU_PLL_CTL3			0x000C
#define CMU_PLL_CTL4			0x0010
#define CMU_PLL_CTL5			0x0014
#define CMU_PLL_CTL6			0x0018
#define CMU_CLK_CH_BASE			0x0020
#define CMU_GPR				0x50000
#define CMU_GPR_AVLSP_MSHC_CFG		(CMU_GPR + 0x20)
#define CMU_MSHC_CLKDIV_SHIFT		16
#define CMU_MSHC_CLKDIV_MASK		(1 << CMU_MSHC_CLKDIV_SHIFT)

#define CMU_CTL_EN			(1 << 0)
#define CMU_RST				(1 << 1)
#define CMU_CLKR_SHIFT			(2)
#define CMU_CLKR_MASK			(0xFFF << CMU_CLKR_SHIFT)
#define CMU_CLKOD_SHIFT			(14)
#define CMU_CLKOD_MASK			(0x7FF << CMU_CLKOD_SHIFT)
#define CMU_BYPASS			(1 << 30)
#define CMU_LOCK			(1U << 31)

#define CMU_CLKFLO_MASK			(~0x0)
#define CMU_CLKFHI_MASK			0xFFFFFF

#define CMU_SWRST			(1 << 1)
#define CMU_SWEN			(1 << 0)

#define CLK_CH_VAL_CLKDIV_MAX		0xFF
#define CLK_CH_VAL_CLKDIV_SHIFT		4
#define CLK_CH_VAL_CLKDIV_MASK		(0xFF << CLK_CH_VAL_CLKDIV_SHIFT)
#define CLK_CH_LOCK_CLKDIV		(1U << 31)
#define CLK_CH_LOCK_CLK_RDY		(1 << 30)
#define CLK_CH_SET_CLKDIV		(1 << 2)
#define CLK_CH_SWRST			(1 << 1)
#define CLK_CH_EN			(1 << 0)

#define CA57_CLKEN			0x800
#define CA57_PCLKDBGCLKEN		0x804
#define CA57_SCLKEN			0x808
#define CA57_ATCLKEN			0x80C
#define CA57_CNTCLKEN			0x810

/* pll */
int64_t cmu_pll_round_rate (uint32_t cmuid, uint64_t parent_hz, uint64_t hz);
int     cmu_pll_set_rate   (uint32_t cmuid, uint64_t parent_hz, uint64_t hz);
int64_t cmu_pll_get_rate   (uint32_t cmuid, uint64_t parent_hz);
int     cmu_pll_enable     (uint32_t cmuid);
int     cmu_pll_is_enabled (uint32_t cmuid);
int     cmu_pll_disable    (uint32_t cmuid);
/* ch */
int64_t cmu_clk_ch_round_rate (uint32_t cmu_clkch, uint32_t parent_cmu, uint64_t hz);
int     cmu_clk_ch_set_rate   (uint32_t cmu_clkch, uint32_t parent_cmu, uint64_t hz);
int64_t cmu_clk_ch_get_rate   (uint32_t cmu_clkch, uint32_t parent_cmu);
int     cmu_clk_ch_enable     (uint32_t cmu_clkch, uint32_t parent_cmu);
int     cmu_clk_ch_is_enabled (uint32_t cmu_clkch, uint32_t parent_cmu);
int     cmu_clk_ch_disable    (uint32_t cmu_clkch, uint32_t parent_cmu);
/* boot */
int     cmu_reconfig_nr           (uint32_t cmuid);
int     cmu_corepll_set_clken_gen (uint32_t cmuid, uint32_t offset, uint8_t divisor);

#endif /* BM1000_CMU_H */
