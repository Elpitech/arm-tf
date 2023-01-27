/*
 * Copyright (c) 2020-2022, Baikal Electronics, JSC. All rights reserved.
 *
 * Author: Pavel Parkhomenko <pavel.parkhomenko@baikalelectronics.ru>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>
#include <libfdt.h>

#include <baikal_def.h>
#include <baikal_fdt.h>
#include <bm1000_private.h>
#include <dw_gpio.h>
#include <platform_def.h>

#include "bm1000_hdmi.h"
#include "bm1000_splash.h"
#include "bm1000_vdu.h"

#define BAIKAL_VDU_DEFAULT_BRIGHTNESS	0x7f  /* 50% duty cycle */
#define BAIKAL_VDU_DEFAULT_PWM_FREQ 	10000

modeline_t lvds_video_mode = {148500000, 2, BAIKAL_LVDS_VESA_24,
			      1920, 88, 44, 148,
			      1080, 4, 5, 36};
modeline_t hdmi_video_mode = {25250000, 0, 0, 640, 16, 96, 48, 480, 10, 2, 33};
int gpio_pin = -1;
int gpio_polarity = -1;

#define fdt_get_panel_timing(name, result) \
	do { \
		prop = fdt_getprop(fdt, node, name, &plen); \
		if (!prop) { \
			WARN("panel-timing: missing %s property\n", (name)); \
			return -1; \
		} \
		if (plen == sizeof(*prop)) { \
			result = fdt32_to_cpu(*prop); \
		} else if (plen == 3 * sizeof(*prop)) { \
			result = fdt32_to_cpu(*(prop + 1)); \
		} else { \
			WARN("panel-timing: illegal specification in %s\n", \
			     (name)); \
			return -1; \
		} \
	} while (0)

int fdt_get_panel(modeline_t *modeline)
{
	void *fdt = (void *)(uintptr_t)BAIKAL_SEC_DTB_BASE;
	int panel_found = 0;
	int gpio_found = 0;
	int node = 0;
	const fdt32_t *prop;
	int node_panel;
	int node_port;
	int plen;
	int ret;

	ret = fdt_open_into(fdt, fdt, BAIKAL_DTB_MAX_SIZE);
	if (ret < 0) {
		ERROR("Invalid Device Tree at %p: error %d\n", fdt, ret);
		return -1;
	}

	while (1) {
		if (panel_found && gpio_found) {
			return 0;
		}
		node = fdt_next_node(fdt, node, NULL);
		if (node < 0) {
			if (panel_found) {
				return 0;
			} else {
				return -1;
			}
		}

		if (!fdt_node_check_compatible(fdt, node, "panel-lvds")) {
			if (!fdt_device_is_enabled(fdt, node)) {
				WARN("panel-lvds: disabled\n");
				return -1;
			}

			node_panel = node;
			prop = fdt_getprop(fdt, node, "data-mapping", &plen);
			if (!prop) {
				WARN("panel-lvds: missing data-mapping property\n");
				return -1;
			}

			if (fdt_stringlist_contains((void *)prop, plen, "jeida-18")) {
				modeline->data_mapping = BAIKAL_LVDS_JEIDA_18;
			} else if (fdt_stringlist_contains((void *)prop, plen, "vesa-24")) {
				modeline->data_mapping = BAIKAL_LVDS_VESA_24;
			} else {
				WARN("panel-lvds: unsupported data mapping type\n");
				return -1;
			}

			node = fdt_subnode_offset(fdt, node, "panel-timing");
			if (node < 0) {
				WARN("panel-lvds: no panel-timing subnode\n");
				return -1;
			}

			fdt_get_panel_timing("clock-frequency", modeline->clock);
			fdt_get_panel_timing("hactive", modeline->hact);
			fdt_get_panel_timing("vactive", modeline->vact);
			fdt_get_panel_timing("hsync-len", modeline->hsync);
			fdt_get_panel_timing("hfront-porch", modeline->hfp);
			fdt_get_panel_timing("hback-porch", modeline->hbp);
			fdt_get_panel_timing("vsync-len", modeline->vsync);
			fdt_get_panel_timing("vfront-porch", modeline->vfp);
			fdt_get_panel_timing("vback-porch", modeline->vbp);

			node_port = fdt_subnode_offset(fdt, node_panel, "port");
			if (node_port < 0) {
				WARN("panel-lvds: no port subnode\n");
				return -1;
			}

			modeline->ports = 0;
			fdt_for_each_subnode(node, fdt, node_port) {
				if (!strncmp(fdt_get_name(fdt, node, NULL), "endpoint", strlen("endpoint"))) {
					modeline->ports++;
				}
			}

			if (modeline->ports == 0) {
				WARN("panel-lvds: port subnode has no endpoints\n");
				return -1;
			}

			panel_found = 1;
			node = node_panel;
			continue;
		}

		if (!fdt_node_check_compatible(fdt, node, "baikal,vdu")) {
			prop = fdt_getprop(fdt, node, "enable-gpios", &plen);
			if (prop && plen == 3 * sizeof(*prop)) {
				gpio_pin = fdt32_to_cpu(*(prop + 1));
				if (!fdt32_to_cpu(*(prop + 2)))
					gpio_polarity = 1;
				else
					gpio_polarity = 0;
				gpio_found = 1;
			}
		}
	}
}

