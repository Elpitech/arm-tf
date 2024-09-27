/*
 * Copyright (c) 2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef VCS_H
#define VCS_H

#ifndef __ASSEMBLER__

#include <stdint.h>

/*
 * Initialize a new VCS console instance and register it with the console
 * framework. The |console| pointer must point to storage that will be valid
 * for the lifetime of the console, such as a global or static local variable.
 * Its contents will be reinitialized from scratch.
 */
int console_vcs_register(uintptr_t baseaddr, uint32_t clock, uint32_t baud,
			 console_t *console);

#endif /*__ASSEMBLER__*/

#endif /* VCS_H */
