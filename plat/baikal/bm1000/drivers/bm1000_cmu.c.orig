/*
 * Copyright (c) 2018-2021, Baikal Electronics, JSC. All rights reserved.
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
#include <lib/utils_def.h>

#define CMU_PLL_CTL0_REG			0x0000
#define CMU_PLL_CTL0_CTL_EN			BIT(0)
#define CMU_PLL_CTL0_RST			BIT(1)
#define CMU_PLL_CTL0_CLKR_MASK			GENMASK(   13,  2)
#define CMU_PLL_CTL0_CLKR_SET(x)		SETMASK(x, 13,  2)
#define CMU_PLL_CTL0_CLKR_SHIFT			2
#define CMU_PLL_CTL0_CLKOD_MASK			GENMASK(   24, 14)
#define CMU_PLL_CTL0_CLKOD_SET(x)		SETMASK(x, 24, 14)
#define CMU_PLL_CTL0_CLKOD_SHIFT		14
#define CMU_PLL_CTL0_BYPASS			BIT(30)
#define CMU_PLL_CTL0_LOCK			BIT(31)

#define CMU_PLL_CTL1_REG			0x0004
#define CMU_PLL_CTL2_REG			0x0008
#define CMU_PLL_CTL2_CLKFHI_MASK		GENMASK(21, 0)

#define CMU_PLL_CTL4_REG			0x0010
#define CMU_PLL_CTL4_PGAIN_LGMLT_MASK		GENMASK(    5,  0)
#define CMU_PLL_CTL4_PGAIN_LGMLT_SET(x)		SETMASK(x,  5,  0)
#define CMU_PLL_CTL4_IGAIN_LGMLT_MASK		GENMASK(   11,  6)
#define CMU_PLL_CTL4_IGAIN_LGMLT_SET(x)		SETMASK(x, 11,  6)
#define CMU_PLL_CTL4_IPGAIN_LGMLT_MASK		GENMASK(   17, 12)
#define CMU_PLL_CTL4_IPGAIN_LGMLT_SET(x)	SETMASK(x, 17, 12)
#define CMU_PLL_CTL4_IIGAIN_LGMLT_MASK		GENMASK(   23, 18)
#define CMU_PLL_CTL4_IIGAIN_LGMLT_SET(x)	SETMASK(x, 23, 18)

#define CMU_PLL_CTL6_REG			0x0018
#define CMU_PLL_CTL6_SWEN			BIT(0)
#define CMU_PLL_CTL6_SWRST			BIT(1)

#define CMU_CLKCH0_CTL_REG			0x0020
#define CMU_CLKCH_CTL_CLK_EN			BIT(0)
#define CMU_CLKCH_CTL_SWRST			BIT(1)
#define CMU_CLKCH_CTL_SET_CLKDIV		BIT(2)
#define CMU_CLKCH_CTL_VAL_CLKDIV_MASK		GENMASK(   11, 4)
#define CMU_CLKCH_CTL_VAL_CLKDIV_SET(x)		SETMASK(x, 11, 4)
#define CMU_CLKCH_CTL_VAL_CLKDIV_MAX		0xff
#define CMU_CLKCH_CTL_VAL_CLKDIV_SHIFT		4
#define CMU_CLKCH_CTL_CLK_RDY			BIT(30)
#define CMU_CLKCH_CTL_LOCK_CLKDIV		BIT(31)

#define CMU_GPR					0x50000
#define CMU_GPR_AVLSP_MSHC_CFG			(CMU_GPR + 0x20)
#define CMU_MSHC_CLKDIV				BIT(16)
#define CMU_MSHC_DIV				16

struct plldivs {
	uint32_t clkr;
	uint32_t clkfhi;
	uint32_t clkflo;
	uint32_t clkod;
};

static void	 cmu_read_plldivs	    (const uintptr_t base, struct plldivs *const plldivs);
static int	 cmu_calc_clkf		    (uint64_t frefclk, uint64_t fpllreq, struct plldivs *plldivs);
static uint64_t	 cmu_calc_fpll		    (const uint64_t frefclk, const struct plldivs *const plldivs);
static void	 cmu_set_clkf		    (struct cmu_desc *cmu, uint64_t frefclk, uint64_t fpllreq);
static int	 cmu_clkch_calc_div	    (const struct cmu_desc *const cmu, const unsigned clkch, uint64_t fclkchreq);
static int	 cmu_clkch_get_div	    (const struct cmu_desc *const cmu, const unsigned clkch);
static uintptr_t cmu_clkch_get_reg	    (const uintptr_t base, const unsigned clkch);
static int	 cmu_clkch_set_div	    (const struct cmu_desc *const cmu, const unsigned clkch, uint32_t div);
static struct cmu_desc *const cmu_desc_get_by_base(const uintptr_t base);
static int	 cmu_pll_lock_debounce	    (const uintptr_t base);
static int	 cmu_runtime_set_core_rate  (struct cmu_desc *cmu, uint64_t frefclk, uint64_t fpllreq);
static int	 cmu_runtime_set_periph_rate(struct cmu_desc *cmu, uint64_t frefclk, uint64_t fpllreq);
static int	 cmu_runtime_set_rate	    (struct cmu_desc *cmu, uint64_t frefclk, uint64_t fpllreq);

static struct cmu_desc cmus[20];

static void cmu_read_plldivs(const uintptr_t base,
			     struct plldivs *const plldivs)
{
	assert(base);
	assert(plldivs);

	plldivs->clkod = ((mmio_read_32(base + CMU_PLL_CTL0_REG) &
		      CMU_PLL_CTL0_CLKOD_MASK) >> CMU_PLL_CTL0_CLKOD_SHIFT) + 1;

	plldivs->clkr  = ((mmio_read_32(base + CMU_PLL_CTL0_REG) &
		      CMU_PLL_CTL0_CLKR_MASK) >> CMU_PLL_CTL0_CLKR_SHIFT) + 1;

	plldivs->clkflo = mmio_read_32(base + CMU_PLL_CTL1_REG);
	plldivs->clkfhi = mmio_read_32(base + CMU_PLL_CTL2_REG) &
			  CMU_PLL_CTL2_CLKFHI_MASK;
}

#define VCO_MAX	1000000000	// 1GHz - we want to set VCO freq close to that
#define CLKNR_MAX	2000	// accept ref. divisor up to this (max is 4096)

/*
 * Calculate best approximation for 'nf' and 'nr' such that
 * vco = parent * nf / nr; nr <= CLKNR_MAX
 */
