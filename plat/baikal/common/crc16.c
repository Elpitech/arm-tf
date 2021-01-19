// Copyright (c) 2020 Baikal Electronics JSC
// Author: Mikhail Ivanov <michail.ivanov@baikalelectronics.ru>

#include <stdint.h>
#include "crc16.h"

unsigned short crc16(const void *buf, unsigned bufsize)
{
	unsigned short crc = 0;
	const uint8_t *ptr = buf;

	while (bufsize--) {
		unsigned i;

		crc ^= *ptr++ << 8;

		for (i = 0; i < 8; ++i) {
			if (crc & 0x8000) {
				crc = crc << 1 ^ 0x1021;
			} else {
				crc <<= 1;
			}
		}
	}

	return crc & 0xffff;
}
