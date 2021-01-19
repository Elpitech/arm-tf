/*
 * Copyright (c) 2018-2020, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <bm1000_cmu.h>
#include <bm1000_private.h>
#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <errno.h>
#include <lib/mmio.h>

struct baikal_clk pclk_list[20];

static uint32_t clkod;
static uint32_t clkr;
static uint32_t clkf_hi;
static uint32_t clkf_lo;

static void          cmu_set_clkf                (struct baikal_clk *pclk, uint64_t prate, uint64_t hz);
static void          cmu_get_dividers            (struct baikal_clk *pclk);
static void          cmu_calc_clkf               (uint64_t prate, uint64_t hz);
static uint64_t      cmu_calc_freq               (uint64_t prate);
static uint32_t      get_clk_ch_reg              (uint32_t cmu_clkch, uint32_t parent_cmu);
static int           cmu_clk_ch_calc_div         (uint32_t cmu_clkch, uint32_t parent_cmu, uint64_t hz);
static int           cmu_clk_ch_set_div          (uint32_t cmu_clkch, uint32_t parent_cmu, uint32_t divider);
static int           cmu_clk_ch_get_div          (uint32_t cmu_clkch, uint32_t parent_cmu);
static baikal_clk_t *pclk_get                    (uint32_t cmuid);
static int           cmu_core_runtime_set_rate   (uint32_t cmuid, uint64_t parent_hz, uint64_t hz);
static int           cmu_runtime_set_rate        (uint32_t cmuid, uint64_t parent_hz, uint64_t hz);
static int           cmu_periph_runtime_set_rate (uint32_t cmuid, uint64_t parent_hz, uint64_t hz);
static int           cmu_mshc_get_div            (uint32_t parent_cmu);
static int           cmu_mshc_set_div            (uint32_t parent_cmu, uint32_t selector);

static void cmu_get_dividers(struct baikal_clk *pclk)
{
	clkod = ((mmio_read_32(pclk->reg + CMU_PLL_CTL0) & CMU_CLKOD_MASK) >>
			CMU_CLKOD_SHIFT) + 1;
	clkr  = ((mmio_read_32(pclk->reg + CMU_PLL_CTL0) & CMU_CLKR_MASK)  >>
			CMU_CLKR_SHIFT)  + 1;

	clkf_lo = mmio_read_32(pclk->reg + CMU_PLL_CTL1) & CMU_CLKFLO_MASK;
	clkf_hi = mmio_read_32(pclk->reg + CMU_PLL_CTL2) & CMU_CLKFHI_MASK;
}

static void cmu_calc_clkf(uint64_t prate, uint64_t hz)
{
	/* This is a F*CKED UP place. FIXME!!!!1111 */
	clkf_hi = ((hz * clkod * clkr) << 1) / prate;
	/* You have to be very careful with the order of operations here */
	clkf_lo = ((hz * clkod) << 33) / (prate / clkr);
}

static uint64_t cmu_calc_freq(uint64_t prate)
{
	/* This is sort of a rounding for minimizing the error of calculation */
	uint64_t lower = ((prate * clkf_lo) & (1ULL << 32))?
		((prate * clkf_lo >> 33) + 1) : (prate * clkf_lo >> 33);

	uint64_t upper = prate * clkf_hi >> 1;
	return (upper + lower) / (clkod * clkr);
}

static void cmu_set_clkf(struct baikal_clk *pclk, uint64_t prate, uint64_t hz)
{
	cmu_get_dividers(pclk);
	cmu_calc_clkf(prate, hz);

	if (clkf_hi & CMU_CLKFHI_MASK) {
		clkf_hi &= CMU_CLKFHI_MASK;
	}

	mmio_write_32(pclk->reg + CMU_PLL_CTL1, (uint32_t)clkf_lo);
	mmio_write_32(pclk->reg + CMU_PLL_CTL2, (uint32_t)clkf_hi);

	INFO("%s: %llu Hz (clkod:0x%x clkr:0x%x clkf_hi:0x%x clkf_lo:0x%x)\n",
		pclk->name, hz, clkod, clkr, clkf_hi, clkf_lo);
}

