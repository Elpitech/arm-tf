/*
 * Copyright (c) 2018-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <bm1000_cmu.h>
#include <bm1000_def.h>
#include <bm1000_private.h>
#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <errno.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>

#define CMU_PLL_CTL0_REG			0x0000
#define CMU_PLL_CTL0_CTL_EN			BIT(0)
#define CMU_PLL_CTL0_RST			BIT(1)
#define CMU_PLL_CTL0_CLKR_MASK			GENMASK(13, 2)
#define CMU_PLL_CTL0_CLKR_SET(x)		SETMASK(x, 13, 2)
#define CMU_PLL_CTL0_CLKR_SHIFT			2
#define CMU_PLL_CTL0_CLKOD_MASK			GENMASK(24, 14)
#define CMU_PLL_CTL0_CLKOD_SET(x)		SETMASK(x, 24, 14)
#define CMU_PLL_CTL0_CLKOD_SHIFT		14
#define CMU_PLL_CTL0_BYPASS			BIT(30)
#define CMU_PLL_CTL0_LOCK			BIT(31)

#define CMU_PLL_CTL1_REG			0x0004
#define CMU_PLL_CTL1_CLKFLO_MASK		GENMASK(31, 0)
#define CMU_PLL_CTL2_REG			0x0008
#define CMU_PLL_CTL2_CLKFHI_MASK		GENMASK(21, 0)

#define CMU_PLL_CTL4_REG			0x0010
#define CMU_PLL_CTL4_PGAIN_LGMLT_MASK		GENMASK(5, 0)
#define CMU_PLL_CTL4_PGAIN_LGMLT_SET(x)		SETMASK(x, 5, 0)
#define CMU_PLL_CTL4_IGAIN_LGMLT_MASK		GENMASK(11, 6)
#define CMU_PLL_CTL4_IGAIN_LGMLT_SET(x)		SETMASK(x, 11, 6)
#define CMU_PLL_CTL4_IPGAIN_LGMLT_MASK		GENMASK(17, 12)
#define CMU_PLL_CTL4_IPGAIN_LGMLT_SET(x)	SETMASK(x, 17, 12)
#define CMU_PLL_CTL4_IIGAIN_LGMLT_MASK		GENMASK(23, 18)
#define CMU_PLL_CTL4_IIGAIN_LGMLT_SET(x)	SETMASK(x, 23, 18)

#define CMU_PLL_CTL6_REG			0x0018
#define CMU_PLL_CTL6_SWEN			BIT(0)
#define CMU_PLL_CTL6_SWRST			BIT(1)

#define CMU_CLKCH0_CTL_REG			0x0020
#define CMU_CLKCH_CTL_CLK_EN			BIT(0)
#define CMU_CLKCH_CTL_SWRST			BIT(1)
#define CMU_CLKCH_CTL_SET_CLKDIV		BIT(2)
#define CMU_CLKCH_CTL_VAL_CLKDIV_MASK		GENMASK(11, 4)
#define CMU_CLKCH_CTL_VAL_CLKDIV_SET(x)		SETMASK(x, 11, 4)
#define CMU_CLKCH_CTL_VAL_CLKDIV_MAX		0xff
#define CMU_CLKCH_CTL_VAL_CLKDIV_SHIFT		4
#define CMU_CLKCH_CTL_CLK_RDY			BIT(30)
#define CMU_CLKCH_CTL_LOCK_CLKDIV		BIT(31)

#define CMU_GPR					0x50000
#define CMU_GPR_AVLSP_MSHC_CFG			(CMU_GPR + 0x20)
#define CMU_MSHC_CLKDIV				BIT(16)
#define CMU_MSHC_DIV				16

#define CLKFHI_MASK				GENMASK(53, 32)
#define CLKFHI_SET(x)				SETMASK(x, 53, 32)
#define CLKFHI_SHIFT				32
#define CLKFLO_MASK				GENMASK(31, 0)
#define CLKFLO_SET(x)				SETMASK(x, 31, 0)
#define CLKFLO_SHIFT				0

struct plldivs {
	uint32_t nr;
	uint32_t od;
	uint32_t nf;
};

static void cmu_pll_div_get(const uintptr_t base, struct plldivs *const plldivs);
static int cmu_pll_div_calc(const uint64_t frefclk, const uint64_t fpllreq, struct plldivs *const plldivs);
static int cmu_pll_div_set(const struct cmu_desc *const cmu, const uint64_t frefclk, const uint64_t fpllreq);
static int cmu_clkch_div_calc(const struct cmu_desc *const cmu, const unsigned clkch, uint64_t fclkchreq);
static int cmu_clkch_div_get(const struct cmu_desc *const cmu, const unsigned clkch);
static uintptr_t cmu_clkch_get_reg(const uintptr_t base, const unsigned clkch);
static int cmu_clkch_div_set(const struct cmu_desc *const cmu, const unsigned clkch, uint32_t div);
static struct cmu_desc *const cmu_desc_get_by_base(const uintptr_t base);
static int cmu_pll_lock_debounce(const uintptr_t base);
static int cmu_set_core_rate(const struct cmu_desc *const cmu, const uint64_t frefclk, const uint64_t fpllreq);
static int cmu_set_periph_rate(const struct cmu_desc *const cmu, uint64_t frefclk, const uint64_t fpllreq);
void mmca57_reconf_sclken(const uintptr_t base, const unsigned div);
static void cmu_pll_set_od(const uintptr_t base, uint32_t od);
static void cmu_pll_set_nr(const uintptr_t base, uint32_t nr);
static void cmu_pll_set_nf(const uintptr_t base, uint32_t nf);

int cmu_pll_is_enabled(const uintptr_t base)
{
	int ret;
	ret = (mmio_read_32(base + CMU_PLL_CTL0_REG) & CMU_PLL_CTL0_CTL_EN) &&
	       (mmio_read_32(base + CMU_PLL_CTL6_REG) & CMU_PLL_CTL6_SWEN);
	INFO("%s: base=0x%lx, is_%s\n", __func__,
		base, ret? "enabled":"disabled");
	return ret;
}

int cmu_pll_enable(const uintptr_t base)
{
	int err;
	if (cmu_pll_is_enabled(base)) {
		INFO("%s: base=0x%lx, already\n", __func__, base);
		return 0;
	}
	/* config */
	mmio_clrbits_32(base + CMU_PLL_CTL6_REG, CMU_PLL_CTL6_SWEN);
	mmio_setbits_32(base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_CTL_EN);
	mmio_clrbits_32(base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_BYPASS);

	/* change */

	/* wait */
	mmio_setbits_32(base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_RST);
	err = cmu_pll_lock_debounce(base);
	if (err) {
		ERROR("%s: base=0x%lx, timeout\n", __func__, base);
		return err;
	}

	/* enable */
	mmio_setbits_32(base + CMU_PLL_CTL6_REG, CMU_PLL_CTL6_SWEN);
	mmio_clrbits_32(base + CMU_PLL_CTL6_REG, CMU_PLL_CTL6_SWRST);

	INFO("%s: base=0x%lx, enable\n", __func__, base);
	return 0;
}

