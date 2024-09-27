/*
 * Copyright (c) 2018-2024, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <arch_helpers.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <drivers/io/io_block.h>
#include <drivers/io/io_driver.h>
#include <drivers/io/io_fip.h>
#include <drivers/io/io_mtd.h>
#include <drivers/io/io_storage.h>
#include <drivers/mmc.h>
#include <lib/utils_def.h>
#include <tools_share/firmware_image_package.h>

#include <baikal_bootflash.h>
#include <baikal_def.h>
#include <platform_def.h>

#ifdef BAIKAL_MMC_FIP
static const io_dev_connector_t *mmc_dev_con;
static uintptr_t mmc_dev_handle;

static uint8_t block_buffer[MMC_BLOCK_SIZE] __aligned(MMC_BLOCK_SIZE);

static const io_block_dev_spec_t mmc_dev_spec = {
	.buffer	= {
		.offset	= (size_t)&block_buffer,
		.length	= sizeof(block_buffer),
	},
	.ops = {
		.read = mmc_read_blocks,
	},
	.block_size = MMC_BLOCK_SIZE,
};

static const io_block_spec_t mmc_fip_block_spec = {
	.offset = BAIKAL_MMC_FIP_NVBASE,
	.length = BAIKAL_MMC_FIP_MAX_SIZE
};

static int open_mmc(const uintptr_t spec)
{
	int result;
	uintptr_t local_image_handle;

	result = io_dev_init(mmc_dev_handle, (uintptr_t)NULL);
	if (result == 0) {
		result = io_open(mmc_dev_handle, spec, &local_image_handle);
		if (result == 0) {
			io_close(local_image_handle);
		}
	}
	return result;
}
#endif

#ifdef BAIKAL_SPI_FIP
static const io_dev_connector_t *spi_dev_con;
static uintptr_t spi_dev_handle;

static int spi_nor_init(unsigned long long *size, unsigned int *erase_size)
{
#ifdef IMAGE_BL1
	/*
	 * BL1 calls 'bootflash_init()' in 'bl1_early_platform_setup()':
	 * just before DRAM initialization.
	 */
	return 0;
#else
	return bootflash_init();
#endif
}

static int spi_nor_read(unsigned int offset, uintptr_t buffer, size_t length, size_t *out_length)
{
	*out_length = length;

	return bootflash_read(offset, (void *)buffer, length);
}

static io_mtd_dev_spec_t spi_nor_dev_spec = {
	.device_size = 32 * 1024 * 1024,
	.ops = {
		.init = spi_nor_init,
		.read = spi_nor_read,
	},
};

static const io_block_spec_t spi_fip_block_spec = {
	.offset = BAIKAL_SPI_FIP_NVBASE,
	.length = BAIKAL_SPI_FIP_MAX_SIZE
};

static int open_spi(const uintptr_t spec)
{
	int result;
	uintptr_t local_image_handle;

	result = io_dev_init(spi_dev_handle, (uintptr_t)NULL);
	if (result == 0) {
		result = io_open(spi_dev_handle, spec, &local_image_handle);
		if (result == 0) {
			io_close(local_image_handle);
		}
	}
	return result;
}
#endif

static const io_uuid_spec_t bl2_uuid_spec = {
	.uuid = UUID_TRUSTED_BOOT_FIRMWARE_BL2,
};

static const io_uuid_spec_t bl31_uuid_spec = {
	.uuid = UUID_EL3_RUNTIME_FIRMWARE_BL31,
};

static const io_uuid_spec_t bl32_uuid_spec = {
	.uuid = UUID_SECURE_PAYLOAD_BL32,
};

static const io_uuid_spec_t bl32_extra1_uuid_spec = {
	.uuid = UUID_SECURE_PAYLOAD_BL32_EXTRA1,
};

static const io_uuid_spec_t bl32_extra2_uuid_spec = {
	.uuid = UUID_SECURE_PAYLOAD_BL32_EXTRA2,
};

static const io_uuid_spec_t bl33_uuid_spec = {
	.uuid = UUID_NON_TRUSTED_FIRMWARE_BL33,
};

static const io_uuid_spec_t hw_config_uuid_spec = {
	.uuid = UUID_HW_CONFIG,
};

static const io_dev_connector_t *fip_dev_con;
static uintptr_t fip_dev_handle;
static enum fip_source_type {
	FIP_SOURCE_NONE,
	FIP_SOURCE_MMC,
	FIP_SOURCE_SPI
} fip_source;

