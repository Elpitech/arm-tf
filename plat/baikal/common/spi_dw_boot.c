/*
 * Copyright (c) 2018-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <stdint.h>

#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <lib/utils_def.h>

#include <baikal_def.h>
#include <dw_gpio.h>
#include <spi_dw.h>

#include "spi_flash_ids.h"

/* Registers */
#define SPI_CTRLR0	*(volatile uint32_t *)(BAIKAL_SPI_BASE + 0x00)
#define SPI_CTRLR1	*(volatile uint32_t *)(BAIKAL_SPI_BASE + 0x04)
#define SPI_SPIENR	*(volatile uint32_t *)(BAIKAL_SPI_BASE + 0x08)
#define SPI_SER		*(volatile uint32_t *)(BAIKAL_SPI_BASE + 0x10)
#define SPI_BAUDR	*(volatile uint32_t *)(BAIKAL_SPI_BASE + 0x14)
#define SPI_SR		*(volatile uint32_t *)(BAIKAL_SPI_BASE + 0x28)
#define SPI_IMR		*(volatile uint32_t *)(BAIKAL_SPI_BASE + 0x2c)
#define SPI_DR		*(volatile uint32_t *)(BAIKAL_SPI_BASE + 0x60)

/* CTRLR0 */
#define SPI_TMOD_OFFSET		8
#define SPI_TMOD_MASK		(3 << SPI_TMOD_OFFSET)
#define SPI_TMOD_TXRX		0
#define SPI_TMOD_TX		1
#define SPI_TMOD_RX		2
#define SPI_TMOD_EEPROMREAD	3

#define SPI_DFS_OFFSET		0
#define SPI_DFS_MASK		(0xf << SPI_DFS_OFFSET)
#define SPI_DFS_GET		(((SPI_CTRLR0 & SPI_DFS_MASK) >> SPI_DFS_OFFSET) + 1)

/* SPIENR */
#define SPI_SPIENR_SPI_DE	0
#define SPI_SPIENR_SPI_EN	1

/* SR */
#define SPI_SR_BUSY		BIT(0)
#define SPI_SR_TFNF		BIT(1) /* Tx FIFO Not Full */
#define SPI_SR_TFE		BIT(2) /* Tx FIFO Empty */
#define SPI_SR_RFNE		BIT(3) /* Rx FIFO Not Empty */
#define SPI_SR_DCOL		BIT(6) /* Data Collision Error */

/* SPI Flash status register */
#define SPI_FLASH_SR_WIP	BIT(0) /* Write In Progress */
#define SPI_FLASH_SR_WEL	BIT(1) /* Write Enable Latch */

/* SPI Flash flag status register */
#define SPI_FLAG_4BYTE		BIT(0) /* 4-byte Address Enabling */

/* SPI Flash commands */
#define CMD_FLASH_RDID		0x9f /* (0, 1-20) Read identification */
#define CMD_FLASH_READ		0x03 /* (3, 1-∞ ) Read Data Bytes */
#define CMD_FLASH_WREN		0x06 /* (0, 0   ) Write Enable */
#define CMD_FLASH_WRDI		0x04 /* (0, 0   ) Write Disable */
#define CMD_FLASH_PP		0x02 /* (3, 256 ) Page Program */
#define CMD_FLASH_SSE		0x20 /* (3, 0   ) SubSector Erase */
#define CMD_FLASH_SE		0xd8 /* (3, 0   ) Sector Erase */
#define CMD_FLASH_RDSR		0x05 /* (0, 1   ) Read Status Register */
#define CMD_FLASH_WRSR		0x01 /* (0, 1-∞ ) Write Status Register */
#define CMD_FLASH_RFSR		0x70 /* (1 to ∞)  Read Flag Status Register */
#define CMD_FLASH_BE		0xc7 /* (0) Bulk Erase */
#define CMD_FLASH_EN4BYTEADDR	0xb7 /* Enter 4-byte address mode */
#define CMD_FLASH_EX4BYTEADDR	0xe9 /* Exit 4-byte address mode */
#define CMD_FLASH_WREAR		0xc5 /* Write Extended Address Register */
#define CMD_FLASH_RDEAR		0xc8 /* Read Extended Address Register */