static void calc_frac(uint64_t prate, uint64_t vco, uint32_t *nf, uint32_t *nr)
{
	int mp = 0, np = 1;
	int mt = 1, nt = vco / prate;
	uint32_t rp = prate, rt = vco % prate, rn;
	int f, mn, nn;

	while (rt) {
		f = rp / rt;
		rn = rp % rt;
		mn = mp + f * mt;
		nn = np + f * nt;
		if (nn > CLKNR_MAX)
			break;
		mp = mt;
		np = nt;
		rp = rt;
		mt = mn;
		nt = nn;
		rt = rn;
	}

	*nf = nt;
	*nr = mt;
}

/*
 * A simple helper function: find max divisor of 'n'
 * between low and high. If there are no divisors return high;
 */
static inline uint32_t max_div(uint32_t n, uint32_t high, uint32_t low)
{
	uint32_t t = high;

	for (t = high; (t >= low) && (n % t); t++)
		;

	return (t < low) ? high : t;
}

static int cmu_calc_clkf(uint64_t frefclk,
			 uint64_t fpllreq,
			 struct plldivs *plldivs)
{
	uint32_t od;

	assert(plldivs);

	plldivs->clkflo = 0;

	if (fpllreq > VCO_MAX) {
		od = 1;
	} else {
		od = VCO_MAX / fpllreq;
		if (od > (1 << 11))
			od = 1 << 11;
		od = max_div((uint32_t)frefclk, od, od / 4 + 1);
	}

	plldivs->clkod = od;
	calc_frac(frefclk, fpllreq * od, &plldivs->clkfhi, &plldivs->clkr);
	plldivs->clkfhi <<= 1;

	if ((plldivs->clkr == 0) || (plldivs->clkr > (1 << 12)) ||
	    (plldivs->clkfhi == 0) || (plldivs->clkfhi > (1 << 18))) {
		ERROR("Out of range: NR = %u, NF = %u!\n", plldivs->clkr,
		      plldivs->clkfhi);
		return -EINVAL;
	}

	return 0;
}

static uint64_t cmu_calc_fpll(const uint64_t frefclk,
			      const struct plldivs *const plldivs)
{
	uint64_t lower, upper;

	assert(plldivs);

	/* This is sort of a rounding for minimizing the error of calculation */
	lower = (frefclk * plldivs->clkflo) & (1ULL << 32) ?
		(frefclk * plldivs->clkflo >> 33) + 1 :
		(frefclk * plldivs->clkflo >> 33);

	upper = frefclk * plldivs->clkfhi >> 1;
	return (upper + lower) / (plldivs->clkod * plldivs->clkr);
}

