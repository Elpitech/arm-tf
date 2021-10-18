/*
 * Copyright (c) 2018-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BM1000_CMU_H
#define BM1000_CMU_H

#include <stdbool.h>
#include <stdint.h>

struct cmu_desc {
	uintptr_t	base;
	const char	*name;
	uint32_t	frefclk;
	uint32_t	fpllmin;
	uint32_t	fpllmax;
	uint32_t	fpllreq;
	bool		deny_pll_reconf;
	int8_t		mshc_clkch;
};

typedef struct cmu_pll_ctl_vals {
	uint16_t	clkod;
	uint16_t	clkr;
	uint32_t	clkfhi;
	uint32_t	clkflo;
	uint8_t		pgain;
	uint8_t		igain;
	uint8_t		ipgain;
	uint8_t		iigain;
} cmu_pll_ctl_vals_t;

#define CMU_REFCLK_DEFAULT_FREQ		25000000
#define CMU_PLL_DEFAULT_FREQ		255000000
#define CMU_PLL_MAX_FREQ		2200000000

struct cmu_desc *const cmu_desc_get_by_idx(const unsigned idx);
bool cmu_is_mmca57(const uintptr_t base);

/* PLL functions */
int64_t	cmu_pll_get_rate  (const uintptr_t base, uint64_t frefclk);
int	cmu_pll_set_rate  (const uintptr_t base, uint64_t frefclk, const uint64_t fpllreq);
int	cmu_pll_disable	  (const uintptr_t base);
int	cmu_pll_enable	  (const uintptr_t base);
int	cmu_pll_is_enabled(const uintptr_t base);
void	cmu_pll_on	  (const uintptr_t base, const cmu_pll_ctl_vals_t *const pllinit);
void	cmu_pll_reconf_nr (struct cmu_desc *cmu);
int64_t	cmu_pll_round_rate(const uintptr_t base, uint64_t frefclk, const uint64_t fpllreq);

/* Clock channel functions */
int64_t	cmu_clkch_get_rate	(const uintptr_t base, const unsigned clkch);
int	cmu_clkch_set_rate	(const uintptr_t base, const unsigned clkch, const uint64_t fclkchreq);
int	cmu_clkch_disable	(const uintptr_t base, const unsigned clkch);
int	cmu_clkch_enable	(const uintptr_t base, const unsigned clkch);
void	cmu_clkch_enable_by_base(const uintptr_t base, const unsigned div);
int	cmu_clkch_is_enabled	(const uintptr_t base, const unsigned clkch);
int64_t	cmu_clkch_round_rate	(const uintptr_t base, const unsigned clkch, const uint64_t fclkchreq);

#endif /* BM1000_CMU_H */
