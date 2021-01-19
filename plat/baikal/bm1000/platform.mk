#
# Copyright (c) 2018-2020, Baikal Electronics JSC. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include lib/libfdt/libfdt.mk

USE_COHERENT_MEM	:=	1

PLAT_INCLUDES		:=	-Iinclude/plat/arm/common/aarch64	\
				-Iplat/baikal/bm1000/drivers		\
				-Iplat/baikal/bm1000/include		\
				-Iplat/baikal/common/drivers		\
				-Iplat/baikal/common/include

# Consider that the platform may release several CPUs out of reset.
COLD_BOOT_SINGLE_CPU	:=	0

ifeq ($(NEED_BL32),yes)
$(eval $(call add_define,BAIKAL_LOAD_BL32))
endif

ifeq ($(BE_HAPS),yes)
$(eval $(call add_define,BE_HAPS))
endif

SPI_DRIVER_SOURCES	:=	plat/baikal/bm1000/drivers/spi_test.c

ifeq ($(BE_TARGET),bfkm)
$(eval $(call add_define,BE_DBM))
SPI_DRIVER_SOURCES	+=	plat/baikal/common/drivers/spi_dw_boot.c
else ifeq ($(BE_TARGET),dbm)
$(eval $(call add_define,BE_DBM))
SPI_DRIVER_SOURCES	+=	plat/baikal/common/drivers/spi_dw_boot.c
else ifeq ($(BE_TARGET),mitx)
$(eval $(call add_define,BE_MITX))
SPI_DRIVER_SOURCES	+=	plat/baikal/common/drivers/spi_scp_boot.c
else ifeq ($(BE_TARGET),qemu)
$(eval $(call add_define,BE_QEMU))
SPI_DRIVER_SOURCES	+=	plat/baikal/common/drivers/spi_dw_boot.c
else
$(error "Error: Unknown BE_TARGET: ${BE_TARGET}.")
endif

ifeq ($(SEMIHOST),yes)
SEMIHOST_SOURCES	:=	drivers/io/io_semihosting.c	\
				lib/semihosting/semihosting.c	\
				lib/semihosting/aarch64/semihosting_call.S
$(eval $(call add_define,SEMIHOST))
endif

ifeq ($(BE_BL1_TST0),yes)
BE_BL1_TESTS		:=	yes
$(eval $(call add_define,BE_BL1_TST0))
endif

ifeq ($(BE_BL1_TST1),yes)
BE_BL1_TESTS		:=	yes
$(eval $(call add_define,BE_BL1_TST1))
endif

ifeq ($(BE_BL1_TESTS),yes)
PLAT_INCLUDES		+=	-Iplat/baikal/bm1000/tests
BE_BL1_TESTS_SOURCES	:=
BE_BL1_TESTS_SOURCES	+=	plat/baikal/bm1000/bm1000_mmlsp.c
BE_BL1_TESTS_SOURCES	+=	plat/baikal/bm1000/bm1000_mmusb.c
BE_BL1_TESTS_SOURCES	+=	plat/baikal/bm1000/bm1000_mmxgbe.c
BE_BL1_TESTS_SOURCES	+=	plat/baikal/bm1000/bm1000_mmmali.c
BE_BL1_TESTS_SOURCES	+=	plat/baikal/bm1000/bm1000_mmpci.c
BE_BL1_TESTS_SOURCES	+=	plat/baikal/bm1000/bm1000_mmvdec.c
BE_BL1_TESTS_SOURCES	+=	plat/baikal/bm1000/tests/mmprep.c
endif

ifeq ($(BE_VCS_CONSOLE),yes)
$(eval $(call add_define,BE_VCS))
$(eval $(call add_define,BE_VCS_TIMESTAMP_ON_LF))
BE_CONSOLE_SOURCES	:=	plat/baikal/bm1000/drivers/vcs_debug_serial.S
else
BE_CONSOLE_SOURCES	:=	drivers/ti/uart/aarch64/16550_console.S
endif

# Use translation tables library v2 by default
ARM_XLAT_TABLES_LIB_V1	:=	0
$(eval $(call assert_boolean,ARM_XLAT_TABLES_LIB_V1))
$(eval $(call add_define,ARM_XLAT_TABLES_LIB_V1))

PLAT_BL_COMMON_SOURCES	:=	plat/baikal/bm1000/bm1000_common.c		\
				plat/baikal/common/aarch64/plat_helpers.S	\
				plat/baikal/common/baikal_console.c		\
				${BE_CONSOLE_SOURCES}				\
				${SPI_DRIVER_SOURCES}

ifeq (${ARM_XLAT_TABLES_LIB_V1}, 1)
PLAT_BL_COMMON_SOURCES	+=	lib/xlat_tables/xlat_tables_common.c	\
				lib/xlat_tables/aarch64/xlat_tables.c
else
include lib/xlat_tables_v2/xlat_tables.mk
PLAT_BL_COMMON_SOURCES	+=	${XLAT_TABLES_LIB_SRCS}
endif

BL1_SOURCES		+=	drivers/arm/ccn/ccn.c				\
				drivers/io/io_dummy.c				\
				drivers/io/io_fip.c				\
				drivers/io/io_memmap.c				\
				drivers/io/io_storage.c				\
				lib/cpus/aarch64/cortex_a57.S			\
				plat/arm/common/arm_ccn.c			\
				plat/baikal/bm1000/bm1000_bl1_logo.c		\
				plat/baikal/bm1000/bm1000_bl1_setup.c		\
				plat/baikal/bm1000/bm1000_macaddr.c		\
				plat/baikal/bm1000/bm1000_mmxgbe.c		\
				plat/baikal/bm1000/bm1000_splash.c		\
				plat/baikal/bm1000/drivers/bm1000_gpio.c	\
				plat/baikal/bm1000/drivers/bm1000_scp.c		\
				plat/baikal/common/aarch64/plat_bl1_helpers.S	\
				plat/baikal/common/baikal_io_storage.c		\
				plat/baikal/common/crc32.c			\
				${BE_BL1_TESTS_SOURCES}				\
				${SEMIHOST_SOURCES}