bool cmu_is_mmca57(const uintptr_t base)
{
	assert(base);

	return base == MMCA57_0_LCRU ||
	       base == MMCA57_1_LCRU ||
	       base == MMCA57_2_LCRU ||
	       base == MMCA57_3_LCRU;
}

static bool cmu_is_mmmali(uintptr_t base)
{
	return base == MMMALI_LCRU;
}

static void cmu_set_clkf(struct cmu_desc *cmu,
			 uint64_t frefclk,
			 uint64_t fpllreq)
{
	struct plldivs plldivs;
	uint32_t reg, clknf;

	assert(cmu);
	assert(cmu->base);

	if (cmu_is_mmca57(cmu->base) || cmu_is_mmmali(cmu->base)) {
		reg  = mmio_read_32(cmu->base + CMU_PLL_CTL0_REG);
		if (reg & (CMU_PLL_CTL0_CLKR_MASK | CMU_PLL_CTL0_CLKOD_MASK)) {
			WARN("%s: (CPU) unexpected NF/OD: %x\n", __func__, reg);
			reg &= ~(CMU_PLL_CTL0_CLKR_MASK | CMU_PLL_CTL0_CLKOD_MASK);
			mmio_write_32(cmu->base + CMU_PLL_CTL0_REG, reg);
		}
		clknf = fpllreq / frefclk;
		mmio_write_32(cmu->base + CMU_PLL_CTL1_REG, 0);
		mmio_write_32(cmu->base + CMU_PLL_CTL2_REG, clknf << 1);
		cmu->fpllreq = frefclk * clknf;
	} else if (!cmu_calc_clkf(frefclk, fpllreq, &plldivs)) {
		INFO("%s <- %llu Hz, clkod:0x%x clkr:0x%x clkfhi:0x%x clkflo:0x%x\n",
		     cmu->name, fpllreq,
		     plldivs.clkod, plldivs.clkr, plldivs.clkfhi, plldivs.clkflo);
		reg  = mmio_read_32(cmu->base + CMU_PLL_CTL0_REG);
		reg &= ~(CMU_PLL_CTL0_CLKR_MASK | CMU_PLL_CTL0_CLKOD_MASK);
		reg |= (plldivs.clkr - 1) << CMU_PLL_CTL0_CLKR_SHIFT;
		reg |= (plldivs.clkod - 1) << CMU_PLL_CTL0_CLKOD_SHIFT;

		mmio_write_32(cmu->base + CMU_PLL_CTL0_REG, reg);
		mmio_write_32(cmu->base + CMU_PLL_CTL1_REG, 0);
		mmio_write_32(cmu->base + CMU_PLL_CTL2_REG, plldivs.clkfhi);
		cmu->fpllreq = (frefclk * plldivs.clkfhi) / plldivs.clkr / plldivs.clkod;
	}
}

static struct cmu_desc *const cmu_desc_get_by_base(const uintptr_t base)
{
	unsigned idx;

	if (!base) {
		goto err;
	}

	for (idx = 0; idx < ARRAY_SIZE(cmus); ++idx) {
		struct cmu_desc *const cmu = cmu_desc_get_by_idx(idx);
		if (cmu->base == base) {
			return cmu;
		}
	}

err:
	ERROR("%s: requested CMU (0x%lx) is not found\n", __func__, base);
	return NULL;
}

struct cmu_desc *const cmu_desc_get_by_idx(const unsigned idx)
{
	if (idx >= ARRAY_SIZE(cmus)) {
		return NULL;
	}

	return &cmus[idx];
}

static uintptr_t cmu_clkch_get_reg(const uintptr_t base, const unsigned clkch)
{
	const struct cmu_desc *const cmu = cmu_desc_get_by_base(base);

	assert(cmu);
	assert(cmu->base);

	return cmu->base + CMU_CLKCH0_CTL_REG + clkch * 0x10;
}

static int cmu_pll_lock_debounce(const uintptr_t base)
{
	uint64_t debounce, timeout;

	assert(base);

	for (timeout = timeout_init_us(100000); !timeout_elapsed(timeout);) {
		debounce = timeout_init_us(10000);
		while (mmio_read_32(base + CMU_PLL_CTL0_REG) &
		       CMU_PLL_CTL0_LOCK) {
			if (timeout_elapsed(debounce)) {
				return 0;
			}
		}
	}

	ERROR("%s(0x%lx): PLL_CTL0.LOCK timeout\n", __func__, base);
	return -ETIMEDOUT;
}

