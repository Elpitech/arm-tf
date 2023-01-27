/*
 * Copyright (c) 2018-2022, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <common/debug.h>
#include <common/runtime_svc.h>
#include <libfdt.h>

#include <baikal_pvt.h>
#include <baikal_scp.h>
#include <baikal_sip_svc.h>
#include <bm1000_cmu.h>
#include <bm1000_smmu.h>
#include <platform_def.h>

#define BAIKAL_SMC_PLAT_CMU_PLL_SET_RATE	0
#define BAIKAL_SMC_PLAT_CMU_PLL_GET_RATE	1
#define BAIKAL_SMC_PLAT_CMU_PLL_ENABLE		2
#define BAIKAL_SMC_PLAT_CMU_PLL_DISABLE		3
#define BAIKAL_SMC_PLAT_CMU_PLL_ROUND_RATE	4
#define BAIKAL_SMC_PLAT_CMU_PLL_IS_ENABLED	5
#define BAIKAL_SMC_PLAT_CMU_CLKCH_SET_RATE	6
#define BAIKAL_SMC_PLAT_CMU_CLKCH_GET_RATE	7
#define BAIKAL_SMC_PLAT_CMU_CLKCH_ENABLE	8
#define BAIKAL_SMC_PLAT_CMU_CLKCH_DISABLE	9
#define BAIKAL_SMC_PLAT_CMU_CLKCH_ROUND_RATE	10
#define BAIKAL_SMC_PLAT_CMU_CLKCH_IS_ENABLED	11
#define BAIKAL_SMC_PLAT_CMU_CLKCH_MSHC_SPEED	12

static const char *cmd2str[] = {
	"PLL_SET",
	"PLL_GET",
	"PLL_EN",
	"PLL_DIS",
	"PLL_ROUND",
	"PLL_IS_EN",
	"CLKCH_SET",
	"CLKCH_GET",
	"CLKCH_EN",
	"CLKCH_DIS",
	"CLKCH_ROUND",
	"CLKCH_IS_EN",
	"CLKCH_MSHC"
};

static int baikal_get_cmu_descriptors(void *fdt)
{
	unsigned cmuidx = 0;
	int offs = 0;

	while (1) {
		const fdt32_t *ac;
		int plen;

		offs = fdt_next_node(fdt, offs, NULL);
		if (offs < 0) {
			break;
		}

		if (!fdt_node_check_compatible(fdt, offs, "baikal,bm1000-cmu")) {
			struct cmu_desc *const cmu = cmu_desc_get_by_idx(cmuidx);
			int num_of_chans;

			if (cmu == NULL) {
				ERROR("%s: unable to get cmu_desc with idx = %u\n",
				      __func__, cmuidx);
				return 2;
			}

			/* clear */
			cmu->base	     = 0;
			cmu->name	     = NULL;
			cmu->frefclk	     = 0;
			cmu->fpllmin	     = 0;
			cmu->fpllmax	     = 0;
			cmu->fpllreq	     = 0;
			cmu->deny_pll_reconf = false;

			ac = fdt_getprop(fdt, offs, "cmu-id", &plen);
			if (ac == NULL) {
				return 2;
			}

			cmu->base = fdt32_to_cpu(*ac);

			ac = fdt_getprop(fdt, offs, "clocks", &plen);
			if (ac != NULL) {
				int offs_par;

				offs_par = fdt_node_offset_by_phandle(fdt, fdt32_to_cpu(*ac));
				ac = fdt_getprop(fdt, offs_par, "clock-frequency", &plen);
				if (ac == NULL) {
					return 2;
				}

				cmu->frefclk = fdt32_to_cpu(*ac);
			} else {
				cmu->frefclk = CMU_REFCLK_DEFAULT_FREQ;
			}

			ac = fdt_getprop(fdt, offs, "clock-frequency", &plen);
			if (ac != NULL) {
				cmu->fpllreq = fdt32_to_cpu(*ac);
			} else {
				cmu->fpllreq = CMU_PLL_DEFAULT_FREQ;
			}

			ac = fdt_getprop(fdt, offs, "clock-indices", &plen);
			if (ac != NULL) {
				num_of_chans = plen / 4;
			} else {
				num_of_chans = 0;
			}

			if (num_of_chans > 1) {
				cmu->deny_pll_reconf = true;
			}

			cmu->name = fdt_getprop(fdt, offs, "clock-output-names", &plen);

			INFO("CMU @ base=0x%08lx name=%s clk=%lu freq=%lu reconf=%d min=%lu max=%lu chans=%d\n",
			     cmu->base,
			     cmu->name,
			     cmu->frefclk,
			     cmu->fpllreq,
			     cmu->deny_pll_reconf,
			     cmu->fpllmin,
			     cmu->fpllmax,
			     num_of_chans);

			++cmuidx;
		}
	}

	return 0;
}

static int baikal_sip_setup(void)
{
	int ret;
	void *fdt = (void *)(uintptr_t)BAIKAL_SEC_DTB_BASE;

	ret = fdt_open_into(fdt, fdt, BAIKAL_DTB_MAX_SIZE);
	if (ret < 0) {
		ERROR("Invalid Device Tree at %p: error %d\n", fdt, ret);
		return -1;
	}

	return baikal_get_cmu_descriptors(fdt);
}

