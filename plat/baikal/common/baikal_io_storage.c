/*
 * Copyright (c) 2018-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <assert.h>
#include <baikal_def.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <drivers/io/io_block.h>
#include <drivers/io/io_driver.h>
#include <drivers/io/io_fip.h>
#include <drivers/io/io_memmap.h>
#include <drivers/io/io_storage.h>
#include <endian.h>
#include <lib/utils_def.h>
#include <libfdt.h>
#include <platform_def.h>
#include <spi_dw.h>
#include <string.h>
#include <tools_share/firmware_image_package.h>

/* IO devices */
static const io_dev_connector_t *fip_dev_con;
static uintptr_t fip_dev_handle;
static const io_dev_connector_t *memmap_dev_con;
static uintptr_t memmap_dev_handle;

static const io_block_spec_t fip_block_spec = {
	.offset = PLAT_SEC_FIP_BASE,
	.length = PLAT_SEC_FIP_MAX_SIZE
};

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

#if TRUSTED_BOARD_BOOT
static const io_uuid_spec_t bl2_cert_uuid_spec = {
	.uuid = UUID_TRUSTED_BOOT_FIRMWARE_BL2_CERT,
};

static const io_uuid_spec_t trusted_key_cert_uuid_spec = {
	.uuid = UUID_TRUSTED_KEY_CERT,
};

static const io_uuid_spec_t bl31_key_cert_uuid_spec = {
	.uuid = UUID_EL3_RUNTIME_FIRMWARE_BL31_KEY_CERT,
};

static const io_uuid_spec_t bl32_key_cert_uuid_spec = {
	.uuid = UUID_SECURE_PAYLOAD_BL32_KEY_CERT,
};

static const io_uuid_spec_t bl33_key_cert_uuid_spec = {
	.uuid = UUID_NON_TRUSTED_FIRMWARE_BL33_KEY_CERT,
};

static const io_uuid_spec_t bl31_cert_uuid_spec = {
	.uuid = UUID_EL3_RUNTIME_FIRMWARE_BL31_CERT,
};

static const io_uuid_spec_t bl32_cert_uuid_spec = {
	.uuid = UUID_SECURE_PAYLOAD_BL32_CERT,
};

static const io_uuid_spec_t bl33_cert_uuid_spec = {
	.uuid = UUID_NON_TRUSTED_FIRMWARE_BL33_CERT,
};
#endif /* TRUSTED_BOARD_BOOT */


static int open_fip(const uintptr_t spec);
static int open_memmap(const uintptr_t spec);

struct plat_io_policy {
	uintptr_t *dev_handle;
	uintptr_t image_spec;
	int (*check)(const uintptr_t spec);
};

/* By default, ARM platforms load images from the FIP */
static const struct plat_io_policy policies[] = {
	[FIP_IMAGE_ID] = {
		&memmap_dev_handle,
		(uintptr_t)&fip_block_spec,
		open_memmap
	},
	[BL2_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl2_uuid_spec,
		open_fip
	},
	[BL31_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl31_uuid_spec,
		open_fip
	},
	[BL32_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl32_uuid_spec,
		open_fip
	},
	[BL32_EXTRA1_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl32_extra1_uuid_spec,
		open_fip
	},
	[BL32_EXTRA2_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl32_extra2_uuid_spec,
		open_fip
	},
	[BL33_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl33_uuid_spec,
		open_fip
	},
#if TRUSTED_BOARD_BOOT
	[BL2_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl2_cert_uuid_spec,
		open_fip
	},
	[TRUSTED_KEY_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&trusted_key_cert_uuid_spec,
		open_fip
	},
	[BL31_KEY_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl31_key_cert_uuid_spec,
		open_fip
	},
	[BL32_KEY_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl32_key_cert_uuid_spec,
		open_fip
	},
	[BL33_KEY_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl33_key_cert_uuid_spec,
		open_fip
	},
	[BL31_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl31_cert_uuid_spec,
		open_fip
	},
	[BL32_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl32_cert_uuid_spec,
		open_fip
	},
	[BL33_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl33_cert_uuid_spec,
		open_fip
	},
#endif /* TRUSTED_BOARD_BOOT */
};

static int open_fip(const uintptr_t spec)
{
	int result;
	uintptr_t local_image_handle;

	/* See if a Firmware Image Package is available */
	result = io_dev_init(fip_dev_handle, (uintptr_t)FIP_IMAGE_ID);
	if (result == 0) {
		result = io_open(fip_dev_handle, spec, &local_image_handle);
		if (result == 0) {
			io_close(local_image_handle);
			return 0;
		}
	}
	return local_image_handle;
}

/* Return 0 for equal uuids. */
static inline int compare_uuids(const uuid_t *uuid1, const uuid_t *uuid2)
{
	return memcmp(uuid1, uuid2, sizeof(uuid_t));
}

static const uuid_t uuid_null = {0};