int cmu_pll_disable(const uintptr_t base)
{
	mmio_clrbits_32(base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_CTL_EN);
	INFO("%s: base=0x%lx, disable\n", __func__, base);
	return 0;
}

int64_t cmu_pll_get_rate(const uintptr_t base, uint64_t frefclk)
{
	uint64_t fpll;
	struct plldivs plldivs;

	if (!frefclk) {
		struct cmu_desc *const cmu = cmu_desc_get_by_base(base);
		if (!cmu) {
			return -1;
		}
		frefclk = cmu->frefclk;
	}

	cmu_pll_div_get(base, &plldivs);
	fpll = (frefclk * plldivs.nf) / (plldivs.nr * plldivs.od);

	INFO("%s: base=0x%lx, rate=%llu\n", __func__, base, fpll);
	return fpll;
}

int64_t cmu_pll_round_rate(const uintptr_t base,
			   uint64_t frefclk,
			   const uint64_t fpllreq)
{
	int ret;
	struct plldivs plldivs;
	uint64_t fpll;

	if (!frefclk) {
		struct cmu_desc *const cmu = cmu_desc_get_by_base(base);
		if (!cmu) {
			return -1;
		}
		frefclk = cmu->frefclk;
	}

	ret = cmu_pll_div_calc(frefclk, fpllreq, &plldivs);
	if (ret) {
		return ret;
	}

	fpll = (frefclk * plldivs.nf) / (plldivs.nr * plldivs.od);

	INFO("%s: base=0x%lx, rate=%llu\n", __func__, base, fpll);
	return fpll;
}

int cmu_pll_set_rate(const uintptr_t base,
		     uint64_t frefclk,
		     const uint64_t fpllreq)
{
	int ret;
	struct cmu_desc *const cmu = cmu_desc_get_by_base(base);

	if (base == MMAVLSP_CMU0_BASE) {
		INFO("%s: base=0x%lx, skip\n", __func__, base);
		return -ENXIO;
	}

	/* prepare */
	if (!cmu) {
		return -ENXIO;
	}
	if (!frefclk) {
		frefclk = cmu->frefclk;
	}
	if (fpllreq == cmu_pll_get_rate(base, frefclk)) {
		INFO("%s: base=0x%lx, rate=%llu, already\n",
			__func__, base, fpllreq);
		return 0;
	}

	/* change */
	if (base == MMCA57_0_CMU0_BASE ||
	    base == MMCA57_1_CMU0_BASE ||
	    base == MMCA57_2_CMU0_BASE ||
	    base == MMCA57_3_CMU0_BASE) {
		ret = cmu_set_core_rate(cmu, frefclk, fpllreq);
	} else {
		ret = cmu_set_periph_rate(cmu, frefclk, fpllreq);
	}
	INFO("%s: base=0x%lx, name=%s, rate=%llu%s\n",
		__func__, base, cmu->name, fpllreq, ret? ", fail" : "");
	return ret;
}

int cmu_clkch_is_enabled(const uintptr_t base, const unsigned clkch)
{
	int ret;
	uintptr_t clkch_reg = cmu_clkch_get_reg(base, clkch);
	if (!clkch_reg) {
		return -ENXIO;
	}

	ret = (mmio_read_32(clkch_reg) & CMU_CLKCH_CTL_CLK_EN) &&
	       cmu_pll_is_enabled(base);

	INFO("%s: base=0x%lx, ch=%d, is_%s\n",
		__func__, base, clkch, ret? "enabled" : "disabled");
	return ret;
}

int cmu_clkch_enable(const uintptr_t base, const unsigned clkch)
{
	uintptr_t clkch_reg;
	uint64_t timeout;

	/* prepare */
	if (!cmu_pll_is_enabled(base)) {
		return -ENODEV;
	}
	if (cmu_clkch_is_enabled(base, clkch)) {
		INFO("%s: base=0x%lx, ch=%u, already\n", __func__, base, clkch);
		return 0;
	}
	clkch_reg = cmu_clkch_get_reg(base, clkch);
	if (!clkch_reg) {
		return -ENXIO;
	}

	/* wait */
	mmio_setbits_32(clkch_reg, CMU_CLKCH_CTL_CLK_EN);
	for (timeout = timeout_init_us(100000); !timeout_elapsed(timeout);) {
		if (mmio_read_32(clkch_reg) & CMU_CLKCH_CTL_CLK_RDY) {
			/* enable */
			mmio_clrbits_32(clkch_reg, CMU_CLKCH_CTL_SWRST);
			INFO("%s: base=0x%lx, ch=%u, enable\n", __func__, base, clkch);
			return 0;
		}
	}

	ERROR("%s: base=0x%lx, ch=%u, lock timeout\n", __func__, base, clkch);
	return -ETIMEDOUT;
}

int cmu_clkch_disable(const uintptr_t base, const unsigned clkch)
{
	uintptr_t clkch_reg = cmu_clkch_get_reg(base, clkch);
	if (!clkch_reg) {
		return -ENXIO;
	}

	mmio_clrbits_32(clkch_reg, CMU_CLKCH_CTL_CLK_EN);
	INFO("%s: base=0x%lx, ch=%d, disable\n", __func__, base, clkch);
	return 0;
}