static baikal_clk_t *pclk_get(uint32_t cmuid)
{
	baikal_clk_t *pclk;
	for (pclk = pclk_list; pclk->hw_id; ++pclk) {
		if (pclk->hw_id == cmuid) {
			return pclk;
		}
	}

	ERROR("%s: requested cmuid (0x%x) is not found\n", __func__, cmuid);
	return NULL;
}

static uint32_t get_clk_ch_reg(uint32_t cmu_clkch, uint32_t parent_cmu)
{
	struct baikal_clk *pclk = pclk_get(parent_cmu);
	if (!pclk) {
		return 0;
	}

	return pclk->reg + CMU_CLK_CH_BASE + cmu_clkch * 0x10;
}

static int cmu_pll_lock_debounce(const struct baikal_clk *const pclk)
{
	uint64_t debounce, timeout;

	assert(pclk);
	assert(pclk->reg);

	for (timeout = timeout_init_us(3000000); !timeout_elapsed(timeout);) {
		debounce = timeout_init_us(10000);
		while (mmio_read_32(pclk->reg + CMU_PLL_CTL0) & CMU_LOCK) {
			if (timeout_elapsed(debounce)) {
				return 0;
			}
		}
	}

	WARN("%s: %s(): LOCK timeout expired\n", pclk->name, __func__);
	return -ETIMEDOUT;
}

int cmu_pll_is_enabled(uint32_t cmuid)
{
	struct baikal_clk *pclk = pclk_get(cmuid);
	return mmio_read_32(pclk->reg + CMU_PLL_CTL0) & CMU_CTL_EN &&
	       mmio_read_32(pclk->reg + CMU_PLL_CTL0) & CMU_LOCK;
}

int cmu_pll_disable(uint32_t cmuid)
{
	struct baikal_clk *pclk = pclk_get(cmuid);
	if (!pclk) {
		return -ENXIO;
	}

	mmio_clrbits_32(pclk->reg + CMU_PLL_CTL0, CMU_CTL_EN);
	return 0;
}

int cmu_pll_enable(uint32_t cmuid)
{
	int ret;
	struct baikal_clk *pclk = pclk_get(cmuid);
	if (!pclk) {
		return -ENXIO;
	}

	if (cmu_pll_is_enabled(cmuid)) {
		WARN("%s: %s(): PLL already enabled\n", pclk->name, __func__);
		return 0;
	}

	int64_t current = cmu_pll_get_rate(cmuid, pclk->parent_freq);
	if (current < 0) {
		return current;
	}

	if (current < pclk->min || current > pclk->max) {
		ERROR("%s: %s(): currently set frequency is out off limits" \
			" [%d..%d] Hz\nEnabling rejected. Please set the rate.\n",
			pclk->name, __func__, pclk->min, pclk->max);
		return -EINVAL;
	}

	INFO("%s: %s() on %lld Hz\n", pclk->name, __func__, current);

	mmio_setbits_32(pclk->reg + CMU_PLL_CTL0, CMU_CTL_EN);
	mmio_clrbits_32(pclk->reg + CMU_PLL_CTL0, CMU_BYPASS);
	mmio_setbits_32(pclk->reg + CMU_PLL_CTL0, CMU_RST);

	ret = cmu_pll_lock_debounce(pclk);
	if (ret) {
		return ret;
	}

	mmio_setbits_32(pclk->reg + CMU_PLL_CTL6, CMU_SWEN);
	mmio_clrbits_32(pclk->reg + CMU_PLL_CTL6, CMU_SWRST);
	return 0;
}

int64_t cmu_pll_get_rate(uint32_t cmuid, uint64_t parent_hz)
{
	uint64_t hz;
	uint64_t prate;
	struct baikal_clk *pclk = pclk_get(cmuid);
	if (!pclk) {
		return -ENXIO;
	}

	if (parent_hz) {
		prate = parent_hz;
	} else {
		prate = pclk->parent_freq;
	}

	cmu_get_dividers(pclk);
	hz = cmu_calc_freq(prate);

	INFO("%s: %llu Hz (clkod:0x%x clkr:0x%x clkf_hi:0x%x clkf_lo:0x%x)\n",
		pclk->name, hz, clkod, clkr, clkf_hi, clkf_lo);

	return hz;
}

