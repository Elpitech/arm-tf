/*
 * Copyright (c) 2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <stdint.h>

#include <common/debug.h>

#include <dw_spi.h>

struct spi_regs {
	uint32_t ctrlr0;
	uint32_t ctrlr1;
	uint32_t ssienr;
	uint32_t mwcr;
	uint32_t ser;
	uint32_t baudr;
	uint32_t txftlr;
	uint32_t rxftlr;
	uint32_t txflr;
	uint32_t rxflr;
	uint32_t sr;
	uint32_t imr;
	uint32_t isr;
	uint32_t risr;
	uint32_t txoicr;
	uint32_t rxoicr;
	uint32_t rxuicr;
	uint32_t msticr;
	uint32_t icr;
	uint32_t dmacr;
	uint32_t dmatdlr;
	uint32_t dmardlr;
	uint32_t idr;
	uint32_t version;
	uint32_t dr[36];
	uint32_t rx_sample_dly;
};

/* CTRLR0 */
#define SPI_CTRLR0_DFS_8BITS		0x7
#define SPI_CTRLR0_TMOD_TX		0x100
#define SPI_CTRLR0_TMOD_EEPROMREAD	0x300

/* SSIENR */
#define SPI_SSIENR_SSI_EN		BIT(0)

/* SR */
#define SPI_SR_BUSY			BIT(0) /* SPI Busy Flag */
#define SPI_SR_TFNF			BIT(1) /* Tx FIFO Not Full */
#define SPI_SR_TFE			BIT(2) /* Tx FIFO Empty */

/* RISR */
#define SPI_RISR_RXOIR			BIT(3) /* Rx FIFO Overflow Raw Interrupt Status */

int dw_spi_eepromread(const uintptr_t base,
		      const unsigned int baudr,
		      const unsigned int line,
		      void *txdata, const size_t txsize,
		      void *rxbuff, const size_t rxsize)
{
	volatile struct spi_regs *const spiregs = (volatile struct spi_regs *const)base;
	uint8_t *ptr, *end;
	int activated = 0;

	assert(spiregs != NULL);
	assert(baudr >= 2 && baudr <= 65534 && !(baudr % 2));
	assert(txdata != NULL && txsize > 0);
	assert(rxbuff != NULL && rxsize > 0 && rxsize <= DW_SPI_MAX_READ);

	/* Disable the SPI controller */
	spiregs->ssienr	= 0;

	/* Set up the SPI controller registers for the transfer */
	spiregs->ctrlr0	= SPI_CTRLR0_TMOD_EEPROMREAD | SPI_CTRLR0_DFS_8BITS;
	spiregs->ctrlr1	= rxsize - 1;
	spiregs->ser	= 0;
	spiregs->baudr	= baudr;
	spiregs->imr	= 0;
	spiregs->dmacr	= 0;
	spiregs->rx_sample_dly = 0;

	/* Enable the SPI controller */
	spiregs->ssienr	= SPI_SSIENR_SSI_EN;

	ptr = txdata;
	end = ptr + txsize;
	do {
		if (spiregs->sr & SPI_SR_TFNF) {
			spiregs->dr[0] = *ptr++;
		} else if (!activated) {
			spiregs->ser = 1 << line; /* start sending */
			activated    = 1 << line;
		}
	} while (ptr != end);

	if (!activated) {
		spiregs->ser = 1 << line; /* start sending */
		activated    = 1 << line;
	}

	ptr = rxbuff;
	end = ptr + rxsize;
	do { /* read incoming data */
		unsigned int remsize, rxflr = spiregs->rxflr;
		/*
		 * Reading of RXFLR.RXTFL value ("Rx FIFO level") allows
		 * to perform "burst" read of multiple Rx FIFO data entries.
		 * The technique provides higher throughput than polling of
		 * SR.RFNE ("Rx FIFO Not Empty") for each Rx FIFO data entry.
		 * Thus, this allows using higher baudrates (lower BAUDR.SCKDV).
		 */
		if (rxflr) {
			remsize = MIN(rxflr, (unsigned int)(end - ptr));
			do {
				*ptr++ = spiregs->dr[0];
			} while (--remsize);
		} else if (spiregs->risr & SPI_RISR_RXOIR) {
			ERROR("spi@%lx: Rx FIFO overflow\n", base);
			return -1;
		}
	} while (ptr != end);

	/* Disable the SPI controller */
	spiregs->ssienr	= 0;

	return 0;
}

int dw_spi_tx(const uintptr_t base,
	      const unsigned int baudr,
	      const unsigned int line,
	      const void *tx0data, const size_t tx0size,
	      const void *tx1data, const size_t tx1size)
{
	volatile struct spi_regs *const spiregs = (volatile struct spi_regs *const)base;
	const uint8_t *ptr, *end;
	int activated = 0;

	assert(spiregs != NULL);
	assert(baudr >= 2 && baudr <= 65534 && !(baudr % 2));

	/* Disable the SPI controller */
	spiregs->ssienr	= 0;

	/* Set up the SPI controller registers for the transfer */
	spiregs->ctrlr0	= SPI_CTRLR0_TMOD_TX | SPI_CTRLR0_DFS_8BITS;
	spiregs->ser	= 0;
	spiregs->baudr	= baudr;
	spiregs->imr	= 0;
	spiregs->dmacr	= 0;

	/* Enable the SPI controller */
	spiregs->ssienr	= SPI_SSIENR_SSI_EN;

	ptr = tx0data;
	end = ptr + tx0size;
	while (ptr != end) {
		if (spiregs->sr & SPI_SR_TFNF) {
			spiregs->dr[0] = *ptr++;
		} else if (!activated) {
			spiregs->ser = 1 << line; /* start sending */
			activated    = 1 << line;
		}
	}

	ptr = tx1data;
	end = ptr + tx1size;
	while (ptr != end) {
		if (spiregs->sr & SPI_SR_TFNF) {
			spiregs->dr[0] = *ptr++;
		} else if (!activated) {
			spiregs->ser = 1 << line; /* start sending */
			activated    = 1 << line;
		}
	}

	if (!activated) {
		spiregs->ser = 1 << line; /* start sending */
		activated    = 1 << line;
	}

	while ((spiregs->sr & (SPI_SR_TFE | SPI_SR_BUSY)) != SPI_SR_TFE)
		;

	/* Disable the SPI controller */
	spiregs->ssienr	= 0;

	return 0;
}
