/*
 * Copyright (c) 2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <baikal_def.h>
#include <baikal_gpio32.h>
#include <lib/mmio.h>

#define GPIO32_OUT	(BAIKAL_GPIO32_BASE + 0x00)
#define GPIO32_DIR	(BAIKAL_GPIO32_BASE + 0x04)

void gpio32_dir_clr(const unsigned pin)
{
	assert(pin < 32);

	mmio_clrbits_32(GPIO32_DIR, 1 << pin);
}

void gpio32_dir_set(const unsigned pin)
{
	assert(pin < 32);

	mmio_setbits_32(GPIO32_DIR, 1 << pin);
}

void gpio32_out_rst(const unsigned pin)
{
	assert(pin < 32);

	mmio_clrbits_32(GPIO32_OUT, 1 << pin);
}

void gpio32_out_set(const unsigned pin)
{
	assert(pin < 32);

	mmio_setbits_32(GPIO32_OUT, 1 << pin);
}
