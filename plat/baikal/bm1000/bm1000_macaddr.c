/*
 * Copyright (c) 2020, Baikal Electronics JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <crc32.h>
#include <spi_dw.h>
#include <stdint.h>
#include "bm1000_macaddr.h"

#define SPI_FLASH_BOARD_IDS_ADDR (4 * 65536 - 4 * 1024) // BL1 reserved area

#pragma pack(1)

typedef struct {
    uint8_t  macaddrs[4][6];
    uint32_t crc32;
} spi_flash_board_ids_t;

#pragma pack()

int baikal_macaddr_get (const baikal_macaddr_t gmac_idx, uint8_t *macaddr) {
    spi_flash_board_ids_t ids;

#ifdef BAIKAL_HARDCODED_MACADDRS
    static const hardcoded_macaddrs[4][6] = {
        {0x4c, 0xa5, 0x15, 0xc0, 0xde, 0xd0},
        {0x4c, 0xa5, 0x15, 0xc0, 0xde, 0xd1},
        {0x4c, 0xa5, 0x15, 0xc0, 0xde, 0xd2},
        {0x4c, 0xa5, 0x15, 0xc0, 0xde, 0xd3}
    };

    memcpy(macaddr, hardcoded_macaddrs[gmac_idx], sizeof hardcoded_macaddrs[0]);
    return 0;
#endif

    dw_spi_read(0, 0, SPI_FLASH_BOARD_IDS_ADDR, &ids, sizeof ids);

    if (ids.crc32 == crc32(&ids, sizeof ids - sizeof ids.crc32)) {
        memcpy(macaddr, ids.macaddrs[gmac_idx], sizeof ids.macaddrs[0]);
        return 0;
    }

    return -1;
}
