/*
 * Copyright (c) 2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>

	.globl	plat_baikal_calc_core_pos

	/* ---------------------------------------------
	 * unsigned plat_baikal_calc_core_pos(u_register_t mpidr);
	 * ---------------------------------------------
	 */
func plat_baikal_calc_core_pos
	ubfx	x1, x0, #MPIDR_AFF1_SHIFT, #MPIDR_AFFINITY_BITS
	ubfx	x2, x0, #MPIDR_AFF2_SHIFT, #MPIDR_AFFINITY_BITS
	add	x0, x1, x2, LSL #2
	ret
endfunc plat_baikal_calc_core_pos
