/*
 * Copyright (c) 2020-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * Author: Pavel Parkhomenko <pavel.parkhomenko@baikalelectronics.ru>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <bm1000_def.h>
#include <bm1000_private.h>
#include <bm1000_smmu.h>
#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>

static int smmu_set_normalize(uint64_t smmu_offset, bool normalize)
{
	uint64_t timeout;

	/* we have to invalidate TLB before setting attribute normalization */
	mmio_write_32(smmu_offset + SMMU_STLBIALL,  0xffffffff);
	mmio_write_32(smmu_offset + SMMU_STLBGSYNC, 0xffffffff);

	timeout = timeout_init_us(100000);
	while ((mmio_read_32(smmu_offset + SMMU_STLBGSTATUS) &
		SMMU_STLBGSTATUS_GSACTIVE) == 1) {
		if (timeout_elapsed(timeout)) {
			ERROR("TLB invalidate timeout\n");
			break;
		}
	}

	if (normalize)
		mmio_setbits_32(smmu_offset + SMMU_SACR, SMMU_SACR_NORMALIZE);
	else
		mmio_clrbits_32(smmu_offset + SMMU_SACR, SMMU_SACR_NORMALIZE);
	return 0;
}

int mmvdec_smmu_set_normalize(bool normalize)
{
	return smmu_set_normalize(MMVDEC_SMMU_BASE, normalize);
}

int mmvdec_smmu_set_domain_cache(int awdomain, int ardomain, int axcache)
{
	uint32_t reg;
	reg = mmio_read_32(MMVDEC_SMMU_CFG2_REG);
	if (ardomain >= 0 && ardomain <= 3) {
		reg &= ~MMVDEC_SMMU_CFG2_REG_ARDOMAIN_MASK;
		reg |= MMVDEC_SMMU_CFG2_REG_ARDOMAIN(ardomain);
	}
	if (awdomain >= 0 && awdomain <= 3) {
		reg &= ~MMVDEC_SMMU_CFG2_REG_AWDOMAIN_MASK;
		reg |= MMVDEC_SMMU_CFG2_REG_AWDOMAIN(awdomain);
	}
	if (axcache >= 0 && axcache <= 0xf) {
		reg &= ~MMVDEC_SMMU_CFG2_REG_AXCACHE_MASK;
		reg |= MMVDEC_SMMU_CFG2_REG_AXCACHE(axcache);
	}
	mmio_write_32(MMVDEC_SMMU_CFG2_REG, reg);
	return 0;
}

int mmvdec_smmu_get_domain_cache()
{
	return mmio_read_32(MMVDEC_SMMU_CFG2_REG);
}

int mmvdec_smmu_set_qos(int awqos, int arqos)
{
	uint32_t reg;
	reg = mmio_read_32(MMVDEC_SMMU_CFG1_REG);
	if (arqos >= 0 && arqos <= 0xf) {
		reg &= ~MMVDEC_SMMU_CFG1_REG_ARQOS_MASK;
		reg |= MMVDEC_SMMU_CFG1_REG_ARQOS(arqos);
	}
	if (awqos >= 0 && awqos <= 0xf) {
		reg &= ~MMVDEC_SMMU_CFG1_REG_AWQOS_MASK;
		reg |= MMVDEC_SMMU_CFG1_REG_AWQOS(awqos);
	}
	mmio_write_32(MMVDEC_SMMU_CFG1_REG, reg);
	return 0;
}
