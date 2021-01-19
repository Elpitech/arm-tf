#ifndef __BAIKAL_SCP_H
#define __BAIKAL_SCP_H

#include <platform_def.h>

#pragma pack(1)
struct ScpService {
	uint32_t	req_id;
	uint32_t	exec_id;
	uint8_t		op;
	uint8_t		st;
	uint16_t	b_size;
	uint32_t	f_offset;
	uint32_t	buff[0];
};
#pragma pack()

uint32_t next_req_id(void);
int scp_busy(void);
int scp_cmd(uint8_t op, uint32_t adr, size_t size);

#endif