int cmu_pll_is_enabled(const uintptr_t base)
{
	assert(base);

	return (mmio_read_32(base + CMU_PLL_CTL0_REG) & CMU_PLL_CTL0_CTL_EN) &&
	       (mmio_read_32(base + CMU_PLL_CTL6_REG) & CMU_PLL_CTL6_SWEN);
}

int cmu_pll_disable(const uintptr_t base)
{
	const struct cmu_desc *const cmu = cmu_desc_get_by_base(base);

	if (!cmu) {
		return -ENXIO;
	}

	mmio_clrbits_32(cmu->base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_CTL_EN);
	return 0;
}

int cmu_pll_enable(const uintptr_t base)
{
	const struct cmu_desc *const cmu = cmu_desc_get_by_base(base);
	int err;
	int64_t fpll;

	if (!cmu) {
		return -ENXIO;
	}

	if (cmu_pll_is_enabled(base)) {
		WARN("%s: %s(): PLL already enabled\n", cmu->name, __func__);
		return 0;
	}

	fpll = cmu_pll_get_rate(base, cmu->frefclk);
	if (fpll < 0) {
		return fpll;
	}

	if (fpll < cmu->fpllmin || fpll > cmu->fpllmax) {
		ERROR("%s: %s(): currently set frequency is out off limits [%u..%u] Hz\n" \
		      "Enabling rejected. Please set the rate.\n", cmu->name,
		      __func__, cmu->fpllmin, cmu->fpllmax);
		return -EINVAL;
	}

	INFO("%s: %s() on %lld Hz\n", cmu->name, __func__, fpll);

	mmio_setbits_32(cmu->base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_CTL_EN);
	mmio_clrbits_32(cmu->base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_BYPASS);
	mmio_setbits_32(cmu->base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_RST);

	err = cmu_pll_lock_debounce(base);
	if (err) {
		return err;
	}

	mmio_setbits_32(cmu->base + CMU_PLL_CTL6_REG, CMU_PLL_CTL6_SWEN);
	mmio_clrbits_32(cmu->base + CMU_PLL_CTL6_REG, CMU_PLL_CTL6_SWRST);
	return 0;
}

void cmu_pll_on(const uintptr_t base, const cmu_pll_ctl_vals_t *const pllinit)
{
	uint32_t ctl0, ctl6;
	int err;

	if (cmu_pll_is_enabled(base)) {
		WARN("PLL 0x%lx is already enabled, skipped\n", base);
		return;
	}

	ctl6 = mmio_read_32(base + CMU_PLL_CTL6_REG);
	ctl6 &= ~CMU_PLL_CTL6_SWEN;
	mmio_write_32(base + CMU_PLL_CTL6_REG, ctl6);

	ctl0 = mmio_read_32(base + CMU_PLL_CTL0_REG);
	ctl0 |= CMU_PLL_CTL0_CTL_EN;
	mmio_write_32(base + CMU_PLL_CTL0_REG, ctl0);
	ctl0 &= ~CMU_PLL_CTL0_BYPASS;
	mmio_write_32(base + CMU_PLL_CTL0_REG, ctl0);
	ctl0 &= ~CMU_PLL_CTL0_CLKOD_MASK;
	ctl0 |= CMU_PLL_CTL0_CLKOD_SET(pllinit->clkod);
	ctl0 &= ~CMU_PLL_CTL0_CLKR_MASK;
	ctl0 |= CMU_PLL_CTL0_CLKR_SET(pllinit->clkr);
	mmio_write_32(base + CMU_PLL_CTL0_REG, ctl0);

	mmio_write_32(base + CMU_PLL_CTL1_REG, pllinit->clkflo);
	mmio_write_32(base + CMU_PLL_CTL2_REG, pllinit->clkfhi);

	mmio_clrsetbits_32(base + CMU_PLL_CTL4_REG,
			   CMU_PLL_CTL4_IIGAIN_LGMLT_MASK |
			   CMU_PLL_CTL4_IPGAIN_LGMLT_MASK |
			   CMU_PLL_CTL4_IGAIN_LGMLT_MASK  |
			   CMU_PLL_CTL4_PGAIN_LGMLT_MASK,
			   CMU_PLL_CTL4_IIGAIN_LGMLT_SET(pllinit->iigain) |
			   CMU_PLL_CTL4_IPGAIN_LGMLT_SET(pllinit->ipgain) |
			   CMU_PLL_CTL4_IGAIN_LGMLT_SET(pllinit->igain)	  |
			   CMU_PLL_CTL4_PGAIN_LGMLT_SET(pllinit->pgain));

	ctl0 |= CMU_PLL_CTL0_RST;
	mmio_write_32(base + CMU_PLL_CTL0_REG, ctl0);

	err = cmu_pll_lock_debounce(base);
	if (err) {
		return;
	}

	ctl6 |= CMU_PLL_CTL6_SWEN;
	mmio_write_32(base + CMU_PLL_CTL6_REG, ctl6);
	ctl6 &= ~CMU_PLL_CTL6_SWRST;
	mmio_write_32(base + CMU_PLL_CTL6_REG, ctl6);
}