void wait_for_vblank(uint64_t vdu_base)
{
	uint32_t irq;

	do {
		irq = mmio_read_32(vdu_base + BAIKAL_VDU_REG_ISR);
	} while (!(irq & BAIKAL_VDU_INTR_VCT));

	mmio_write_32(vdu_base + BAIKAL_VDU_REG_ISR, BAIKAL_VDU_INTR_VCT);

	do {
		irq = mmio_read_32(vdu_base + BAIKAL_VDU_REG_ISR);
	} while (!(irq & BAIKAL_VDU_INTR_VCT));
}

void vdu_set_fb(uint64_t vdu_base, uint32_t fb_base, modeline_t *mode, int fb_cpp)
{
	uint32_t ctl;
	uint32_t fb_end;
	uint32_t fb_size;

	if (!fb_cpp || fb_cpp > 4) {
		return;
	}

	fb_size = mode->hact * mode->vact * fb_cpp;
	fb_end = ((fb_base + fb_size - 1) & ~BAIKAL_VDU_MRR_DEAR_RQ_MASK) |
			BAIKAL_VDU_MRR_OUTSTND_RQ(4);

	/* Update BPP in CR1 */
	ctl = mmio_read_32(vdu_base + BAIKAL_VDU_REG_CR1);
	ctl &= ~BAIKAL_VDU_CR1_BPP_MASK;
	switch (fb_cpp) {
	case 2:
		ctl |= BAIKAL_VDU_CR1_BPP16;
		break;
	default: /* 3 or 4 */
		ctl |= BAIKAL_VDU_CR1_BPP24;
		break;
	}

	/* Set up DMAC */
	mmio_write_32(vdu_base + BAIKAL_VDU_REG_DBAR, fb_base);
	mmio_write_32(vdu_base + BAIKAL_VDU_REG_MRR, fb_end);
	mmio_write_32(vdu_base + BAIKAL_VDU_REG_CR1, ctl);
}

