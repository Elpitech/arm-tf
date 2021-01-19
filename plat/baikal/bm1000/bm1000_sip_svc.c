/*
 * Copyright (c) 2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <baikal_scp.h>
#include <baikal_sip_svc_flash.h>
#include <bm1000_cmu.h>
#include <bm1000_pvt.h>
#include <bm1000_smmu.h>
#include <bm1000_vdu.h>
#include <common/debug.h>
#include <common/runtime_svc.h>
#include <lib/mmio.h>
#include <libfdt.h>
#include <platform_def.h>
#include <tools_share/uuid.h>

#define BAIKAL_SMC_LCRU_ID		0x82000000
#define BAIKAL_SMC_PVT_ID		0x82000001
#define BAIKAL_SMC_VDU_UPDATE		0x82000100
#define BAIKAL_SMC_SCP_LOG_DISABLE	0x82000200
#define BAIKAL_SMC_SCP_LOG_ENABLE	0x82000201
#define BAIKAL_SMC_VDEC_SMMU_SET_CACHE	0x82000300
#define BAIKAL_SMC_VDEC_SMMU_GET_CACHE	0x82000301

static int baikal_get_cmu_descriptors(void *fdt)
{
	int offs = 0;
	int offs_par;
	int i = 0;
	int plen;

	const fdt32_t *ac;

	while (1) {

		/* next */
		offs = fdt_next_node(fdt, offs, NULL);
		if (offs < 0)
			break;

		/* compatible */
		if (!(fdt_node_check_compatible(fdt, offs, "baikal,cmu"))) {
			extern struct baikal_clk pclk_list[];

			/* clear */
			pclk_list[i].reg               = 0;
			pclk_list[i].hw_id             = 0;
			pclk_list[i].parent_freq       = 0;
			pclk_list[i].current_freq      = 0;
			pclk_list[i].min               = 0;
			pclk_list[i].max               = 0;
			pclk_list[i].name              = NULL;
			pclk_list[i].is_cpu            = false;
			pclk_list[i].deny_reconfig_pll = false;
			pclk_list[i].cmu_clkch_mshc    = -1;

			/* cmu-id */
			ac = fdt_getprop(fdt, offs, "cmu-id", &plen);
			if (!ac)
				return 2;
			pclk_list[i].reg = fdt32_to_cpu(*ac);
			pclk_list[i].hw_id = fdt32_to_cpu(*ac);

			/* clocks */
			ac = fdt_getprop(fdt, offs, "clocks", &plen);
			if (!ac) {
				pclk_list[i].parent_freq = DEFAULT_PARENT_RATE;
			}
			else {
				offs_par = fdt_node_offset_by_phandle(fdt, fdt32_to_cpu(*ac));
				ac = fdt_getprop(fdt, offs_par, "clock-frequency", &plen);
				if (!ac)
					return 2;
				pclk_list[i].parent_freq = fdt32_to_cpu(*ac);
			}

			/* min */
			ac = fdt_getprop(fdt, offs, "min", &plen);
			if (!ac)
				return 2;
			pclk_list[i].min = fdt32_to_cpu(*ac);

			/* max */
			ac = fdt_getprop(fdt, offs, "max", &plen);
			if (!ac)
				return 2;

			pclk_list[i].max = fdt32_to_cpu(*ac);


			/* !!!!!!!!!!!! Yet Another Fool Protection !!!!!!!!!!!!!! *
			 * Please, NEVER ever set the PLL frequency to more than   *
			 * !!!!!!!!!!!!           2,2GHz            !!!!!!!!!!!!!! */
			if ( pclk_list[i].max > 2200000000 )
				pclk_list[i].max = 2200000000;
			/* !!!!!!!!!!!! Yet Another Fool Protection !!!!!!!!!!!!!! *
			 * Please, NEVER ever set the PLL frequency to more than   *
			 * !!!!!!!!!!!!           2,2GHz            !!!!!!!!!!!!!! */

			/* clock-frequency */
			ac = fdt_getprop(fdt, offs, "clock-frequency", &plen);
			if (!ac)
				pclk_list[i].current_freq = DEFAULT_FREQUENCY;
			else
				pclk_list[i].current_freq = fdt32_to_cpu(*ac);

			/* is_cpu */
			ac = fdt_getprop(fdt, offs, "clock-output-names", &plen);
			if ((fdt_stringlist_contains((void *)ac, plen, "baikal-ca57_cmu0")) ||
			    (fdt_stringlist_contains((void *)ac, plen, "baikal-ca57_cmu1")) ||
			    (fdt_stringlist_contains((void *)ac, plen, "baikal-ca57_cmu2")) ||
			    (fdt_stringlist_contains((void *)ac, plen, "baikal-ca57_cmu3")))
			{
				pclk_list[i].is_cpu = true;
			}

			/* clock-indices */
			ac = fdt_getprop(fdt, offs, "clock-indices", &plen);
			int num_of_chans = plen / 4;
			if (num_of_chans > 1) {
				pclk_list[i].deny_reconfig_pll = true;
			}

			/* clock-output-names */
			pclk_list[i].name = fdt_getprop(fdt, offs, "clock-output-names", &plen);

			/* mshc-channel */
			if (num_of_chans > 1) {
				int idx = fdt_stringlist_search(fdt, offs, "clock-names", "mshc_tx_x2");
				if (idx >= 0){
					const uint32_t *p = fdt_getprop(fdt, offs, "clock-indices", NULL);
					pclk_list[i].cmu_clkch_mshc = fdt32_to_cpu(p[idx]);
				}
			}

			/* print */
			INFO("Device Tree node @ 0x%x: name %s, parent frequency %d, is%s CPU\n", pclk_list[i].reg,
			     pclk_list[i].name, pclk_list[i].parent_freq,
			     (pclk_list[i].is_cpu)? "" : " NOT");

			/* next */
			i++;
		}
	}
	return 0;
}