int64_t cmu_pll_get_rate(const uintptr_t base, uint64_t frefclk)
{
	uint64_t fpll;
	struct cmu_desc *cmu = cmu_desc_get_by_base(base);
	struct plldivs plldivs;

	if (!cmu) {
		return -ENXIO;
	}

	if (!frefclk) {
		frefclk = cmu->frefclk;
	}

	if (frefclk != cmu->frefclk)
		WARN("%s: CMU-ID %lx: frefclk %u != cmu frefclk %u\n",
			__func__, cmu->base, (uint32_t)frefclk,
			(uint32_t) cmu->frefclk);

	cmu_read_plldivs(base, &plldivs);
	fpll = cmu_calc_fpll(frefclk, &plldivs);

	if (fpll != cmu->fpllreq) {
		WARN("%s: %s: current_freq (%u) different from actual (%llu)\n",
			__func__, cmu->name, (uint32_t)cmu->fpllreq, fpll);
		cmu->fpllreq = fpll;
	}

	INFO("%s -> %llu Hz, clkod:0x%x clkr:0x%x clkfhi:0x%x clkflo:0x%x\n",
	     cmu->name, fpll,
	     plldivs.clkod, plldivs.clkr, plldivs.clkfhi, plldivs.clkflo);

	return fpll;
}

static int cmu_runtime_set_periph_rate(struct cmu_desc *cmu,
				       uint64_t frefclk,
				       uint64_t fpllreq)
{
	int err;

	assert(cmu);
	assert(cmu->base);

	if (!frefclk) {
		frefclk = cmu->frefclk;
	}

	if (frefclk != cmu->frefclk)
		WARN("%s: CMU-ID %lx: frefclk %u != cmu frefclk %u\n",
			__func__, cmu->base, (uint32_t)frefclk,
			(uint32_t) cmu->frefclk);

	mmio_clrbits_32(cmu->base + CMU_PLL_CTL6_REG, CMU_PLL_CTL6_SWEN);
	cmu_set_clkf(cmu, frefclk, fpllreq);
	mmio_setbits_32(cmu->base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_RST);

	err = cmu_pll_lock_debounce(cmu->base);
	if (err) {
		return err;
	}

	mmio_setbits_32(cmu->base + CMU_PLL_CTL6_REG, CMU_PLL_CTL6_SWEN);
	return 0;
}

int64_t cmu_pll_round_rate(const uintptr_t base,
			   uint64_t frefclk,
			   const uint64_t fpllreq)
{
	const struct cmu_desc *const cmu = cmu_desc_get_by_base(base);
	struct plldivs plldivs;

	if (!cmu) {
		return -ENXIO;
	}

	if (fpllreq <= cmu->fpllmin) {
		return cmu->fpllmin;
	}

	if (fpllreq >= cmu->fpllmax) {
		return cmu->fpllmax;
	}

	if (!frefclk) {
		frefclk = cmu->frefclk;
	}

	if (frefclk != cmu->frefclk)
		WARN("%s: CMU-ID %lx: frefclk %u != cmu frefclk %u\n",
			__func__, base, (uint32_t)frefclk, (uint32_t) cmu->frefclk);
	if (cmu_is_mmca57(base) || cmu_is_mmmali(base)) {
		plldivs.clkod = 1;
		plldivs.clkr = 1;
		plldivs.clkfhi = ((fpllreq + frefclk / 2) / frefclk) << 1;
	} else {
		cmu_calc_clkf(frefclk, fpllreq, &plldivs);
	}

	return (frefclk * plldivs.clkfhi / 2) / plldivs.clkr / plldivs.clkod;
}

static int cmu_runtime_set_core_rate(struct cmu_desc *cmu,
				     uint64_t frefclk,
				     uint64_t fpllreq)
{
	int delta;
	int64_t fpll;

	assert(cmu);
	assert(cmu->base);

	fpll = cmu_pll_get_rate(cmu->base, frefclk);
	if (fpll < 0) {
		return -ENXIO;
	}

	delta = fpllreq - fpll;
	if (!delta) {
		return 0;
	}

	cmu_set_clkf(cmu, frefclk, fpllreq);
	cmu_pll_lock_debounce(cmu->base);
	return 0;
}