void vdu_init(uint64_t vdu_base, uint32_t fb_base, modeline_t *mode)
{
	uint32_t ctl, pwmfr, gpio, hfp = mode->hfp, hsw = mode->hsync;

	/* Set Timings */
	if (vdu_base == MMAVLSP_VDU_BASE && mode->ports == 2) {
		--hfp;
		++hsw;
	}

	mmio_write_32(
			vdu_base + BAIKAL_VDU_REG_HTR,
			HOR_AXIS_PANEL(
					mode->hbp,
					hfp,
					hsw,
					mode->hact
			)
	);

	mmio_write_32(vdu_base + BAIKAL_VDU_REG_VTR1,
		      VER_AXIS_PANEL(mode->vbp,
				     mode->vfp,
				     mode->vsync));

	mmio_write_32(vdu_base + BAIKAL_VDU_REG_VTR2, mode->vact);

	/* Disable all interrupts from the VDU */
	mmio_write_32(vdu_base + BAIKAL_VDU_REG_IMR, 0);

	/* Clear all interrupts */
	mmio_write_32(vdu_base + BAIKAL_VDU_REG_ISR, 0x3ffff);

	/* Set up VCT interrupt to detect VFP */
	mmio_write_32(vdu_base + BAIKAL_VDU_REG_ISCR, BAIKAL_VDU_ISCR_VSC_VFP);

	/* Read CR1, clear all bits except BPP (these were set before by vdu_set_fb()) */
	ctl = mmio_read_32(vdu_base + BAIKAL_VDU_REG_CR1) & BAIKAL_VDU_CR1_BPP_MASK;

	/* Set flags */
	ctl |= BAIKAL_VDU_CR1_LCE | BAIKAL_VDU_CR1_DEP | BAIKAL_VDU_CR1_FDW_16_WORDS;

	/* Set 18 or 24 bit output */
	if (vdu_base == MMAVLSP_VDU_BASE) { /* LVDS VDU */
		if (mode->data_mapping == BAIKAL_LVDS_JEIDA_18) {
			ctl |= BAIKAL_VDU_CR1_OPS_LCD18;
		} else { /* BAIKAL_LVDS_VESA_24 */
			ctl |= BAIKAL_VDU_CR1_OPS_LCD24;
		}

		/* Turn UHD Formatter off */
		mmio_write_32(vdu_base + BAIKAL_VDU_REG_GPIOR, 0);

		/* Hold PWM Clock Domain Reset, disable clocking */
		mmio_write_32(vdu_base + BAIKAL_VDU_REG_PWMFR, 0);

		pwmfr = BAIKAL_VDU_PWMFR_PWMFCD(mode->clock / BAIKAL_VDU_DEFAULT_PWM_FREQ - 1) | BAIKAL_VDU_PWMFR_PWMFCI;
		mmio_write_32(vdu_base + BAIKAL_VDU_REG_PWMFR, pwmfr);

		mmio_write_32(vdu_base + BAIKAL_VDU_REG_PWMDCR, BAIKAL_VDU_DEFAULT_BRIGHTNESS);

		/* Release PWM Clock Domain Reset, enable clocking */
		mmio_write_32(vdu_base + BAIKAL_VDU_REG_PWMFR, pwmfr | BAIKAL_VDU_PWMFR_PWMPCR | BAIKAL_VDU_PWMFR_PWMFCE);

		/* Enable backlight */
		if (gpio_polarity != -1) {
			if (gpio_polarity) {
				gpio_out_set(MMAVLSP_GPIO32_BASE, gpio_pin);
			} else {
				gpio_out_rst(MMAVLSP_GPIO32_BASE, gpio_pin);
			}
			gpio_dir_set(MMAVLSP_GPIO32_BASE, gpio_pin);
		}

	} else { /* HDMI VDU */
		ctl |= BAIKAL_VDU_CR1_OPS_LCD24;
	}

	/* Reset pixel clock domain */
	mmio_write_32(vdu_base + BAIKAL_VDU_REG_PCTR, BAIKAL_VDU_PCTR_PCI);
	mdelay(5);
	mmio_write_32(vdu_base + BAIKAL_VDU_REG_PCTR, BAIKAL_VDU_PCTR_PCR + BAIKAL_VDU_PCTR_PCI);

	/* Turn VDU on */
	mmio_write_32(vdu_base + BAIKAL_VDU_REG_CR1, ctl);

	/* Turn UHD Formatter on (LVDS only) */
	if (vdu_base == MMAVLSP_VDU_BASE) {
		gpio = BAIKAL_VDU_GPIOR_UHD_ENB;
		switch (mode->ports) {
		case 4:
			gpio |= BAIKAL_VDU_GPIOR_UHD_QUAD_PORT;
			break;
		case 2:
			gpio |= BAIKAL_VDU_GPIOR_UHD_DUAL_PORT;
			break;
		case 1:
			gpio |= BAIKAL_VDU_GPIOR_UHD_SNGL_PORT;
			break;
		}

		mmio_write_32(vdu_base + BAIKAL_VDU_REG_GPIOR, gpio);
	}

}

