/*
 * Copyright (c) 2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DDR_MASTER_H
#define DDR_MASTER_H

#include <stdint.h>

#include "ddr_main.h"
#include "ddr_spd.h"

struct ctl_content {
	uint32_t MSTR_;
	uint32_t SCHED_;
	uint32_t SCHED1_;
	uint32_t CRCPARCTL0_;
	uint32_t CRCPARCTL1_;
	uint32_t DBICTL_;
	uint32_t DIMMCTL_;
	uint32_t PCCFG_;

	uint32_t INIT0_;
	uint32_t INIT3_;
	uint32_t INIT4_;
	uint32_t INIT6_;
	uint32_t INIT7_;

	uint32_t RANKCTL_;

	uint32_t DFITMG0_;
	uint32_t DFITMG1_;
	uint32_t DFIUPD0_;
	uint32_t DFIUPD2_;

	uint32_t DRAMTMG0_;
	uint32_t DRAMTMG1_;
	uint32_t DRAMTMG2_;
	uint32_t DRAMTMG3_;
	uint32_t DRAMTMG4_;
	uint32_t DRAMTMG5_;
	uint32_t DRAMTMG8_;
	uint32_t DRAMTMG9_;
	uint32_t DRAMTMG11_;
	uint32_t DRAMTMG12_;

	uint32_t ADDRMAP0_;
	uint32_t ADDRMAP1_;
	uint32_t ADDRMAP2_;
	uint32_t ADDRMAP3_;
	uint32_t ADDRMAP4_;
	uint32_t ADDRMAP5_;
	uint32_t ADDRMAP6_;
	uint32_t ADDRMAP7_;
	uint32_t ADDRMAP8_;

	uint32_t SARBASE0_;
	uint32_t SARSIZE0_;
	uint32_t SARBASE1_;
	uint32_t SARSIZE1_;
	uint32_t SARBASE2_;
	uint32_t SARSIZE2_;

	uint32_t RFSHTMG_;

	uint32_t ODTCFG_;
	uint32_t ODTMAP_;

	uint32_t DQMAP0_;
	uint32_t DQMAP1_;
	uint32_t DQMAP2_;
	uint32_t DQMAP3_;
	uint32_t DQMAP4_;
	uint32_t DQMAP5_;

	uint32_t ZQCTL0_;
	uint32_t ECCCFG0_;
	uint32_t ECCCFG1_;
};

struct phy_content {
	uint32_t DCR_;
	uint32_t PGCR1_;
	uint32_t PGCR2_;
	uint32_t PGCR4_;
	uint32_t PGCR6_;
	uint32_t PLLCR_;
	uint32_t DX8GCR0_;
	uint32_t PTR1_;
	uint32_t PTR3_;
	uint32_t PTR4_;
	uint32_t DTPR0_;
	uint32_t DTPR1_;
	uint32_t DTPR2_;
	uint32_t DTPR3_;
	uint32_t DTPR4_;
	uint32_t DTPR5_;
	uint32_t MR0_;
	uint32_t MR1_;
	uint32_t MR2_;
	uint32_t MR3_;
	uint32_t MR4_;
	uint32_t MR5_;
	uint32_t MR6_;
	uint32_t RDIMMGCR0_;
	uint32_t RDIMMGCR1_;
	uint32_t RDIMMGCR2_;
	uint32_t RDIMMCR0_;
	uint32_t RDIMMCR1_;
	uint32_t RDIMMCR2_;
	uint32_t DTCR0_;
	uint32_t DTCR1_;
	uint32_t DSGCR_;
	uint32_t ZQCR_;
	unsigned int dbus_half;
};

void phy_set_default_vals(struct phy_content *phy);
void ddrc_set_default_vals(struct ctl_content *ddrc);
int phy_config_content(struct phy_content *phy,
			const struct ddr_configuration *data);
int ddrc_config_content(struct ctl_content *ddrc,
			const struct ddr_configuration *data);
int ddr_config_by_spd(int port, struct ddr_configuration *data);

#endif /* DDR_MASTER_H */