int64_t cmu_clkch_get_rate(const uintptr_t base, const unsigned clkch)
{
	const struct cmu_desc *const cmu = cmu_desc_get_by_base(base);
	uint32_t div;
	int64_t fpll;
	int64_t rate;

	if (!cmu) {
		return -ENXIO;
	}
	fpll = cmu_pll_get_rate(base, cmu->frefclk);
	if (fpll < 0) {
		return -ENXIO;
	}
	div = cmu_clkch_div_get(cmu, clkch);
	if (div <= 0) {
		return -ENXIO;
	}
	rate = fpll / div;

	INFO("%s: rate=%lld\n", __func__, rate);
	return rate;
}

int64_t cmu_clkch_round_rate(const uintptr_t base,
			     const unsigned clkch,
			     const uint64_t fclkchreq)
{
	const struct cmu_desc *const cmu = cmu_desc_get_by_base(base);
	int64_t ret;

	if (!cmu) {
		return -ENXIO;
	}

	if (cmu->deny_pll_reconf) {
		int64_t fpll;
		int div;

		fpll = cmu_pll_get_rate(base, cmu->frefclk);
		if (fpll < 0) {
			return fpll;
		}
		div = cmu_clkch_div_calc(cmu, clkch, fclkchreq);
		if (div <= 0) {
			return div;
		}
		ret = fpll / div;

	} else {
		ret = cmu_pll_round_rate(base, cmu->frefclk, fclkchreq);
	}
	INFO("%s: base=0x%lx, ch=%u, rate=%llu, round=%lld\n",
	     __func__, base, clkch, fclkchreq, ret);
	return ret;
}

int cmu_clkch_set_rate(const uintptr_t base,
		       const unsigned clkch,
		       const uint64_t fclkchreq)
{
	int ret = 0;
	const struct cmu_desc *const cmu = cmu_desc_get_by_base(base);

	if (!cmu) {
		return -ENXIO;
	}

	if (fclkchreq == cmu_clkch_get_rate(base, clkch)) {
		INFO("%s: base=0x%lx, rate=%llu, already\n", __func__, base, fclkchreq);
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

		div = cmu_clkch_div_calc(cmu, clkch, fclkchreq);

		if (cmu_clkch_is_enabled(base, clkch)) {
			cmu_clkch_disable(base, clkch);
			if (cmu_clkch_div_set(cmu, clkch, div)) {
				return -ETIMEDOUT;
			}
			if (cmu_clkch_enable(base, clkch)) {
				return -ETIMEDOUT;
			}
		} else if (cmu_clkch_div_set(cmu, clkch, div)) {
			return -ETIMEDOUT;
		}

	} else {
		ret = cmu_pll_set_rate(base, cmu->frefclk, fclkchreq);
	}

	INFO("%s: base=0x%lx, ch=%u, rate=%llu\n", __func__, base, clkch, fclkchreq);
	return ret;
}

static void cmu_pll_div_get(const uintptr_t base, struct plldivs *const plldivs)
{
	uint64_t clkflo;
	uint64_t clkfhi;

	plldivs->od = ((mmio_read_32(base + CMU_PLL_CTL0_REG) &
		      CMU_PLL_CTL0_CLKOD_MASK) >> CMU_PLL_CTL0_CLKOD_SHIFT) + 1;

	plldivs->nr = ((mmio_read_32(base + CMU_PLL_CTL0_REG) &
		      CMU_PLL_CTL0_CLKR_MASK) >> CMU_PLL_CTL0_CLKR_SHIFT) + 1;

	clkflo = mmio_read_32(base + CMU_PLL_CTL1_REG) & CMU_PLL_CTL1_CLKFLO_MASK;
	clkfhi = mmio_read_32(base + CMU_PLL_CTL2_REG) & CMU_PLL_CTL2_CLKFHI_MASK;
	plldivs->nf = ((clkfhi << CLKFHI_SHIFT) + (clkflo << CLKFLO_SHIFT)) / 0x200000000;

	INFO("%s: base=0x%lx, nr=%d, od=%d, nf=%d\n",
		__func__, base, plldivs->nr, plldivs->od, plldivs->nf);
}

static int cmu_pll_div_calc(const uint64_t frefclk,
			     const uint64_t fpllreq,
			     struct plldivs *const plldivs)
{
	if (!frefclk || !fpllreq || !plldivs) {
		return -ENXIO;;
	}

	uint64_t fout = 0;
	uint64_t fvco = 0;
	const uint64_t fvco_max = 2200000000; /* 2.2 GHz */
	/* const uint64_t fout_max = 1500000000; */ /* 1.5 GHz */
	const uint64_t fout_max = 2200000000; /* 2.2 GHz */
	uint32_t od_max = fvco_max / fpllreq;
	uint32_t od, od2 = -1;
	uint32_t nr, nr2 = -1;
	uint32_t nf, nf2 = -1;
	int32_t err;
	uint32_t err2 = -1;
	bool done = false;

	/*
	INFO("%s: od_max=%d\n", __func__, od_max);
	INFO("%s: Freq=%lld\n", __func__, fpllreq);
	*/

	for (nr = 1; nr < 200; nr++) {
		for (od = 1; od <= od_max; od++) {

			nf = (fpllreq * nr * od) / frefclk;
			fout = (frefclk * nf) / (nr * od);
			fvco = (frefclk * nf) / nr;
			err = (1000 * fout) / fpllreq - 1000;
			if (err < 0) {
				err = -err;
			}
			if (fvco > fvco_max || fout > fout_max) {
				continue;
			}

			/* INFO("%s: NR=%d, OD=%d, NF=%d, Err=%d\n", __func__, nr, od, nf, err); */

			if (err < err2) {
				done = true;
				nr2 = nr;
				od2 = od;
				nf2 = nf;
				err2 = err;
			}
			if (err == 0) {
				done = true;
				goto exit;
			}
		}
	}

exit:
	if (done) {
		plldivs->nr = nr2;
		plldivs->od = od2;
		plldivs->nf = nf2;
		INFO("%s: nr=%d, od=%d, nf=%d, err=%d, rate=%lld\n",
			__func__, nr2, od2, nf2, err2, fout);
		return 0;
	} else {
		return -1;
	}
}

