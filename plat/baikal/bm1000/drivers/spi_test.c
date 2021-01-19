/*
 * Copyright (c) 2018-2020, Baikal Electronics JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <lib/utils_def.h>
#include <platform_def.h>
#include <spi_dw.h>

#define ADR (9*1024*1024)
uint8_t buf[512];

int compare_template (const void *s11, uint8_t template, size_t count)
{
    const uint8_t *s1 = s11;
    while (count-- > 0) {
        if (*s1++ != template){
            return -1;
        }
    }
    return 0;
}

void setmem_template (void *s11, uint8_t template, size_t count)
{
    uint8_t *s = s11;
    while (count-- > 0) {
        *s++ = template;
    }
}

int dw_spi_test (int p, int l)
{
    int ret = 0;

    // ERASE
    #if 1
        if (dw_spi_erase(p,l,ADR,SECTOR_SIZE,SECTOR_SIZE))
            --ret;
        if (dw_spi_read (p,l,ADR,buf,sizeof(buf)))
            --ret;
        if (compare_template(buf,0xff,sizeof(buf)))
            --ret;
    #endif

    // WRITE
    #if 1
        int data = 0xD1;
        if (dw_spi_erase(p,l,ADR,SECTOR_SIZE,SECTOR_SIZE))
            --ret;
        setmem_template(buf,data,sizeof(buf));
        if (dw_spi_write (p,l,ADR,buf,sizeof(buf)))
            --ret;
        setmem_template(buf,0x00,sizeof(buf));
        if (dw_spi_read (p,l,ADR,buf,sizeof(buf)))
            --ret;
        if (compare_template(buf,data,sizeof(buf)))
            --ret;
    #endif

    return ret;
}
