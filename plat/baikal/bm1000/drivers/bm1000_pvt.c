#include <arch_helpers.h>
#include <bm1000_private.h>
#include <bm1000_pvt.h>
#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <errno.h>
#include <lib/mmio.h>
#include <platform_def.h>

uint32_t get_pvt_addr_by_id(uint32_t pvt_id)
{
	switch(pvt_id) {
		case 0:
			return A57_0_PVTCC_ADDR;
		case 1:
			return A57_1_PVTCC_ADDR;
		case 2:
			return A57_2_PVTCC_ADDR;
		case 3:
			return A57_3_PVTCC_ADDR;
		case 4:
			return MALI_PVTCC_ADDR;
		default:
			return 1;
	}
	return 1;
}

uint32_t pvt_read_reg(uint32_t pvt_id, uint32_t offset)
{
	uint32_t reg;

	if ((offset & ~PVT_AREA_MASK) != 0)
		return 1;
	/* Ensure that offset in pvt address range */
	offset &= PVT_AREA_MASK;
	reg = get_pvt_addr_by_id(pvt_id);
	return mmio_read_32(reg + offset);
}

uint32_t pvt_write_reg(uint32_t pvt_id, uint32_t offset, uint32_t val)
{
	uint32_t reg;

	if ((offset & ~PVT_AREA_MASK) != 0)
		return 1;
	/* Ensure that offset in pvt address range */
	offset &= PVT_AREA_MASK;
	reg = get_pvt_addr_by_id(pvt_id);
	mmio_write_32(reg + offset, val);
	return 0;
}
