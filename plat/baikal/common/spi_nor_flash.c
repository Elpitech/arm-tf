/*
 * Copyright (c) 2018-2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <stdint.h>

#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <lib/utils_def.h>

#include <baikal_def.h>
#include <dw_gpio.h>
#include <dw_spi.h>
#include <spi_nor_flash.h>

#define SPINOR_CMD_MAX_SIZE	(1 + 4)
#define SPINOR_ID_MAX_SIZE	20
#define SPINOR_PAGE_SIZE	256

/* SPI NOR Flash opcodes */
#define SPINOR_OP_WREN		0x06 /* Write enable */
#define SPINOR_OP_RDSR		0x05 /* Read status register */
#define SPINOR_OP_READ		0x03 /* Read data bytes (low frequency) */
#define SPINOR_OP_PP		0x02 /* Page program (up to 256 bytes) */
#define SPINOR_OP_BE_4K		0x20 /* Erase 4 KiB block */
#define SPINOR_OP_RDID		0x9f /* Read JEDEC ID */
#define SPINOR_OP_EN4B		0xb7 /* Enter 4-byte address mode */
#define SPINOR_OP_EX4B		0xe9 /* Exit 4-byte address mode */

/* SPI NOR Flash status register */
#define SPINOR_SR_WIP		BIT(0) /* Write In Progress */
#define SPINOR_SR_WEL		BIT(1) /* Write Enable Latch */

static unsigned int addr_nbytes;

static int spi_nor_flash_requires_en4b(uint32_t addr, size_t size);
static void spi_nor_flash_set_3byte_cmd(uint32_t addr, uint8_t *cmd);
static void spi_nor_flash_set_4byte_cmd(uint32_t addr, uint8_t *cmd);
static int spi_nor_flash_switch_addr_mode(int line, unsigned int nbytes);
static int spi_nor_flash_exec(int line, uint8_t op, uint32_t addr, void *buf, uint32_t lenbuf);
static int spi_nor_flash_detect(int line);
static int spi_nor_flash_wait(int line);
static int spi_nor_flash_wren(int line);

static int spi_nor_flash_requires_en4b(const uint32_t addr, const size_t size)
{
	return (addr >= 0x1000000 || addr + size > 0x1000000) && addr_nbytes != 4;
}

static void spi_nor_flash_set_3byte_cmd(const uint32_t addr, uint8_t *cmd)
{
	cmd[1] = (addr >> 16) & 0xff;
	cmd[2] = (addr >>  8) & 0xff;
	cmd[3] = (addr >>  0) & 0xff;
}

static void spi_nor_flash_set_4byte_cmd(const uint32_t addr, uint8_t *cmd)
{
	cmd[1] = (addr >> 24) & 0xff;
	cmd[2] = (addr >> 16) & 0xff;
	cmd[3] = (addr >>  8) & 0xff;
	cmd[4] = (addr >>  0) & 0xff;
}

static int spi_nor_flash_switch_addr_mode(const int line, const unsigned int nbytes)
{
	int err;

	assert(nbytes == 3 || nbytes == 4);

	INFO("SPI NOR Flash: switch to %u-byte address mode\n", nbytes);

	err = spi_nor_flash_wren(line);
	if (err) {
		return err;
	}

	err = spi_nor_flash_exec(line,
				 nbytes == 4 ? SPINOR_OP_EN4B : SPINOR_OP_EX4B,
				 0, 0, 0);
	if (err) {
		return err;
	}

	addr_nbytes = nbytes;

	return 0;
}