static int cmu_pll_div_set(const struct cmu_desc *const cmu,
			 uint64_t frefclk,
			 const uint64_t fpllreq)
{
	int ret;
	struct plldivs current;
	struct plldivs calc;

	/* prepare */
	if (!cmu) {
		return -ENXIO;
	}
	if (!frefclk) {
		frefclk = cmu->frefclk;
	}
	cmu_pll_div_get(cmu->base, &current);
	ret = cmu_pll_div_calc(frefclk, fpllreq, &calc);
	if (ret) {
		return ret;
	}

	/* change */
	/* total drop */
	if (calc.nf < current.nf) {
		cmu_pll_set_nf(cmu->base, calc.nf);
	}
	if (calc.od > current.od) {
		cmu_pll_set_od(cmu->base, calc.od);
	}
	if (calc.nr > current.nr) {
		cmu_pll_set_nr(cmu->base, calc.nr);
	}

	/* up */
	if (calc.nf > current.nf) {
		cmu_pll_set_nf(cmu->base, calc.nf);
	}
	if (calc.od < current.od) {
		cmu_pll_set_od(cmu->base, calc.od);
	}
	if (calc.nr < current.nr) {
		cmu_pll_set_nr(cmu->base, calc.nr);
	}

	return 0;
}

static int cmu_clkch_div_get(const struct cmu_desc *const cmu,
			     const unsigned clkch)
{
	uintptr_t clkch_reg;
	uint32_t div;
	uint32_t reg;
	int64_t fpll;

	clkch_reg = cmu_clkch_get_reg(cmu->base, clkch);
	if (!clkch_reg) {
		return -ENXIO;
	}

	/* div */
	reg = mmio_read_32(clkch_reg);
	div = (reg & CMU_CLKCH_CTL_VAL_CLKDIV_MASK) >> CMU_CLKCH_CTL_VAL_CLKDIV_SHIFT;
	if (!div) {
		return -ENXIO;
	}
	if (clkch_reg == MMAVLSP_CMU0_CLKCHCTL_MSHC_TX_X2
		&& !(mmio_read_32(cmu->base + CMU_GPR_AVLSP_MSHC_CFG) & CMU_MSHC_CLKDIV)) {
		div *= CMU_MSHC_DIV;
	}

	/* pll */
	fpll = cmu_pll_get_rate(cmu->base, cmu->frefclk);
	if (!fpll) {
		return -ENXIO;
	}

	INFO("%s: base=0x%lx, ch=%d, rate=%llu, div=%d\n",
		__func__, cmu->base, clkch, fpll/div, div);

	return div;
}

static int cmu_clkch_div_calc(const struct cmu_desc *const cmu,
			      const unsigned clkch,
			      uint64_t fclkchreq)
{
	uintptr_t clkch_reg;
	int div;
	int64_t fpll;
	int max;

	if (!fclkchreq) {
		return -ENXIO;
	}

	clkch_reg = cmu_clkch_get_reg(cmu->base, clkch);

	/* calc */
	fpll = cmu_pll_get_rate(cmu->base, cmu->frefclk);
	if (fpll < 0) {
		return fpll;
	}

	div = fpll / fclkchreq;

	/* bound */
	max = CMU_CLKCH_CTL_VAL_CLKDIV_MAX;
	if (clkch_reg == MMAVLSP_CMU0_CLKCHCTL_MSHC_TX_X2) {
		max *= CMU_MSHC_DIV;
	}
	if (div < 1) {
		div = 1;
	}
	if (div > max) {
		div = max;
	}

	/* round */
	if (clkch_reg == MMAVLSP_CMU0_CLKCHCTL_MSHC_TX_X2 && div > CMU_CLKCH_CTL_VAL_CLKDIV_MAX) {
		div /= CMU_MSHC_DIV;
		div *= CMU_MSHC_DIV;
	}

	INFO("%s: base=0x%lx, ch=%d, rate=%llu, div=%d\n",
		__func__, cmu->base, clkch, fclkchreq, div);

	return div;
}

static int cmu_clkch_div_set(const struct cmu_desc *const cmu,
			     const unsigned clkch,
			     uint32_t div)
{
	uintptr_t clkch_reg;
	uint64_t timeout;
	uint32_t reg;

	/* prepare */
	if (div > CMU_CLKCH_CTL_VAL_CLKDIV_MAX * CMU_MSHC_DIV) {
		return -EINVAL;
	}

	clkch_reg = cmu_clkch_get_reg(cmu->base, clkch);
	if (!clkch_reg) {
		return -ENXIO;
	}

	/* change */
	if (clkch_reg == MMAVLSP_CMU0_CLKCHCTL_MSHC_TX_X2) {
		if (div > CMU_CLKCH_CTL_VAL_CLKDIV_MAX) {
			div /= CMU_MSHC_DIV;
			mmio_clrbits_32(cmu->base + CMU_GPR_AVLSP_MSHC_CFG, CMU_MSHC_CLKDIV);
		} else {
			mmio_setbits_32(cmu->base + CMU_GPR_AVLSP_MSHC_CFG, CMU_MSHC_CLKDIV);
		}
	}
	reg = mmio_read_32(clkch_reg);
	reg &= ~CMU_CLKCH_CTL_VAL_CLKDIV_MASK;
	reg |=  CMU_CLKCH_CTL_VAL_CLKDIV_SET(div) | CMU_CLKCH_CTL_SET_CLKDIV;
	mmio_write_32(clkch_reg, reg);

	/* wait */
	for (timeout = timeout_init_us(100000); !timeout_elapsed(timeout);) {
		if (mmio_read_32(clkch_reg) & CMU_CLKCH_CTL_LOCK_CLKDIV) {
			INFO("%s: base=0x%lx, ch=%d, div=%d\n",
				__func__, cmu->base, clkch, div);
			return 0;
		}
	}

	ERROR("%s: base=0x%lx, ch=%u, div=%u, lock timeout\n",
		__func__, cmu->base, clkch, div);
	return -ETIMEDOUT;
}