static uint64_t baikal_pvt_handler(uint32_t cmd,
				   uint64_t x1,
				   uint64_t x2,
				   uint64_t x3)
{
	switch (cmd) {
	case PVT_READ:
		return (uint64_t)pvt_read_reg((uint32_t)x1, (uint32_t)x2);
	case PVT_WRITE:
		return (uint64_t)pvt_write_reg((uint32_t)x1, (uint32_t)x2, (uint32_t)x3);
	}

	ERROR("%s: unhandled SMC 0x%x\n", __func__, cmd);
	return SMC_UNK;
}

static int64_t baikal_plat_sip_handler(uint32_t id,
				       uint64_t cmd,
				       uint64_t x1,
				       uint64_t x2)
{
	INFO("%s: SMC%ld=%s\n", __func__, cmd, cmd2str[cmd]);

	switch (cmd) {
	case BAIKAL_SMC_PLAT_CMU_PLL_SET_RATE:
		return cmu_pll_set_rate(id, x2, x1);
	case BAIKAL_SMC_PLAT_CMU_PLL_GET_RATE:
		return cmu_pll_get_rate(id, x2);
	case BAIKAL_SMC_PLAT_CMU_PLL_ENABLE:
		return cmu_pll_enable(id);
	case BAIKAL_SMC_PLAT_CMU_PLL_DISABLE:
		return cmu_pll_disable(id);
	case BAIKAL_SMC_PLAT_CMU_PLL_ROUND_RATE:
		return cmu_pll_round_rate(id, x2, x1);
	case BAIKAL_SMC_PLAT_CMU_PLL_IS_ENABLED:
		return cmu_pll_is_enabled(id);
	case BAIKAL_SMC_PLAT_CMU_CLKCH_SET_RATE:
		return cmu_clkch_set_rate((uint32_t)x2, id, x1);
	case BAIKAL_SMC_PLAT_CMU_CLKCH_GET_RATE:
		return cmu_clkch_get_rate((uint32_t)x2, id);
	case BAIKAL_SMC_PLAT_CMU_CLKCH_ENABLE:
		return cmu_clkch_enable((uint32_t)x2, id);
	case BAIKAL_SMC_PLAT_CMU_CLKCH_DISABLE:
		return cmu_clkch_disable((uint32_t)x2, id);
	case BAIKAL_SMC_PLAT_CMU_CLKCH_ROUND_RATE:
		return cmu_clkch_round_rate((uint32_t)x2, id, x1);
	case BAIKAL_SMC_PLAT_CMU_CLKCH_IS_ENABLED:
		return cmu_clkch_is_enabled((uint32_t)x2, id);
	}

	ERROR("%s: unhandled SMC: cmd:0x%lx id:0x%x\n", __func__, cmd, id);
	return SMC_UNK;
}

static uintptr_t sip_smc_handler(uint32_t smc_fid,
				 u_register_t x1,
				 u_register_t x2,
				 u_register_t x3,
				 u_register_t x4,
				 void *cookie,
				 void *handle,
				 u_register_t flags)
{
	uint64_t ret = 0;
	uint64_t data[4];

	if (is_caller_secure(flags)) {
		ERROR("%s: SMC secure world's call (0x%x)\n", __func__, smc_fid);
		SMC_RET1(handle, SMC_UNK);
	}

	switch (smc_fid) {
	case BAIKAL_SMC_PVT_ID:
		ret = baikal_pvt_handler(x1, x2, x3, x4);
		break;
	case BAIKAL_SMC_VDEC_SMMU_SET_CACHE:
		ret = mmvdec_smmu_set_domain_cache(x1, x2, x3);
		break;
	case BAIKAL_SMC_VDEC_SMMU_GET_CACHE:
		ret = mmvdec_smmu_get_domain_cache();
		break;
	case BAIKAL_SMC_VDU_UPDATE:
		ret = scp_cmd('V', x1, x2);
		break;
	case BAIKAL_SMC_SCP_LOG_DISABLE:
		ret = scp_cmd('T', 0, 0);
		break;
	case BAIKAL_SMC_SCP_LOG_ENABLE:
		ret = scp_cmd('t', 0, 0);
		break;
	case BAIKAL_SMC_FLASH_ERASE:
	case BAIKAL_SMC_FLASH_INIT:
	case BAIKAL_SMC_FLASH_POSITION:
	case BAIKAL_SMC_FLASH_PULL:
	case BAIKAL_SMC_FLASH_PUSH:
	case BAIKAL_SMC_FLASH_READ:
	case BAIKAL_SMC_FLASH_WRITE:
		ret = baikal_smc_flash_handler(smc_fid, x1, x2, x3, x4, data);
		if (ret) {
			break;
		} else if (smc_fid == BAIKAL_SMC_FLASH_PULL) {
			SMC_RET4(handle, data[0], data[1], data[2], data[3]);
		} else if (smc_fid == BAIKAL_SMC_FLASH_INIT) {
			SMC_RET3(handle, ret, data[0], data[1]);
		}
		break;
	case BAIKAL_SMC_FLASH_LOCK:
		ret = scp_cmd('L', x1, 0);
		break;
	default:
		ret = baikal_plat_sip_handler(x1, x2, x3, x4);
		break;
	}

	SMC_RET1(handle, ret);
}

DECLARE_RT_SVC(
	baikal_sip_svc,
	OEN_SIP_START,
	OEN_SIP_END,
	SMC_TYPE_FAST,
	baikal_sip_setup,
	sip_smc_handler
);