void hdmi_phy_power_on(void)
{
	uint32_t i, val;

	val = mmio_read_32(BAIKAL_HDMI_PHY_CONF0);
	val |= BAIKAL_HDMI_PHY_CONF0_GEN2_TXPWRON_MASK;
	val &= ~BAIKAL_HDMI_PHY_CONF0_GEN2_PDDQ_MASK;
	mmio_write_32(BAIKAL_HDMI_PHY_CONF0, val);

	/* Wait for PHY PLL lock */
	for (i = 0; i < 5; ++i) {
		val = mmio_read_32(BAIKAL_HDMI_PHY_STAT0) & BAIKAL_HDMI_PHY_TX_PHY_LOCK;
		if (val) {
			break;
		}

		mdelay(1);
	}
}

void hdmi_phy_configure(void)
{
	uint32_t val;

	/* Leave low power consumption mode by asserting SVSRET */
	val = mmio_read_32(BAIKAL_HDMI_PHY_CONF0);
	val |= BAIKAL_HDMI_PHY_CONF0_SPARECTRL_MASK;
	mmio_write_32(BAIKAL_HDMI_PHY_CONF0, val);

	/* PHY reset. The reset signal is active high on Gen2 PHYs */
	mmio_write_32(BAIKAL_HDMI_MC_PHYRSTZ, BAIKAL_HDMI_MC_PHYRSTZ_DEASSERT);
	mmio_write_32(BAIKAL_HDMI_MC_PHYRSTZ, 0);

	/* TODO start comment */
	val = mmio_read_32(BAIKAL_HDMI_MC_HEACPHY_RST);
	val |= BAIKAL_HDMI_MC_HEACPHY_RST_ASSERT;
	mmio_write_32(BAIKAL_HDMI_MC_HEACPHY_RST, val);

	val = mmio_read_32(BAIKAL_HDMI_PHY_TST0);
	val |= BAIKAL_HDMI_PHY_TST0_TSTCLR_MASK;
	mmio_write_32(BAIKAL_HDMI_PHY_TST0, val);

	val = mmio_read_32(BAIKAL_HDMI_PHY_TST0);
	val &= ~BAIKAL_HDMI_PHY_TST0_TSTCLR_MASK;
	mmio_write_32(BAIKAL_HDMI_PHY_TST0, val);
	/* TODO end comment */

	hdmi_phy_power_on();
}

