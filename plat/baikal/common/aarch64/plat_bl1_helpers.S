/*
 * Copyright (c) 2018-2020, Baikal Electronics JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>
#include <assert_macros.S>
#include <platform_def.h>

	.globl	plat_crash_console_init
	.globl	plat_report_exception
	.globl	plat_reset_handler
	.globl	platform_mem_init

.macro	log_cout msg
	adr	x4, info_str
	bl	asm_print_str
	adr	x4, \msg + 1
	bl	asm_print_str
.endm

#ifdef BE_HAPS
func tzc_init
	mov	x9, x30
	/* Initialize TZCs in transparent mode */
	mov	x0, 0x122
	lsl	x0, x0, 20	// 0x1220_0000 TZC0 base
	mov	x1, 0x322
	lsl	x1, x1, 20	// 0x3220_0000 TZC1 base
	mov	w2, 0xf		// value for GateKeeper 0x8
	str	w2, [x0, 8]
	str	w2, [x1, 8]
	mov	w2, 0xc000000f	// value for REGION_ATTRIBUTES_0
	str	w2, [x0, 0x110]
	str	w2, [x1, 0x110]
	mov	w2, -1		// 0xffff_ffff value for REGION_ID_ACCESS_0
	str	w2, [x0, 0x114]
	str	w2, [x1, 0x114]
	log_cout s_tzc
	mov	x30, x9
	ret
endfunc tzc_init
#endif

func memtest
	mov	x9, x30
	log_cout s_memtest
	mov	x30, x9
	ret
endfunc memtest

func platform_mem_init
	mov	x8, x30
	log_cout s_mem
#ifdef BE_HAPS
	bl	tzc_init
#endif
	mov	x30, x8
	ret
endfunc platform_mem_init

	/* ---------------------------------------------
	 * void plat_report_exception(unsigned int type)
	 * Function to report an unhandled exception
	 * with platform-specific means.
	 * eg. On HIKEY platform, it updates the LEDs
	 * to indicate where we are
	 * ---------------------------------------------
	 */
func plat_report_exception
	mov	x8, x30
//	mov	x4, x3
//	bl	asm_print_hex

	/*TODO: Turn on LED according to x0 (0 -- f) ! What led do we have via GPIO?*/
	mrs	x2, currentel
	and	x2, x2, #0xc0
	/* Check EL1 */
	cmp	x2, #0x04
	beq	plat_report_el1

	adr	x4, plat_err_str
	bl	asm_print_str

	adr	x4, esr_el3_str
	bl	asm_print_str

	mrs	x4, esr_el3
	bl	asm_print_hex

	adr	x4, elr_el3_str
	bl	asm_print_str

	mrs	x4, elr_el3
	bl	asm_print_hex
	b	plat_report_end

plat_report_el1:
	adr	x4, plat_err_str
	bl	asm_print_str

	adr	x4, esr_el1_str
	bl	asm_print_str

	mrs	x4, esr_el1
	bl	asm_print_hex

	adr	x4, elr_el1_str
	bl	asm_print_str

	mrs	x4, elr_el1
	bl	asm_print_hex
plat_report_end:
	mov	x30, x8
	ret
endfunc plat_report_exception

func init_gpr
// init GPR...
	mov	x0, xzr
	mov	x1, xzr
	mov	x2, xzr
	mov	x3, xzr
	mov	x4, xzr
	mov	x5, xzr
	mov	x6, xzr
	mov	x7, xzr
//	mov	x8, xzr
	mov	x9, xzr
	mov	x10, xzr
	mov	x11, xzr
	mov	x12, xzr
	mov	x13, xzr
	mov	x14, xzr
	mov	x15, xzr
	mov	x16, xzr
	mov	x17, xzr
	mov	x18, xzr
//	mov	x19, xzr
	mov	x20, xzr
	mov	x21, xzr
	mov	x22, xzr
	mov	x23, xzr
	mov	x24, xzr
	mov	x25, xzr
	mov	x26, xzr
	mov	x27, xzr
	mov	x28, xzr
	mov	x29, xzr
	ret
endfunc	init_gpr

/* -----------------------------------------------------
 * void plat_reset_handler(void);
 * -----------------------------------------------------
 */

func plat_reset_handler
	mov	x8, x30
#ifdef BE_HAPS
	mov	x16, #0xff00	// COMMON_AREA
	mov	w1, 'R'
	str	x1, [x16, 8]
	str	x1, [x16, 16]
#endif
// A57 SMPEN - it is done (in A57 errata update)
//	mrs	x0, s3_1_c15_c2_1
//	orr	x0, x0, #(0x1 << 6)
//	msr	s3_1_c15_c2_1, x0

	bl	plat_is_my_cpu_primary
	cbz	w0, 1f

// early initialize vectors to catch errors
	adr	x0, bl1_exceptions
	msr	vbar_el3, x0
	isb

	bl	init_gpr
// TODO: AVLSP initialize(pll, resets? it is in SCP done)
// Initialize console
	bl	plat_crash_console_init
	log_cout s_hello

1:
	mov	x30, x8
	ret
endfunc plat_reset_handler

.section .rodata.rev_err_str, "aS"

.macro log_info tag, code, msg
\tag:
// code for I2C display
	.byte	\code
// message to UART
	.asciz	"\msg"
.endm

	log_info s_hello  , 21, "UART ok\n"
	log_info s_tzc    , 23, "TZC transparent\n"
	log_info s_mem    , 25, "Memory init...\n"

info_str:
	.asciz	"NOTICE:  "
plat_err_str:
	.asciz "\nPlatform exception reporting:"
esr_el3_str:
	.asciz "\nESR_EL3: "
elr_el3_str:
	.asciz "\nELR_EL3: "
esr_el1_str:
	.asciz "\nESR_EL1: "
elr_el1_str:
	.asciz "\nELR_EL1: "
