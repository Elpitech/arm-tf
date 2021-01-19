/*
 * GPIO support library for Baikal BE-M1000 SoC
 *
 * Copyright (C) 2020 Baikal Electronics JSC
 * Authors: Pavel Parkhomenko <Pavel.Parkhomenko@baikalelectronics.ru>
 *          Ramil Zaripov <Ramil.Zaripov@baikalelectronics.ru>
 */

#include <platform_def.h>

#define GPIO_DATA  *(volatile uint32_t*) ((intptr_t) (GPIO_BASE + 0x00))
#define GPIO_DIR   *(volatile uint32_t*) ((intptr_t) (GPIO_BASE + 0x04))

void gpio_config_pin(int line)
{
    GPIO_DIR  |= (1 << line);
    GPIO_DATA |= (1 << line);
}

void gpio_set_pin(int line)
{
    GPIO_DATA &= ~(1 << line);
}

void gpio_clear_pin(int line)
{
    GPIO_DATA |= (1 << line);
}