void hdmi_init_av_composer(modeline_t *mode)
{
	uint32_t val, mask;

	val = mmio_read_32(BAIKAL_HDMI_TX_INVID0);
	val |= (1 << BAIKAL_HDMI_TX_INVID0_VIDEO_MAPPING_OFFSET);
	mmio_write_32(BAIKAL_HDMI_TX_INVID0, val);
	val = mmio_read_32(BAIKAL_HDMI_FC_INVIDCONF);
	mask = BAIKAL_HDMI_FC_INVIDCONF_VSYNC_IN_POLARITY_MASK |
	       BAIKAL_HDMI_FC_INVIDCONF_HSYNC_IN_POLARITY_MASK |
	       BAIKAL_HDMI_FC_INVIDCONF_DE_IN_POLARITY_MASK    |
	       BAIKAL_HDMI_FC_INVIDCONF_R_V_BLANK_IN_OSC_MASK  |
	       BAIKAL_HDMI_FC_INVIDCONF_IN_I_P_MASK;

	val &= ~mask;
	val |= BAIKAL_HDMI_FC_INVIDCONF_VSYNC_IN_POLARITY_ACTIVE_LOW |
	       BAIKAL_HDMI_FC_INVIDCONF_HSYNC_IN_POLARITY_ACTIVE_LOW |
	       BAIKAL_HDMI_FC_INVIDCONF_DE_IN_POLARITY_ACTIVE_HIGH   |
	       BAIKAL_HDMI_FC_INVIDCONF_R_V_BLANK_IN_OSC_ACTIVE_LOW  |
	       BAIKAL_HDMI_FC_INVIDCONF_IN_I_P_PROGRESSIVE;

	mmio_write_32(BAIKAL_HDMI_FC_INVIDCONF, val);
	mmio_write_32(BAIKAL_HDMI_FC_INHACTV1, mode->hact >> 8);
	mmio_write_32(BAIKAL_HDMI_FC_INHACTV0, mode->hact & 0xff);
	mmio_write_32(BAIKAL_HDMI_FC_INVACTV1, mode->vact >> 8);
	mmio_write_32(BAIKAL_HDMI_FC_INVACTV0, mode->vact & 0xff);
	mmio_write_32(BAIKAL_HDMI_FC_INHBLANK1, (mode->hfp + mode->hsync + mode->hbp) >> 8);
	mmio_write_32(BAIKAL_HDMI_FC_INHBLANK0, (mode->hfp + mode->hsync + mode->hbp) & 0xff);
	mmio_write_32(BAIKAL_HDMI_FC_INVBLANK,  (mode->vfp + mode->vsync + mode->vbp) & 0xff);
	mmio_write_32(BAIKAL_HDMI_FC_HSYNCINDELAY1, mode->hfp >> 8);
	mmio_write_32(BAIKAL_HDMI_FC_HSYNCINDELAY0, mode->hfp & 0xff);
	mmio_write_32(BAIKAL_HDMI_FC_VSYNCINDELAY,  mode->vfp & 0xff);
	mmio_write_32(BAIKAL_HDMI_FC_HSYNCINWIDTH1, mode->hsync >> 8);
	mmio_write_32(BAIKAL_HDMI_FC_HSYNCINWIDTH0, mode->hsync & 0xff);
	mmio_write_32(BAIKAL_HDMI_FC_VSYNCINWIDTH,  mode->vsync & 0xff);

	val = mmio_read_32(BAIKAL_HDMI_FC_INVIDCONF);
	val &= ~BAIKAL_HDMI_FC_INVIDCONF_DVI_MODEZ_MASK;
	val |= BAIKAL_HDMI_FC_INVIDCONF_DVI_MODEZ_DVI_MODE;
	mmio_write_32(BAIKAL_HDMI_FC_INVIDCONF, val);
}

void hdmi_enable_video_path(modeline_t *mode)
{
	uint32_t val;

	/* control period minimum duration */
	mmio_write_32(BAIKAL_HDMI_FC_CTRLDUR, 12);
	mmio_write_32(BAIKAL_HDMI_FC_EXCTRLDUR, 32);
	mmio_write_32(BAIKAL_HDMI_FC_EXCTRLSPAC, 1);

	/* Set to fill TMDS data channels */
	mmio_write_32(BAIKAL_HDMI_FC_CH0PREAM, 0x0b);
	mmio_write_32(BAIKAL_HDMI_FC_CH1PREAM, 0x16);
	mmio_write_32(BAIKAL_HDMI_FC_CH2PREAM, 0x21);

	/* Enable pixel clock and tmds data path */
	val = 0x7f;
	val &= ~BAIKAL_HDMI_MC_CLKDIS_PIXELCLK_DISABLE;
	mmio_write_32(BAIKAL_HDMI_MC_CLKDIS, val);

	val &= ~BAIKAL_HDMI_MC_CLKDIS_TMDSCLK_DISABLE;
	mmio_write_32(BAIKAL_HDMI_MC_CLKDIS, val);

	/*
	 * After each CLKDIS reset it is mandatory to
	 * set up VSYNC active edge delay (in lines).
	 */
	mmio_write_32(BAIKAL_HDMI_FC_VSYNCINWIDTH, (mode->vsync - mode->vfp) & 0xff);
}