static int read_fip_from_flash(uintptr_t local_image_handle, uint32_t offset)
{
	fip_toc_entry_t entry = {0};
	int result, size = 0;
	size_t bytes_read;
	uint8_t *addr = (uint8_t *)PLAT_SEC_FIP_BASE;
	int src = offset;

	/* Read FIP Header part */
	result = io_seek(local_image_handle, IO_SEEK_SET, sizeof(fip_toc_header_t));
	if (result != 0) {
		WARN("fip_file_open: failed to seek\n");
		result = -ENOENT;
	}

	result = dw_spi_read(0, src, addr, sizeof(fip_toc_header_t));
	flush_dcache_range((uintptr_t)addr, sizeof(fip_toc_header_t));
	if (result != 0) {
		WARN("dw_spi_read: Failed to read fip header\n");
		result = -ENOENT;
	}
	src  += sizeof(fip_toc_header_t);
	addr += sizeof(fip_toc_header_t);

	do {
		result = dw_spi_read(0, src, addr, sizeof(fip_toc_entry_t));
		flush_dcache_range((uintptr_t)addr, sizeof(fip_toc_entry_t));
		if (result != 0) {
			WARN("dw_spi_read: Failed to read fip file header\n");
			result = -ENOENT;
		}

		result = io_read(local_image_handle,
				 (uintptr_t)&entry,
				 sizeof(entry),
				 &bytes_read);
		if (result != 0) {
			WARN("Failed to read FIP (%i)\n", result);
		}
		src  += sizeof(fip_toc_entry_t);
		addr += sizeof(fip_toc_entry_t);

		size += entry.size;

	} while (compare_uuids(&entry.uuid, &uuid_null) != 0);

	result = dw_spi_read(0, src, addr, size);
	flush_dcache_range((uintptr_t)addr, size);
	if (result != 0) {
		WARN("dw_spi_read: Failed to read fip\n");
		result = -ENOENT;
	}

	return result;
}

static int read_fdt_from_flash(uintptr_t local_image_handle, uint32_t offset)
{
	uint8_t *dst = (void *)PLAT_BAIKAL_DT_BASE;
	int err;
	const struct fdt_header *const fdt_header = (void *)dst;
	uint32_t fdt_totalsize;

	err = dw_spi_read(0, offset, dst, sizeof(struct fdt_header));
	flush_dcache_range((uintptr_t)dst, sizeof(struct fdt_header));
	if (err) {
		WARN("%s: failed to read FDT header\n", __func__);
		return -ENOENT;
	}

	if (be32toh(fdt_header->magic) != FDT_MAGIC) {
		WARN("%s: FDT header magic is wrong\n", __func__);
		return -ENOENT;
	}

	fdt_totalsize = be32toh(fdt_header->totalsize);
	if (fdt_totalsize < sizeof(struct fdt_header) ||
	    fdt_totalsize > 0x10000) { /* 64 KiB for FDT */
		WARN("%s: FDT total size is wrong: %u\n", __func__, fdt_totalsize);
		return -ENOENT;
	}

	dst    += sizeof(struct fdt_header);
	offset += sizeof(struct fdt_header);
	err = dw_spi_read(0, offset, dst, fdt_totalsize - sizeof(struct fdt_header));
	flush_dcache_range((uintptr_t)dst, fdt_totalsize - sizeof(struct fdt_header));
	if (err) {
		WARN("%s: failed to read FDT\n", __func__);
		return -ENOENT;
	}

	return 0;
}

static int read_flash(uintptr_t local_image_handle)
{
	int result = 0;

	result |= read_fdt_from_flash(local_image_handle, BAIKAL_FLASH_MAP_FDT);
	result |= read_fip_from_flash(local_image_handle, BAIKAL_FLASH_MAP_FIP);

	return result;
}

static inline int is_valid_header(fip_toc_header_t *header)
{
	if ((header->name == TOC_HEADER_NAME) && (header->serial_number != 0)) {
		return 1;
	} else {
		return 0;
	}
}

static int open_memmap(const uintptr_t spec)
{
	int result;
	uintptr_t local_image_handle;

	result = io_dev_init(memmap_dev_handle, (uintptr_t)NULL);

	if (result == 0) {
		result = io_open(memmap_dev_handle, spec, &local_image_handle);
		if (result == 0) {
#ifndef IMAGE_BL1
			if (!(is_valid_header((fip_toc_header_t *)PLAT_SEC_FIP_BASE))) {
				result = read_flash(local_image_handle);
			}
#else
			result = read_flash(local_image_handle);
#endif
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

	io_result = register_io_dev_memmap(&memmap_dev_con);
	assert(io_result == 0);

	/* Open connections to devices and cache the handles */
	io_result = io_dev_open(fip_dev_con, (uintptr_t)NULL,
				&fip_dev_handle);
	assert(io_result == 0);

	io_result = io_dev_open(memmap_dev_con, (uintptr_t)NULL,
				&memmap_dev_handle);
	assert(io_result == 0);

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
	int result;
	const struct plat_io_policy *policy;

	assert(image_id < ARRAY_SIZE(policies));

	policy = &policies[image_id];
	result = policy->check(policy->image_spec);
	if (result == 0) {
		*image_spec = policy->image_spec;
		*dev_handle = *(policy->dev_handle);
	}

	return result;
}
