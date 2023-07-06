/*
 * Copyright (c) 2022, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <lib/mmio.h>

#include "ddr_bs1000.h"

void ddr_io_lcru_write(int port, uint32_t offs, int value)
{
	if (port < 3) {
		mmio_write_32(DDR0_CMU0_BASE +
			DDR_CTRL_BASE_OFF * port + offs, value);
	} else {
		mmio_write_32(DDR3_CMU0_BASE +
			DDR_CTRL_BASE_OFF * (port % 3) + offs, value);
	}
}

void ddr_io_gpr_write(int port, uint32_t offs, int value)
{
	if (port < 3) {
		mmio_write_32(DDR0_GPR_BASE +
			DDR_CTRL_BASE_OFF * port + offs, value);
	} else {
		mmio_write_32(DDR3_GPR_BASE +
			DDR_CTRL_BASE_OFF * (port % 3) + offs, value);
	}
}

void ddr_io_phy_write(int port, uint32_t offs, int value)
{
	if (port < 3) {
		mmio_write_32(DDR0_PHY_BASE +
			DDR_CTRL_BASE_OFF * port + offs, value);
	} else {
		mmio_write_32(DDR3_PHY_BASE +
			DDR_CTRL_BASE_OFF * (port % 3) + offs, value);
	}
}

void ddr_io_ctrl_write(int port, uint32_t offs, int value)
{
	if (port < 3) {
		mmio_write_32(DDR0_CTRL_BASE +
			DDR_CTRL_BASE_OFF * port + offs, value);
	} else {
		mmio_write_32(DDR3_CTRL_BASE +
			DDR_CTRL_BASE_OFF * (port % 3) + offs, value);
	}
}

uint32_t ddr_io_lcru_read(int port, uint32_t offs)
{
	if (port < 3) {
		return mmio_read_32(DDR0_CMU0_BASE +
			DDR_CTRL_BASE_OFF * port + offs);
	} else {
		return mmio_read_32(DDR3_CMU0_BASE +
			DDR_CTRL_BASE_OFF * (port % 3) + offs);
	}
}

uint32_t ddr_io_gpr_read(int port, uint32_t offs)
{
	if (port < 3) {
		return mmio_read_32(DDR0_GPR_BASE +
			DDR_CTRL_BASE_OFF * port + offs);
	} else {
		return mmio_read_32(DDR3_GPR_BASE +
			DDR_CTRL_BASE_OFF * (port % 3) + offs);
	}
}

uint32_t ddr_io_phy_read(int port, uint32_t offs)
{
	if (port < 3) {
		return mmio_read_32(DDR0_PHY_BASE +
			DDR_CTRL_BASE_OFF * port + offs);
	} else {
		return mmio_read_32(DDR3_PHY_BASE +
			DDR_CTRL_BASE_OFF * (port % 3) + offs);
	}
}

uint32_t ddr_io_ctrl_read(int port, uint32_t offs)
{
	if (port < 3) {
		return mmio_read_32(DDR0_CTRL_BASE +
			DDR_CTRL_BASE_OFF * port + offs);
	} else {
		return mmio_read_32(DDR3_CTRL_BASE +
			DDR_CTRL_BASE_OFF * (port % 3) + offs);
	}
}
