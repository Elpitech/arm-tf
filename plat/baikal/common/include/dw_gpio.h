/*
 * Copyright (c) 2021-2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DW_GPIO_H
#define DW_GPIO_H

void gpio_dir_clr(uintptr_t base, unsigned int pin);
void gpio_dir_set(uintptr_t base, unsigned int pin);
void gpio_out_rst(uintptr_t base, unsigned int pin);
void gpio_out_set(uintptr_t base, unsigned int pin);

#endif /* DW_GPIO_H */
