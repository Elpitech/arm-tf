/*
 * Copyright (c) 2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>

#include <arch_helpers.h>
#include <lib/mmio.h>

#include <bm1000_ccn.h>
#include <bm1000_def.h>

void ccn_hnf_sam_setup(unsigned int snf0, unsigned int snf1)
{
	if (!snf0 && !snf1) {
		return;
	} else if (!snf0) {
		snf0 = snf1;
	} else if (!snf1) {
		snf1 = snf0;
	}

	mmio_write_64(CCN_HNF_BASE + 0x00008, snf0);
	mmio_write_64(CCN_HNF_BASE + 0x10008, snf0);
	mmio_write_64(CCN_HNF_BASE + 0x20008, snf0);
	mmio_write_64(CCN_HNF_BASE + 0x30008, snf0);

	mmio_write_64(CCN_HNF_BASE + 0x40008, snf1);
	mmio_write_64(CCN_HNF_BASE + 0x50008, snf1);
	mmio_write_64(CCN_HNF_BASE + 0x60008, snf1);
	mmio_write_64(CCN_HNF_BASE + 0x70008, snf1);

	dsbish();
	isb();
}

void ccn_xp_set_qos(const unsigned int xp,
		    const unsigned int dev,
		    const unsigned int qos)
{
	uint64_t val;

	val = mmio_read_64(CCN_XP_DEV_QOS_CONTROL(xp, dev));
	val &= ~CCN_XP_DEV_QOS_OVERRIDE_MASK;
	val |=	CCN_XP_DEV_QOS_OVERRIDE(qos) | CCN_XP_DEV_QOS_OVERRIDE_EN;
	mmio_write_64(CCN_XP_DEV_QOS_CONTROL(xp, dev), val);
}
