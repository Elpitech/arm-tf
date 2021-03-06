#
# Copyright (c) 2018-2021, Baikal Electronics, JSC. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include lib/libfdt/libfdt.mk
include lib/xlat_tables_v2/xlat_tables.mk

USE_COHERENT_MEM	:=	1

PLAT_INCLUDES		:=	-Iinclude/plat/arm/common/aarch64	\
				-Iplat/baikal/bm1000/drivers		\
				-Iplat/baikal/bm1000/drivers/ddr	\
				-Iplat/baikal/bm1000/include		\
				-Iplat/baikal/common/include

ifeq ($(NEED_BL32),yes)
$(eval $(call add_define,BAIKAL_LOAD_BL32))
endif

ifeq ($(BAIKAL_TARGET),dbm)
$(eval $(call add_define,BAIKAL_DBM))
SPI_DRIVER_SOURCES	:=	plat/baikal/common/spi_dw_boot.c
else ifeq ($(BAIKAL_TARGET),mbm)
$(eval $(call add_define,BAIKAL_MBM))
$(eval $(call add_define_val,BOARD_VER,0))
SPI_DRIVER_SOURCES	:=	plat/baikal/common/spi_scp_boot.c
else ifeq ($(BAIKAL_TARGET),mbm_dual_flash)
$(eval $(call add_define,BAIKAL_MBM))
$(eval $(call add_define_val,BOARD_VER,2))
$(eval $(call add_define_val,BAIKAL_BOOT_SPI_CS_GPIO_PIN,15))
SPI_DRIVER_SOURCES	:=	plat/baikal/common/spi_dw_boot.c
else ifeq ($(BAIKAL_TARGET),mitx)
$(eval $(call add_define,BAIKAL_MBM))
$(eval $(call add_define,BOARD_VER))
ifneq ($(DUAL_FLASH),no)
$(eval $(call add_define_val,BAIKAL_BOOT_SPI_CS_GPIO_PIN,15))
SPI_DRIVER_SOURCES	:=	plat/baikal/common/spi_dw_boot.c
else
SPI_DRIVER_SOURCES	:=	plat/baikal/common/spi_scp_boot.c
endif
else ifeq ($(BAIKAL_TARGET),qemu)
$(eval $(call add_define,BAIKAL_QEMU))
SPI_DRIVER_SOURCES	:=	plat/baikal/common/spi_dw_boot.c
else
$(error "Error: unknown BAIKAL_TARGET=${BAIKAL_TARGET}")
endif

PLAT_BL_COMMON_SOURCES	:=	drivers/ti/uart/aarch64/16550_console.S		\
				plat/baikal/bm1000/bm1000_common.c		\
				plat/baikal/bm1000/drivers/bm1000_scp.c		\
				plat/baikal/common/aarch64/plat_helpers.S	\
				plat/baikal/common/baikal_console.c		\
				plat/baikal/common/baikal_gpio32.c		\
				${SPI_DRIVER_SOURCES}				\
				${XLAT_TABLES_LIB_SRCS}

BL1_SOURCES		+=	drivers/arm/ccn/ccn.c				\
				drivers/delay_timer/delay_timer.c		\
				drivers/delay_timer/generic_delay_timer.c	\
				drivers/io/io_dummy.c				\
				drivers/io/io_fip.c				\
				drivers/io/io_memmap.c				\
				drivers/io/io_storage.c				\
				lib/cpus/aarch64/cortex_a57.S			\
				plat/arm/common/arm_ccn.c			\
				plat/baikal/bm1000/bm1000_bl1_logo.c		\
				plat/baikal/bm1000/bm1000_bl1_setup.c		\
				plat/baikal/bm1000/bm1000_mmxgbe.c		\
				plat/baikal/bm1000/bm1000_splash.c		\
				plat/baikal/bm1000/drivers/bm1000_cmu.c		\
				plat/baikal/bm1000/drivers/bm1000_i2c.c		\
				plat/baikal/bm1000/drivers/bm1000_smbus.c	\
				plat/baikal/bm1000/drivers/ddr/ddr_init.c	\
				plat/baikal/bm1000/drivers/ddr/ddr_lcru.c	\
				plat/baikal/bm1000/drivers/ddr/ddr_main.c	\
				plat/baikal/bm1000/drivers/ddr/ddr_master.c	\
				plat/baikal/common/aarch64/plat_bl1_helpers.S	\
				plat/baikal/common/baikal_common.c		\
				plat/baikal/common/baikal_io_storage.c		\
				plat/baikal/common/crc16.c			\
				plat/baikal/common/memtest.c			\
				plat/baikal/common/ndelay.c
