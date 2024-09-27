/*
 * Copyright (c) 2021-2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MEMTEST_H
#define MEMTEST_H

#include <stdint.h>

int memtest_pattern8(uintptr_t base, size_t size, unsigned int incr, uint8_t pattern);
int memtest_pattern16(uintptr_t base, size_t size, unsigned int incr, uint16_t pattern);
int memtest_pattern32(uintptr_t base, size_t size, unsigned int incr, uint32_t pattern);
int memtest_pattern64(uintptr_t base, size_t size, unsigned int incr, uint64_t pattern);

int memtest_rand8(uintptr_t base, size_t size, unsigned int incr, uint64_t seed);
int memtest_rand16(uintptr_t base, size_t size, unsigned int incr, uint64_t seed);
int memtest_rand32(uintptr_t base, size_t size, unsigned int incr, uint64_t seed);
int memtest_rand64(uintptr_t base, size_t size, unsigned int incr, uint64_t seed);

#endif /* MEMTEST_H */
