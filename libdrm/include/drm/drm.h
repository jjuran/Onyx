/*
* Copyright (c) 2017 Pedro Falcato
* This file is part of Onyx, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef DRM_H_INCLUDED
#define DRM_H_INCLUDED
#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif

/* ioctl() requests supported by /dev/drm **MIGHT CHANGE WITH VERSION, DO NOT USE DIRECTLY** */
enum DRM_REQUEST
{
	DRM_REQUEST_DRMINFO,
	DRM_REQUEST_GET_FB,
	DRM_REQUEST_MODESET
};

#define __DRM_INFO_MAX 60

struct drm_info
{
	char drm_version[__DRM_INFO_MAX];
	char video_driver[__DRM_INFO_MAX];
	char card[__DRM_INFO_MAX];
};

struct drm_fb
{
	volatile unsigned char *framebuffer;
	uint32_t width;
	uint32_t height;
	uint32_t bpp;
	uint32_t pitch;
};
struct drm_modeset_args
{
	uint32_t width;
	uint32_t height;
	uint32_t bpp;
};
int drm_initialize(struct drm_info **out);
int drm_request_info(struct drm_info *info);
int drm_modeset(unsigned int width, unsigned int height, unsigned int bpp);
struct drm_fb* drm_map_fb(void);
#ifdef __cplusplus
}
#endif

#endif
