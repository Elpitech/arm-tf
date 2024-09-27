/*
 * Copyright (c) 2020-2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BAIKAL_SCP_H
#define BAIKAL_SCP_H

#include <stdint.h>

void *scp_buf(void);
int scp_cmd(uint8_t op, uint32_t arg0, uint32_t arg1);

#endif /* BAIKAL_SCP_H */
