/*
 * Copyright (c) 2018-2022, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>

#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>

#include <bm1000_cmu.h>
#include <bm1000_def.h>
#include <bm1000_private.h>
#include <platform_def.h>

#include "../bm1000_mmca57.h"

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
static void cmu_pll_set_od(const uintptr_t base, const uint32_t od);
static void cmu_pll_set_nr(const uintptr_t base, const uint32_t nr);
static void cmu_pll_set_nf(const uintptr_t base, const uint32_t nf);

static const uintptr_t mmca57_bases[] = {
	MMCA57_0_BASE,
	MMCA57_1_BASE,
	MMCA57_2_BASE,
	MMCA57_3_BASE
};
CASSERT(ARRAY_SIZE(mmca57_bases) == PLATFORM_CLUSTER_COUNT, assert_mmca57_bases_size);

int cmu_pll_is_enabled(const uintptr_t base)
{
	const int ret = (mmio_read_32(base + CMU_PLL_CTL0_REG) & CMU_PLL_CTL0_CTL_EN) &&
			(mmio_read_32(base + CMU_PLL_CTL6_REG) & CMU_PLL_CTL6_SWEN);

	INFO("%s: base=0x%08lx %s\n", __func__, base, ret ? "enabled" : "disabled");
	return ret;
}

int cmu_pll_enable(const uintptr_t base)
{
	int err;

	if (cmu_pll_is_enabled(base)) {
		INFO("%s: base=0x%08lx already enabled\n", __func__, base);
		return 0;
	}

	mmio_clrbits_32(base + CMU_PLL_CTL6_REG, CMU_PLL_CTL6_SWEN);
	mmio_setbits_32(base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_CTL_EN);
	mmio_clrbits_32(base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_BYPASS);
	mmio_setbits_32(base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_RST);

	err = cmu_pll_lock_debounce(base);
	if (err) {
		ERROR("%s: base=0x%08lx timeout\n", __func__, base);
		return err;
	}

	INFO("%s: base=0x%08lx enable\n", __func__, base);
	mmio_setbits_32(base + CMU_PLL_CTL6_REG, CMU_PLL_CTL6_SWEN);
	mmio_clrbits_32(base + CMU_PLL_CTL6_REG, CMU_PLL_CTL6_SWRST);
	return 0;
}

int cmu_pll_disable(const uintptr_t base)
{
	INFO("%s: base=0x%08lx disable\n", __func__, base);
	mmio_clrbits_32(base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_CTL_EN);
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
	INFO("%s: base=0x%08lx rate=%lu\n", __func__, base, fpll);
	return fpll;
}

int64_t cmu_pll_round_rate(const uintptr_t base, uint64_t frefclk, const uint64_t fpllreq)
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
	INFO("%s: base=0x%08lx rate=%lu\n", __func__, base, fpll);
	return fpll;
}

int cmu_pll_set_rate(const uintptr_t base, uint64_t frefclk, const uint64_t fpllreq)
{
	int err;
	struct cmu_desc *const cmu = cmu_desc_get_by_base(base);

	if (base == MMAVLSP_CMU0_BASE) {
		INFO("%s: base=0x%08lx skip\n", __func__, base);
		return -ENXIO;
	}

	if (!cmu) {
		return -ENXIO;
	}

	if (!frefclk) {
		frefclk = cmu->frefclk;
	}

	if (fpllreq == cmu_pll_get_rate(base, frefclk)) {
		INFO("%s: base=0x%08lx rate=%lu already set\n", __func__, base, fpllreq);
		return 0;
	}

	if (base == MMCA57_0_CMU0_BASE ||
	    base == MMCA57_1_CMU0_BASE ||
	    base == MMCA57_2_CMU0_BASE ||
	    base == MMCA57_3_CMU0_BASE) {
		err = cmu_set_core_rate(cmu, frefclk, fpllreq);
	} else {
		err = cmu_set_periph_rate(cmu, frefclk, fpllreq);
	}

	INFO("%s: base=0x%08lx name=%s rate=%lu%s\n",
		__func__, base, cmu->name, fpllreq, err ? ", fail" : "");

	return err;
}

int cmu_clkch_is_enabled(const uintptr_t base, const unsigned clkch)
{
	int ret;
	uintptr_t clkch_reg = cmu_clkch_get_reg(base, clkch);

	if (!clkch_reg) {
		return -ENXIO;
	}

	ret = (mmio_read_32(clkch_reg) & CMU_CLKCH_CTL_CLK_EN) && cmu_pll_is_enabled(base);
	INFO("%s: base=0x%08lx ch=%u %s\n", __func__, base, clkch, ret ? "enabled" : "disabled");
	return ret;
}

int cmu_clkch_enable(const uintptr_t base, const unsigned clkch)
{
	uintptr_t clkch_reg;
	uint64_t timeout;

	if (!cmu_pll_is_enabled(base)) {
		return -ENODEV;
	}

	if (cmu_clkch_is_enabled(base, clkch)) {
		INFO("%s: base=0x%08lx ch=%u already enabled\n", __func__, base, clkch);
		return 0;
	}

	clkch_reg = cmu_clkch_get_reg(base, clkch);
	if (!clkch_reg) {
		return -ENXIO;
	}

	mmio_setbits_32(clkch_reg, CMU_CLKCH_CTL_CLK_EN);
	for (timeout = timeout_init_us(100000); !timeout_elapsed(timeout);) {
		if (mmio_read_32(clkch_reg) & CMU_CLKCH_CTL_CLK_RDY) {
			INFO("%s: base=0x%08lx ch=%u enable\n", __func__, base, clkch);
			mmio_clrbits_32(clkch_reg, CMU_CLKCH_CTL_SWRST);
			return 0;
		}
	}

	ERROR("%s: base=0x%08lx ch=%u lock timeout\n", __func__, base, clkch);
	return -ETIMEDOUT;
}

int cmu_clkch_disable(const uintptr_t base, const unsigned clkch)
{
	uintptr_t clkch_reg = cmu_clkch_get_reg(base, clkch);

	if (!clkch_reg) {
		return -ENXIO;
	}

	INFO("%s: base=0x%08lx ch=%u disable\n", __func__, base, clkch);
	mmio_clrbits_32(clkch_reg, CMU_CLKCH_CTL_CLK_EN);
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
	INFO("%s: rate=%ld\n", __func__, rate);
	return rate;
}

int64_t cmu_clkch_round_rate(const uintptr_t base, const unsigned clkch, const uint64_t fclkchreq)
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

	INFO("%s: base=0x%08lx ch=%u rate=%lu round=%ld\n",
	     __func__, base, clkch, fclkchreq, ret);

	return ret;
}

int cmu_clkch_set_rate(const uintptr_t base, const unsigned clkch, const uint64_t fclkchreq)
{
	int ret = 0;
	const struct cmu_desc *const cmu = cmu_desc_get_by_base(base);

	if (!cmu) {
		return -ENXIO;
	}

	if (fclkchreq == cmu_clkch_get_rate(base, clkch)) {
		INFO("%s: base=0x%08lx rate=%lu already set\n", __func__, base, fclkchreq);
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

	INFO("%s: base=0x%08lx ch=%u rate=%lu\n", __func__, base, clkch, fclkchreq);
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

	INFO("%s: base=0x%08lx nr=%d od=%d nf=%d\n",
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
	const uint64_t fout_max = 2200000000; /* 2.2 GHz */
	uint32_t od_max = fvco_max / fpllreq;
	uint32_t od, od2 = -1;
	uint32_t nr, nr2 = -1;
	uint32_t nf, nf2 = -1;
	int32_t err;
	uint32_t err2 = -1;
	bool done = false;

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
		INFO("%s: nr=%d od=%d nf=%d err=%d rate=%ld\n",
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

	dsb();

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

static int cmu_clkch_div_get(const struct cmu_desc *const cmu, const unsigned clkch)
{
	uintptr_t clkch_reg;
	uint32_t div;
	uint32_t reg;
	int64_t fpll;

	clkch_reg = cmu_clkch_get_reg(cmu->base, clkch);
	if (!clkch_reg) {
		return -ENXIO;
	}

	reg = mmio_read_32(clkch_reg);
	div = (reg & CMU_CLKCH_CTL_VAL_CLKDIV_MASK) >> CMU_CLKCH_CTL_VAL_CLKDIV_SHIFT;
	if (!div) {
		return -ENXIO;
	}

	if (clkch_reg == MMAVLSP_CMU0_CLKCHCTL_MSHC_TX_X2
		&& !(mmio_read_32(cmu->base + CMU_GPR_AVLSP_MSHC_CFG) & CMU_MSHC_CLKDIV)) {
		div *= CMU_MSHC_DIV;
	}

	fpll = cmu_pll_get_rate(cmu->base, cmu->frefclk);
	if (!fpll) {
		return -ENXIO;
	}

	INFO("%s: base=0x%08lx ch=%u rate=%lu div=%d\n",
		__func__, cmu->base, clkch, fpll / div, div);

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

	INFO("%s: base=0x%08lx ch=%u rate=%lu div=%d\n",
		__func__, cmu->base, clkch, fclkchreq, div);

	return div;
}

static int cmu_clkch_div_set(const struct cmu_desc *const cmu, const unsigned clkch, uint32_t div)
{
	uintptr_t clkch_reg;
	uint64_t timeout;

	if (div > CMU_CLKCH_CTL_VAL_CLKDIV_MAX * CMU_MSHC_DIV) {
		return -EINVAL;
	}

	clkch_reg = cmu_clkch_get_reg(cmu->base, clkch);
	if (!clkch_reg) {
		return -ENXIO;
	}

	if (clkch_reg == MMAVLSP_CMU0_CLKCHCTL_MSHC_TX_X2) {
		if (div > CMU_CLKCH_CTL_VAL_CLKDIV_MAX) {
			div /= CMU_MSHC_DIV;
			mmio_clrbits_32(cmu->base + CMU_GPR_AVLSP_MSHC_CFG, CMU_MSHC_CLKDIV);
		} else {
			mmio_setbits_32(cmu->base + CMU_GPR_AVLSP_MSHC_CFG, CMU_MSHC_CLKDIV);
		}
	}

	mmio_clrsetbits_32(clkch_reg,
		CMU_CLKCH_CTL_VAL_CLKDIV_MASK,
		CMU_CLKCH_CTL_VAL_CLKDIV_SET(div) | CMU_CLKCH_CTL_SET_CLKDIV);

	for (timeout = timeout_init_us(100000); !timeout_elapsed(timeout);) {
		if (mmio_read_32(clkch_reg) & CMU_CLKCH_CTL_LOCK_CLKDIV) {
			INFO("%s: base=0x%08lx ch=%u div=%d\n", __func__, cmu->base, clkch, div);
			return 0;
		}
	}

	ERROR("%s: base=0x%08lx ch=%u div=%u lock timeout\n", __func__, cmu->base, clkch, div);
	return -ETIMEDOUT;
}

/* TODO: rework, it is similar to cmu_clkch_div_set() */
static void cmu_pllinit_div_set(const uintptr_t base,
				const cmu_pll_ctl_vals_t *const pllinit)
{
	int nf = (((uint64_t)pllinit->clkfhi << CLKFHI_SHIFT) +
		  ((uint64_t)pllinit->clkflo << CLKFLO_SHIFT)) / 0x200000000;
	int od = pllinit->clkod + 1;
	int nr = pllinit->clkr + 1;

	INFO("%s: base=0x%08lx nr=%d od=%d nf=%d\n", __func__, base, nr, od, nf);
	cmu_pll_set_nr(base, nr);
	cmu_pll_set_od(base, od);
	cmu_pll_set_nf(base, nf);

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

static int cmu_set_core_rate(const struct cmu_desc *const cmu,
			     uint64_t frefclk,
			     const uint64_t fpllreq)
{
	int err;

	if (!cmu) {
		return -ENXIO;
	}

	if (!frefclk) {
		frefclk = cmu->frefclk;
	}

	if (fpllreq > 1300000000 && mmca57_get_sclk_div(cmu->base) < 2) {
		mmca57_reconf_sclken(cmu->base, 2);
	}

	err = cmu_pll_div_set(cmu, frefclk, fpllreq);
	if (err) {
		return err;
	}

	if (cmu->base == mmca57_bases[(read_mpidr() >> MPIDR_AFF1_SHIFT) & MPIDR_AFFLVL_MASK]) {
		err = cmu_pll_lock_debounce(cmu->base);
	}

	return err;
}

static int cmu_set_periph_rate(const struct cmu_desc *const cmu,
			       uint64_t frefclk,
			       const uint64_t fpllreq)
{
	int err;

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

	/*
	mmio_clrbits_32(cmu->base + CMU_PLL_CTL6_REG, CMU_PLL_CTL6_SWEN);
	mmio_setbits_32(cmu->base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_CTL_EN);
	mmio_clrbits_32(cmu->base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_BYPASS);
	*/

	err = cmu_pll_div_set(cmu, frefclk, fpllreq);
	if (err) {
		return err;
	}

	mmio_setbits_32(cmu->base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_RST);
	/*
	err = cmu_pll_lock_debounce(cmu->base);
	if (err) {
		return err;
	}
	*/

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

	INFO("%s: base=0x%08lx fail\n", __func__, base);
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
				return 0;
			}
		}
	}

	ERROR("%s: base=0x%08lx timeout\n", __func__, base);
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

