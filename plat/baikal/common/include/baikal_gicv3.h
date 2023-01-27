/*
 * Copyright (c) 2020-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BAIKAL_GICV3_H
#define BAIKAL_GICV3_H

void baikal_gic_driver_init(void);
void baikal_gic_init(void);
void baikal_gic_cpuif_enable(void);
void baikal_gic_cpuif_disable(void);
void baikal_gic_pcpu_init(void);

#endif /* BAIKAL_GICV3_H */