/* TODO: rework */
/* equivalent: cmu_clkch_div_set */
static void cmu_pllinit_div_set(const uintptr_t base,
				const cmu_pll_ctl_vals_t *const pllinit)
{
	int nf = (((uint64_t)pllinit->clkfhi << CLKFHI_SHIFT) +
		       ((uint64_t)pllinit->clkflo << CLKFLO_SHIFT))
		       / 0x200000000;
	int od = pllinit->clkod+1;
	int nr = pllinit->clkr+1;

	INFO("%s: base=0x%lx, nr=%d, od=%d, nf=%d\n",
		__func__, base, nr, od, nf);

	cmu_pll_set_nr(base, nr);
	cmu_pll_set_od(base, od);
	cmu_pll_set_nf(base, nf);

	/* IIGAIN, IPGAIN, IGAIN, PGAIN */
	mmio_clrsetbits_32(base + CMU_PLL_CTL4_REG,
			   CMU_PLL_CTL4_IIGAIN_LGMLT_MASK |
			   CMU_PLL_CTL4_IPGAIN_LGMLT_MASK |
			   CMU_PLL_CTL4_IGAIN_LGMLT_MASK  |
			   CMU_PLL_CTL4_PGAIN_LGMLT_MASK,
			   CMU_PLL_CTL4_IIGAIN_LGMLT_SET(pllinit->iigain) |
			   CMU_PLL_CTL4_IPGAIN_LGMLT_SET(pllinit->ipgain) |
			   CMU_PLL_CTL4_IGAIN_LGMLT_SET(pllinit->igain)	  |
			   CMU_PLL_CTL4_PGAIN_LGMLT_SET(pllinit->pgain));
}

static int cmu_set_core_rate (const struct cmu_desc *const cmu,
			      uint64_t frefclk,
			      const uint64_t fpllreq)
{
	int err;

	/* prepare */
	if (!cmu) {
		return -ENXIO;
	}
	if (!frefclk) {
		frefclk = cmu->frefclk;
	}

	/* change */
	mmca57_reconf_sclken(cmu->base, 2);
	err = cmu_pll_div_set(cmu, frefclk, fpllreq);
	if (err) {
		return err;
	}
	/* wait */
	/*
	if (cmu_pll_is_enabled(cmu->base)) {
		err = cmu_pll_lock_debounce(cmu->base);
	}
	*/
	return err;
}

static int cmu_set_periph_rate(const struct cmu_desc *const cmu,
			       uint64_t frefclk,
			       const uint64_t fpllreq)
{
	int err;

	/* prepare */
	if (!cmu) {
		return -ENXIO;
	}
	if (!frefclk) {
		frefclk = cmu->frefclk;
	}

	/*
	if (!cmu_pll_is_enabled(cmu->base)) {
		return cmu_pll_div_set(cmu, frefclk, fpllreq);
	}
	*/

	/* config */
	/*
	mmio_clrbits_32(cmu->base + CMU_PLL_CTL6_REG, CMU_PLL_CTL6_SWEN);
	mmio_setbits_32(cmu->base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_CTL_EN);
	mmio_clrbits_32(cmu->base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_BYPASS);
	*/

	/* change */
	err = cmu_pll_div_set(cmu, frefclk, fpllreq);
	if (err) {
		return err;
	}

	/* wait */
	mmio_setbits_32(cmu->base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_RST);
	/*
	err = cmu_pll_lock_debounce(cmu->base);
	if (err) {
		return err;
	}
	*/

	/* enable */
	/*
	mmio_setbits_32(cmu->base + CMU_PLL_CTL6_REG, CMU_PLL_CTL6_SWEN);
	mmio_clrbits_32(cmu->base + CMU_PLL_CTL6_REG, CMU_PLL_CTL6_SWRST);
	*/

	return 0;
}

int cmu_set_periph_rate2(const uintptr_t base, uint64_t frefclk, const uint64_t fpllreq)
{
	int ret;
	struct plldivs current;
	struct plldivs calc;

	cmu_pll_div_get(base, &current);
	ret = cmu_pll_div_calc(frefclk, fpllreq, &calc);
	if (ret) {
		return ret;
	}

	/* change */
	/* total drop */
	if (calc.nf < current.nf) {
		cmu_pll_set_nf(base, calc.nf);
	}
	if (calc.od > current.od) {
		cmu_pll_set_od(base, calc.od);
	}
	if (calc.nr > current.nr) {
		cmu_pll_set_nr(base, calc.nr);
	}

	/* up */
	if (calc.nf > current.nf) {
		cmu_pll_set_nf(base, calc.nf);
	}
	if (calc.od < current.od) {
		cmu_pll_set_od(base, calc.od);
	}
	if (calc.nr < current.nr) {
		cmu_pll_set_nr(base, calc.nr);
	}

	mmio_setbits_32(base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_RST);
	return 0;
}

static struct cmu_desc *const cmu_desc_get_by_base(const uintptr_t base)
{
	int idx;

	if (!base) {
		return NULL;
	}

	for (idx = 0; ; ++idx) {
		struct cmu_desc *const cmu = cmu_desc_get_by_idx(idx);
		if (!cmu) {
			break;
		}
		if (cmu->base == base) {
			return cmu;
		}
	}
	INFO("%s: base=0x%lx, fail\n", __func__, base);
	return NULL;
}

static uintptr_t cmu_clkch_get_reg(const uintptr_t base, const unsigned clkch)
{
	return base + CMU_CLKCH0_CTL_REG + clkch * 0x10;
}