/* TODO: rework, it is silimar to {cmu_clkch_div_set() + cmu_clkch_enable()} */
void cmu_clkch_enable_by_base(const uintptr_t base, const unsigned div)
{
	uint64_t timeout;

	mmio_clrbits_32(base, CMU_CLKCH_CTL_CLK_EN);
	mmio_clrsetbits_32(base,
		CMU_CLKCH_CTL_VAL_CLKDIV_MASK,
		CMU_CLKCH_CTL_VAL_CLKDIV_SET(div) | CMU_CLKCH_CTL_SET_CLKDIV);

	timeout = timeout_init_us(10000);
	while (!(mmio_read_32(base) & CMU_CLKCH_CTL_LOCK_CLKDIV)) {
		if (timeout_elapsed(timeout)) {
			ERROR("%s: base=0x%08lx div=%u timeout\n", __func__, base, div);
			return;
		}
	}

	mmio_setbits_32(base, CMU_CLKCH_CTL_CLK_EN);
	timeout = timeout_init_us(10000);
	while (!(mmio_read_32(base) & CMU_CLKCH_CTL_CLK_RDY)) {
		if (timeout_elapsed(timeout)) {
			ERROR("%s: base=0x%08lx div=%u timeout\n", __func__, base, div);
			return;
		}
	}

	INFO("%s: base=0x%08lx enable\n", __func__, base);
	mmio_clrbits_32(base, CMU_CLKCH_CTL_SWRST);
}

