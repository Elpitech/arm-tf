/** @file  baikal_splash.h

 Copyright (C) 2020 Baikal Electronics JSC

 Author: Pavel Parkhomenko <Pavel.Parkhomenko@baikalelectronics.ru>

 **/

#ifndef __BAIKAL_SPLASH_H
#define __BAIKAL_SPLASH_H

#include <stdint.h>

typedef enum {
	BAIKAL_LVDS_JEIDA_18,
	BAIKAL_LVDS_VESA_24
} lvds_data_mapping_t;

typedef struct {
	uint32_t clock;
	int ports;
	lvds_data_mapping_t data_mapping;
	uint32_t hact;
	uint32_t hfp;
	uint32_t hsync;
	uint32_t hbp;
	uint32_t vact;
	uint32_t vfp;
	uint32_t vsync;
	uint32_t vbp;
} modeline_t;

extern modeline_t hdmi_video_mode;
extern modeline_t *lvds_video_mode;

typedef struct {
	// 2-byte signature is omitted (placed just before file_size)
	uint32_t file_size;
	uint32_t reserved;
	uint32_t pixel_offset;
	uint32_t dib_header_size;
	uint32_t width;
	uint32_t height;
	uint16_t planes : 16;
	uint16_t bpp : 16;
} bmp_header_t;

void hdmi_early_splash(uint8_t *bmp_file);
void bmp_get_dimensions(uint8_t *bmp_file, int *width, int *height);
int bmp_to_fb(uintptr_t fb, modeline_t *mode, uint8_t *bmp_file, int dx, int dy, int clear_screen);
void vdu_init(uint64_t vdu_base, uint32_t fb_base, modeline_t *mode);
void vdu_set_fb(uint64_t vdu_base, uint32_t fb_base, modeline_t *mode, int fb_cpp);
void wait_for_vblank(uint64_t vdu_base);
int fdt_get_panel(modeline_t *modeline);

#endif /* __BAIKAL_SPLASH_H */