static int cmu_runtime_set_rate(struct cmu_desc *cmu,
				uint64_t frefclk,
				uint64_t fpllreq)
{
	assert(cmu);
	assert(cmu->base);

	INFO("%s: %s(): %llu\n", cmu->name, __func__, fpllreq);
	if (cmu_is_mmca57(cmu->base) || cmu_is_mmmali(cmu->base)) {
		return cmu_runtime_set_core_rate(cmu, frefclk, fpllreq);
	} else {
		return cmu_runtime_set_periph_rate(cmu, frefclk, fpllreq);
	}
}

int cmu_pll_set_rate(const uintptr_t base,
		     uint64_t frefclk,
		     const uint64_t fpllreq)
{
	struct cmu_desc *const cmu = cmu_desc_get_by_base(base);

	if (!cmu) {
		return -ENXIO;
	}

	if (fpllreq < cmu->fpllmin || fpllreq > cmu->fpllmax) {
		ERROR("%s: %s(): requested frequency is out of limits [%u..%u] Hz\n",
		      cmu->name, __func__, cmu->fpllmin, cmu->fpllmax);
		return -EINVAL;
	}

	if (!frefclk) {
		frefclk = cmu->frefclk;
	}

	if (fpllreq == cmu_pll_get_rate(base, frefclk)) {
		INFO("%s: %s(): already set to %u Hz\n", cmu->name, __func__,
		     cmu->fpllreq);
		return 0;
	}

	if (cmu_pll_is_enabled(base)) {
		return cmu_runtime_set_rate(cmu, frefclk, fpllreq);
	} else {
		INFO("%s: PLL is not enabled, set it to %llu Hz\n", cmu->name,
		     fpllreq);
		cmu_set_clkf(cmu, frefclk, fpllreq);
		return 0;
	}
}

int cmu_clkch_enable(const uintptr_t base, const unsigned clkch)
{
	uintptr_t clkch_reg;
	uint64_t timeout;

	INFO("%s: cmu 0x%lx clkch %u\n", __func__, base, clkch);

	if (!cmu_pll_is_enabled(base)) {
		return -ENODEV;
	}

	if (cmu_clkch_is_enabled(base, clkch)) {
		WARN("%s: clock channel #%u of PLL already enabled\n",
		     __func__, clkch);
		return 0;
	}

	clkch_reg = cmu_clkch_get_reg(base, clkch);
	if (!clkch_reg) {
		return -ENXIO;
	}

	mmio_setbits_32(clkch_reg, CMU_CLKCH_CTL_CLK_EN);

	for (timeout = timeout_init_us(100000); !timeout_elapsed(timeout);) {
		if (mmio_read_32(clkch_reg) & CMU_CLKCH_CTL_CLK_RDY) {
			mmio_clrbits_32(clkch_reg, CMU_CLKCH_CTL_SWRST);
			return 0;
		}
	}

	ERROR("%s(0x%lx, %u): CLKCH_CTL.CLK_RDY timeout\n",
	      __func__, base, clkch);
	return -ETIMEDOUT;
}

void cmu_clkch_enable_by_base(const uintptr_t base, const unsigned div)
{
	uint32_t clkch_reg = mmio_read_32(base);
	uint64_t timeout;

	if (clkch_reg & CMU_CLKCH_CTL_CLK_EN) {
		WARN("CLKCH 0x%lx is already enabled, skipped\n", base);
		return;
	}

	clkch_reg &= ~CMU_CLKCH_CTL_VAL_CLKDIV_MASK;
	clkch_reg |=  CMU_CLKCH_CTL_VAL_CLKDIV_SET(div) |
		      CMU_CLKCH_CTL_SET_CLKDIV;
	mmio_write_32(base, clkch_reg);

	timeout = timeout_init_us(10000);
	while (!(mmio_read_32(base) & CMU_CLKCH_CTL_LOCK_CLKDIV)) {
		if (timeout_elapsed(timeout)) {
			ERROR("%s(0x%lx, %u): CLKCH_CTL.LOCK_CLKDIV timeout\n",
			      __func__, base, div);
			break;
		}
	}

	mmio_setbits_32(base, CMU_CLKCH_CTL_CLK_EN);

	timeout = timeout_init_us(10000);
	while (!(mmio_read_32(base) & CMU_CLKCH_CTL_CLK_RDY)) {
		if (timeout_elapsed(timeout)) {
			ERROR("%s(0x%lx, %u): CLKCH_CTL.CLK_RDY timeout\n",
			      __func__, base, div);
			break;
		}
	}

	mmio_clrbits_32(base, CMU_CLKCH_CTL_SWRST);
}

