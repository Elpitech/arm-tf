// Copyright (c) 2020 Baikal Electronics JSC
// Author: Mikhail Ivanov <michail.ivanov@baikalelectronics.ru>

#ifndef __ARMTF_SPD_H
#define __ARMTF_SPD_H

unsigned short spd_get_baseconf_crc(const void* const baseconf);
unsigned long long spd_get_baseconf_dimm_capacity(const void* const baseconf);

#endif // __ARMTF_SPD_H