/* Put address */
#define ADR_MODE_3BYTE		3
#define SPI_ADR_LEN_3BYTE	3
#define SPI_SET_ADDRESS_3BYTE(a, b)		\
({	uint8_t *_b = (void *)(b);		\
	_b[1] = (((a) >> 8 * 2) & 0xff);	\
	_b[2] = (((a) >> 8 * 1) & 0xff);	\
	_b[3] = (((a) >> 8 * 0) & 0xff);	\
})

#define ADR_MODE_4BYTE		4
#define SPI_ADR_LEN_4BYTE	4
#define SPI_SET_ADDRESS_4BYTE(a, b)		\
({	uint8_t *_b = (void *)(b);		\
	_b[1] = (((a) >> 8 * 3) & 0xff);	\
	_b[2] = (((a) >> 8 * 2) & 0xff);	\
	_b[3] = (((a) >> 8 * 1) & 0xff);	\
	_b[4] = (((a) >> 8 * 0) & 0xff);	\
})

#define SPI_CMD_LEN			(1 + SPI_ADR_LEN_4BYTE)
#define SPI_MAX_READ			UL(0x10000)
#define SPI_MAX_WRITE			UL(256) /* (3, 256) Page Program */
#define SPI_TIMEOUT_BAUDR(b, d, f)	((100 * (d) * (b)) / (f)) /* (100 * 8 bit * BAUDR) / 50 MHz */
#define SPI_TIMEOUT_ERASE		1000000

static unsigned adr_mode;

static int wait_busy(void);
static int wait_rx(void);
static int wait_tx(void);

static int transfer(const unsigned line,
		    void *cmd_, uint32_t cmd_len,
		    void *tx_, uint32_t tx_len,
		    void *rx_, uint32_t rx_len,
		    const unsigned baudr)
{
	int err = -1;
	uint8_t *cmd = cmd_;
	uint8_t *tx = tx_;
	uint8_t *rx = rx_;
	uint8_t *cmdend	= (void *)((intptr_t)cmd + (intptr_t)cmd_len);
	uint8_t *txend	= (void *)((intptr_t)tx	 + (intptr_t)tx_len);
	uint8_t *rxend	= (void *)((intptr_t)rx	 + (intptr_t)rx_len);

	if (cmd == NULL || !cmd_len || line > 3 || baudr < 2 || baudr > 65534) {
		ERROR("SPI: %s: incorrect args\n", __func__);
		return -EINVAL;
	}

	SPI_SPIENR = SPI_SPIENR_SPI_DE;
	SPI_CTRLR0 = 7 << SPI_DFS_OFFSET;
#ifdef BAIKAL_DBS
	SPI_BAUDR = MAX(baudr, 16U);
#else
	SPI_BAUDR = baudr;
#endif
	SPI_IMR	   = 0;
	SPI_SER	   = 0; /* disable all lines */

#ifdef BAIKAL_BOOT_SPI_CS_GPIO_PIN
	gpio_out_rst(BAIKAL_SPI_GPIO_BASE, BAIKAL_BOOT_SPI_CS_GPIO_PIN);
	gpio_dir_set(BAIKAL_SPI_GPIO_BASE, BAIKAL_BOOT_SPI_CS_GPIO_PIN);
#endif

	if (rx_len) {
		SPI_CTRLR0 = (SPI_CTRLR0 & ~SPI_TMOD_MASK) | (SPI_TMOD_EEPROMREAD << SPI_TMOD_OFFSET);
	} else {
		SPI_CTRLR0 = (SPI_CTRLR0 & ~SPI_TMOD_MASK) | (SPI_TMOD_TX << SPI_TMOD_OFFSET);
	}

	switch ((SPI_CTRLR0 & SPI_TMOD_MASK) >> SPI_TMOD_OFFSET) {
	case SPI_TMOD_TX:
		SPI_SPIENR = SPI_SPIENR_SPI_EN; /* enable FIFO */
		while (cmd < cmdend) {
			if (SPI_SR & SPI_SR_TFNF) {
				SPI_DR = *cmd++;
			} else if (!SPI_SER) {
				SPI_SER = 1 << line; /* start sending */
			}
		}

		SPI_SER = 1 << line; /* start sending */
		while (tx < txend) {
			if (SPI_SR & SPI_SR_TFNF) {
				SPI_DR = *tx++;
			} else {
				err = wait_tx();
				if (err) {
					goto exit;
				}
			}
		}

		err = wait_busy();
		if (err) {
			goto exit;
		}

		break;

	case SPI_TMOD_EEPROMREAD:
		if (rx == NULL || !rx_len || rx_len > SPI_MAX_READ) {
			ERROR("SPI: %s: eeprom\n", __func__);
			err = -EINVAL;
			goto exit;
		}

		SPI_CTRLR1 = rx_len - 1; /* set read size */
		SPI_SPIENR = SPI_SPIENR_SPI_EN; /* enable FIFO */
		while (cmd < cmdend) {
			if (SPI_SR & SPI_SR_TFNF) {
				SPI_DR = *cmd++;
			} else if (!SPI_SER) {
				SPI_SER = 1 << line; /* start sending */
			}
		}

		SPI_SER = 1 << line; /* start sending */
		while (rx < rxend) { /* read incoming data */
			if (SPI_SR & SPI_SR_RFNE) {
				*rx++ = SPI_DR;
			} else {
				err = wait_rx();
				if (err) {
					goto exit;
				}
			}
		}

		break;

	case SPI_TMOD_TXRX:
	case SPI_TMOD_RX:
	default:
		ERROR("SPI: %s: mode\n", __func__);
		err = -EINVAL;
		goto exit;
	}

	err = 0;

exit:
#ifdef BAIKAL_BOOT_SPI_CS_GPIO_PIN
	gpio_dir_clr(BAIKAL_SPI_GPIO_BASE, BAIKAL_BOOT_SPI_CS_GPIO_PIN);
#endif
	SPI_SPIENR = SPI_SPIENR_SPI_DE;

	if (err) {
		ERROR("SPI: transmission error, %d\n", err);
	}

	return err;
}

