/** @file  baikal_smmu.h

 Copyright (C) 2020 Baikal Electronics JSC

 Author: Pavel Parkhomenko <Pavel.Parkhomenko@baikalelectronics.ru>

 **/

#include <bm1000_private.h>
#include <platform_def.h>
#include <stdbool.h>

#define SMMU_SACR                   0x10
#define SMMU_SACR_NORMALIZE         (1 << 27)
#define SMMU_STLBIALL               0x60
#define SMMU_STLBGSYNC              0x70
#define SMMU_STLBGSTATUS            0x74
#define SMMU_STLBGSTATUS_GSACTIVE   (1 << 0)

#define MMVDEC_SMMU_CFG1_REG               MMVDEC_GPR(0x10)
#define MMVDEC_SMMU_CFG1_REG_AWQOS(x)      CTL_BIT_SET(x, 23, 20)
#define MMVDEC_SMMU_CFG1_REG_AWQOS_MASK    CTL_BIT_MASK(23, 20)
#define MMVDEC_SMMU_CFG1_REG_ARQOS(x)      CTL_BIT_SET(x, 19, 16)
#define MMVDEC_SMMU_CFG1_REG_ARQOS_MASK    CTL_BIT_MASK(19, 16)
#define MMVDEC_SMMU_CFG1_REG_AWPROT(x)     CTL_BIT_SET(x, 6, 4)
#define MMVDEC_SMMU_CFG1_REG_AWPROT_MASK   CTL_BIT_MASK(6, 4)
#define MMVDEC_SMMU_CFG1_REG_ARPROT(x)     CTL_BIT_SET(x, 2, 0)
#define MMVDEC_SMMU_CFG1_REG_ARPROT_MASK   CTL_BIT_MASK(2, 0)

#define MMVDEC_SMMU_CFG2_REG               MMVDEC_GPR(0x18)
#define MMVDEC_SMMU_CFG2_REG_ARDOMAIN(x)   CTL_BIT_SET(x, 13, 12)
#define MMVDEC_SMMU_CFG2_REG_ARDOMAIN_MASK CTL_BIT_MASK(13, 12)
#define MMVDEC_SMMU_CFG2_REG_AWDOMAIN(x)   CTL_BIT_SET(x, 5, 4)
#define MMVDEC_SMMU_CFG2_REG_AWDOMAIN_MASK CTL_BIT_MASK(5, 4)
#define MMVDEC_SMMU_CFG2_REG_AXCACHE(x)    CTL_BIT_SET(x, 3, 0)
#define MMVDEC_SMMU_CFG2_REG_AXCACHE_MASK  CTL_BIT_MASK(3, 0)

#define MMVDEC_SMMU_BASE                   0x24080000

int mmvdec_smmu_set_normalize(bool normalize);
int mmvdec_smmu_set_domain_cache(int awdomain, int ardomain, int axcache);
int mmvdec_smmu_get_domain_cache(void);
int mmvdec_smmu_set_qos(int awqos, int arqos);
