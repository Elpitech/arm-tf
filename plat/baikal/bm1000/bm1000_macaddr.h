/*
 * Copyright (c) 2020, Baikal Electronics JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __BAIKAL_MACADDR_H
#define __BAIKAL_MACADDR_H

typedef enum {
    BAIKAL_MACADDR_GMAC0,
    BAIKAL_MACADDR_GMAC1,
    BAIKAL_MACADDR_XGMAC0,
    BAIKAL_MACADDR_XGMAC1
} baikal_macaddr_t;

int baikal_macaddr_get (const baikal_macaddr_t gmac_idx, uint8_t *macaddr);

#endif // __BAIKAL_MACADDR_H