static int cmu_periph_runtime_set_rate(uint32_t cmuid, uint64_t parent_hz, uint64_t hz)
{
	int ret;
	uint64_t prate;
	struct baikal_clk *pclk = pclk_get(cmuid);
	if (!pclk) {
		return -ENXIO;
	}

	if (parent_hz) {
		prate = parent_hz;
	} else {
		prate = pclk->parent_freq;
	}

	mmio_clrbits_32(pclk->reg + CMU_PLL_CTL6, CMU_SWEN);
	cmu_set_clkf(pclk, prate, hz);
	mmio_setbits_32(pclk->reg + CMU_PLL_CTL0, CMU_RST);

	ret = cmu_pll_lock_debounce(pclk);
	if (ret) {
		return ret;
	}

	mmio_setbits_32(pclk->reg + CMU_PLL_CTL6, CMU_SWEN);
	return 0;
}

int64_t cmu_pll_round_rate(uint32_t cmuid, uint64_t parent_hz, uint64_t hz)
{
	uint64_t prate;

	struct baikal_clk *pclk = pclk_get(cmuid);
	if (!pclk) {
		return -ENXIO;
	}

	if (hz < pclk->min) {
		return pclk->min;
	}

	if (hz > pclk->max) {
		return pclk->max;
	}

	if (parent_hz) {
		prate = parent_hz;
	} else {
		prate = pclk->parent_freq;
	}

	cmu_get_dividers(pclk);
	cmu_calc_clkf(prate, hz);

	if (clkf_hi & CMU_CLKFHI_MASK) {
		clkf_hi &= CMU_CLKFHI_MASK;
	}

	return cmu_calc_freq(prate);
}

static int cmu_core_runtime_set_rate(uint32_t cmuid, uint64_t parent_hz, uint64_t hz)
{
	struct baikal_clk *pclk = pclk_get(cmuid);
	if (!pclk) {
		return -ENXIO;
	}

	int64_t cur_hz = cmu_pll_get_rate(cmuid, parent_hz);
	if (cur_hz < 0) {
		return -ENXIO;
	}

	/* We only can change frequency in runtime when delta is less than 10% */
#ifndef BE_QEMU
	int delta = hz - cur_hz;
#else
	int delta = 0;
#endif
	if (!delta) {
		return 0;
	}

	int abs_delta = delta < 0? -delta : delta;
	if (abs_delta > cur_hz / 10) {
		cmu_core_runtime_set_rate(cmuid, parent_hz, delta > 0?
			(hz - cur_hz / 10) : (hz + cur_hz / 10));
	}

	cmu_set_clkf(pclk, parent_hz, cur_hz + delta);
	WAIT_DELAY((1),	40000 /* 40us*/,);
	return 0;
}

static int cmu_runtime_set_rate(uint32_t cmuid, uint64_t parent_hz, uint64_t hz)
{
	struct baikal_clk *pclk = pclk_get(cmuid);
	if (!pclk) {
		return -ENXIO;
	}

	INFO("%s: %s(): %llu\n", pclk->name, __func__, hz);
	if (pclk->is_cpu) {
		return cmu_core_runtime_set_rate(cmuid, parent_hz, hz);
	} else {
		return cmu_periph_runtime_set_rate(cmuid, parent_hz, hz);
	}
}