static int open_fip(const uintptr_t spec)
{
	int result;
	uintptr_t local_image_handle;

	if (fip_source == FIP_SOURCE_NONE) {
#if defined(BAIKAL_MMC_FIP)
		INFO("Try reading MMC FIP\n");
		fip_source = FIP_SOURCE_MMC;
#elif defined(BAIKAL_SPI_FIP)
		INFO("Try reading SPI FIP\n");
		fip_source = FIP_SOURCE_SPI;
#else
		return -ENOENT;
#endif
	}

	/* See if a Firmware Image Package is available */
	result = io_dev_init(fip_dev_handle, (uintptr_t)FIP_IMAGE_ID);
#if defined(BAIKAL_MMC_FIP) && defined(BAIKAL_SPI_FIP)
	if (result != 0 && fip_source == FIP_SOURCE_MMC) {
		INFO("Try reading SPI FIP\n");
		fip_source = FIP_SOURCE_SPI;
		result = io_dev_init(fip_dev_handle, (uintptr_t)FIP_IMAGE_ID);
	}
#endif
	if (result == 0) {
		result = io_open(fip_dev_handle, spec, &local_image_handle);
		if (result == 0) {
			io_close(local_image_handle);
		}
	}
	return result;
}

void plat_baikal_io_setup(void)
{
	int io_result;

	io_result = register_io_dev_fip(&fip_dev_con);
	assert(io_result == 0);
#ifdef BAIKAL_MMC_FIP
	io_result = register_io_dev_block(&mmc_dev_con);
	assert(io_result == 0);
#endif
#ifdef BAIKAL_SPI_FIP
	io_result = register_io_dev_mtd(&spi_dev_con);
	assert(io_result == 0);
#endif
	/* Open connections to devices and cache the handles */
	io_result = io_dev_open(fip_dev_con, (uintptr_t)NULL,
				&fip_dev_handle);
	assert(io_result == 0);
#ifdef BAIKAL_MMC_FIP
	io_result = io_dev_open(mmc_dev_con, (uintptr_t)&mmc_dev_spec,
				&mmc_dev_handle);
	assert(io_result == 0);
#endif
#ifdef BAIKAL_SPI_FIP
	io_result = io_dev_open(spi_dev_con, (uintptr_t)&spi_nor_dev_spec,
				&spi_dev_handle);
	assert(io_result == 0);
#endif
	/* Ignore improbable errors in release builds */
	(void)io_result;
}

/*
 * Return an IO device handle and specification which can be used to access
 * an image. Use this to enforce platform load policy
 */
int plat_get_image_source(unsigned int image_id, uintptr_t *dev_handle,
			  uintptr_t *image_spec)
{
	uintptr_t checked_image_spec;
	int result = -ENOENT;

	switch (image_id) {
	case FIP_IMAGE_ID:
#ifdef BAIKAL_MMC_FIP
		if (fip_source == FIP_SOURCE_MMC) {
			checked_image_spec = (uintptr_t)&mmc_fip_block_spec;
			result = open_mmc(checked_image_spec);
			if (result == 0) {
				*image_spec = checked_image_spec;
				*dev_handle = mmc_dev_handle;
				return result;
			}
		}
#endif
#ifdef BAIKAL_SPI_FIP
		if (fip_source == FIP_SOURCE_SPI) {
			checked_image_spec = (uintptr_t)&spi_fip_block_spec;
			result = open_spi(checked_image_spec);
			if (result == 0) {
				*image_spec = checked_image_spec;
				*dev_handle = spi_dev_handle;
				return result;
			}
		}
#endif
		return result;
	case BL2_IMAGE_ID:
		checked_image_spec = (uintptr_t)&bl2_uuid_spec;
		break;
	case BL31_IMAGE_ID:
		checked_image_spec = (uintptr_t)&bl31_uuid_spec;
		break;
	case BL32_IMAGE_ID:
		checked_image_spec = (uintptr_t)&bl32_uuid_spec;
		break;
	case BL32_EXTRA1_IMAGE_ID:
		checked_image_spec = (uintptr_t)&bl32_extra1_uuid_spec;
		break;
	case BL32_EXTRA2_IMAGE_ID:
		checked_image_spec = (uintptr_t)&bl32_extra2_uuid_spec;
		break;
	case BL33_IMAGE_ID:
		checked_image_spec = (uintptr_t)&bl33_uuid_spec;
		break;
	case HW_CONFIG_ID:
		checked_image_spec = (uintptr_t)&hw_config_uuid_spec;
		break;
	default:
		return result;
	}

	result = open_fip(checked_image_spec);
	if (result == 0) {
		*image_spec = checked_image_spec;
		*dev_handle = fip_dev_handle;
	}

	return result;
}
