/*
 * Copyright (c) 2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BM1000_CCN_H
#define BM1000_CCN_H

void ccn_hnf_sam_setup(unsigned int snf0, unsigned int snf1);
void ccn_xp_set_qos(unsigned int xp, unsigned int dev, unsigned int qos);

#endif /* BM1000_CCN_H */