static int spi_nor_flash_exec(const int line,
			      const uint8_t op,
			      const uint32_t addr,
			      void *buf,
			      const uint32_t lenbuf)
{
	int err;
	uint8_t cmd[SPINOR_CMD_MAX_SIZE];
	uint8_t *in = 0, *out = 0;
	uint32_t lencmd = 0, lenin = 0, lenout = 0;

	cmd[0] = op;
	lencmd += sizeof(op);

	/* Prepare arguments for the SPI transaction */
	switch (op) {
	case SPINOR_OP_RDID:
	case SPINOR_OP_RDSR:
		out = buf;
		lenout = lenbuf;
		break;
	case SPINOR_OP_READ:
		out = buf;
		lenout = lenbuf;
		/* fallthrough */
	case SPINOR_OP_BE_4K:
		if (addr_nbytes == 3) {
			spi_nor_flash_set_3byte_cmd(addr, cmd);
			lencmd += addr_nbytes;
		} else if (addr_nbytes == 4) {
			spi_nor_flash_set_4byte_cmd(addr, cmd);
			lencmd += addr_nbytes;
		} else {
			ERROR("SPI NOR Flash: incorrect address mode\n");
			return -ECAPMODE;
		}
		break;
	case SPINOR_OP_EN4B:
	case SPINOR_OP_EX4B:
	case SPINOR_OP_WREN:
		break;
	case SPINOR_OP_PP:
		if (lenbuf > SPINOR_PAGE_SIZE) {
			ERROR("SPI NOR Flash: incorrect lenbuf: %u\n", lenbuf);
			return -EFAULT;
		}

		in = buf;
		lenin = lenbuf;
		if (addr_nbytes == 3) {
			spi_nor_flash_set_3byte_cmd(addr, cmd);
			lencmd += addr_nbytes;
		} else if (addr_nbytes == 4) {
			spi_nor_flash_set_4byte_cmd(addr, cmd);
			lencmd += addr_nbytes;
		} else {
			ERROR("SPI NOR Flash: incorrect address mode\n");
			return -ECAPMODE;
		}

		break;
	default:
		ERROR("SPI NOR Flash: unknown op: 0x%x\n", op);
		return -EINVAL;
	}
#if defined(BAIKAL_BOOTFLASH_CS_GPIO_BASE) && defined(BAIKAL_BOOTFLASH_CS_GPIO_PIN)
	gpio_out_rst(BAIKAL_BOOTFLASH_CS_GPIO_BASE, BAIKAL_BOOTFLASH_CS_GPIO_PIN); /* GPIO = 0 */
	gpio_dir_set(BAIKAL_BOOTFLASH_CS_GPIO_BASE, BAIKAL_BOOTFLASH_CS_GPIO_PIN);
#endif
	if (lenout) {
		err = dw_spi_eepromread(BAIKAL_BOOTFLASH_SPI_BASE, BAIKAL_BOOTFLASH_SPI_BAUDR, line, cmd, lencmd, out, lenout);
	} else {
		err = dw_spi_tx(BAIKAL_BOOTFLASH_SPI_BASE, BAIKAL_BOOTFLASH_SPI_BAUDR, line, cmd, lencmd, in, lenin);
	}
#if defined(BAIKAL_BOOTFLASH_CS_GPIO_BASE) && defined(BAIKAL_BOOTFLASH_CS_GPIO_PIN)
	gpio_out_set(BAIKAL_BOOTFLASH_CS_GPIO_BASE, BAIKAL_BOOTFLASH_CS_GPIO_PIN); /* GPIO = 1 */
	gpio_dir_clr(BAIKAL_BOOTFLASH_CS_GPIO_BASE, BAIKAL_BOOTFLASH_CS_GPIO_PIN);
#endif
	return err;
}

static int spi_nor_flash_detect(const int line)
{
	int err;
	uint8_t id[SPINOR_ID_MAX_SIZE];
	int try = 10;

	while (try--) {
		err = spi_nor_flash_exec(line, SPINOR_OP_RDID, 0, id, sizeof(id));
		if (err) {
			return err;
		}

		if ((id[0] == 0x00 && id[1] == 0x00 && id[2] == 0x00) ||
		    (id[0] == 0xff && id[1] == 0xff && id[2] == 0xff)) {
			return -1;
		}
	}

	INFO("SPI NOR Flash: chip detected, JEDEC ID: %02x%02x%02x\n", id[0], id[1], id[2]);
	return 0;
}

static int spi_nor_flash_wait(const int line)
{
	int err;
	uint8_t status;
	const uint64_t timeout = timeout_init_us(1000 * 1000);

	do {
		err = spi_nor_flash_exec(line, SPINOR_OP_RDSR, 0, &status, 1);
		if (err) {
			return err;
		}

		if (timeout_elapsed(timeout)) {
			err = -ETIMEDOUT;
			return err;
		}
	} while (status & SPINOR_SR_WIP);

	return 0;
}