int cmu_pll_set_rate(uint32_t cmuid, uint64_t parent_hz, uint64_t hz)
{
	uint64_t prate;
	struct baikal_clk *pclk = pclk_get(cmuid);
	if (!pclk) {
		return -ENXIO;
	}

	if (hz < pclk->min || hz > pclk->max) {
		ERROR("%s: %s(): requested frequency is out of limits [%d..%d] Hz\n",
			pclk->name, __func__, pclk->min, pclk->max);
		return -EINVAL;
	}

	if (parent_hz) {
		prate = parent_hz;
	} else {
		prate = pclk->parent_freq;
	}

	pclk->current_freq = hz;
	if (hz == cmu_pll_get_rate(cmuid, parent_hz)) {
		INFO("%s: %s(): already set to %u Hz\n", pclk->name, __func__,
			pclk->current_freq);
		return 0;
	}

	if (cmu_pll_is_enabled(cmuid)) {
		return cmu_runtime_set_rate(cmuid, prate, hz);
	} else {
		INFO("%s: PLL is not enabled, set it to %llu Hz\n", pclk->name,
			hz);
		cmu_set_clkf(pclk, prate, hz);
		return 0;
	}
}

int cmu_clk_ch_enable(uint32_t cmu_clkch, uint32_t parent_cmu)
{
	INFO("%s: parent_cmu 0x%x cmu_clkch %u\n", __func__, parent_cmu, cmu_clkch);
	int timeout = 0;

	if (!(cmu_pll_is_enabled(parent_cmu))) {
		return -ENODEV;
	}

	if (cmu_clk_ch_is_enabled(cmu_clkch, parent_cmu)) {
		WARN("%s: clock channel # %u of PLL already enabled\n",
			__func__, cmu_clkch);
		return 0;
	}

	uint32_t clk_ch_reg = get_clk_ch_reg(cmu_clkch, parent_cmu);
	if (!clk_ch_reg) {
		return -ENXIO;
	}

	uint32_t reg = mmio_read_32(clk_ch_reg);

	reg |= CLK_CH_EN;
	mmio_write_32(clk_ch_reg, reg);

	while (!(mmio_read_32(clk_ch_reg) & CLK_CH_LOCK_CLK_RDY)) {
		timeout++;
		if (timeout > 10000) {
			WARN("%s: LOCK_CLK_RDY timeout expired\n", __func__);
			return -ETIMEDOUT;
		}
	}

	reg = mmio_read_32(clk_ch_reg);
	reg &= ~CLK_CH_SWRST;
	mmio_write_32(clk_ch_reg, reg);
	return 0;
}

int cmu_clk_ch_disable(uint32_t cmu_clkch, uint32_t parent_cmu)
{
	uint32_t clk_ch_reg = get_clk_ch_reg(cmu_clkch, parent_cmu);
	if (!clk_ch_reg) {
		return -ENXIO;
	}

	mmio_clrbits_32(clk_ch_reg, CLK_CH_EN);
	return 0;
}

int64_t cmu_clk_ch_get_rate(uint32_t cmu_clkch, uint32_t parent_cmu)
{
	struct baikal_clk *pclk = pclk_get(parent_cmu);
	if (!pclk) {
		return -ENXIO;
	}

	int64_t pll_freq = cmu_pll_get_rate(parent_cmu, pclk->parent_freq);
	if (pll_freq < 0) {
		return -ENXIO;
	}

	uint32_t divider = cmu_clk_ch_get_div(cmu_clkch, parent_cmu);
	INFO("%s: CMU: %lld\n", __func__, pll_freq / divider);
	return pll_freq / divider;
}

int cmu_clk_ch_is_enabled(uint32_t cmu_clkch, uint32_t parent_cmu)
{
	uint32_t clk_ch_reg = get_clk_ch_reg(cmu_clkch, parent_cmu);
	if (!clk_ch_reg) {
		return -ENXIO;
	}

	return (mmio_read_32(clk_ch_reg) & CLK_CH_EN) &&
		cmu_pll_is_enabled(parent_cmu);
}