static int exec(int line,
	uint8_t cmd_op,
	uint32_t address,
	void *buf,
	uint32_t lenbuf,
	int baudr)
{
	uint8_t cmd[SPI_CMD_LEN];
	uint8_t *in = 0, *out = 0;
	uint32_t lencmd = 0, lenin = 0, lenout = 0;

	/* Save the SPI flash instruction */
	cmd[0] = cmd_op;
	lencmd += 1;

	/* Prepare arguments for the SPI transaction */
	switch (cmd_op) {
	case CMD_FLASH_RDID:
	case CMD_FLASH_RDSR:
	case CMD_FLASH_RDEAR:
	case CMD_FLASH_RFSR:
		out = buf;
		lenout = lenbuf;
		break;

	case CMD_FLASH_READ:
		out = buf;
		lenout = lenbuf;
	case CMD_FLASH_SSE:
	case CMD_FLASH_SE:
		if (adr_mode == ADR_MODE_4BYTE) {
			SPI_SET_ADDRESS_4BYTE(address, cmd);
			lencmd += SPI_ADR_LEN_4BYTE;
		} else if (adr_mode == ADR_MODE_3BYTE) {
			SPI_SET_ADDRESS_3BYTE(address, cmd);
			lencmd += SPI_ADR_LEN_3BYTE;
		} else {
			ERROR("SPI: %s: incorrect address mode\n", __func__);
			return -ECAPMODE;
		}

		break;

	case CMD_FLASH_WREAR:
	case CMD_FLASH_WRSR:
		in = buf;
		lenin = 1;
		break;

	case CMD_FLASH_EN4BYTEADDR:
	case CMD_FLASH_EX4BYTEADDR:
	case CMD_FLASH_WRDI:
	case CMD_FLASH_WREN:
	case CMD_FLASH_BE:
		break;

	case CMD_FLASH_PP:
		if (lenbuf > SPI_MAX_WRITE) {
			ERROR("SPI: %s: incorrect lenbuf: %u\n", __func__, lenbuf);
			return -EFAULT;
		}

		in = buf;
		lenin = lenbuf;
		if (adr_mode == ADR_MODE_4BYTE) {
			SPI_SET_ADDRESS_4BYTE(address, cmd);
			lencmd += SPI_ADR_LEN_4BYTE;
		} else if (adr_mode == ADR_MODE_3BYTE) {
			SPI_SET_ADDRESS_3BYTE(address, cmd);
			lencmd += SPI_ADR_LEN_3BYTE;
		} else {
			ERROR("SPI: %s: incorrect address mode\n", __func__);
			return -ECAPMODE;
		}
		break;

	default:
		ERROR("SPI: %s: unknown cmd_op: 0x%x\n", __func__, cmd_op);
		return -EINVAL;
	}

	return transfer(line, cmd, lencmd, in, lenin, out, lenout, baudr);
}