int cmu_clkch_disable(const uintptr_t base, const unsigned clkch)
{
	uintptr_t clkch_reg = cmu_clkch_get_reg(base, clkch);
	if (!clkch_reg) {
		return -ENXIO;
	}

	mmio_clrbits_32(clkch_reg, CMU_CLKCH_CTL_CLK_EN);
	return 0;
}

int64_t cmu_clkch_get_rate(const uintptr_t base, const unsigned clkch)
{
	const struct cmu_desc *const cmu = cmu_desc_get_by_base(base);
	uint32_t div;
	int64_t fpll;

	if (!cmu) {
		return -ENXIO;
	}

	fpll = cmu_pll_get_rate(base, cmu->frefclk);
	if (fpll < 0) {
		return -ENXIO;
	}

	div = cmu_clkch_get_div(cmu, clkch);
	INFO("%s: CMU: %lld\n", __func__, fpll / div);
	return fpll / div;
}

int cmu_clkch_is_enabled(const uintptr_t base, const unsigned clkch)
{
	uintptr_t clkch_reg = cmu_clkch_get_reg(base, clkch);
	if (!clkch_reg) {
		return -ENXIO;
	}

	return (mmio_read_32(clkch_reg) & CMU_CLKCH_CTL_CLK_EN) &&
	       cmu_pll_is_enabled(base);
}

int64_t cmu_clkch_round_rate(const uintptr_t base,
			     const unsigned clkch,
			     const uint64_t fclkchreq)
{
	const struct cmu_desc *const cmu = cmu_desc_get_by_base(base);
	int64_t ret;

	if (!cmu) {
		ret = -ENXIO;
		INFO("%s(0x%lx, #%u, %llu Hz): error %lld\n",
		     __func__, base, clkch, fclkchreq, ret);
		return ret;
	}

	if (cmu->deny_pll_reconf) {
		int64_t fpll;
		int div;

		if (fclkchreq >= cmu->fpllmax) {
			return cmu->fpllmax;
		}

		fpll = cmu_pll_get_rate(base, cmu->frefclk);
		if (fpll < 0) {
			return fpll;
		}

		div = cmu_clkch_calc_div(cmu, clkch, fclkchreq);
		ret = fpll / div;
		INFO("%s(0x%lx, #%u, %llu Hz): deny PLL reconf, %lld Hz\n",
		     __func__, base, clkch, fclkchreq, ret);
		return ret;
	} else {
		ret = cmu_pll_round_rate(base, cmu->frefclk, fclkchreq);
		INFO("%s(0x%lx, #%u, %llu Hz): %lld Hz\n",
		     __func__, base, clkch, fclkchreq, ret);
		return ret;
	}
}

static int cmu_clkch_set_div(const struct cmu_desc *const cmu,
			     const unsigned clkch,
			     uint32_t div)
{
	uintptr_t clkch_reg;
	uint64_t timeout;

	assert(cmu);
	assert(cmu->base);

	if (div > CMU_CLKCH_CTL_VAL_CLKDIV_MAX * CMU_MSHC_DIV) {
		return -EINVAL;
	}

	clkch_reg = cmu_clkch_get_reg(cmu->base, clkch);
	if (!clkch_reg) {
		return -ENXIO;
	}

	if (clkch == cmu->mshc_clkch) {
		if (div > CMU_CLKCH_CTL_VAL_CLKDIV_MAX) {
			div /= CMU_MSHC_DIV;
			mmio_clrbits_32(cmu->base + CMU_GPR_AVLSP_MSHC_CFG,
					CMU_MSHC_CLKDIV);
		} else {
			mmio_setbits_32(cmu->base + CMU_GPR_AVLSP_MSHC_CFG,
					CMU_MSHC_CLKDIV);
		}
	}

	mmio_clrsetbits_32(clkch_reg,
			   CMU_CLKCH_CTL_VAL_CLKDIV_MASK,
			   div << CMU_CLKCH_CTL_VAL_CLKDIV_SHIFT |
			   CMU_CLKCH_CTL_SET_CLKDIV);

	for (timeout = timeout_init_us(100000); !timeout_elapsed(timeout);) {
		if (mmio_read_32(clkch_reg) & CMU_CLKCH_CTL_LOCK_CLKDIV) {
			return 0;
		}
	}

	ERROR("%s(0x%lx, %u, %u): CLKCH_CTL.LOCK_CLKDIV timeout\n",
	      __func__, cmu->base, clkch, div);
	return -ETIMEDOUT;
}