int64_t cmu_clk_ch_round_rate(uint32_t cmu_clkch, uint32_t parent_cmu, uint64_t hz)
{
	INFO("%s: CMU: 0x%x\n", __func__, parent_cmu);
	struct baikal_clk *pclk = pclk_get(parent_cmu);
	if (!pclk) {
		return -ENXIO;
	}

	if (pclk->deny_reconfig_pll == true) {
		if (hz > pclk->max) {
			return pclk->max;
		}

		int64_t current_freq = cmu_pll_get_rate(parent_cmu, pclk->parent_freq);
		if (current_freq < 0) {
			return current_freq;
		}

		int divider = cmu_clk_ch_calc_div(cmu_clkch, parent_cmu, hz);
		INFO("%s: (current_freq / div) %lld\n", __func__,
			current_freq / divider);
		return current_freq / divider;
	} else {
		INFO("%s: cmu_pll_round_rate %lld\n", __func__,
			cmu_pll_round_rate(parent_cmu, hz, pclk->parent_freq));
		return cmu_pll_round_rate(parent_cmu, pclk->parent_freq, hz);
	}
}

static int cmu_clk_ch_set_div(uint32_t cmu_clkch, uint32_t parent_cmu, uint32_t divider)
{
	int timeout = 0;

	struct baikal_clk *pclk = pclk_get(parent_cmu);
	if (!pclk) {
		return -ENXIO;
	}

	if (divider > (CLK_CH_VAL_CLKDIV_MAX * CMU_MSHC_DIV)) {
		return -EINVAL;
	}

	uint32_t clk_ch_reg = get_clk_ch_reg(cmu_clkch, parent_cmu);
	if (!clk_ch_reg) {
		return -ENXIO;
	}

	if (cmu_clkch == pclk->cmu_clkch_mshc) {
		if (divider > CLK_CH_VAL_CLKDIV_MAX) {
			divider /= CMU_MSHC_DIV;
			cmu_mshc_set_div(parent_cmu, CMU_MSHC_DIV_EN);
		} else {
			cmu_mshc_set_div(parent_cmu, CMU_MSHC_DIV_DIS);
		}
	}

	uint32_t reg = mmio_read_32(clk_ch_reg);
	reg &= ~CLK_CH_VAL_CLKDIV_MASK;
	reg |= divider << CLK_CH_VAL_CLKDIV_SHIFT;
	reg |= CLK_CH_SET_CLKDIV;
	mmio_write_32(clk_ch_reg, reg);

	while (!(mmio_read_32(clk_ch_reg) & CLK_CH_LOCK_CLKDIV)) {
		timeout++;
		if (timeout > 10000) {
			WARN("%s: LOCK_CLKDIV timeout expired\n", __func__);
			return -ETIMEDOUT;
		}
	}

	return 0;
}

int cmu_clk_ch_set_rate(uint32_t cmu_clkch, uint32_t parent_cmu, uint64_t hz)
{
	INFO("%s: parent_cmu 0x%x cmu_clkch %u, %llu Hz\n", __func__, parent_cmu, cmu_clkch, hz);

	struct baikal_clk *pclk = pclk_get(parent_cmu);
	if (!pclk) {
		return -ENXIO;
	}

	if (hz == cmu_clk_ch_get_rate(cmu_clkch, parent_cmu)) {
		INFO("%s: requested frequency (%llu Hz) is already set\n",
			__func__, hz);
		return 0;
	}

	if (pclk->deny_reconfig_pll == true) {
		if (!(cmu_pll_is_enabled(parent_cmu))) {
			return -ENODEV;
		}

		if (hz != cmu_clk_ch_round_rate(cmu_clkch, parent_cmu, hz)) {
			return -EINVAL;
		}

		int divider = cmu_clk_ch_calc_div(cmu_clkch, parent_cmu, hz);
		INFO("%s: divider 0x%x\n", __func__, divider);

		if (cmu_clk_ch_is_enabled(cmu_clkch, parent_cmu)) {
			cmu_clk_ch_disable(cmu_clkch, parent_cmu);

			if (cmu_clk_ch_set_div(cmu_clkch, parent_cmu, divider)) {
				return -ETIMEDOUT;
			}

			if (cmu_clk_ch_enable(cmu_clkch, parent_cmu)) {
				return -ETIMEDOUT;
			}
		} else {
			if (cmu_clk_ch_set_div(cmu_clkch, parent_cmu, divider)) {
				return -ETIMEDOUT;
			}
		}
	} else {
		return cmu_pll_set_rate(parent_cmu, pclk->parent_freq, hz);
	}

	return 0;
}