override BL1_LINKERFILE	:=	plat/baikal/common/bl1.ld.S

BL2_SOURCES		+=	common/desc_image_load.c			\
				drivers/io/io_dummy.c				\
				drivers/io/io_fip.c				\
				drivers/io/io_memmap.c				\
				drivers/io/io_storage.c				\
				plat/baikal/bm1000/bm1000_bl2_setup.c		\
				plat/baikal/bm1000/drivers/bm1000_i2c.c		\
				plat/baikal/bm1000/drivers/bm1000_scp.c		\
				plat/baikal/bm1000/drivers/bm1000_smbus.c	\
				plat/baikal/bm1000/dt.c				\
				plat/baikal/common/baikal_bl2_mem_params_desc.c	\
				plat/baikal/common/baikal_image_load.c		\
				plat/baikal/common/baikal_io_storage.c		\
				plat/baikal/common/crc16.c			\
				plat/baikal/common/spd.c			\
				$(LIBFDT_SRCS)					\
				${SEMIHOST_SOURCES}

ifeq (${SPD},opteed)
BL2_SOURCES		+=	lib/optee/optee_utils.c
endif

include drivers/arm/gic/v3/gicv3.mk

BL31_SOURCES		+=	drivers/arm/ccn/ccn.c				\
				lib/cpus/aarch64/aem_generic.S			\
				lib/cpus/aarch64/cortex_a57.S			\
				plat/arm/common/arm_ccn.c			\
				plat/baikal/bm1000/bm1000_bl31_logo.c		\
				plat/baikal/bm1000/bm1000_bl31_sdk_version_logo.c		\
				plat/baikal/bm1000/bm1000_bl31_setup.c		\
				plat/baikal/bm1000/bm1000_macaddr.c		\
				plat/baikal/bm1000/bm1000_mmlsp.c		\
				plat/baikal/bm1000/bm1000_mmmali.c		\
				plat/baikal/bm1000/bm1000_mmpci.c		\
				plat/baikal/bm1000/bm1000_mmusb.c		\
				plat/baikal/bm1000/bm1000_mmvdec.c		\
				plat/baikal/bm1000/bm1000_mmxgbe.c		\
				plat/baikal/bm1000/bm1000_pm.c			\
				plat/baikal/bm1000/bm1000_sip_svc.c		\
				plat/baikal/bm1000/bm1000_splash.c		\
				plat/baikal/bm1000/bm1000_topology.c		\
				plat/baikal/bm1000/drivers/bm1000_ca57_lcru.c	\
				plat/baikal/bm1000/drivers/bm1000_cmu.c		\
				plat/baikal/bm1000/drivers/bm1000_gpio.c	\
				plat/baikal/bm1000/drivers/bm1000_pvt.c		\
				plat/baikal/bm1000/drivers/bm1000_scp.c		\
				plat/baikal/bm1000/drivers/bm1000_smmu.c	\
				plat/baikal/bm1000/drivers/bm1000_vdu.c		\
				plat/baikal/common/baikal_gicv3.c		\
				plat/baikal/common/baikal_sip_svc_flash.c	\
				plat/baikal/common/crc32.c			\
				plat/common/plat_gicv3.c			\
				plat/common/plat_psci_common.c			\
				${GICV3_SOURCES}				\
				$(LIBFDT_SRCS)

ifeq ($(BE_TARGET),mitx)
BL31_SOURCES		+=	plat/baikal/bm1000/drivers/bm1000_i2c.c
endif

ifeq ($(notdir $(CC)),armclang)
TF_CFLAGS_aarch64	+=	-mcpu=cortex-a57
else ifneq ($(findstring clang,$(notdir $(CC))),)
TF_CFLAGS_aarch64	+=	-mcpu=cortex-a57
else
TF_CFLAGS_aarch64	+=	-mtune=cortex-a57
endif

# Add the build options to pack Trusted OS Extra1 and Trusted OS Extra2 images
# in the FIP if the platform requires.
ifneq ($(BL32_EXTRA1),)
$(eval $(call FIP_ADD_IMG,BL32_EXTRA1,--tos-fw-extra1))
endif
ifneq ($(BL32_EXTRA2),)
$(eval $(call FIP_ADD_IMG,BL32_EXTRA2,--tos-fw-extra2))
endif

BL32_RAM_LOCATION	:=	tdram
ifeq (${BL32_RAM_LOCATION}, tsram)
BL32_RAM_LOCATION_ID = SEC_SRAM_ID
else ifeq (${BL32_RAM_LOCATION}, tdram)
BL32_RAM_LOCATION_ID = SEC_DRAM_ID
else
$(error "Unsupported BL32_RAM_LOCATION value")
endif

# Process flags
$(eval $(call add_define,BL32_RAM_LOCATION_ID))

# Errata workarounds for Cortex-A57 (our r1p3):
ERRATA_A57_806969	:=	0
ERRATA_A57_813419	:=	0
ERRATA_A57_813420	:=	0
ERRATA_A57_826974	:=	0
ERRATA_A57_826977	:=	0
ERRATA_A57_828024	:=	0
ERRATA_A57_829520	:=	0
ERRATA_A57_833471	:=	0
ERRATA_A57_859972	:=	1
