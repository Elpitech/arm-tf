/*
 * Copyright (c) 2018-2020, Baikal Electronics JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>
#include <assert_macros.S>
#include <platform_def.h>

	.globl	plat_baikal_calc_core_pos
	.globl	plat_crash_console_flush
	.globl	plat_crash_console_init
	.globl	plat_crash_console_putc
	.globl	plat_get_my_entrypoint
	.globl	plat_is_my_cpu_primary
	.globl	plat_my_core_pos
	.globl	plat_secondary_cold_boot_setup

/*
 *  unsigned int plat_baikal_calc_core_pos(u_register_t mpidr);
 *  With this function: CorePos = (ClusterId * 2) + CoreId
 */
func plat_baikal_calc_core_pos
	and	x1, x0, #MPIDR_CPU_MASK
	and	x0, x0, #MPIDR_CLUSTER_MASK
	add	x0, x1, x0, LSR #7
	ret
endfunc plat_baikal_calc_core_pos

	/* ---------------------------------------------
	 * int plat_crash_console_flush()
	 * Function to force a write of all buffered
	 * data that hasn't been output.
	 * Out : return -1 on error else return 0.
	 * Clobber list : x0, x1
	 * ---------------------------------------------
	 */
func plat_crash_console_flush
	mov_imm	x0, PLAT_BAIKAL_CRASH_UART_BASE
	b	console_16550_core_flush
endfunc plat_crash_console_flush

        /* ---------------------------------------------
         * int plat_crash_console_init(void)
         * Function to initialize the crash console
         * without a C Runtime to print crash report.
         * Clobber list : x0, x1, x2
         * ---------------------------------------------
         */
func plat_crash_console_init
	mov_imm x0, PLAT_BAIKAL_CRASH_UART_BASE
	mov_imm x1, PLAT_BAIKAL_CRASH_UART_CLK_IN_HZ
	mov_imm x2, PLAT_BAIKAL_CONSOLE_BAUDRATE
endfunc plat_crash_console_init

	/* ---------------------------------------------
	 * int plat_crash_console_putc(int c)
	 * Function to print a character on the crash
	 * console without a C Runtime.
	 * Clobber list : x1, x2
	 * ---------------------------------------------
	 */
func plat_crash_console_putc
	mov_imm x1, PLAT_BAIKAL_CRASH_UART_BASE
	b       console_16550_core_putc
endfunc plat_crash_console_putc

func plat_get_my_entrypoint
	/* TODO support warm boot */
	mov	x0, #0
	ret
endfunc plat_get_my_entrypoint

	/* -----------------------------------------------------
	 * unsigned int plat_is_my_cpu_primary (void);
	 *
	 * Find out whether the current cpu is the primary
	 * cpu.
	 * -----------------------------------------------------
	 */
func plat_is_my_cpu_primary
	mrs	x0, mpidr_el1
	and	x0, x0, #(MPIDR_CLUSTER_MASK | MPIDR_CPU_MASK)
	cmp	x0, #BAIKAL_PRIMARY_CPU
	cset	w0, eq
	ret
endfunc plat_is_my_cpu_primary

func plat_my_core_pos
	mrs	x0, mpidr_el1
	b	plat_baikal_calc_core_pos
endfunc plat_my_core_pos

	/* -----------------------------------------------------
	 * void plat_secondary_cold_boot_setup (void);
	 *
	 * This function performs any platform specific actions
	 * needed for a secondary cpu after a cold reset e.g
	 * mark the cpu's presence, mechanism to place it in a
	 * holding pen etc.
	 * -----------------------------------------------------
	 */
func plat_secondary_cold_boot_setup
	/* Calculate address of our hold entry */
	bl	plat_my_core_pos
	mov_imm	x2, PLAT_BAIKAL_HOLD_BASE

	/* Wait until we have a go */
poll_mailbox:
	ldr	x1, [x2, x0, LSL #3]
	cbz	x1, 1f
	mov_imm	x0, PLAT_BAIKAL_TRUSTED_MAILBOX_BASE
	ldr	x1, [x0]
	br	x1
1:
	wfe
	b	poll_mailbox
endfunc plat_secondary_cold_boot_setup