/* Might be a useful hack, to set NR to parent frequency / 10^6 */
int cmu_reconfig_nr(uint32_t cmuid)
{
	/* Target VCO reference clock is 250 kHz. */
	/* May help us avoid fractional clkf in most cases. */
	uint32_t vco_ref = 250000;
	uint64_t prate;
	uint64_t hz;
	uint32_t reg;
	int ret;
	struct baikal_clk *pclk = pclk_get(cmuid);
	if (!pclk) {
		return -ENXIO;
	}

	prate = pclk->parent_freq;
	hz = pclk->current_freq;

	if (hz < pclk->min || hz > pclk->max) {
		ERROR("%s: %s(): currently set frequency is out off limits" \
			" [%d..%d] Hz\nConfiguration rejected.\n",
			pclk->name, __func__, pclk->min, pclk->max);
		return -EINVAL;
	}

	/* Maximum allowed CLKR is 4095 */
	clkr = prate / vco_ref - 1;
	if (clkr > 4095) {
		/* This condition would unlikely occur (Fref > 256 MHz). */
		/* Dirty hack - set VCO reference to 250 kHz. */
		clkr = prate / 250000 - 1;
	}

	reg = mmio_read_32(pclk->reg + CMU_PLL_CTL0);
	reg &= CMU_CLKR_MASK;
	reg >>= CMU_CLKR_SHIFT;
	if (reg == clkr) {
		INFO("%s: %s(): requested clkr (0x%x) is already set\n",
			pclk->name, __func__, reg + 1);
		return 0;
	}

	mmio_clrbits_32(pclk->reg + CMU_PLL_CTL6, CMU_SWEN);
	mmio_clrbits_32(pclk->reg + CMU_PLL_CTL0, CMU_CLKR_MASK);

	reg  = mmio_read_32(pclk->reg + CMU_PLL_CTL0);
	reg &= ~(CMU_CLKR_MASK);
	reg |= clkr << CMU_CLKR_SHIFT;

	mmio_write_32(pclk->reg + CMU_PLL_CTL0, reg);
	INFO("%s: %s(): %llu Hz\n", pclk->name, __func__, hz);
	cmu_set_clkf(pclk, prate, hz);
	mmio_setbits_32(pclk->reg + CMU_PLL_CTL0, CMU_RST);

	ret = cmu_pll_lock_debounce(pclk);
	if (ret) {
		return ret;
	}

	mmio_setbits_32(pclk->reg + CMU_PLL_CTL6, CMU_SWEN);
	return 0;
}

int cmu_corepll_set_clken_gen(uint32_t cmuid, uint32_t offset, uint8_t divisor)
{
	struct baikal_clk *pclk = pclk_get(cmuid);
	if (!pclk) {
		return -ENXIO;
	}

	uint32_t reg = mmio_read_32(pclk->reg + offset);
	reg &= ~CLK_CH_VAL_CLKDIV_MASK;
	reg |= divisor << CLK_CH_VAL_CLKDIV_SHIFT;
	reg |= CLK_CH_SET_CLKDIV;
	mmio_write_32(pclk->reg + offset, reg);

	/* remove lock bit check, because lock bit set when pll enable */
	mmio_setbits_32(pclk->reg + offset, CLK_CH_EN);
	return 0;
}

