/*
 * Copyright (c) 2020-2022, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <crc.h>

static inline uint16_t crc16iter(uint16_t crc) __attribute__((always_inline));
static inline uint32_t crc32iter(uint32_t crc) __attribute__((always_inline));

uint16_t crc16(const void *data, size_t size, uint16_t crc)
{
	const uint8_t *ptr = data;

	for (; size >= 8; size -= 8) {
		crc = crc16iter(crc ^= *ptr++ << 8);
		crc = crc16iter(crc ^= *ptr++ << 8);
		crc = crc16iter(crc ^= *ptr++ << 8);
		crc = crc16iter(crc ^= *ptr++ << 8);
		crc = crc16iter(crc ^= *ptr++ << 8);
		crc = crc16iter(crc ^= *ptr++ << 8);
		crc = crc16iter(crc ^= *ptr++ << 8);
		crc = crc16iter(crc ^= *ptr++ << 8);
	}

	for (; size; --size) {
		crc = crc16iter(crc ^= *ptr++ << 8);
	}

	return crc;
}

static inline uint16_t crc16iter(uint16_t crc)
{
	crc = crc << 1 ^ (0x1021 & ~((crc >> 15) - 1));
	crc = crc << 1 ^ (0x1021 & ~((crc >> 15) - 1));
	crc = crc << 1 ^ (0x1021 & ~((crc >> 15) - 1));
	crc = crc << 1 ^ (0x1021 & ~((crc >> 15) - 1));
	crc = crc << 1 ^ (0x1021 & ~((crc >> 15) - 1));
	crc = crc << 1 ^ (0x1021 & ~((crc >> 15) - 1));
	crc = crc << 1 ^ (0x1021 & ~((crc >> 15) - 1));
	crc = crc << 1 ^ (0x1021 & ~((crc >> 15) - 1));
	return crc;
}

uint32_t crc32(const void *data, size_t size, uint32_t crc)
{
	const uint8_t *ptr = data;

	crc = ~crc;

	for (; size >= 8; size -= 8) {
		crc = crc32iter(crc ^= *ptr++);
		crc = crc32iter(crc ^= *ptr++);
		crc = crc32iter(crc ^= *ptr++);
		crc = crc32iter(crc ^= *ptr++);
		crc = crc32iter(crc ^= *ptr++);
		crc = crc32iter(crc ^= *ptr++);
		crc = crc32iter(crc ^= *ptr++);
		crc = crc32iter(crc ^= *ptr++);
	}

	for (; size; --size) {
		crc = crc32iter(crc ^= *ptr++);
	}

	return ~crc;
}

static inline uint32_t crc32iter(uint32_t crc)
{
	crc = crc >> 1 ^ (0xedb88320 & ~((crc & 1) - 1));
	crc = crc >> 1 ^ (0xedb88320 & ~((crc & 1) - 1));
	crc = crc >> 1 ^ (0xedb88320 & ~((crc & 1) - 1));
	crc = crc >> 1 ^ (0xedb88320 & ~((crc & 1) - 1));
	crc = crc >> 1 ^ (0xedb88320 & ~((crc & 1) - 1));
	crc = crc >> 1 ^ (0xedb88320 & ~((crc & 1) - 1));
	crc = crc >> 1 ^ (0xedb88320 & ~((crc & 1) - 1));
	crc = crc >> 1 ^ (0xedb88320 & ~((crc & 1) - 1));
	return crc;
}
