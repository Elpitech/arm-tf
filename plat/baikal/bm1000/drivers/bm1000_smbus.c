/*
 * Copyright (c) 2020-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <bm1000_smbus.h>
#include <stdint.h>

struct smbus_regs {
	uint32_t cr1;
	uint32_t cr2;
	uint32_t fbcr1;
	uint32_t fifo;
	uint32_t scd1;
	uint32_t scd2;
	uint32_t adr1;
	uint32_t adr2;
	uint32_t isr1;
	uint32_t imr1;
	uint32_t ivr1;
	uint32_t fbcr2;
	uint32_t rsbcr1;
	uint32_t rsbcr2;
	uint32_t srsbcr1;
	uint32_t srsbcr2;
	uint32_t rssfifo;
	uint32_t isr2;
	uint32_t imr2;
	uint32_t ivr2;
	uint32_t reserved0[5];
	uint32_t sfr;
	uint32_t reserved1[2];
	uint32_t tocr;
	uint32_t reserved2[27];
	uint32_t cir1;
	uint32_t cir2;
};

#define CR1_IRT		BIT(0)
#define CR1_TRS		BIT(1)
#define CR1_MSS		BIT(2)
#define CR1_IEB		BIT(3)
#define CR1_SAS		BIT(4)
#define CR1_GCA		BIT(5)

#define CR2_FRT		BIT(0)
#define CR2_FTE		BIT(1)
#define CR2_HBD		BIT(2)
#define CR2_RSE		BIT(3)
#define CR2_RSF		BIT(4)

#define SCD2_SHT	BIT(7)

#define ISR1_FUR	BIT(0)
#define ISR1_FOR	BIT(1)
#define ISR1_FER	BIT(2)
#define ISR1_RNK	BIT(3)
#define ISR1_ALD	BIT(4)
#define ISR1_FFE	BIT(5)
#define ISR1_TCS	BIT(6)

#define ISR2_MSH	BIT(4)

#define FIFO_SIZE	U(16)

#define SCL_CLK_DIV	499
CASSERT(SCL_CLK_DIV < 1024, assert_smbus_scl_clk_div);

unsigned smbus_txrx(const uintptr_t base,
		    const unsigned targetaddr,
		    const void *const txbuf,
		    const unsigned txbufsize,
		    void *const rxbuf,
		    const unsigned rxbufsize)
{
	volatile struct smbus_regs *const smbusregs = (volatile struct smbus_regs *const)base;
	unsigned rxedsize = 0;
	uint8_t *const rxptr = (uint8_t *)rxbuf;
	const uint8_t *const txptr = (uint8_t *)txbuf;

	assert(smbusregs != NULL);
	assert(targetaddr <= 0x7f);
	assert(txbuf != NULL || !txbufsize);
	assert(rxbuf != NULL || !rxbufsize);

	smbusregs->cr1   = CR1_IRT;
	smbusregs->cr1   = 0;
	smbusregs->cr2	 = 0;
	smbusregs->scd1  = SCL_CLK_DIV & 0xff;
	smbusregs->scd2  = SCD2_SHT | (SCL_CLK_DIV >> 8);
	smbusregs->adr1  = targetaddr;
	smbusregs->imr1  = 0;
	smbusregs->imr2  = 0;
	smbusregs->fbcr2 = 0;
	smbusregs->cr1   = CR1_IEB;

	if (txbufsize > 0) {
		unsigned txedsize;

		smbusregs->cr1 |= CR1_TRS;

		for (txedsize = 0; txedsize < txbufsize;) {
			unsigned bytecount;
			bool holdbus = false;

			bytecount = txbufsize - txedsize;
			if (bytecount > FIFO_SIZE) {
				bytecount = FIFO_SIZE;
				holdbus = true;
			}

			smbusregs->fbcr1 = bytecount;
			do {
				smbusregs->fifo = txptr[txedsize++];
				if (smbusregs->isr1 & ISR1_FOR) {
					goto exit;
				}
			} while (--bytecount);

			if (holdbus) {
				smbusregs->cr2 |= CR2_HBD;
			} else {
				smbusregs->cr2 &= ~CR2_HBD;
			}

			smbusregs->isr1	= ISR1_TCS | ISR1_FFE | ISR1_ALD |
					  ISR1_RNK | ISR1_FER | ISR1_FOR |
					  ISR1_FUR;

			smbusregs->isr2 = ISR2_MSH;

			smbusregs->cr2 |= CR2_FTE;

			while (!(smbusregs->isr1 &
				 (ISR1_TCS | ISR1_ALD | ISR1_RNK)) &&
			       !(smbusregs->isr2 & ISR2_MSH));

			if (smbusregs->isr1 & (ISR1_ALD | ISR1_RNK)) {
				goto exit;
			}
		}

		smbusregs->cr1 &= ~CR1_TRS;
	}

	for (rxedsize = 0; rxedsize < rxbufsize;) {
		unsigned bytecount = MIN(rxbufsize - rxedsize, FIFO_SIZE);

		smbusregs->fbcr1 = bytecount;
		smbusregs->isr1  = ISR1_TCS | ISR1_FFE | ISR1_ALD | ISR1_RNK |
				   ISR1_FER | ISR1_FOR | ISR1_FUR;

		smbusregs->cr2   = CR2_FTE;

		while ((smbusregs->cr2 & CR2_FTE) &&
		       !(smbusregs->isr1 & (ISR1_TCS | ISR1_ALD | ISR1_RNK)));

		if (smbusregs->isr1 & (ISR1_ALD | ISR1_RNK)) {
			goto exit;
		}

		do {
			rxptr[rxedsize] = smbusregs->fifo;

			if (smbusregs->isr1 & ISR1_FUR) {
				goto exit;
			}

			++rxedsize;
		} while (--bytecount);
	}

exit:
	smbusregs->cr1 = 0;
	return rxedsize;
}