static int cmu_pll_lock_debounce(const uintptr_t base)
{
	uint64_t debounce, timeout;

	for (timeout = timeout_init_us(100000); !timeout_elapsed(timeout);) {
		debounce = timeout_init_us(10000);
		while (mmio_read_32(base + CMU_PLL_CTL0_REG) & CMU_PLL_CTL0_LOCK) {
			if (timeout_elapsed(debounce)) {
				INFO("%s: base=0x%lx, stable\n", __func__, base);
				return 0;
			}
		}
	}
	ERROR("%s: base=0x%lx, timeout\n", __func__, base);
	return -ETIMEDOUT;
}

struct cmu_desc *const cmu_desc_get_by_idx(const unsigned idx)
{
	static struct cmu_desc cmus[20];
	if (idx >= ARRAY_SIZE(cmus)) {
		return NULL;
	}
	return &cmus[idx];
}

/*
 * TODO: rework, fixme pls
 * equivalent: cmu_clkch_div_set + cmu_clkch_enable
 */
void cmu_clkch_enable_by_base(const uintptr_t base, const unsigned div)
{
	uint64_t timeout;
	uint32_t reg;

	/* disable() */
	mmio_clrbits_32(base, CMU_CLKCH_CTL_CLK_EN);

	/* change */
	reg = mmio_read_32(base);
	reg &= ~CMU_CLKCH_CTL_VAL_CLKDIV_MASK;
	reg |=  CMU_CLKCH_CTL_VAL_CLKDIV_SET(div) | CMU_CLKCH_CTL_SET_CLKDIV;
	mmio_write_32(base, reg);
	/* wait */
	timeout = timeout_init_us(10000);
	while (!(mmio_read_32(base) & CMU_CLKCH_CTL_LOCK_CLKDIV)) {
		if (timeout_elapsed(timeout)) {
			ERROR("%s: base=0x%lx, div=%u, timeout\n", __func__, base, div);
			return;
		}
	}

	/* enable */
	mmio_setbits_32(base, CMU_CLKCH_CTL_CLK_EN);
	timeout = timeout_init_us(10000);
	while (!(mmio_read_32(base) & CMU_CLKCH_CTL_CLK_RDY)) {
		if (timeout_elapsed(timeout)) {
			ERROR("%s: base=0x%lx, div=%u, timeout\n", __func__, base, div);
			return;
		}
	}
	mmio_clrbits_32(base, CMU_CLKCH_CTL_SWRST);

	INFO("%s: base=0x%lx, enable\n", __func__, base);
}

/* TODO: rework */
/* equivalent: cmu_pll_set_rate() */
void cmu_pll_on(const uintptr_t base, const cmu_pll_ctl_vals_t *const pllinit)
{
	int err;

	if (base == MMAVLSP_CMU0_BASE) {
		INFO("%s: base=0x%lx, skip\n", __func__, base);
		return;
	}

	/* config */
	mmio_clrbits_32(base + CMU_PLL_CTL6_REG, CMU_PLL_CTL6_SWEN);
	mmio_setbits_32(base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_CTL_EN);
	mmio_clrbits_32(base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_BYPASS);

	/* change */
	cmu_pllinit_div_set(base, pllinit);

	/* wait */
	mmio_setbits_32(base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_RST);
	err = cmu_pll_lock_debounce(base);
	if (err) {
		return;
	}

	/* enable */
	mmio_setbits_32(base + CMU_PLL_CTL6_REG, CMU_PLL_CTL6_SWEN);
	mmio_clrbits_32(base + CMU_PLL_CTL6_REG, CMU_PLL_CTL6_SWRST);

	INFO("%s: base=0x%lx, ON\n", __func__, base);
}

static void cmu_pll_set_od(const uintptr_t base, uint32_t od)
{
	uint32_t reg;
	reg = mmio_read_32(base + CMU_PLL_CTL0_REG);
	reg &= ~CMU_PLL_CTL0_CLKOD_MASK;
	reg |= CMU_PLL_CTL0_CLKOD_SET(od-1);
	mmio_write_32(base + CMU_PLL_CTL0_REG, reg);
}

static void cmu_pll_set_nr(const uintptr_t base, uint32_t nr)
{
	uint32_t reg;
	reg = mmio_read_32(base + CMU_PLL_CTL0_REG);
	reg &= ~CMU_PLL_CTL0_CLKR_MASK;
	reg |= CMU_PLL_CTL0_CLKR_SET(nr-1);
	mmio_write_32(base + CMU_PLL_CTL0_REG, reg);
}

static void cmu_pll_set_nf(const uintptr_t base, uint32_t nf)
{
	uint32_t hi = ((nf * 0x200000000) & CLKFHI_MASK) >> CLKFHI_SHIFT;
	uint32_t lo = ((nf * 0x200000000) & CLKFLO_MASK) >> CLKFLO_SHIFT;

	mmio_write_32(base + CMU_PLL_CTL2_REG, hi);
	mmio_write_32(base + CMU_PLL_CTL1_REG, lo);
}