static int spi_nor_flash_wren(const int line)
{
	int err;
	uint8_t status;

	err = spi_nor_flash_exec(line, SPINOR_OP_WREN, 0, 0, 0);
	if (err) {
		return err;
	}

	err = spi_nor_flash_exec(line, SPINOR_OP_RDSR, 0, &status, 1);
	if (err) {
		return err;
	}

	if (!(status & SPINOR_SR_WEL)) {
		ERROR("SPI NOR Flash: write enable latch is not set\n");
	}

	return !(status & SPINOR_SR_WEL);
}

int spi_nor_flash_init(const int line)
{
	int err;

	err = spi_nor_flash_detect(line);
	if (err) {
		ERROR("SPI NOR Flash: not found\n");
		return err;
	}

	/* Use 3-byte address mode by default */
	return spi_nor_flash_switch_addr_mode(line, 3);
}

int spi_nor_flash_erase(const int line, uint32_t addr, size_t size)
{
	int err;

	VERBOSE("SPI NOR Flash: erase 0x%x / 0x%lx\n", addr, size);

	if (size % BAIKAL_BOOTFLASH_SUBSECTOR) {
		ERROR("SPI NOR Flash: wrong erase size\n");
		return -1;
	}

	while (size) {
		err = spi_nor_flash_wren(line);
		if (err) {
			return err;
		}

		if (spi_nor_flash_requires_en4b(addr, BAIKAL_BOOTFLASH_SUBSECTOR)) {
			err = spi_nor_flash_switch_addr_mode(line, 4);
			if (err) {
				return err;
			}
		}

		err = spi_nor_flash_exec(line, SPINOR_OP_BE_4K, addr, 0, 0);
		if (err) {
			return err;
		}

		err = spi_nor_flash_wait(line);
		if (err) {
			return err;
		}

		addr += BAIKAL_BOOTFLASH_SUBSECTOR;
		size -= BAIKAL_BOOTFLASH_SUBSECTOR;
	}

	return 0;
}

int spi_nor_flash_read(const int line, uint32_t addr, void *data, size_t size)
{
	int err;
	uint8_t *pdata = data;

	VERBOSE("SPI NOR Flash: read 0x%x / 0x%lx\n", addr, size);

	while (size) {
		size_t chunk = MIN(size, DW_SPI_MAX_READ);

		if (spi_nor_flash_requires_en4b(addr, chunk)) {
			err = spi_nor_flash_switch_addr_mode(line, 4);
			if (err) {
				return err;
			}
		}

		err = spi_nor_flash_exec(line, SPINOR_OP_READ, addr, pdata, chunk);
		if (err) {
			return err;
		}

		addr  += chunk;
		pdata += chunk;
		size  -= chunk;
	}

	return 0;
}

int spi_nor_flash_write(const int line, uint32_t addr, void *data, size_t size)
{
	int err;
	uint8_t *pdata = data;

	VERBOSE("SPI NOR Flash: write 0x%x / 0x%lx\n", addr, size);

	while (size) {
		size_t chunk = MIN(size, (size_t)SPINOR_PAGE_SIZE);
		int p1 = addr / SPINOR_PAGE_SIZE; /* page number */
		int p2 = (addr + chunk) / SPINOR_PAGE_SIZE;

		if (p1 != p2) { /* page overflow ? */
			chunk = p2 * SPINOR_PAGE_SIZE - addr; /* fix chunk size */
		}

		if (spi_nor_flash_requires_en4b(addr, chunk)) {
			err = spi_nor_flash_switch_addr_mode(line, 4);
			if (err) {
				return err;
			}
		}

		err = spi_nor_flash_wren(line);
		if (err) {
			return err;
		}

		err = spi_nor_flash_exec(line, SPINOR_OP_PP, addr, pdata, chunk);
		if (err) {
			return err;
		}

		err = spi_nor_flash_wait(line);
		if (err) {
			return err;
		}

		addr  += chunk;
		pdata += chunk;
		size  -= chunk;
	}

	return 0;
}
