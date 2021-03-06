/*
 * Copyright (c) 2020-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <bm1000_i2c.h>
#include <stdint.h>

struct i2c_regs {
	uint32_t ic_con;
	uint32_t ic_tar;
	uint32_t ic_sar;
	uint32_t ic_hs_maddr;
	uint32_t ic_data_cmd;
	uint32_t ic_ss_scl_hcnt;
	uint32_t ic_ss_scl_lcnt;
	uint32_t ic_fs_scl_hcnt;
	uint32_t ic_fs_scl_lcnt;
	uint32_t ic_hs_scl_hcnt;
	uint32_t ic_hs_scl_lcnt;
	uint32_t ic_intr_stat;
	uint32_t ic_intr_mask;
	uint32_t ic_raw_intr_stat;
	uint32_t ic_rx_tl;
	uint32_t ic_tx_tl;
	uint32_t ic_clr_intr;
	uint32_t ic_clr_rx_under;
	uint32_t ic_clr_rx_over;
	uint32_t ic_clr_tx_over;
	uint32_t ic_clr_rd_req;
	uint32_t ic_clr_tx_abrt;
	uint32_t ic_clr_rx_done;
	uint32_t ic_clr_activity;
	uint32_t ic_clr_stop_det;
	uint32_t ic_clr_start_det;
	uint32_t ic_clr_gen_call;
	uint32_t ic_enable;
	uint32_t ic_status;
	uint32_t ic_txflr;
	uint32_t ic_rxflr;
	uint32_t ic_sda_hold;
	uint32_t ic_tx_abrt_source;
	uint32_t reserved0;
	uint32_t ic_dma_cr;
	uint32_t ic_dma_tdlr;
	uint32_t ic_dma_rdlr;
	uint32_t ic_sda_setup;
	uint32_t ic_ack_general_call;
	uint32_t ic_enable_status;
	uint32_t ic_fs_spklen;
	uint32_t ic_hs_spklen;
	uint32_t reserved1[19];
	uint32_t ic_comp_param1;
	uint32_t ic_comp_version;
	uint32_t ic_comp_type;
};

#define IC_SPEED_MODE_STANDARD		1
#define IC_SPEED_MODE_FAST		2
#define IC_SPEED_MODE_MAX		3

#define IC_CON_MASTER_MODE		BIT(0)
#define IC_CON_SPEED			(IC_SPEED_MODE_FAST << 1)
#define IC_CON_IC_RESTART_EN		BIT(5)
#define IC_CON_IC_SLAVE_DISABLE		BIT(6)

#define IC_DATA_CMD_CMD			BIT(8)

#define IC_RAW_INTR_STAT_TX_ABRT	BIT(6)
#define IC_RAW_INTR_STAT_STOP_DET	BIT(9)

#define IC_ENABLE_ENABLE		BIT(0)

#define IC_STATUS_TFNF			BIT(1)
#define IC_STATUS_TFE			BIT(2)
#define IC_STATUS_RFNE			BIT(3)
#define IC_STATUS_RFF			BIT(4)
#define IC_STATUS_MST_ACTIVITY		BIT(5)

#define IC_CLK				166
#define NANO_TO_MICRO			1000
#define MIN_FS_SCL_HIGHTIME		600
#define MIN_FS_SCL_LOWTIME		1300

int i2c_txrx(const uintptr_t base,
	     const unsigned addr,
	     const void *const txbuf,
	     const unsigned txbufsize,
	     void *const rxbuf,
	     const unsigned rxbufsize)
{
	int err;
	volatile struct i2c_regs *const i2cregs = (volatile struct i2c_regs *const)base;
	unsigned rxedsize = 0;
	uint8_t *const rxptr = (uint8_t *)rxbuf;
	unsigned txedsize = 0;
	const uint8_t *const txptr = (uint8_t *)txbuf;

	assert(i2cregs != NULL);
	assert(addr <= 0x7f);
	assert(txbuf != NULL || !txbufsize);
	assert(rxbuf != NULL || !rxbufsize);

	i2cregs->ic_enable = 0;
	i2cregs->ic_con = IC_CON_IC_SLAVE_DISABLE | IC_CON_SPEED | IC_CON_MASTER_MODE;
	i2cregs->ic_tar = addr;
	i2cregs->ic_rx_tl = 0;
	i2cregs->ic_tx_tl = 0;
	i2cregs->ic_intr_mask = 0;
	i2cregs->ic_fs_scl_hcnt = (IC_CLK * MIN_FS_SCL_HIGHTIME) / NANO_TO_MICRO;
	i2cregs->ic_fs_scl_lcnt = (IC_CLK * MIN_FS_SCL_LOWTIME)  / NANO_TO_MICRO;
	i2cregs->ic_enable = IC_ENABLE_ENABLE;

	for (;;) {
		const unsigned ic_status = i2cregs->ic_status;

		if (rxedsize < rxbufsize && (ic_status & IC_STATUS_RFNE)) {
			rxptr[rxedsize++] = i2cregs->ic_data_cmd;
			continue;
		}

		if (i2cregs->ic_raw_intr_stat & IC_RAW_INTR_STAT_TX_ABRT) {
			err = -1;
			break;
		} else if (txedsize < txbufsize + rxbufsize) {
			if (ic_status & IC_STATUS_TFNF) {
				if (txedsize < txbufsize) {
					i2cregs->ic_data_cmd = txptr[txedsize];
				} else {
					i2cregs->ic_data_cmd = IC_DATA_CMD_CMD;
				}
				++txedsize;
			} else if (!(ic_status & IC_STATUS_MST_ACTIVITY)) {
				err = -1;
				break;
			}
		} else if ( (ic_status & IC_STATUS_TFE) &&
			   !(ic_status & IC_STATUS_MST_ACTIVITY)) {
			err = 0;
			break;
		}
	}

	i2cregs->ic_enable = 0;

	if (err) {
		return -1;
	}

	return rxedsize;
}
