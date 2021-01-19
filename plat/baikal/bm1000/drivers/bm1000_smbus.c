/*
 * Copyright (c) 2020, Baikal Electronics JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <bm1000_smbus.h>
#include <stdint.h>

#define BAIKAL_SMBUS_BASE	0x20270000
#define BAIKAL_SMBUS_OFFSET	0x10000
#define BAIKAL_SMBUS_REGS(bus)	(volatile struct smbus_regs *const)(uintptr_t)(BAIKAL_SMBUS_BASE + BAIKAL_SMBUS_OFFSET * (bus))

struct smbus_regs {
	uint32_t cr1;		// Control Register 1
	uint32_t cr2;		// Control Register 2
	uint32_t fbcr1;		// FIFO Byte Count Register 1
	uint32_t fifo;		// FIFO Memory
	uint32_t scd1;		// SCL Clock Divider Register 1
	uint32_t scd2;		// SCL Clock Divider Register 2
	uint32_t adr1;		// Address Register 1
	uint32_t adr2;		// Address Register 2
	uint32_t isr1;		// Interrupt Status Register 1
	uint32_t imr1;		// Interrupt Mask Register 1
	uint32_t ivr1;		// Interrupt Vector Register 1
	uint32_t fbcr2;		// FIFO Byte Count Register 2
	uint32_t rsbcr1;	// Repeated Start Byte Count Register 1
	uint32_t rsbcr2;	// Repeated Start Byte Count Register 2
	uint32_t srsbcr1;	// Repeated Start Byte Count Register 1 (Slave)
	uint32_t srsbcr2;	// Repeated Start Byte Count Register 2 (Slave)
	uint32_t rssfifo;	// Repeated Start Slave FIFO Memory (Slave)
	uint32_t isr2;		// Interrupt Status Register 2
	uint32_t imr2;		// Interrupt Mask Register 2
	uint32_t ivr2;		// Interrupt Vector Register 2
	uint32_t reserved0[5];
	uint32_t sfr;		// Programmable SDA/SCL Spike Filter Width
	uint32_t reserved1[2];
	uint32_t tocr;		// SCL Low Timeout Counter Register
	uint32_t reserved2[27];
	uint32_t cir1;		// Core Identification Register 1
	uint32_t cir2;		// Core Identification Register 2
};

#define CR1_GCA		(1 << 5)	// Generall Call Address Enable
#define CR1_SAS		(1 << 4)	// Slave Address Size
#define CR1_IEB		(1 << 3)	// SMBus Controller Enable
#define CR1_MSS		(1 << 2)	// Master / Slave Select
#define CR1_TRS		(1 << 1)	// Transmitter / Receiver Select
#define CR1_IRT		(1 << 0)	// SMBus Reset

#define CR2_RSF		(1 << 4)	// Repeater Start Slave (RSS) FIFO
#define CR2_RSE		(1 << 3)	// Repeated Start Enable
#define CR2_HBD		(1 << 2)	// Hold SMBus for more FIFO Data
#define CR2_FTE		(1 << 1)	// FIFO Mode - Transfer Enable (Master)
#define CR2_FRT		(1 << 0)	// FIFO Reset

#define ISR1_TCS	(1 << 6)	// Transfer Completed - SMBus Stop Assered
#define ISR1_FFE	(1 << 5)	// FIFO Almost Full (Rx mode) of Empty (Tx mode)
#define ISR1_ALD	(1 << 4)	// Arbitration Loss Detected
#define ISR1_RNK	(1 << 3)	// Receive NACK
#define ISR1_FER	(1 << 2)	// FIFO Error - Underrun or Overrun
#define ISR1_FOR	(1 << 1)	// FIFO - Overrun
#define ISR1_FUR	(1 << 0)	// FIFO - Underrun

unsigned smbus_txrx(const unsigned bus, const unsigned addr, const void *const txbuf, const unsigned txbufsize, void *const rxbuf, const unsigned rxbufsize)
{
	volatile struct smbus_regs *const smbusregs = BAIKAL_SMBUS_REGS(bus);
	unsigned rxedsize = 0;
	uint8_t *const rxptr = (uint8_t *)rxbuf;
	const uint8_t *const txptr = (uint8_t *)txbuf;

	smbusregs->cr1   = CR1_IRT;
	smbusregs->cr1   = 0;
	smbusregs->scd1  = 0xff;
	smbusregs->scd2  = 0x83;
	smbusregs->adr1  = addr;
	smbusregs->imr1  = 0;
	smbusregs->imr2  = 0;
	smbusregs->fbcr2 = 0;
	smbusregs->cr1   = CR1_IEB;

	if (txbufsize > 0) {
		unsigned txedsize;
		smbusregs->cr1 |= CR1_TRS;

		for (txedsize = 0; txedsize < txbufsize;) {
			unsigned bytecount = (txbufsize - txedsize >= 16) ? 16 : txbufsize;

			smbusregs->fbcr1 = bytecount;

			while (bytecount--) {
				smbusregs->fifo = txptr[txedsize++];
			}

			smbusregs->isr1	= ISR1_TCS | ISR1_FFE | ISR1_ALD | ISR1_RNK | ISR1_FER | ISR1_FOR | ISR1_FUR;
			smbusregs->cr2	= CR2_FTE;

			while ((smbusregs->cr2 & CR2_FTE) && !(smbusregs->isr1 & ISR1_TCS));

			if (smbusregs->isr1 & ISR1_RNK) {
				goto exit;
			}
		}

		smbusregs->cr1 &= ~CR1_TRS;
	}

	for (rxedsize = 0; rxedsize < rxbufsize;) {
		unsigned bytecount = (rxbufsize - rxedsize >= 16) ? 16 : rxbufsize;

		smbusregs->fbcr1 = bytecount;
		smbusregs->isr1  = ISR1_TCS | ISR1_FFE | ISR1_ALD | ISR1_RNK | ISR1_FER | ISR1_FOR | ISR1_FUR;
		smbusregs->cr2   = CR2_FTE;

		while ((smbusregs->cr2 & CR2_FTE) && !(smbusregs->isr1 & ISR1_TCS));

		if (smbusregs->isr1 & ISR1_RNK) {
			goto exit;
		}

		while (bytecount--) {
			rxptr[rxedsize++] = smbusregs->fifo;

			if ((smbusregs->isr1 & ISR1_FUR) == ISR1_FUR) {
				goto exit;
			}
		}
	}

exit:
	smbusregs->cr1 = 0;
	return rxedsize;
}