/* TODO: rework, it is similar to cmu_pll_set_rate() */
void cmu_pll_on(const uintptr_t base, const cmu_pll_ctl_vals_t *const pllinit)
{
	int err;

	if (base == MMAVLSP_CMU0_BASE) {
		INFO("%s: base=0x%08lx skip\n", __func__, base);
		return;
	}

	mmio_clrbits_32(base + CMU_PLL_CTL6_REG, CMU_PLL_CTL6_SWEN);
	mmio_setbits_32(base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_CTL_EN);
	mmio_clrbits_32(base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_BYPASS);

	cmu_pllinit_div_set(base, pllinit);

	mmio_setbits_32(base + CMU_PLL_CTL0_REG, CMU_PLL_CTL0_RST);
	err = cmu_pll_lock_debounce(base);
	if (err) {
		return;
	}

	INFO("%s: base=0x%08lx enable\n", __func__, base);
	mmio_setbits_32(base + CMU_PLL_CTL6_REG, CMU_PLL_CTL6_SWEN);
	mmio_clrbits_32(base + CMU_PLL_CTL6_REG, CMU_PLL_CTL6_SWRST);
}

static void cmu_pll_set_od(const uintptr_t base, const uint32_t od)
{
	mmio_clrsetbits_32(base + CMU_PLL_CTL0_REG,
		CMU_PLL_CTL0_CLKOD_MASK,
		CMU_PLL_CTL0_CLKOD_SET(od - 1));
}

static void cmu_pll_set_nr(const uintptr_t base, const uint32_t nr)
{
	mmio_clrsetbits_32(base + CMU_PLL_CTL0_REG,
		CMU_PLL_CTL0_CLKR_MASK,
		CMU_PLL_CTL0_CLKR_SET(nr - 1));
}

static void cmu_pll_set_nf(const uintptr_t base, const uint32_t nf)
{
	const uint32_t hi = ((nf * 0x200000000) & CLKFHI_MASK) >> CLKFHI_SHIFT;
	const uint32_t lo = ((nf * 0x200000000) & CLKFLO_MASK) >> CLKFLO_SHIFT;

	mmio_write_32(base + CMU_PLL_CTL2_REG, hi);
	mmio_write_32(base + CMU_PLL_CTL1_REG, lo);
}
