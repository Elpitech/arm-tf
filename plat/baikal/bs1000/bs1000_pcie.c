/*
 * Copyright (c) 2022, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <lib/mmio.h>
#include <libfdt.h>
#include <plat/common/platform.h>

#include <baikal_fdt.h>
#include <bs1000_def.h>

#include "bs1000_pcie.h"

#define PCIE_TYPE_RC	1
#define PCIE_TYPE_EP	2

void pcie_init(void)
{
	void *fdt = (void *)(uintptr_t)BAIKAL_SEC_DTB_BASE;
	unsigned idx;
	int node = -1;

	const uintptr_t pcie_dbis[14] = {
		PCIE0_P0_DBI_BASE,
		PCIE0_P1_DBI_BASE,
		PCIE1_P0_DBI_BASE,
		PCIE1_P1_DBI_BASE,
		PCIE2_P0_DBI_BASE,
		PCIE2_P1_DBI_BASE,
		PCIE3_P0_DBI_BASE,
		PCIE3_P1_DBI_BASE,
		PCIE3_P2_DBI_BASE,
		PCIE3_P3_DBI_BASE,
		PCIE4_P0_DBI_BASE,
		PCIE4_P1_DBI_BASE,
		PCIE4_P2_DBI_BASE,
		PCIE4_P3_DBI_BASE
	};

	const uintptr_t pcie_nic_cfg_ps[ARRAY_SIZE(pcie_dbis)] = {
		PCIE0_NIC_CFG_P0,
		PCIE0_NIC_CFG_P1,
		PCIE1_NIC_CFG_P0,
		PCIE1_NIC_CFG_P1,
		PCIE2_NIC_CFG_P0,
		PCIE2_NIC_CFG_P1,
		PCIE3_NIC_CFG_P0,
		PCIE3_NIC_CFG_P1,
		PCIE3_NIC_CFG_P2,
		PCIE3_NIC_CFG_P3,
		PCIE4_NIC_CFG_P0,
		PCIE4_NIC_CFG_P1,
		PCIE4_NIC_CFG_P2,
		PCIE4_NIC_CFG_P3
	};

	const uintptr_t pcie_nic_slv_ps[ARRAY_SIZE(pcie_dbis)] = {
		PCIE0_NIC_SLV_P0,
		PCIE0_NIC_SLV_P1,
		PCIE1_NIC_SLV_P0,
		PCIE1_NIC_SLV_P1,
		PCIE2_NIC_SLV_P0,
		PCIE2_NIC_SLV_P1,
		PCIE3_NIC_SLV_P0,
		PCIE3_NIC_SLV_P1,
		PCIE3_NIC_SLV_P2,
		PCIE3_NIC_SLV_P3,
		PCIE4_NIC_SLV_P0,
		PCIE4_NIC_SLV_P1,
		PCIE4_NIC_SLV_P2,
		PCIE4_NIC_SLV_P3
	};

	unsigned pcie_lanes[ARRAY_SIZE(pcie_dbis)];
	unsigned pcie_types[ARRAY_SIZE(pcie_dbis)];

	if (fdt_open_into(fdt, fdt, BAIKAL_DTB_MAX_SIZE)) {
		return;
	}

	memset(pcie_lanes, 0, sizeof(pcie_lanes));
	memset(pcie_types, 0, sizeof(pcie_types));

	/* Configure PCIe in accordance with FDT */
	for (;;) {
		bool endpoint;
		unsigned lanes = 0;
		const uint32_t *prop;
		int proplen;
		uint64_t reg;

		node = fdt_next_node(fdt, node, NULL);
		if (node < 0) {
			break;
		}

		if (!fdt_device_is_enabled(fdt, node)) {
			continue;
		}

		if (fdt_node_check_compatible(fdt, node, "baikal,bs1000-pcie") == 0) {
			endpoint = false;
		} else if (fdt_node_check_compatible(fdt, node, "baikal,bs1000-pcie-ep") == 0) {
			endpoint = true;
		} else {
			continue;
		}

		prop = fdt_getprop(fdt, node, "reg", &proplen);
		if (prop == NULL || proplen <= 0 || proplen % 16) {
			continue;
		}

		reg  = fdt32_to_cpu(prop[0]);
		reg <<= 32;
		reg |= fdt32_to_cpu(prop[1]);

		prop = fdt_getprop(fdt, node, "num-lanes", &proplen);
		if (prop != NULL && proplen == 4) {
			lanes = fdt32_to_cpu(prop[0]);
		}

		for (idx = 0; idx < ARRAY_SIZE(pcie_dbis); ++idx) {
			if (reg == pcie_dbis[idx]) {
				pcie_lanes[idx] = lanes;
				pcie_types[idx] = endpoint ? PCIE_TYPE_EP : PCIE_TYPE_RC;
				break;
			}
		}

		if (idx == ARRAY_SIZE(pcie_dbis)) {
			continue;
		}
	}

	for (idx = 0; idx < ARRAY_SIZE(pcie_dbis); ++idx) {
		if (pcie_types[idx] == 0) {
			continue;
		}

		/* Enable PCIe non-secure access */
		mmio_write_32(pcie_nic_cfg_ps[idx], NIC_GPV_REGIONSEC_NONSECURE);

		switch (pcie_dbis[idx]) {
		case PCIE0_P0_DBI_BASE:
		case PCIE0_P1_DBI_BASE:
			mmio_write_32(PCIE0_NIC_CFG_PHY, NIC_GPV_REGIONSEC_NONSECURE);
			break;
		case PCIE1_P0_DBI_BASE:
		case PCIE1_P1_DBI_BASE:
			mmio_write_32(PCIE1_NIC_CFG_PHY, NIC_GPV_REGIONSEC_NONSECURE);
			break;
		case PCIE2_P0_DBI_BASE:
		case PCIE2_P1_DBI_BASE:
			mmio_write_32(PCIE2_NIC_CFG_PHY, NIC_GPV_REGIONSEC_NONSECURE);
			break;
		case PCIE3_P0_DBI_BASE:
		case PCIE3_P1_DBI_BASE:
		case PCIE3_P2_DBI_BASE:
		case PCIE3_P3_DBI_BASE:
			mmio_write_32(PCIE3_NIC_CFG_PHY, NIC_GPV_REGIONSEC_NONSECURE);
			break;
		case PCIE4_P0_DBI_BASE:
		case PCIE4_P1_DBI_BASE:
		case PCIE4_P2_DBI_BASE:
		case PCIE4_P3_DBI_BASE:
			mmio_write_32(PCIE4_NIC_CFG_PHY, NIC_GPV_REGIONSEC_NONSECURE);
			break;
		}

		mmio_write_32(pcie_nic_slv_ps[idx], NIC_GPV_REGIONSEC_NONSECURE);

		/* Set PCIe device type */
		switch (pcie_dbis[idx]) {
		case PCIE0_P0_DBI_BASE:
			if (pcie_types[idx] == PCIE_TYPE_EP) {
				mmio_clrbits_32(PCIE0_GPR_SS_MODE_CTL,
						PCIE_GPR_SS_MODE_CTL_DEVTYPE_MASK);
			} else {
				mmio_clrsetbits_32(PCIE0_GPR_SS_MODE_CTL,
						   PCIE_GPR_SS_MODE_CTL_DEVTYPE_MASK,
						   0x40);
			}

			break;
		case PCIE1_P0_DBI_BASE:
			if (pcie_types[idx] == PCIE_TYPE_EP) {
				mmio_clrbits_32(PCIE1_GPR_SS_MODE_CTL,
						PCIE_GPR_SS_MODE_CTL_DEVTYPE_MASK);
			} else {
				mmio_clrsetbits_32(PCIE1_GPR_SS_MODE_CTL,
						   PCIE_GPR_SS_MODE_CTL_DEVTYPE_MASK,
						   0x40);
			}

			break;
		case PCIE2_P0_DBI_BASE:
			if (pcie_types[idx] == PCIE_TYPE_EP) {
				mmio_clrbits_32(PCIE2_GPR_SS_MODE_CTL,
						PCIE_GPR_SS_MODE_CTL_DEVTYPE_MASK);
			} else {
				mmio_clrsetbits_32(PCIE2_GPR_SS_MODE_CTL,
						   PCIE_GPR_SS_MODE_CTL_DEVTYPE_MASK,
						   0x40);
			}

			break;
		default:
			if (pcie_types[idx] == PCIE_TYPE_EP) {
				ERROR("pcie@0x%lx: incorrect device type\n", pcie_dbis[idx]);
				plat_panic_handler();
			}

			break;
		}
	}

	/* Set PCIe subsystem mode */
	if (pcie_types[1] > 0) {
		if (pcie_lanes[0] <= 8 &&
		    pcie_lanes[1] <= 8) {
			mmio_clrsetbits_32(PCIE0_GPR_SS_MODE_CTL,
					   PCIE_GPR_SS_MODE_CTL_SSMODE_MASK,
					   0x2);
		} else {
			ERROR("PCIe0: incorrect subsystem mode\n");
			plat_panic_handler();
		}
	} else if (pcie_types[0] > 0) {
		if (pcie_lanes[0] <= 16) {
			mmio_clrbits_32(PCIE0_GPR_SS_MODE_CTL,
					PCIE_GPR_SS_MODE_CTL_SSMODE_MASK);
		} else {
			ERROR("PCIe0: incorrect subsystem mode\n");
			plat_panic_handler();
		}
	}

	if (pcie_types[3] > 0) {
		if (pcie_lanes[2] <= 8 &&
		    pcie_lanes[3] <= 8) {
			mmio_clrsetbits_32(PCIE1_GPR_SS_MODE_CTL,
					   PCIE_GPR_SS_MODE_CTL_SSMODE_MASK,
					   0x2);
		} else {
			ERROR("PCIe1: incorrect subsystem mode\n");
			plat_panic_handler();
		}
	} else if (pcie_types[2] > 0) {
		if (pcie_lanes[2] <= 16) {
			mmio_clrbits_32(PCIE1_GPR_SS_MODE_CTL,
					PCIE_GPR_SS_MODE_CTL_SSMODE_MASK);
		} else {
			ERROR("PCIe1: incorrect subsystem mode\n");
			plat_panic_handler();
		}
	}

	if (pcie_types[5] > 0) {
		if (pcie_lanes[4] <= 8 &&
		    pcie_lanes[5] <= 8) {
			mmio_clrsetbits_32(PCIE2_GPR_SS_MODE_CTL,
					   PCIE_GPR_SS_MODE_CTL_SSMODE_MASK,
					   0x2);
		} else {
			ERROR("PCIe2: incorrect subsystem mode\n");
			plat_panic_handler();
		}
	} else if (pcie_types[4] > 0) {
		if (pcie_lanes[4] <= 16) {
			mmio_clrbits_32(PCIE2_GPR_SS_MODE_CTL,
					PCIE_GPR_SS_MODE_CTL_SSMODE_MASK);
		} else {
			ERROR("PCIe2: incorrect subsystem mode\n");
			plat_panic_handler();
		}
	}

	if (pcie_types[7] > 0) {
		if (pcie_lanes[6] <= 4 &&
		    pcie_lanes[7] <= 4 &&
		    pcie_lanes[8] <= 4 &&
		    pcie_lanes[9] <= 4) {
			mmio_clrsetbits_32(PCIE3_GPR_SS_MODE_CTL,
					   PCIE_GPR_SS_MODE_CTL_SSMODE_MASK,
					   0x2);
		} else {
			ERROR("PCIe3: incorrect subsystem mode\n");
		}
	} else if (pcie_types[9] > 0) {
		if (pcie_lanes[6] <= 8 &&
		    pcie_lanes[8] <= 4 &&
		    pcie_lanes[9] <= 4) {
			mmio_clrsetbits_32(PCIE3_GPR_SS_MODE_CTL,
					   PCIE_GPR_SS_MODE_CTL_SSMODE_MASK,
					   0x3);
		} else {
			ERROR("PCIe3: incorrect subsystem mode\n");
		}
	} else if (pcie_types[8] > 0) {
		if (pcie_lanes[6] <= 8 &&
		    pcie_lanes[8] <= 8) {
			mmio_clrsetbits_32(PCIE3_GPR_SS_MODE_CTL,
					   PCIE_GPR_SS_MODE_CTL_SSMODE_MASK,
					   0x1);
		} else {
			ERROR("PCIe3: incorrect subsystem mode\n");
		}
	} else if (pcie_types[6] > 0) {
		if (pcie_lanes[6] <= 16) {
			mmio_clrbits_32(PCIE3_GPR_SS_MODE_CTL,
					PCIE_GPR_SS_MODE_CTL_SSMODE_MASK);
		} else {
			ERROR("PCIe3: incorrect subsystem mode\n");
		}
	}

	if (pcie_types[11] > 0) {
		if (pcie_lanes[10] <= 4 &&
		    pcie_lanes[11] <= 4 &&
		    pcie_lanes[12] <= 4 &&
		    pcie_lanes[13] <= 4) {
			mmio_clrsetbits_32(PCIE4_GPR_SS_MODE_CTL,
					   PCIE_GPR_SS_MODE_CTL_SSMODE_MASK,
					   0x2);
		} else {
			ERROR("PCIe4: incorrect subsystem mode\n");
		}
	} else if (pcie_types[13] > 0) {
		if (pcie_lanes[10] <= 8 &&
		    pcie_lanes[12] <= 4 &&
		    pcie_lanes[13] <= 4) {
			mmio_clrsetbits_32(PCIE4_GPR_SS_MODE_CTL,
					   PCIE_GPR_SS_MODE_CTL_SSMODE_MASK,
					   0x3);
		} else {
			ERROR("PCIe4: incorrect subsystem mode\n");
		}
	} else if (pcie_types[12] > 0) {
		if (pcie_lanes[10] <= 8 &&
		    pcie_lanes[12] <= 8) {
			mmio_clrsetbits_32(PCIE4_GPR_SS_MODE_CTL,
					   PCIE_GPR_SS_MODE_CTL_SSMODE_MASK,
					   0x1);
		} else {
			ERROR("PCIe4: incorrect subsystem mode\n");
		}
	} else if (pcie_types[10] > 0) {
		if (pcie_lanes[10] <= 16) {
			mmio_clrbits_32(PCIE4_GPR_SS_MODE_CTL,
					PCIE_GPR_SS_MODE_CTL_SSMODE_MASK);
		} else {
			ERROR("PCIe4: incorrect subsystem mode\n");
		}
	}

	/* Assert power-up reset */
	for (idx = 0; idx < ARRAY_SIZE(pcie_dbis); ++idx) {
		if (pcie_types[idx] == 0) {
			continue;
		}

		switch (pcie_dbis[idx]) {
		case PCIE0_P0_DBI_BASE:
		case PCIE0_P1_DBI_BASE:
			mmio_setbits_32(PCIE0_GPR_PWRUP_RST_CTL, PCIE_GPR_PWRUP_RST_CTL_PWRUP_RST);
			break;
		case PCIE1_P0_DBI_BASE:
		case PCIE1_P1_DBI_BASE:
			mmio_setbits_32(PCIE1_GPR_PWRUP_RST_CTL, PCIE_GPR_PWRUP_RST_CTL_PWRUP_RST);
			break;
		case PCIE2_P0_DBI_BASE:
		case PCIE2_P1_DBI_BASE:
			mmio_setbits_32(PCIE2_GPR_PWRUP_RST_CTL, PCIE_GPR_PWRUP_RST_CTL_PWRUP_RST);
			break;
		case PCIE3_P0_DBI_BASE:
		case PCIE3_P1_DBI_BASE:
		case PCIE3_P2_DBI_BASE:
		case PCIE3_P3_DBI_BASE:
			mmio_setbits_32(PCIE3_GPR_PWRUP_RST_CTL, PCIE_GPR_PWRUP_RST_CTL_PWRUP_RST);
			break;
		case PCIE4_P0_DBI_BASE:
		case PCIE4_P1_DBI_BASE:
		case PCIE4_P2_DBI_BASE:
		case PCIE4_P3_DBI_BASE:
			mmio_setbits_32(PCIE4_GPR_PWRUP_RST_CTL, PCIE_GPR_PWRUP_RST_CTL_PWRUP_RST);
			break;
		}
	}

	/* Clear PERST */
	mmio_clrbits_32(PCIE0_GPR_PWRUP_RST_CTL,
			PCIE_GPR_PWRUP_RST_CTL_P0_PERST |
			PCIE_GPR_PWRUP_RST_CTL_P1_PERST);

	mmio_clrbits_32(PCIE1_GPR_PWRUP_RST_CTL,
			PCIE_GPR_PWRUP_RST_CTL_P0_PERST |
			PCIE_GPR_PWRUP_RST_CTL_P1_PERST);

	mmio_clrbits_32(PCIE2_GPR_PWRUP_RST_CTL,
			PCIE_GPR_PWRUP_RST_CTL_P0_PERST |
			PCIE_GPR_PWRUP_RST_CTL_P1_PERST);

	mmio_clrbits_32(PCIE3_GPR_PWRUP_RST_CTL,
			PCIE_GPR_PWRUP_RST_CTL_P0_PERST |
			PCIE_GPR_PWRUP_RST_CTL_P1_PERST |
			PCIE_GPR_PWRUP_RST_CTL_P2_PERST |
			PCIE_GPR_PWRUP_RST_CTL_P3_PERST);

	mmio_clrbits_32(PCIE4_GPR_PWRUP_RST_CTL,
			PCIE_GPR_PWRUP_RST_CTL_P0_PERST |
			PCIE_GPR_PWRUP_RST_CTL_P1_PERST |
			PCIE_GPR_PWRUP_RST_CTL_P2_PERST |
			PCIE_GPR_PWRUP_RST_CTL_P3_PERST);

	/* Change PERST source to the register value */
	mmio_clrbits_32(PCIE0_GPR_PWRUP_RST_CTL,
			PCIE_GPR_PWRUP_RST_CTL_P0_PERST_EN |
			PCIE_GPR_PWRUP_RST_CTL_P1_PERST_EN);

	mmio_clrbits_32(PCIE1_GPR_PWRUP_RST_CTL,
			PCIE_GPR_PWRUP_RST_CTL_P0_PERST_EN |
			PCIE_GPR_PWRUP_RST_CTL_P1_PERST_EN);

	mmio_clrbits_32(PCIE2_GPR_PWRUP_RST_CTL,
			PCIE_GPR_PWRUP_RST_CTL_P0_PERST_EN |
			PCIE_GPR_PWRUP_RST_CTL_P1_PERST_EN);

	mmio_clrbits_32(PCIE3_GPR_PWRUP_RST_CTL,
			PCIE_GPR_PWRUP_RST_CTL_P0_PERST_EN |
			PCIE_GPR_PWRUP_RST_CTL_P1_PERST_EN |
			PCIE_GPR_PWRUP_RST_CTL_P2_PERST_EN |
			PCIE_GPR_PWRUP_RST_CTL_P3_PERST_EN);

	mmio_clrbits_32(PCIE4_GPR_PWRUP_RST_CTL,
			PCIE_GPR_PWRUP_RST_CTL_P0_PERST_EN |
			PCIE_GPR_PWRUP_RST_CTL_P1_PERST_EN |
			PCIE_GPR_PWRUP_RST_CTL_P2_PERST_EN |
			PCIE_GPR_PWRUP_RST_CTL_P3_PERST_EN);

	/* Deassert power-up reset */
	for (idx = 0; idx < ARRAY_SIZE(pcie_dbis); ++idx) {
		if (pcie_types[idx] == 0) {
			continue;
		}

		switch (pcie_dbis[idx]) {
		case PCIE0_P0_DBI_BASE:
		case PCIE0_P1_DBI_BASE:
			mmio_clrbits_32(PCIE0_GPR_PWRUP_RST_CTL, PCIE_GPR_PWRUP_RST_CTL_PWRUP_RST);
			break;
		case PCIE1_P0_DBI_BASE:
		case PCIE1_P1_DBI_BASE:
			mmio_clrbits_32(PCIE1_GPR_PWRUP_RST_CTL, PCIE_GPR_PWRUP_RST_CTL_PWRUP_RST);
			break;
		case PCIE2_P0_DBI_BASE:
		case PCIE2_P1_DBI_BASE:
			mmio_clrbits_32(PCIE2_GPR_PWRUP_RST_CTL, PCIE_GPR_PWRUP_RST_CTL_PWRUP_RST);
			break;
		case PCIE3_P0_DBI_BASE:
		case PCIE3_P1_DBI_BASE:
		case PCIE3_P2_DBI_BASE:
		case PCIE3_P3_DBI_BASE:
			mmio_clrbits_32(PCIE3_GPR_PWRUP_RST_CTL, PCIE_GPR_PWRUP_RST_CTL_PWRUP_RST);
			break;
		case PCIE4_P0_DBI_BASE:
		case PCIE4_P1_DBI_BASE:
		case PCIE4_P2_DBI_BASE:
		case PCIE4_P3_DBI_BASE:
			mmio_clrbits_32(PCIE4_GPR_PWRUP_RST_CTL, PCIE_GPR_PWRUP_RST_CTL_PWRUP_RST);
			break;
		}
	}

	/* set up ITS TGTADDR */
	mmio_write_32(PCIE0_GPR_ITS0_TGTADDR_CTL, GIC_ITS0_TRANSLATER >> 16);
	mmio_write_32(PCIE0_GPR_ITS1_TGTADDR_CTL, GIC_ITS1_TRANSLATER >> 16);
	mmio_write_32(PCIE1_GPR_ITS0_TGTADDR_CTL, GIC_ITS2_TRANSLATER >> 16);
	mmio_write_32(PCIE1_GPR_ITS1_TGTADDR_CTL, GIC_ITS3_TRANSLATER >> 16);
	mmio_write_32(PCIE2_GPR_ITS0_TGTADDR_CTL, GIC_ITS4_TRANSLATER >> 16);
	mmio_write_32(PCIE2_GPR_ITS1_TGTADDR_CTL, GIC_ITS5_TRANSLATER >> 16);
	mmio_write_32(PCIE3_GPR_ITS0_TGTADDR_CTL, GIC_ITS6_TRANSLATER >> 16);
	mmio_write_32(PCIE3_GPR_ITS1_TGTADDR_CTL, GIC_ITS7_TRANSLATER >> 16);
	mmio_write_32(PCIE3_GPR_ITS2_TGTADDR_CTL, GIC_ITS8_TRANSLATER >> 16);
	mmio_write_32(PCIE3_GPR_ITS3_TGTADDR_CTL, GIC_ITS9_TRANSLATER >> 16);
	mmio_write_32(PCIE3_GPR_ITS4_TGTADDR_CTL, GIC_ITS10_TRANSLATER >> 16);
	mmio_write_32(PCIE4_GPR_ITS0_TGTADDR_CTL, GIC_ITS11_TRANSLATER >> 16);
	mmio_write_32(PCIE4_GPR_ITS1_TGTADDR_CTL, GIC_ITS12_TRANSLATER >> 16);
	mmio_write_32(PCIE4_GPR_ITS2_TGTADDR_CTL, GIC_ITS13_TRANSLATER >> 16);
	mmio_write_32(PCIE4_GPR_ITS3_TGTADDR_CTL, GIC_ITS14_TRANSLATER >> 16);
	mmio_write_32(PCIE4_GPR_ITS4_TGTADDR_CTL, GIC_ITS15_TRANSLATER >> 16);
}
