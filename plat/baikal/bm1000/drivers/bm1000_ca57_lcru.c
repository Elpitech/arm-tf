/*
 * Copyright (c) 2018-2020, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <bm1000_ca57_lcru.h>
#include <bm1000_cmu.h>
#include <common/debug.h>
#include <lib/mmio.h>
#include <platform_def.h>

#define DRIVER_NAME "bm1000_ca57_lcru"
#define SPI_DW_BOOT_LOWLEVEL_DEBUG 0

static const uint32_t be_ca57_lcru[] = {
    CA57_LCRU_0,
    CA57_LCRU_1,
    CA57_LCRU_2,
    CA57_LCRU_3,
};

int be_ca57_lcru_enable_core(u_register_t mpidr)
{
    int ret;

    uint8_t cluster = (mpidr >> 8);
    uint8_t core = mpidr & 0x3;

    uint32_t lcru_base = be_ca57_lcru[cluster];
    uint32_t reg;

    if (!((mmio_read_32(lcru_base + BK_CA57_RST_REG) & (RST_CORERESET | RST_CPUORESET) << core))) {
        INFO("Core %lx already enabled \n", mpidr);
        return 0;
    }

    if (!(mmio_read_32(lcru_base + BK_CA57_PLL_REG_CTL) & PLL_CTL_EN)) {
        ret = cmu_pll_enable(lcru_base);
        if (ret)
            return 1;
    }

   // if (!(mmio_read_32(lcru_base + BK_CA57_PLL_REG_CTL6) & PLL_CTL_EN)) {
   //     ret = be_ca57_lcru_enable_clock(cluster);
   //     if (ret)
   //         return 1;
   // }

    reg = mmio_read_32(lcru_base + BK_CA57_RST_REG);
    reg &= ~(RST_L2RESET | RST_BRDGRST | RST_BRDGRST2 | RST_BRDGRST3 | RST_BRDGRST4 | RST_PRESETDBG | RST_GICRST);
    mmio_write_32(lcru_base + BK_CA57_RST_REG, reg);
    reg &= ~((RST_CORERESET | RST_CPUORESET) << core);
    mmio_write_32(lcru_base + BK_CA57_RST_REG, reg);

    if (!((mmio_read_32(lcru_base + BK_CA57_RST_REG) & (RST_CORERESET | RST_CPUORESET) << core)))
        return 0;
    else {
        WARN("Failed to enable core %lx \n", mpidr);
        return 1;
    }
}