ifneq ($(BAIKAL_TARGET),qemu)
BL1_SOURCES		+=	plat/baikal/bm1000/drivers/ddr/ddr_odt_settings.c\
				plat/baikal/bm1000/drivers/ddr/ddr_spd.c
endif

override BL1_LINKERFILE	:=	plat/baikal/common/bl1.ld.S

BL2_SOURCES		+=	common/desc_image_load.c			\
				drivers/io/io_dummy.c				\
				drivers/io/io_fip.c				\
				drivers/io/io_memmap.c				\
				drivers/io/io_storage.c				\
				plat/baikal/bm1000/bm1000_bl2_setup.c		\
				plat/baikal/bm1000/drivers/bm1000_i2c.c		\
				plat/baikal/bm1000/drivers/bm1000_smbus.c	\
				plat/baikal/bm1000/dt.c				\
				plat/baikal/common/baikal_bl2_mem_params_desc.c	\
				plat/baikal/common/baikal_image_load.c		\
				plat/baikal/common/baikal_io_storage.c		\
				plat/baikal/common/crc16.c			\
				plat/baikal/common/spd.c			\
				$(LIBFDT_SRCS)

ifeq (${SPD},opteed)
BL2_SOURCES		+=	lib/optee/optee_utils.c
endif

include drivers/arm/gic/v3/gicv3.mk

BL31_SOURCES		+=	drivers/arm/ccn/ccn.c				\
				drivers/delay_timer/delay_timer.c		\
				drivers/delay_timer/generic_delay_timer.c	\
				lib/cpus/aarch64/aem_generic.S			\
				lib/cpus/aarch64/cortex_a57.S			\
				plat/arm/common/arm_ccn.c			\
				plat/baikal/bm1000/bm1000_bl31_logo.c		\
				plat/baikal/bm1000/bm1000_bl31_sdk_version_logo.c\
				plat/baikal/bm1000/bm1000_bl31_setup.c		\
				plat/baikal/bm1000/bm1000_mmavlsp.c		\
				plat/baikal/bm1000/bm1000_mmca57.c		\
				plat/baikal/bm1000/bm1000_mmmali.c		\
				plat/baikal/bm1000/bm1000_mmpcie.c		\
				plat/baikal/bm1000/bm1000_mmusb.c		\
				plat/baikal/bm1000/bm1000_mmvdec.c		\
				plat/baikal/bm1000/bm1000_mmxgbe.c		\
				plat/baikal/bm1000/bm1000_pm.c			\
				plat/baikal/bm1000/bm1000_sip_svc.c		\
				plat/baikal/bm1000/bm1000_splash.c		\
				plat/baikal/bm1000/bm1000_topology.c		\
				plat/baikal/bm1000/drivers/bm1000_cmu.c		\
				plat/baikal/bm1000/drivers/bm1000_i2c.c		\
				plat/baikal/bm1000/drivers/bm1000_pvt.c		\
				plat/baikal/bm1000/drivers/bm1000_smmu.c	\
				plat/baikal/bm1000/dt.c				\
				plat/baikal/common/baikal_bl31_setup.c		\
				plat/baikal/common/baikal_common.c		\
				plat/baikal/common/baikal_gicv3.c		\
				plat/baikal/common/baikal_sip_svc_flash.c	\
				plat/common/plat_gicv3.c			\
				plat/common/plat_psci_common.c			\
				${GICV3_SOURCES}				\
				$(LIBFDT_SRCS)

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

# Errata workarounds for Cortex-A57 (r1p3):
ERRATA_A57_806969	:=	0
ERRATA_A57_813419	:=	0
ERRATA_A57_813420	:=	0
ERRATA_A57_826974	:=	0
ERRATA_A57_826977	:=	0
ERRATA_A57_828024	:=	0
ERRATA_A57_829520	:=	0
ERRATA_A57_833471	:=	0
ERRATA_A57_859972	:=	1
