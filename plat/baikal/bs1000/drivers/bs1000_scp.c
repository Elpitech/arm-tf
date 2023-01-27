/*
 * Copyright (c) 2021-2022, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>

#include <arch_helpers.h>
#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>

#include <baikal_def.h>
#include <baikal_scp.h>
#include <platform_def.h>

#pragma pack(1)
struct scp_service {
	uint32_t	req_id;
	uint32_t	exec_id;
	uint8_t		op;
	uint8_t		st;
	uint16_t	ret;
	uint32_t	to_scp[2];
	uint32_t	from_scp[1];
	uint32_t	reserved[2];
};
#pragma pack()

void *scp_buf(void)
{
	volatile struct scp_service *const scp =
		(volatile struct scp_service *const)SCP_SERVICE_BASE;

	return (void *)scp->from_scp;
}

static bool scp_busy(void)
{
	volatile struct scp_service *const scp =
		(volatile struct scp_service *const)SCP_SERVICE_BASE;

	return mmio_read_32(MAILBOX_IRB0_SCP2AP_STATUS) ||
		scp->req_id != scp->exec_id ||
		(scp->st != 'E' && scp->st != 'G');
}

int scp_cmd(uint8_t op, uint32_t arg0, uint32_t arg1)
{
	volatile struct scp_service *const scp =
		(volatile struct scp_service *const)SCP_SERVICE_BASE;

	static uint32_t id = 1;
	char s_op[2] = {op, 0};
	uint64_t timeout;

	if (scp_busy()) {
		WARN("%s %s busy\n", __func__, s_op);
		return -EBUSY;
	}

	switch (op) {
	case 'V':
		scp->op = op;
		break;
	case 'R':
	case 'W':
		scp->op = op;
		scp->to_scp[0] = arg0;
		scp->to_scp[1] = arg1;
		break;
	default:
		WARN("%s %s unsupported\n", __func__, s_op);
		return -EFAULT;
	}

	scp->req_id = ++id;

	dmbst();

	mmio_write_32(MAILBOX_IRB0_AP2SCP_SET, 1); /* send signal */

	timeout = timeout_init_us(1000000);
	while (scp_busy()) {
		if (timeout_elapsed(timeout)) {
			WARN("%s %s timeout\n", __func__, s_op);
			return -ETIMEDOUT;
		}
	}

	dmbsy();

	if (scp->st == 'G') {
		return 0;
	}

	WARN("%s %s failed\n", __func__, s_op);
	return -EFAULT;
}
