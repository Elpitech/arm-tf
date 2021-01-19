/*
 * Copyright (c) 2020, Baikal Electronics JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BAIKAL_SIP_SVC_FLASH_H
#define BAIKAL_SIP_SVC_FLASH_H

#include <stdint.h>

struct flash_sector_info {
    uint32_t sector_size;
    uint32_t sector_num;
};

#define BAIKAL_SMC_FLASH		0x82000002
#define BAIKAL_SMC_FLASH_WRITE		(BAIKAL_SMC_FLASH + 0)
#define BAIKAL_SMC_FLASH_READ		(BAIKAL_SMC_FLASH + 1)
#define BAIKAL_SMC_FLASH_ERASE		(BAIKAL_SMC_FLASH + 2)
#define BAIKAL_SMC_FLASH_PUSH		(BAIKAL_SMC_FLASH + 3)
#define BAIKAL_SMC_FLASH_PULL		(BAIKAL_SMC_FLASH + 4)
#define BAIKAL_SMC_FLASH_POSITION	(BAIKAL_SMC_FLASH + 5)
#define BAIKAL_SMC_FLASH_INFO		(BAIKAL_SMC_FLASH + 6)

int baikal_smc_flash_handler(uint32_t smc_fid,
			     u_register_t x1,
			     u_register_t x2,
			     u_register_t x3,
			     u_register_t x4);

const void* baikal_smc_flash_get_next_data(void);
const struct flash_sector_info* baikal_smc_flash_get_sector_info(void);

#endif /* BAIKAL_SIP_SVC_FLASH_H */