static int wait_tx(void)
{
	const uint64_t timeout = timeout_init_us(SPI_TIMEOUT_BAUDR(SPI_BAUDR, SPI_DFS_GET, 50));

	for (;;) {
		const uint8_t spisr = SPI_SR;

		if (spisr & SPI_SR_TFNF) {
			return 0;
		} else if (spisr & SPI_SR_DCOL) {
			return -EBADMSG;
		} else if (timeout_elapsed(timeout)) {
			return -ETIMEDOUT;
		}
	}
}

static int wait_rx(void)
{
	const uint64_t timeout = timeout_init_us(SPI_TIMEOUT_BAUDR(SPI_BAUDR, SPI_DFS_GET, 50));

	for (;;) {
		const uint8_t spisr = SPI_SR;

		if (spisr & SPI_SR_RFNE) {
			return 0;
		} else if (spisr & SPI_SR_DCOL) {
			return -EBADMSG;
		} else if (timeout_elapsed(timeout)) {
			return -ETIMEDOUT;
		}
	}
}

static int wait_busy(void)
{
	const uint64_t timeout = timeout_init_us(SPI_TIMEOUT_BAUDR(SPI_BAUDR, SPI_DFS_GET, 50));

	for (;;) {
		const uint8_t spisr = SPI_SR;

		if ((spisr & SPI_SR_TFE) && !(spisr & SPI_SR_BUSY)) {
			return 0;
		} else if (spisr & SPI_SR_DCOL) {
			return -EBADMSG;
		} else if (timeout_elapsed(timeout)) {
			return -ETIMEDOUT;
		}
	}
}

static int status(int line, void *status_)
{
	const int baudr = 4;

	return exec(line, CMD_FLASH_RDSR, 0, status_, 1, baudr);
}

int dw_spi_mode(int line)
{
	uint8_t flag;
	const int baudr = 4;

	exec(line, CMD_FLASH_RFSR, 0, &flag, 1, baudr);
	return flag & SPI_FLAG_4BYTE ? ADR_MODE_4BYTE : ADR_MODE_3BYTE;
}

static int wren(int line)
{
	int err;
	uint8_t st;
	const int baudr = 4;

	err = exec(line, CMD_FLASH_WREN, 0, 0, 0, baudr);
	if (err) {
		return err;
	}

	err = status(line, &st);
	if (err) {
		return err;
	}

	return !(st & SPI_FLASH_SR_WEL);
}

static int wait(int line)
{
	int err = -1;
	uint8_t st;
	uint64_t timeout = timeout_init_us(SPI_TIMEOUT_ERASE);

	do {
		if (status(line, &st)) {
			goto exit;
		} else if (timeout_elapsed(timeout)) {
			ERROR("SPI: timeout, %s\n", __func__);
			goto exit;
		}
	} while (st & SPI_FLASH_SR_WIP);

	err = 0;
exit:
	return err;
}

int dw_spi_erase(int line, uint32_t adr, size_t size, size_t sector_size)
{
	int err;

	VERBOSE("SPI: %s(0x%x, 0x%lx)\n", __func__, adr, size);

	if (!adr_mode)
		dw_spi_init(line);

	if (!sector_size) {
		ERROR("SPI: %s: incorrect sector_size: %lu\n", __func__, sector_size);
		return -EINVAL;
	}

	while (size) {
		const int baudr = 4;

		err = wren(line);
		if (err) {
			return err;
		}

		err = exec(line, CMD_FLASH_SE, adr, 0, 0, baudr);
		if (err) {
			return err;
		}

		err = wait(line);
		if (err) {
			return err;
		}

		adr  += sector_size;
		size -= MIN(size, sector_size);
	}

	return 0;
}

