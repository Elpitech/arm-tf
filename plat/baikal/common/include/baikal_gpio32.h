/*
 * Copyright (c) 2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BAIKAL_GPIO32_H
#define BAIKAL_GPIO32_H

void gpio32_dir_clr(const unsigned pin);
void gpio32_dir_set(const unsigned pin);
void gpio32_out_rst(const unsigned pin);
void gpio32_out_set(const unsigned pin);

#endif /* BAIKAL_GPIO32_H */
