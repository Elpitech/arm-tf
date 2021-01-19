#ifndef __BAIKAL_PVT_H__
#define __BAIKAL_PVT_H__

#include <lib/mmio.h>
#include <stdbool.h>

#define PVT_READ		0
#define PVT_WRITE		1

uint32_t pvt_read_reg(uint32_t pvt_id, uint32_t offset);
uint32_t pvt_write_reg(uint32_t pvt_id, uint32_t offset, uint32_t val);

#endif /* BAIKAL_PVT_H */