static int cmu_clk_ch_calc_div(uint32_t cmu_clkch, uint32_t parent_cmu, uint64_t hz)
{
	int64_t current_freq;
	int divider;
	int max = CLK_CH_VAL_CLKDIV_MAX;

	struct baikal_clk *pclk = pclk_get(parent_cmu);
	if (!pclk) {
		return -ENXIO;
	}

	if (cmu_clkch == pclk->cmu_clkch_mshc) {
		max *= CMU_MSHC_DIV;
	}

	current_freq = cmu_pll_get_rate(parent_cmu, pclk->parent_freq);
	if (current_freq < 0) {
		return current_freq;
	}

	/* raw */
	divider = current_freq / hz;
	if (divider < 1) {
		divider = 1;
	}

	/* mshc */
	if (cmu_clkch == pclk->cmu_clkch_mshc) {
		if (divider > CLK_CH_VAL_CLKDIV_MAX) {
			divider /= CMU_MSHC_DIV; /* round */
			divider *= CMU_MSHC_DIV;
		}
	}

	if (divider > max) {
		divider = max;
	}

	return divider;
}

static int cmu_clk_ch_get_div(uint32_t cmu_clkch, uint32_t parent_cmu)
{
	struct baikal_clk *pclk = pclk_get(parent_cmu);
	if (!pclk) {
		return -ENXIO;
	}

	uint32_t clk_ch_reg = get_clk_ch_reg(cmu_clkch, parent_cmu);
	if (!clk_ch_reg) {
		return -ENXIO;
	}

	uint32_t reg = mmio_read_32(clk_ch_reg);
	uint32_t divider = (reg & CLK_CH_VAL_CLKDIV_MASK) >> CLK_CH_VAL_CLKDIV_SHIFT;

	if (cmu_clkch == pclk->cmu_clkch_mshc) {
		if (cmu_mshc_get_div(parent_cmu) == CMU_MSHC_DIV_EN) {
			divider *= CMU_MSHC_DIV;
		}
	}

	return divider;
}

static int cmu_mshc_get_div (uint32_t parent_cmu)
{
	struct baikal_clk *pclk = pclk_get(parent_cmu);
	if (!pclk) {
		return -ENXIO;
	}

	uint32_t reg = mmio_read_32(pclk->reg + CMU_GPR_AVLSP_MSHC_CFG);
	return (reg & CMU_MSHC_CLKDIV_MASK) >> CMU_MSHC_CLKDIV_SHIFT;
}

static int cmu_mshc_set_div (uint32_t parent_cmu, uint32_t selector)
{
	struct baikal_clk *pclk = pclk_get(parent_cmu);
	if (!pclk) {
		return -ENXIO;
	}

	if ((selector != CMU_MSHC_DIV_EN) && (selector != CMU_MSHC_DIV_DIS)) {
		return -EINVAL;
	}

	uint32_t reg = mmio_read_32(pclk->reg + CMU_GPR_AVLSP_MSHC_CFG);
	reg &= ~CMU_MSHC_CLKDIV_MASK;
	reg |= selector << CMU_MSHC_CLKDIV_SHIFT;
	mmio_write_32(pclk->reg + CMU_GPR_AVLSP_MSHC_CFG, reg);
	return 0;
}

#if 0
static uint8_t find_clk_ch_divider(uint64_t hz, uint32_t parent_cmu)
{
	int divider;
	struct baikal_clk *pclk = pclk_get(parent_cmu);
	int64_t current_freq = cmu_pll_get_rate(parent_cmu, pclk->parent_freq);
	if (current_freq < 0) {
		return current_freq;
	}

	if (hz > current_freq) {
		divider = 1;
	} else if (current_freq / hz > 0xff) {
		divider = 0xff;
	} else {
		divider = current_freq / hz;
	}

	return divider;
}

int cmu_mshc_speed_tune(uint32_t selector)
{
	struct baikal_clk *pclk = pclk_get_by_name("baikal-avlsp_cmu0");
	if (!pclk) {
		return -ENXIO;
	}

	uint32_t reg = mmio_read_32(pclk->reg + CMU_GPR_AVLSP_MSHC_CFG);
	reg &= ~(CMU_MSHC_CLKDIV_MASK);
	reg |= selector << CMU_MSHC_CLKDIV_SHIFT;
	mmio_write_32(pclk->reg + CMU_GPR_AVLSP_MSHC_CFG, reg);
	reg = mmio_read_32(pclk->reg + CMU_GPR_AVLSP_MSHC_CFG);
	return 0;
}
#endif
