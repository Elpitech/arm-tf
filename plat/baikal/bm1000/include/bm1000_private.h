/*
 * Copyright (c) 2018-2020, Baikal Electronics JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __BAIKAL_PRIVATE_H
#define __BAIKAL_PRIVATE_H

#include <stdint.h>

// Bit general
#define CTL_BIT(n) (1u << (n))
#define CTL_BIT_MASK(u,l) (((1 << ((u) - (l) + 1)) - 1) << (l))
#define CTL_BIT_SET(x, u, l) ((x << l) & CTL_BIT_MASK(u,l))
#define CTL_BIT_GET(x, u, l) (((x) & CTL_BIT_MASK(u,l)) >> (l))

// LCRU general
#define LCRU_CMU0	0
#define LCRU_CMU1	0x10000
#define LCRU_CMU2	0x20000
#define LCRU_GPI	0x30000
#define LCRU_SREG	0x40000
#define LCRU_GPR	0x50000
#define LCRU_PVT	0x60000
#define LCRU_AXI_o	0x70000

#define LCRU_CTL0	0
#define LCRU_CTL0_LOCK					CTL_BIT(31)
#define LCRU_CTL0_BYPASS				CTL_BIT(30)
#define LCRU_CTL0_BYPASS_NLK_ENB			CTL_BIT(29)
#define LCRU_CTL0_CLKOD_SHIFT				(14)
#define LCRU_CTL0_CLKOD_MASK				CTL_BIT_MASK(24, 14)
#define LCRU_CTL0_CLKOD_SET(x)				CTL_BIT_SET(x, 24, 14)
#define LCRU_CTL0_CLKR_MASK				CTL_BIT_MASK(13, 2)
#define LCRU_CTL0_CLKR_SET(x)				CTL_BIT_SET(x, 13, 2)
#define LCRU_CTL0_RST					CTL_BIT(1)
#define LCRU_CTL0_CTRL_EN				CTL_BIT(0)

#define LCRU_CTL1	4
#define LCRU_CTL2	8

#define LCRU_CTL3	0xC

#define LCRU_CTL3_NOFNACC				CTL_BIT(7)
#define LCRU_CTL3_LOWFNACC				CTL_BIT(6)
#define LCRU_CTL3_TEST_SEL_MASK				CTL_BIT_MASK(5, 3)
#define LCRU_CTL3_TEST_SEL_SET(x)			CTL_BIT_SET(x, 5, 3)
#define LCRU_CTL3_CLKHD_SET(x)				CTL_BIT_SET(x, 2, 1)
#define LCRU_CTL3_TEST					CTL_BIT(0)

#define LCRU_CTL4	0x10

#define LCRU_CTL4_FBSEL					CTL_BIT(31)
#define LCRU_CTL4_LOCK_SEL_SET(x)			CTL_BIT_SET(x, 30,27)
#define LCRU_CTL4_LOCK_TYPE				CTL_BIT(26)
#define LCRU_CTL4_IBW_NLK_ENB				CTL_BIT(25)
#define LCRU_CTL4_CONSEN				CTL_BIT(24)
#define LCRU_CTL4_IIGAIN_LGMLT_MASK			CTL_BIT_MASK(  23, 18)
#define LCRU_CTL4_IIGAIN_LGMLT_SET(x)			CTL_BIT_SET(x, 23, 18)
#define LCRU_CTL4_IPGAIN_LGMLT_MASK			CTL_BIT_MASK(  17, 12)
#define LCRU_CTL4_IPGAIN_LGMLT_SET(x)			CTL_BIT_SET(x, 17, 12)
#define LCRU_CTL4_IGAIN_LGMLT_MASK			CTL_BIT_MASK(  11,6)
#define LCRU_CTL4_IGAIN_LGMLT_SET(x)			CTL_BIT_SET(x, 11,6)
#define LCRU_CTL4_PGAIN_LGMLT_MASK			CTL_BIT_MASK(   5,0)
#define LCRU_CTL4_PGAIN_LGMLT_SET(x)			CTL_BIT_SET(x,  5,0)

#define LCRU_CTL5	0x14
#define LCRU_CTL5_SCAN_OUTSEL_VCAL			CTL_BIT(16)
#define LCRU_CTL5_SCAN_OUTSEL_FCW			CTL_BIT(15)
#define LCRU_CTL5_SCAN_INSEL_FCW			CTL_BIT(14)
#define LCRU_CTL5_SCAN_EN				CTL_BIT(13)
#define LCRU_CTL5_SCAN_DOUT				CTL_BIT(12)
#define LCRU_CTL5_SCAN_DIN				CTL_BIT(11)
#define LCRU_CTL5_SCAN_CLK				CTL_BIT(10)
#define LCRU_CTL5_VCOCAL_EN				CTL_BIT(9)
#define LCRU_CTL5_VCOCAL_OFFBYP				CTL_BIT(8)
#define LCRU_CTL5_VCOCAL_OFF_SET(x)			CTL_BIT_SET(x, 7,3)
#define LCRU_CTL5_VCOCAL_RDIV_LGFCT_SET(x)		CTL_BIT_SET(x, 2,0)

#define LCRU_CTL6	0x18
#define LCRU_CTL6_SWRST					CTL_BIT(1)
#define LCRU_CTL6_SWEN					CTL_BIT(0)

#define LCRU_CLKCH_BASE	0x20
#define LCRU_CLKCH_STEP	0x10
#define LCRU_CLKCH_OFFSET(x)	(LCRU_CLKCH_BASE + (LCRU_CLKCH_STEP * x))

#define LCRU_CLKCH_LOCK_CLKDIV				CTL_BIT(31)
#define LCRU_CLKCH_CLK_RDY				CTL_BIT(30)
#define LCRU_CLKCH_VAL_CLKDIV_MASK			CTL_BIT_MASK(11,4)
#define LCRU_CLKCH_VAL_CLKDIV(x)			CTL_BIT_SET(x, 11,4)
#define LCRU_CLKCH_SET_CLKDIV				CTL_BIT(2)
#define LCRU_CLKCH_SWRST				CTL_BIT(1)
#define LCRU_CLKCH_CLK_EN				CTL_BIT(0)

/* Move peripheral device to non-secure world */
#define SECURE_MODE_ON					0
#define SECURE_MODE_OFF					1
#define SECURE_MODE(addr, mode)	mmio_write_32(addr, mode)