static int baikal_sip_setup(void)
{
	int ret;
	void *fdt = (void *)(uintptr_t)PLAT_BAIKAL_DT_BASE;

	ret = fdt_open_into(fdt, fdt, PLAT_BAIKAL_DT_MAX_SIZE);
	if (ret < 0) {
		ERROR("Invalid Device Tree at %p: error %d\n", fdt, ret);
		return -1;
	}
	return baikal_get_cmu_descriptors(fdt);
}

#ifndef BE_QEMU
uint64_t baikal_pvt_handler(uint32_t cmd,
				   uint64_t x1,
				   uint64_t x2,
				   uint64_t x3,
				   void *cookie,
				   void *handle,
				   uint64_t flags)
{
	switch (cmd) {
	case PVT_READ:
		return (uint64_t)pvt_read_reg((uint32_t)x1, (uint32_t)x2);
	case PVT_WRITE:
		return (uint64_t)pvt_write_reg((uint32_t)x1, (uint32_t)x2, (uint32_t)x3);
	default:
		ERROR("%s: unhandled SMC (0x%x)\n", __func__, cmd);
		SMC_RET1(handle, SMC_UNK);
	}
	return 0;
}
#endif

uint64_t baikal_plat_sip_handler(uint32_t id,
				   uint64_t cmd,
				   uint64_t x1,
				   uint64_t x2,
				   void *cookie,
				   void *handle,
				   uint64_t flags)
{
	INFO("%s: SMC (0x%llx)\n", __func__, cmd);
	switch (cmd) {

	/* pll */
	case CMU_PLL_SET_RATE:
		return cmu_pll_set_rate(id, x2, x1);
	case CMU_PLL_GET_RATE:
		return cmu_pll_get_rate(id, x2);
	case CMU_PLL_ENABLE:
		return cmu_pll_enable(id);
	case CMU_PLL_DISABLE:
		return cmu_pll_disable(id);
	case CMU_PLL_ROUND_RATE:
		return cmu_pll_round_rate(id, x2, x1);
	case CMU_PLL_IS_ENABLED:
		return cmu_pll_is_enabled(id);

	/* ch */
	case CMU_CLK_CH_SET_RATE:
		return cmu_clk_ch_set_rate(id, (uint32_t)x2, x1);
	case CMU_CLK_CH_GET_RATE:
		return cmu_clk_ch_get_rate(id, (uint32_t)x2);
	case CMU_CLK_CH_ENABLE:
		return cmu_clk_ch_enable(id, (uint32_t)x2);
	case CMU_CLK_CH_DISABLE:
		return cmu_clk_ch_disable(id, (uint32_t)x2);
	case CMU_CLK_CH_ROUND_RATE:
		return cmu_clk_ch_round_rate(id, (uint32_t)x2, x1);
	case CMU_CLK_CH_IS_ENABLED:
		return cmu_clk_ch_is_enabled(id, (uint32_t)x2);

	/* err */
	default:
		ERROR("%s: unhandled SMC (0x%x)\n", __func__, id);
		SMC_RET1(handle, SMC_UNK);
	}

	return 0;
}


