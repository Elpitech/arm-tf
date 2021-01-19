/*
 * GPIO support library header file for Baikal BE-M1000 SoC
 *
 * Copyright (C) 2020 Baikal Electronics JSC
 * Authors: Pavel Parkhomenko <Pavel.Parkhomenko@baikalelectronics.ru>
 *          Ramil Zaripov <Ramil.Zaripov@baikalelectonics.ru>
 */

#ifndef __BAIKAL_GPIO_H
#define __BAIKAL_GPIO_H

void gpio_config_pin(int line);

void gpio_set_pin(int line);

void gpio_clear_pin(int line);

#endif
