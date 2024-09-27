/*
 * Copyright (c) 2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DBM_BMC_H
#define DBM_BMC_H

#define DBM_BMC_CMD_WAKE_SRC_PWR	BIT(0)
#define DBM_BMC_CMD_WAKE_SRC_TIMER	BIT(1)
#define DBM_BMC_CMD_WAKE_SRC_KEY	BIT(2)
#define DBM_BMC_CMD_WAKE_SRC_USB	BIT(3)
#define DBM_BMC_CMD_WAKE_SRC_LAN	BIT(4)
#define DBM_BMC_CMD_WAKE_SRC_WAN	BIT(5)
#define DBM_BMC_CMD_WAKE_SRC_PCIE	BIT(6)

int dbm_bmc_pwr_get_wake_src(void);
void dbm_bmc_emu_pwr_off(uint32_t ms);
void dbm_bmc_pwr_off(uint32_t ms);
void dbm_bmc_pwr_rst(uint32_t ms);

#endif /* DBM_BMC_H */
