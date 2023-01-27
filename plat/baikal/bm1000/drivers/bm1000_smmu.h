/*
 * Copyright (c) 2020-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * Author: Pavel Parkhomenko <pavel.parkhomenko@baikalelectronics.ru>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BM1000_SMMU_H
#define BM1000_SMMU_H

#include <stdbool.h>

#define SMMU_SACR                   0x10
#define SMMU_SACR_NORMALIZE         (1 << 27)
#define SMMU_STLBIALL               0x60
#define SMMU_STLBGSYNC              0x70
#define SMMU_STLBGSTATUS            0x74
#define SMMU_STLBGSTATUS_GSACTIVE   (1 << 0)

#define MMVDEC_SMMU_CFG1_REG               (MMVDEC_GPR_BASE + 0x10)
#define MMVDEC_SMMU_CFG1_REG_AWQOS(x)	   SETMASK(x, 23, 20)
#define MMVDEC_SMMU_CFG1_REG_AWQOS_MASK	   GENMASK(   23, 20)
#define MMVDEC_SMMU_CFG1_REG_ARQOS(x)	   SETMASK(x, 19, 16)
#define MMVDEC_SMMU_CFG1_REG_ARQOS_MASK	   GENMASK(   19, 16)
#define MMVDEC_SMMU_CFG1_REG_AWPROT(x)	   SETMASK(x,  6,  4)
#define MMVDEC_SMMU_CFG1_REG_AWPROT_MASK   GENMASK(    6,  4)
#define MMVDEC_SMMU_CFG1_REG_ARPROT(x)	   SETMASK(x,  2,  0)
#define MMVDEC_SMMU_CFG1_REG_ARPROT_MASK   GENMASK(    2,  0)

#define MMVDEC_SMMU_CFG2_REG               (MMVDEC_GPR_BASE + 0x18)
#define MMVDEC_SMMU_CFG2_REG_ARDOMAIN(x)   SETMASK(x, 13, 12)
#define MMVDEC_SMMU_CFG2_REG_ARDOMAIN_MASK GENMASK(   13, 12)
#define MMVDEC_SMMU_CFG2_REG_AWDOMAIN(x)   SETMASK(x,  5,  4)
#define MMVDEC_SMMU_CFG2_REG_AWDOMAIN_MASK GENMASK(    5,  4)
#define MMVDEC_SMMU_CFG2_REG_AXCACHE(x)	   SETMASK(x,  3,  0)
#define MMVDEC_SMMU_CFG2_REG_AXCACHE_MASK  GENMASK(    3,  0)

int mmvdec_smmu_set_normalize(bool normalize);
int mmvdec_smmu_set_domain_cache(int awdomain, int ardomain, int axcache);
int mmvdec_smmu_get_domain_cache(void);
int mmvdec_smmu_set_qos(int awqos, int arqos);

#endif /* BM1000_SMMU_H */
