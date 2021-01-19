/*
 * Copyright (c) 2020, Baikal Electronics JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <baikal_console.h>
#include <common/debug.h>
#include <drivers/console.h>
#include <drivers/ti/uart/uart_16550.h>
#include <platform_def.h>

static console_t baikal_boot_console;

void baikal_console_boot_end(void)
{
	(void)console_flush();
	(void)console_unregister(&baikal_boot_console);
}

void baikal_console_boot_init(void)
{
	int rc;

	rc = console_16550_register(PLAT_BAIKAL_BOOT_UART_BASE,
				    0,
				    PLAT_BAIKAL_CONSOLE_BAUDRATE,
				    &baikal_boot_console);

	if (rc == 0) {
		/*
		 * The crash console doesn't use the multi console API, it uses
		 * the core console functions directly. It is safe to call panic
		 * and let it print debug information.
		 */
		panic();
	}
#if DEBUG
	console_set_scope(&baikal_boot_console,	CONSOLE_FLAG_BOOT  |
						CONSOLE_FLAG_CRASH |
						CONSOLE_FLAG_RUNTIME);
#endif
}
