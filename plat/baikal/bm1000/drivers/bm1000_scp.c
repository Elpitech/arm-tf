/*
 * Copyright (c) 2018-2020, Baikal Electronics JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <assert.h>
#include <baikal_scp.h>
#include <bm1000_private.h>
#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <errno.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>
#include <platform_def.h>
#include <spi_dw.h>
#include <string.h>

static	uint32_t current_req_id = 1;
uint32_t next_req_id(void) {
	return ++current_req_id;
}

int scp_busy(void) {
	volatile struct ScpService *s_scp = (volatile struct ScpService *)SCP_SERVICE_BASE;
	return	((s_scp->req_id != s_scp->exec_id ||
		 (s_scp->st != 'E' && s_scp->st != 'G')) ||
		 (mmio_read_32(SCP2AP_STATUS_R(0)) != 0));

}

int scp_cmd(uint8_t op, uint32_t adr, size_t size) {
	volatile struct ScpService *s_scp = (volatile struct ScpService *)SCP_SERVICE_BASE;
	char	s_op[2] = {op, 0};
	int	ok = 0;
//	INFO("op %s adr %x, %lx\n", s_op, adr, size);
	s_scp->op = op; // Read
	s_scp->b_size = size;
	s_scp->f_offset = adr + 512*1024;
	s_scp->req_id = next_req_id();
	mmio_write_32(AP2SCP_SET_R(0), 1);	// Send signal
	WAIT_DELAY(
		((s_scp->req_id != s_scp->exec_id ||
		 (s_scp->st != 'E' && s_scp->st != 'G')) ||
		 (mmio_read_32(SCP2AP_STATUS_R(0)) != 0)
		 ),
		10000000000, // 10s
		ok = 1
		);
	if(ok == 0 && s_scp->st == 'G') {
		return 0;
	} else {
		if(ok) {
			ERROR("SPI %s timeout (%x, %lx)\n", s_op, adr, size);
		} else {
			ERROR("SPI %s fault (%x, %lx)\n", s_op, adr, size);
			ok = 2;
		}
	}
	return ok;
}