/*
 * This function is responsible for handling all SiP calls from the NS world
 */
uint64_t sip_smc_handler(uint32_t smc_fid,
			 uint64_t x1,
			 uint64_t x2,
			 uint64_t x3,
			 uint64_t x4,
			 void *cookie,
			 void *handle,
			 uint64_t flags)
{
	uint64_t ret = 0;

	/* Determine which security state this SMC originated from */
	if (is_caller_secure(flags)) {
		ERROR("%s: SMC secure world's call (0x%x)\n", __func__, smc_fid);
		SMC_RET1(handle, SMC_UNK);
	}

	switch (smc_fid) {
#ifndef BE_QEMU
	case BAIKAL_SMC_PVT_ID:
		ret = baikal_pvt_handler(x1, x2, x3, x4, cookie, handle, flags);
		break;
#endif
	case BAIKAL_SMC_VDEC_SMMU_SET_CACHE:
		ret = mmvdec_smmu_set_domain_cache(x1, x2, x3);
		break;
	case BAIKAL_SMC_VDEC_SMMU_GET_CACHE:
		ret = mmvdec_smmu_get_domain_cache();
		break;
	case BAIKAL_SMC_VDU_UPDATE:
		ret = baikal_vdu_update(x1, x2);
		break;
	case BAIKAL_SMC_SCP_LOG_DISABLE:
		ret = scp_cmd('T', 0, 0);
		break;
	case BAIKAL_SMC_SCP_LOG_ENABLE:
		ret = scp_cmd('t', 0, 0);
		break;
	case BAIKAL_SMC_FLASH_PULL:
	{
		const uint64_t* data = baikal_smc_flash_get_next_data();
		if (data) {
			SMC_RET4(handle, data[0], data[1], data[2], data[3]);
		}

		SMC_RET1(handle, -1);
	}
	case BAIKAL_SMC_FLASH_WRITE:
	case BAIKAL_SMC_FLASH_READ:
	case BAIKAL_SMC_FLASH_ERASE:
	case BAIKAL_SMC_FLASH_PUSH:
	case BAIKAL_SMC_FLASH_POSITION:
		if (baikal_smc_flash_handler(smc_fid, x1, x2, x3, x4) == 0) {
			SMC_RET1(handle, SMC_OK);
		}

		SMC_RET1(handle, -1);
	case BAIKAL_SMC_FLASH_INFO:
	{
		int err;
		const struct flash_sector_info* info;

		err = baikal_smc_flash_handler(smc_fid, x1, x2, x3, x4);
		info = baikal_smc_flash_get_sector_info();

		if (!err && info) {
			SMC_RET2(handle, info->sector_size, info->sector_num);
		}

		SMC_RET1(handle, -1);
	}
	default:
		ret = baikal_plat_sip_handler(x1, x2, x3, x4, cookie, handle, flags);
		break;
	}

	SMC_RET1(handle, ret);
}

/* Define a runtime service descriptor for fast SMC calls */
DECLARE_RT_SVC(
	baikal_sip_svc,
	OEN_SIP_START,
	OEN_SIP_END,
	SMC_TYPE_FAST,
	baikal_sip_setup,
	(rt_svc_handle_t)sip_smc_handler
);
