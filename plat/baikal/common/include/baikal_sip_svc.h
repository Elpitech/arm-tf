/*
 * Copyright (c) 2020-2022, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BAIKAL_SIP_SVC_H
#define BAIKAL_SIP_SVC_H

#include <stdint.h>

#define BAIKAL_SMC_LCRU_ID		0x82000000
#define BAIKAL_SMC_PVT_ID		(BAIKAL_SMC_LCRU_ID + 0x001)
#define BAIKAL_SMC_FLASH		(BAIKAL_SMC_LCRU_ID + 0x002)
#define BAIKAL_SMC_VDU_UPDATE		(BAIKAL_SMC_LCRU_ID + 0x100)
#define BAIKAL_SMC_SCP_LOG_DISABLE	(BAIKAL_SMC_LCRU_ID + 0x200)
#define BAIKAL_SMC_SCP_LOG_ENABLE	(BAIKAL_SMC_LCRU_ID + 0x201)
#define BAIKAL_SMC_VDEC_SMMU_SET_CACHE	(BAIKAL_SMC_LCRU_ID + 0x300)
#define BAIKAL_SMC_VDEC_SMMU_GET_CACHE	(BAIKAL_SMC_LCRU_ID + 0x301)
#define BAIKAL_SMC_CLK			(BAIKAL_SMC_LCRU_ID + 0x400)

#define BAIKAL_SMC_CLK_ROUND		(BAIKAL_SMC_CLK + 0)
#define BAIKAL_SMC_CLK_SET		(BAIKAL_SMC_CLK + 1)
#define BAIKAL_SMC_CLK_GET		(BAIKAL_SMC_CLK + 2)
#define BAIKAL_SMC_CLK_ENABLE		(BAIKAL_SMC_CLK + 3)
#define BAIKAL_SMC_CLK_DISABLE		(BAIKAL_SMC_CLK + 4)
#define BAIKAL_SMC_CLK_IS_ENABLED	(BAIKAL_SMC_CLK + 5)

#define BAIKAL_SMC_FLASH_WRITE		(BAIKAL_SMC_FLASH + 0)
#define BAIKAL_SMC_FLASH_READ		(BAIKAL_SMC_FLASH + 1)
#define BAIKAL_SMC_FLASH_ERASE		(BAIKAL_SMC_FLASH + 2)
#define BAIKAL_SMC_FLASH_PUSH		(BAIKAL_SMC_FLASH + 3)
#define BAIKAL_SMC_FLASH_PULL		(BAIKAL_SMC_FLASH + 4)
#define BAIKAL_SMC_FLASH_POSITION	(BAIKAL_SMC_FLASH + 5)
#define BAIKAL_SMC_FLASH_INIT		(BAIKAL_SMC_FLASH + 6)
#define BAIKAL_SMC_FLASH_LOCK		(BAIKAL_SMC_FLASH + 7)

#define BAIKAL_SMC_GMAC			(BAIKAL_SMC_LCRU_ID + 0x500)
#define BAIKAL_SMC_GMAC_DIV2_ENABLE	(BAIKAL_SMC_GMAC + 0)
#define BAIKAL_SMC_GMAC_DIV2_DISABLE	(BAIKAL_SMC_GMAC + 1)

#define BAIKAL_SMC_LSP_MUX		(BAIKAL_SMC_LCRU_ID + 0x600)

int64_t baikal_smc_flash_handler(const uint32_t smc,
			     const uint64_t x1,
			     const uint64_t x2,
			     const uint64_t x3,
			     const uint64_t x4,
			     uint64_t *data);

int64_t baikal_smc_clk_handler  (const uint32_t smc,
			     const uint64_t x1,
			     const uint64_t x2,
			     const uint64_t x3,
			     const uint64_t x4);

int64_t baikal_smc_gmac_handler(const uint32_t smc,
			     const uint64_t x1,
			     const uint64_t x2,
			     const uint64_t x3,
			     const uint64_t x4);

#endif /* BAIKAL_SIP_SVC_H */
