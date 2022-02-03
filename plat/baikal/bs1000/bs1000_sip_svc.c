/*
 * Copyright (c) 2020-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <common/runtime_svc.h>

#include <baikal_sip_svc.h>
#include <bs1000_cmu.h>


uintptr_t sip_smc_handler(int smc_fid,
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
	case BAIKAL_SMC_FLASH_WRITE:
	case BAIKAL_SMC_FLASH_READ:
	case BAIKAL_SMC_FLASH_ERASE:
	case BAIKAL_SMC_FLASH_PULL:
	case BAIKAL_SMC_FLASH_PUSH:
	case BAIKAL_SMC_FLASH_POSITION:
	case BAIKAL_SMC_FLASH_INFO:
		ret = baikal_smc_flash_handler(smc_fid, x1, x2, x3, x4, data);
		if (ret) {
			break;
		} else if (smc_fid == BAIKAL_SMC_FLASH_PULL) {
			SMC_RET4(handle, data[0], data[1], data[2], data[3]);
		} else if (smc_fid == BAIKAL_SMC_FLASH_INFO) {
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
	(rt_svc_handle_t)sip_smc_handler
);