// Timers
// 160 MHZ? (QEMU)
// #define CNT_HZ 160000000LL
// 8MHZ haps
#define CNT_HZ 50000000LL

#define NS_TICKS(ns) (ns * CNT_HZ / 1000000000LL)

#define WAIT_DELAY(condition, delay_ns, timeout_info)					\
	do {										\
	uint64_t s_cntpct = read_cntpct_el0();						\
	while (condition) {								\
		if ((read_cntpct_el0() - s_cntpct) >= NS_TICKS(delay_ns)) {		\
			timeout_info;							\
			break;								\
		}									\
	}										\
	} while (0);

#define WAIT_SEC_VERBOSE(secs)								\
	do {										\
		int wait_cnt;								\
		for (wait_cnt = 0; wait_cnt < secs; ++wait_cnt) {			\
			tf_printf("\rWait %d", wait_cnt);				\
			WAIT_DELAY((1),	1000000000LL /* 1s*/,);				\
		}									\
		tf_string_print("...\n");						\
	} while (0);

// PLL
#define CLKCH_ENABLE(base, divider, delay_ns)						\
	do {										\
	uint32_t	clkch_reg = mmio_read_32(base);					\
	if ((clkch_reg & LCRU_CLKCH_CLK_EN) == LCRU_CLKCH_CLK_EN) {			\
		WARN("CLKCH 0x%x is already enabled, skipped\n", base);			\
		break;									\
	}										\
	clkch_reg &= ~LCRU_CLKCH_VAL_CLKDIV_MASK;					\
	clkch_reg |= LCRU_CLKCH_VAL_CLKDIV(divider) | LCRU_CLKCH_SET_CLKDIV;		\
	mmio_write_32(base, clkch_reg);							\
	WAIT_DELAY(									\
		((mmio_read_32(base) & LCRU_CLKCH_LOCK_CLKDIV) == 0),			\
		delay_ns,								\
		ERROR("CLKCH lock 0x%x timeout!\n", base)				\
		);									\
	clkch_reg = mmio_read_32(base);							\
	clkch_reg |= LCRU_CLKCH_CLK_EN;							\
	mmio_write_32(base, clkch_reg);							\
	WAIT_DELAY(									\
		((mmio_read_32(base) & LCRU_CLKCH_CLK_RDY) == 0),			\
		delay_ns,								\
		ERROR("CLKCH ready 0x%x timeout!\n", base)				\
		);									\
	clkch_reg = mmio_read_32(base);							\
	clkch_reg &= ~LCRU_CLKCH_SWRST;							\
	mmio_write_32(base, clkch_reg);							\
	} while (0);

#define CLKCH_ON(base, divider) CLKCH_ENABLE(base, divider, 1000000)

typedef struct _PllCtlInitValues {
	uint16_t ClkOD;
	uint16_t ClkR;
	uint32_t ClkFHi;
	uint32_t ClkFLo;
	uint8_t PGain;
	uint8_t IGain;
	uint8_t IPGain;
	uint8_t IIGain;
} PllCtlInitValues;

void pll_on(uintptr_t cmu_base, const PllCtlInitValues *const pllinit, const char *err_msg);

// Routines
void baikal_configure_mmu_el1(unsigned long total_base, unsigned long total_size,
			unsigned long ro_start, unsigned long ro_limit,
			unsigned long coh_start, unsigned long coh_limit);

void baikal_configure_mmu_el3(unsigned long total_base, unsigned long total_size,
			unsigned long ro_start, unsigned long ro_limit,
			unsigned long coh_start, unsigned long coh_limit);

unsigned int plat_baikal_calc_core_pos(u_register_t mpidr);

int dt_add_psci_node(void *fdt);
int dt_add_psci_cpu_enable_methods(void *fdt);
int dt_update_memory(void *fdt, const unsigned long long rangedescs[][2], const unsigned rangenum);

void mmlsp_on(void);
void mmlsp_toNSW(void);

void mmusb_on(void);
void mmusb_toNSW(void);
void mmusb_chc(void);
void mmusb_initSATA(void);

void mmxgbe_on(void);
void mmxgbe_toNSW(void);

void mmmali_on(void);
void mmmali_toNSW(void);

void mmpci_on(void);
void mmpci_toNSW(void);

void mmvdec_on(void);
void mmvdec_toNSW(void);

#endif /*__BAIKAL_PRIVATE_H*/
