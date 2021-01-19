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
#include <string.h>

int baikal_vdu_update(uint64_t baddr, uint64_t eaddr)
{
	volatile struct ScpService *s_scp = (volatile struct ScpService *)SCP_SERVICE_BASE;
	if(scp_busy()) return -EBUSY;
//	INFO("op %s adr %x, %lx\n", s_op, adr, size);
	s_scp->op = 'V'; // VDU update
	s_scp->b_size = 0;
	s_scp->f_offset = 0;
	s_scp->buff[0] = baddr;
	s_scp->buff[1] = eaddr;
	s_scp->req_id = next_req_id();
	mmio_write_32(AP2SCP_SET_R(0), 1);	// Send signal
	return  0;
}


