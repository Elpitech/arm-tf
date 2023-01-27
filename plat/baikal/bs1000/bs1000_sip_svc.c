/*
 * Copyright (c) 2020-2022, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <common/runtime_svc.h>
#include <baikal_pvt.h>
#include <baikal_sip_svc.h>
#include <bs1000_cmu.h>
#include <bs1000_def.h>
#include <bs1000_scp_lcru.h>

static uint64_t baikal_pvt_handler(uint32_t cmd,
				   uint64_t x1,
				   uint64_t x2,
				   uint64_t x3)
{
	if (cmd == PVT_READ) {
		return (uint64_t)pvt_read_reg((uint32_t)x1, (uint32_t)x2);
	} else if (cmd == PVT_WRITE) {
		return (uint64_t)pvt_write_reg((uint32_t)x1, (uint32_t)x2, (uint32_t)x3);
	}

	ERROR("%s: unhandled SMC 0x%x\n", __func__, cmd);
	return SMC_UNK;
}

static uint64_t baikal_lsp_mux_handler(uint64_t mode)
{
	uint32_t val;
	int err;

	err = scp_lcru_read(SCP_GPR_LSP_CTL, &val);
	if (err)
		return err;

	val &= ~SCP_GPR_LSP_CTL_SEL_PERIPH_MASK;
	val |= (uint32_t)mode << SCP_GPR_LSP_CTL_SEL_PERIPH_SHIFT;
	err = scp_lcru_write(SCP_GPR_LSP_CTL, val);

	return err;
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
	uint64_t ret = SMC_UNK;
	uint64_t data[4];

	if (is_caller_secure(flags)) {
		ERROR("%s: SMC secure world's call (0x%x)\n", __func__, smc_fid);
		SMC_RET1(handle, SMC_UNK);
	}

	switch (smc_fid) {
	case BAIKAL_SMC_PVT_ID:
		ret = baikal_pvt_handler(x1, x2, x3, x4);
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

	case BAIKAL_SMC_CLK_ROUND:
	case BAIKAL_SMC_CLK_SET:
	case BAIKAL_SMC_CLK_GET:
	case BAIKAL_SMC_CLK_ENABLE:
	case BAIKAL_SMC_CLK_DISABLE:
	case BAIKAL_SMC_CLK_IS_ENABLED:
		ret = baikal_smc_clk_handler(smc_fid, x1, x2, x3, x4);
		break;

	case BAIKAL_SMC_GMAC_DIV2_ENABLE:
	case BAIKAL_SMC_GMAC_DIV2_DISABLE:
		ret = baikal_smc_gmac_handler(smc_fid, x1, x2, x3, x4);
		break;

	case BAIKAL_SMC_LSP_MUX:
		ret = baikal_lsp_mux_handler(x1);
		break;

	default:
		ERROR("%s: unhandled SMC (0x%x)\n", __func__, smc_fid);
		SMC_RET1(handle, SMC_UNK);
	}

	SMC_RET1(handle, ret);
}

DECLARE_RT_SVC(
	baikal_sip_svc,
	OEN_SIP_START,
	OEN_SIP_END,
	SMC_TYPE_FAST,
	cmu_desc_init,
	sip_smc_handler
);