int cmu_clkch_set_rate(const uintptr_t base,
		       const unsigned clkch,
		       const uint64_t fclkchreq)
{
	INFO("%s: CMU 0x%lx clkch %u, %llu Hz\n", __func__, base, clkch,
	     fclkchreq);

	const struct cmu_desc *const cmu = cmu_desc_get_by_base(base);

	if (!cmu) {
		return -ENXIO;
	}

	if (fclkchreq == cmu_clkch_get_rate(base, clkch)) {
		INFO("%s: requested frequency (%llu Hz) is already set\n",
		     __func__, fclkchreq);
		return 0;
	}

	if (cmu->deny_pll_reconf) {
		int div;

		if (!cmu_pll_is_enabled(base)) {
			return -ENODEV;
		}

		if (fclkchreq != cmu_clkch_round_rate(base, clkch, fclkchreq)) {
			return -EINVAL;
		}

		div = cmu_clkch_calc_div(cmu, clkch, fclkchreq);
		INFO("%s: divider 0x%x\n", __func__, div);

		if (cmu_clkch_is_enabled(base, clkch)) {
			cmu_clkch_disable(base, clkch);

			if (cmu_clkch_set_div(cmu, clkch, div)) {
				return -ETIMEDOUT;
			}

			if (cmu_clkch_enable(base, clkch)) {
				return -ETIMEDOUT;
			}
		} else if (cmu_clkch_set_div(cmu, clkch, div)) {
			return -ETIMEDOUT;
		}
	} else {
		return cmu_pll_set_rate(base, cmu->frefclk, fclkchreq);
	}

	return 0;
}

void cmu_pll_reconf_nr(struct cmu_desc *cmu)
{
	int err;

	assert(cmu);
	assert(cmu->base);

	if (cmu->fpllreq < cmu->fpllmin || cmu->fpllreq > cmu->fpllmax) {
		ERROR("%s: %s(): requested frequency is out off limits [%u..%u] Hz\n" \
		      "Configuration rejected.\n", cmu->name, __func__,
		      cmu->fpllmin, cmu->fpllmax);
		return;
	}

	mmio_clrbits_32(cmu->base + CMU_PLL_CTL6_REG, CMU_PLL_CTL6_SWEN);
	mmio_clrbits_32(cmu->base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_CLKR_MASK);

	INFO("%s: %s(): %u Hz\n", cmu->name, __func__, cmu->fpllreq);
	cmu_set_clkf(cmu, cmu->frefclk, cmu->fpllreq);
	mmio_setbits_32(cmu->base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_RST);

	err = cmu_pll_lock_debounce(cmu->base);
	if (err) {
		return;
	}

	mmio_setbits_32(cmu->base + CMU_PLL_CTL6_REG, CMU_PLL_CTL6_SWEN);
}

static int cmu_clkch_calc_div(const struct cmu_desc *const cmu,
			      const unsigned clkch,
			      uint64_t fclkchreq)
{
	int div;
	int64_t fpll;
	int max = CMU_CLKCH_CTL_VAL_CLKDIV_MAX;

	assert(cmu);
	assert(cmu->base);

	if (clkch == cmu->mshc_clkch) {
		max *= CMU_MSHC_DIV;
	}

	fpll = cmu_pll_get_rate(cmu->base, cmu->frefclk);
	if (fpll < 0) {
		return fpll;
	}

	/* raw */
	div = fpll / fclkchreq;
	if (div < 1) {
		div = 1;
	}

	if (clkch == cmu->mshc_clkch && div > CMU_CLKCH_CTL_VAL_CLKDIV_MAX) {
		/* Round MSHC clock channel divider */
		div /= CMU_MSHC_DIV;
		div *= CMU_MSHC_DIV;
	}

	if (div > max) {
		div = max;
	}

	return div;
}

static int cmu_clkch_get_div(const struct cmu_desc *const cmu,
			     const unsigned clkch)
{
	uintptr_t clkch_reg;
	uint32_t div;
	uint32_t reg;

	assert(cmu);
	assert(cmu->base);

	clkch_reg = cmu_clkch_get_reg(cmu->base, clkch);
	if (!clkch_reg) {
		return -ENXIO;
	}

	reg = mmio_read_32(clkch_reg);
	div = (reg & CMU_CLKCH_CTL_VAL_CLKDIV_MASK) >>
	      CMU_CLKCH_CTL_VAL_CLKDIV_SHIFT;

	if (clkch == cmu->mshc_clkch &&
	    !(mmio_read_32(cmu->base + CMU_GPR_AVLSP_MSHC_CFG) &
	      CMU_MSHC_CLKDIV)) {
		div *= CMU_MSHC_DIV;
	}

	return div;
}
