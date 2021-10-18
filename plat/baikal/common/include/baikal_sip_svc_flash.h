/*
 * Copyright (c) 2020-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BAIKAL_SIP_SVC_FLASH_H
#define BAIKAL_SIP_SVC_FLASH_H

#include <stdint.h>

#define BAIKAL_SMC_FLASH		0x82000002
#define BAIKAL_SMC_FLASH_WRITE		(BAIKAL_SMC_FLASH + 0)
#define BAIKAL_SMC_FLASH_READ		(BAIKAL_SMC_FLASH + 1)
#define BAIKAL_SMC_FLASH_ERASE		(BAIKAL_SMC_FLASH + 2)
#define BAIKAL_SMC_FLASH_PUSH		(BAIKAL_SMC_FLASH + 3)
#define BAIKAL_SMC_FLASH_PULL		(BAIKAL_SMC_FLASH + 4)
#define BAIKAL_SMC_FLASH_POSITION	(BAIKAL_SMC_FLASH + 5)
#define BAIKAL_SMC_FLASH_INFO		(BAIKAL_SMC_FLASH + 6)

int baikal_smc_flash_handler(const uint32_t smc_fid,
			     const uint64_t x1,
			     const uint64_t x2,
			     const uint64_t x3,
			     const uint64_t x4,
			     uint64_t *data);

#endif /* BAIKAL_SIP_SVC_FLASH_H */
