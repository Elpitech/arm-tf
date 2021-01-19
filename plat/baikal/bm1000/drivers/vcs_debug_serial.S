
#include <arch.h>
#include <asm_macros.S>

#include "../../../drivers/console/aarch64/console.S"

	.globl	console_core_init
	.globl	console_core_putc
	.globl	console_core_getc
	.globl	console_core_flush

	/* -----------------------------------------------
	 * int console_core_init(unsigned long base_addr,
	 * unsigned int uart_clk, unsigned int baud_rate)
	 * Function to initialize the console without a
	 * C Runtime to print debug information. This
	 * function will be accessed by console_init and
	 * crash reporting.
	 * In: x0 - console base address
	 *     w1 - Uart clock in Hz
	 *     w2 - Baud rate
	 * Out: return 1 on success
	 * Clobber list : x1, x2, x3
	 * -----------------------------------------------
	 */
func console_core_init
	ret
endfunc console_core_init

	/* --------------------------------------------------------
	 * int console_core_putc(int c, unsigned int base_addr)
	 * Function to output a character over the console. It
	 * returns the character printed on success or -1 on error.
	 * In : w0 - character to be printed
	 *      x1 - console base address
	 * Out : return -1 on error else return character.
	 * Clobber list : x2
	 * --------------------------------------------------------
	 */
func console_core_putc
	mov	x1, #0xffec
#ifdef	BE_VCS_TIMESTAMP_ON_LF
	cmp	w0, #10
	bne	1f
	mov	x2, 32
	strb	w2, [X1]
	mov	x2, #':
	strb	w2, [X1]
	mov	x2, 32
	strb	w2, [X1]
	mov	x2, 7
	strb	w2, [X1]
1:
#endif
	strb	w0, [x1]
	ret
endfunc console_core_putc

	/* ---------------------------------------------
	 * int console_core_getc(void)
	 * Function to get a character from the console.
	 * It returns the character grabbed on success
	 * or -1 on error.
	 * In : w0 - console base address
	 * Out : return -1 on error else return character.
	 * Clobber list : x0, x1
	 * ---------------------------------------------
	 */
func console_core_getc
	mov	w0, #-1
	ret
endfunc console_core_getc

	/* ---------------------------------------------
	 * int console_core_flush(uintptr_t base_addr)
	 * Function to force a write of all buffered
	 * data that hasn't been output.
	 * In : x0 - console base address
	 * Out : return -1 on error else return 0.
	 * Clobber list : x0, x1
	 * ---------------------------------------------
	 */
func console_core_flush
	/* Placeholder */
	mov	w0, #0
	ret
endfunc console_core_flush