int dw_spi_write(int line, uint32_t adr, void *data, size_t size)
{
	int err;
	int part;
	uint8_t *pdata = data;

	VERBOSE("SPI: %s(0x%x, 0x%lx)\n", __func__, adr, size);

	if (!adr_mode)
		dw_spi_init(line);

	while (size) {
		const int baudr = 16;

		part = MIN(size, SPI_MAX_WRITE);

		/* fix size */
		int p1 = adr / SPI_MAX_WRITE; /* page number */
		int p2 = (adr + part) / SPI_MAX_WRITE;
		if (p1 != p2) { /* page overflow ? */
			p2 *= SPI_MAX_WRITE; /* page base address */
			part = p2 - adr;
		}

		err = wren(line);
		if (err) {
			return err;
		}

		err = exec(line, CMD_FLASH_PP, adr, pdata, part, baudr);
		if (err) {
			return err;
		}

		err = wait(line);
		if (err) {
			return err;
		}

		adr   += part;
		pdata += part;
		size  -= part;
	}

	return 0;
}

int dw_spi_read(int line, uint32_t adr, void *data, size_t size)
{
	int err;
	int part;
	uint8_t *pdata = data;
	int baudr = 16;

	VERBOSE("SPI: %s(0x%x, 0x%lx)\n", __func__, adr, size);

	if (!adr_mode)
		dw_spi_init(line);

	while (size) {
		part = MIN(size, SPI_MAX_READ);
		err = exec(line, CMD_FLASH_READ, adr, pdata, part, baudr);
		if (err) {
			return err;
		}

		adr   += part;
		pdata += part;
		size  -= part;
	}

	return 0;
}

int dw_spi_3byte(int line)
{
	const int baudr = 4;
	int err;

	VERBOSE("SPI: %s\n", __func__);

	err = wren(line);
	if (err) {
		return err;
	}

	err = exec(line, CMD_FLASH_EX4BYTEADDR, 0, 0, 0, baudr);
	if (err) {
		return err;
	}

	adr_mode = ADR_MODE_3BYTE;
	return 0;
}

int dw_spi_4byte(int line)
{
	const int baudr = 4;
	int err;

	VERBOSE("SPI: %s\n", __func__);

	err = wren(line);
	if (err) {
		return err;
	}

	err = exec(line, CMD_FLASH_EN4BYTEADDR, 0, 0, 0, baudr);
	if (err) {
		return err;
	}

	adr_mode = ADR_MODE_4BYTE;
	return 0;
}

const struct flash_info *dw_spi_get_info(int line)
{
	const int baudr = 4;
	unsigned int i;
	uint8_t id[SPI_NOR_MAX_ID_LEN];

	if (exec(line, CMD_FLASH_RDID, 0, id, sizeof(id), baudr)) {
		ERROR("SPI: error while reading JEDEC ID\n");
		return NULL;
	}

	for (i = 0; i < ARRAY_SIZE(spi_flash_ids); ++i) {
		const struct flash_info *info;
		info = &spi_flash_ids[i];
		if (!memcmp(info->id, id, sizeof(id))) {
			INFO("SPI Flash: JEDEC ID 0x%02x%02x%02x, density %u MiB\n",
			     id[0], id[1], id[2],
			     (1 << info->sector_log2size) *
			     (1 << info->sector_log2count) / 1024 / 1024);

			return info;
		}
	}

	ERROR("SPI: unknown JEDEC ID 0x%02x%02x%02x\n", id[0], id[1], id[2]);
	return NULL;
}

void dw_spi_init(int line)
{
	const struct flash_info *info;

	VERBOSE("SPI: %s\n", __func__);

	info = dw_spi_get_info(line);
	if (info == NULL) {
		ERROR("SPI: %s: info is NULL\n", __func__);
		return;
	}

	dw_spi_3byte(line);

#ifdef BAIKAL_FLASH_4BYTEADR
	if ((1 << info->sector_log2size) *
	    (1 << info->sector_log2count) > 16 * 1024 * 1024) {
		dw_spi_4byte(line);
	}
#endif
}