void cmu_info(void)
{
	static const uintptr_t cmus[] = {
		MMAVLSP_CMU0_BASE,
		MMAVLSP_CMU1_BASE,
		MMCA57_0_CMU0_BASE,
		MMCA57_1_CMU0_BASE,
		MMCA57_2_CMU0_BASE,
		MMCA57_3_CMU0_BASE,
		MMCCN_CMU0_BASE,
		MMCORESIGHT_CMU0_BASE,
		MMDDR0_CMU0_BASE,
		MMDDR1_CMU0_BASE,
		MMMALI_CMU0_BASE,
		MMPCIE_CMU0_BASE,
		MMUSB_CMU0_BASE,
		MMVDEC_CMU0_BASE,
		MMXGBE_CMU0_BASE,
		MMXGBE_CMU1_BASE
	};

	unsigned i;

	for (i = 0; i < ARRAY_SIZE(cmus); ++i) {
		static const char *cmu_names[] = {
			"AVLSP CMU#0",
			"AVLSP CMU#1",
			"CA57#0 CMU#0",
			"CA57#1 CMU#0",
			"CA57#2 CMU#0",
			"CA57#3 CMU#0",
			"CCN CMU#0",
			"CoreSight CMU#0",
			"DDR#0 CMU#0",
			"DDR#1 CMU#0",
			"Mali CMU#0",
			"PCIe CMU#0",
			"USB CMU#0",
			"VDec CMU#0",
			"XGbE CMU#0",
			"XGbE CMU#1"
		};

		static const char *clkchs_avlsp_cmu0[] = {
			"GPIO",
			"UART1",
			"UART2",
			"APB",
			"SPI",
			"eSPI",
			"I2C1",
			"I2C2",
			"Timer1",
			"Timer2",
			"Timer3",
			"Timer4",
			"DMAC",
			"SMBus1",
			"SMBus2",
			"HDA_SYS_CLK",
			"HDA_CLK48",
			"MSHC_AXI",
			"MSHC_AHB",
			"MSHC_CCLK_TX_X2",
			"MSHC_BCLK",
			"MSHC_TMCLK",
			"MSHC_CQETMCLK",
			0,
			0,
			0,
			"VDU_AXI",
			"SMMU"
		};

		static const char *clkchs_avlsp_cmu1[] = {
			"ipixelclk"
		};

		static const char *clkchs_ccn_cmu0[] = {
			"Main CCN"
		};

		static const char *clkchs_coresight_cmu0[] = {
			"System",
			"Trace",
			"General Timer Counter",
			"Timestamp"
		};

		static const char *clkchs_ddr_cmu0[] = {
			"AXI",
			"Core",
			"NIC stub",
			"APB"
		};

		static const char *clkchs_mali_cmu0[] = {
			"Mali AXI"
		};

		static const char *clkchs_pcie_cmu0[] = {
			"X4_0_MSTR",
			"X4_0_SLV",
			"X4_0_CFG",
			"X4_1_MSTR",
			"X4_1_SLV",
			"X4_1_CFG",
			"X8_MSTR",
			"X8_SLV",
			"X8_CFG",
			"TCU_CFG",
			"TBU0",
			"TBU1",
			"TBU2"
		};

		static const char *clkchs_usb_cmu0[] = {
			"SATA_PHY_REF",
			"SATA_ACLK_CTRL0",
			"SATA_ACLK_CTRL1",
			"USB2_PHY0_REF",
			"USB2_PHY1_REF",
			"USB2_ACLK",
			"USB2_CLK_SOFITP",
			"USB3_PHY0_REF",
			"USB3_PHY1_REF",
			"USB3_PHY2_REF",
			"USB3_PHY3_REF",
			"USB3_ACLK",
			"USB3_CLK_SOFITP",
			"USB3_CLK_SUSPEND",
			"SMMU_ACLK",
			"DMAC_ACLK",
			"GIC_ACLK"
		};

		static const char *clkchs_vdec_cmu0[] = {
			"VDec"
		};

		static const char *clkchs_xgbe_cmu0[] = {
			"CSR0",
			"CSR1",
			"XGBE0_REF",
			"XGBE0_ACLK",
			"XGBE0_PTP",
			"XGBE1_REF",
			"XGBE1_ACLK",
			"XGBE1_PTP",
			"GMAC0_ACLK",
			"GMAC0_PTPCLK",
			"GMAC0_TX2CLK",
			"GMAC1_ACLK",
			"GMAC1_PTPCLK",
			"GMAC1_TX2CLK",
			"MMU",
			"HDMI_VIDEO_ACLK",
			"HDMI_AUDIO_ACLK",
			"HDMI_SFR0"
		};

		static const char *clkchs_xgbe_cmu1[] = {
			"HDMI_SFR1"
		};

		unsigned clkch;
		unsigned div;
		unsigned freq;
		uint64_t nf;
		unsigned nr;
		unsigned od;
		unsigned pllen;

		pllen = (mmio_read_32(cmus[i] + CMU_PLL_CTL0_REG) & CMU_PLL_CTL0_CTL_EN) &&
			(mmio_read_32(cmus[i] + CMU_PLL_CTL6_REG) & CMU_PLL_CTL6_SWEN);

		nf = mmio_read_32(cmus[i] + CMU_PLL_CTL2_REG);
		nf <<= CLKFHI_SHIFT;
		nf |= mmio_read_32(cmus[i] + CMU_PLL_CTL1_REG);
		nf /= 0x200000000;

		nr = mmio_read_32(cmus[i] + CMU_PLL_CTL0_REG);
		nr &=  CMU_PLL_CTL0_CLKR_MASK;
		nr >>= CMU_PLL_CTL0_CLKR_SHIFT;
		nr += 1;

		od = mmio_read_32(cmus[i] + CMU_PLL_CTL0_REG);
		od &=  CMU_PLL_CTL0_CLKOD_MASK;
		od >>= CMU_PLL_CTL0_CLKOD_SHIFT;
		od += 1;

		if (cmus[i] == MMAVLSP_CMU1_BASE || cmus[i] == MMXGBE_CMU1_BASE) {
			freq = pllen ? 27000000 * nf / nr / od : 0;
		} else {
			freq = pllen ? 25000000 * nf / nr / od : 0;
		}

		INFO("%s: %u Hz\n", cmu_names[i], freq);

		switch (cmus[i]) {
		case MMAVLSP_CMU0_BASE:
			for (clkch = 0; clkch < ARRAY_SIZE(clkchs_avlsp_cmu0); ++clkch) {
				if (!clkchs_avlsp_cmu0[clkch]) {
					continue;
				}

				div = (mmio_read_32(cmu_clkch_get_reg(cmus[i], clkch)) &
				      CMU_CLKCH_CTL_VAL_CLKDIV_MASK) >>
				      CMU_CLKCH_CTL_VAL_CLKDIV_SHIFT;

				if (clkch == 19 && !(mmio_read_32(cmus[i] + CMU_GPR_AVLSP_MSHC_CFG) & CMU_MSHC_CLKDIV)) {
					div *= CMU_MSHC_DIV;
				}

				INFO("%s %s: %llu Hz\n", cmu_names[i], clkchs_avlsp_cmu0[clkch],
				     div ? freq / div : ULL(0));
			}

			break;
		case MMAVLSP_CMU1_BASE:
			for (clkch = 0; clkch < ARRAY_SIZE(clkchs_avlsp_cmu1); ++clkch) {
				div = (mmio_read_32(cmu_clkch_get_reg(cmus[i], clkch)) &
				      CMU_CLKCH_CTL_VAL_CLKDIV_MASK) >>
				      CMU_CLKCH_CTL_VAL_CLKDIV_SHIFT;

				INFO("%s %s: %llu Hz\n", cmu_names[i], clkchs_avlsp_cmu1[clkch],
				     div ? freq / div : ULL(0));
			}

			break;
		case MMCCN_CMU0_BASE:
			for (clkch = 0; clkch < ARRAY_SIZE(clkchs_ccn_cmu0); ++clkch) {
				div = (mmio_read_32(cmu_clkch_get_reg(cmus[i], clkch)) &
				      CMU_CLKCH_CTL_VAL_CLKDIV_MASK) >>
				      CMU_CLKCH_CTL_VAL_CLKDIV_SHIFT;

				INFO("%s %s: %llu Hz\n", cmu_names[i], clkchs_ccn_cmu0[clkch],
				     div ? freq / div : ULL(0));
			}

			break;
		case MMCORESIGHT_CMU0_BASE:
			for (clkch = 0; clkch < ARRAY_SIZE(clkchs_coresight_cmu0); ++clkch) {
				div = (mmio_read_32(cmu_clkch_get_reg(cmus[i], clkch)) &
				      CMU_CLKCH_CTL_VAL_CLKDIV_MASK) >>
				      CMU_CLKCH_CTL_VAL_CLKDIV_SHIFT;

				INFO("%s %s: %llu Hz\n", cmu_names[i], clkchs_coresight_cmu0[clkch],
				     div ? freq / div : ULL(0));
			}

			break;
		case MMDDR0_CMU0_BASE:
		case MMDDR1_CMU0_BASE:
			for (clkch = 0; clkch < ARRAY_SIZE(clkchs_ddr_cmu0); ++clkch) {
				div = (mmio_read_32(cmu_clkch_get_reg(cmus[i], clkch)) &
				      CMU_CLKCH_CTL_VAL_CLKDIV_MASK) >>
				      CMU_CLKCH_CTL_VAL_CLKDIV_SHIFT;

				INFO("%s %s: %llu Hz\n", cmu_names[i], clkchs_ddr_cmu0[clkch],
				     div ? freq / div : ULL(0));
			}

			break;
		case MMMALI_CMU0_BASE:
			for (clkch = 0; clkch < ARRAY_SIZE(clkchs_mali_cmu0); ++clkch) {
				div = (mmio_read_32(cmu_clkch_get_reg(cmus[i], clkch)) &
				      CMU_CLKCH_CTL_VAL_CLKDIV_MASK) >>
				      CMU_CLKCH_CTL_VAL_CLKDIV_SHIFT;

				INFO("%s %s: %llu Hz\n", cmu_names[i], clkchs_mali_cmu0[clkch],
				     div ? freq / div : ULL(0));
			}

			break;
		case MMPCIE_CMU0_BASE:
			for (clkch = 0; clkch < ARRAY_SIZE(clkchs_pcie_cmu0); ++clkch) {
				div = (mmio_read_32(cmu_clkch_get_reg(cmus[i], clkch)) &
				      CMU_CLKCH_CTL_VAL_CLKDIV_MASK) >>
				      CMU_CLKCH_CTL_VAL_CLKDIV_SHIFT;

				INFO("%s %s: %llu Hz\n", cmu_names[i], clkchs_pcie_cmu0[clkch],
				     div ? freq / div : ULL(0));
			}

			break;
		case MMUSB_CMU0_BASE:
			for (clkch = 0; clkch < ARRAY_SIZE(clkchs_usb_cmu0); ++clkch) {
				div = (mmio_read_32(cmu_clkch_get_reg(cmus[i], clkch)) &
				      CMU_CLKCH_CTL_VAL_CLKDIV_MASK) >>
				      CMU_CLKCH_CTL_VAL_CLKDIV_SHIFT;

				INFO("%s %s: %llu Hz\n", cmu_names[i], clkchs_usb_cmu0[clkch],
				     div ? freq / div : ULL(0));
			}

			break;
		case MMVDEC_CMU0_BASE:
			for (clkch = 0; clkch < ARRAY_SIZE(clkchs_vdec_cmu0); ++clkch) {
				div = (mmio_read_32(cmu_clkch_get_reg(cmus[i], clkch)) &
				      CMU_CLKCH_CTL_VAL_CLKDIV_MASK) >>
				      CMU_CLKCH_CTL_VAL_CLKDIV_SHIFT;

				INFO("%s %s: %llu Hz\n", cmu_names[i], clkchs_vdec_cmu0[clkch],
				     div ? freq / div : ULL(0));
			}

			break;
		case MMXGBE_CMU0_BASE:
			for (clkch = 0; clkch < ARRAY_SIZE(clkchs_xgbe_cmu0); ++clkch) {
				div = (mmio_read_32(cmu_clkch_get_reg(cmus[i], clkch)) &
				      CMU_CLKCH_CTL_VAL_CLKDIV_MASK) >>
				      CMU_CLKCH_CTL_VAL_CLKDIV_SHIFT;

				INFO("%s %s: %llu Hz\n", cmu_names[i], clkchs_xgbe_cmu0[clkch],
				     div ? freq / div : ULL(0));
			}

			break;
		case MMXGBE_CMU1_BASE:
			for (clkch = 0; clkch < ARRAY_SIZE(clkchs_xgbe_cmu1); ++clkch) {
				div = (mmio_read_32(cmu_clkch_get_reg(cmus[i], clkch)) &
				      CMU_CLKCH_CTL_VAL_CLKDIV_MASK) >>
				      CMU_CLKCH_CTL_VAL_CLKDIV_SHIFT;

				INFO("%s %s: %llu Hz\n", cmu_names[i], clkchs_xgbe_cmu1[clkch],
				     div ? freq / div : ULL(0));
			}

			break;
		}
	}
}