void bmp_get_dimensions(uint8_t *bmp_file, int *width, int *height)
{
	bmp_header_t bmp;
	memcpy(&bmp, bmp_file + 2, sizeof(bmp)); /* skip 2-byte BMP signature */
	*width = bmp.width;
	*height = bmp.height;
}

int bmp_to_fb(uintptr_t fb, modeline_t *mode, uint8_t *bmp_file, int dx, int dy, int clear_screen)
{
	bmp_header_t bmp;
	int fb_cpp;
	int fb_width;
	int fb_height;
	int fb_size;
	int fb_line_width;
	int line_width;
	int x_offs;
	int y_offs;
	int data_size;
	uint8_t *last_line;
	uint8_t *first_line;
	uint8_t *src;
	uint8_t *dst;
	uint8_t *s;
	uint8_t *d;

	memcpy(&bmp, bmp_file + 2, sizeof(bmp)); /* skip 2-byte BMP signature */
	line_width = bmp.width * bmp.bpp / 8;
	if (line_width % 4 != 0) {
		line_width = (line_width / 4 + 1) * 4;
	}

	data_size = bmp.pixel_offset + bmp.height * line_width;
	if (data_size != bmp.file_size) {
		ERROR("BMP file is corrupt\n");
		return 0;
	}

	fb_width = mode->hact;
	fb_height = mode->vact;
	if (bmp.width > fb_width) {
		ERROR("BMP width exceed maximum (%d)\n", fb_width);
		return 0;
	}

	if (bmp.height > fb_height) {
		ERROR("BMP height exceed maximum (%d)\n", fb_height);
		return 0;
	}

	if ((bmp.bpp != 16 && bmp.bpp != 24 && bmp.bpp != 32) ||
		bmp.planes != 1) {
		ERROR("Only 16-, 24- and 32-bpp single-plane BMP formats are supported\n");
		return 0;
	}

	if (bmp.bpp == 16) {
		fb_cpp = 2;
	} else { /* bmp.bpp == 24 or 32 */
		fb_cpp = 4;
	}

	fb_line_width = fb_width * fb_cpp;
	fb_size = fb_line_width * fb_height;
	if (clear_screen) {
		memset((void *)fb, 0, fb_size);
	}

	x_offs = (fb_width - bmp.width) / 2 + dx;
	y_offs = (fb_height - bmp.height) / 2 + dy;
	last_line = bmp_file + bmp.pixel_offset;
	first_line = last_line + (bmp.height - 1) * line_width;
	dst = (void *)fb + y_offs * fb_line_width + x_offs * fb_cpp;
	for (src = first_line; src >= last_line; src -= line_width) {
		if (fb_cpp == 4 && bmp.bpp == 24) { /* convert 3-byte BMP to 4-byte FB */
			for (s = src, d = dst; s < src + line_width; s += 3, d += 4) {
				memcpy(d, s, 3);
			}
		} else {
			memcpy(dst, src, line_width);
		}

		dst += fb_line_width;
	}

	flush_dcache_range(fb, fb_size);
	return fb_cpp;
}

void hdmi_early_splash(uint8_t *bmp_file)
{
	int fb_cpp;
#ifdef BAIKAL_HDMI_CLKEN_GPIO_PIN
	gpio_out_rst(MMAVLSP_GPIO32_BASE, BAIKAL_HDMI_CLKEN_GPIO_PIN);
	gpio_dir_set(MMAVLSP_GPIO32_BASE, BAIKAL_HDMI_CLKEN_GPIO_PIN);
#endif
	hdmi_init_av_composer(&hdmi_video_mode);
	hdmi_phy_configure();
	hdmi_enable_video_path(&hdmi_video_mode);
	if (bmp_file) {
		fb_cpp = bmp_to_fb((uintptr_t)FB1_BASE, &hdmi_video_mode, bmp_file, 0, 0, 1);
		vdu_set_fb(MMXGBE_VDU_BASE, FB1_BASE, &hdmi_video_mode, fb_cpp);
	}
	vdu_init(MMXGBE_VDU_BASE, FB1_BASE, &hdmi_video_mode);
}
