/*
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Frame Buffer header for Samsung S5P TVOUT driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __S5P_TVOUT_FB_H_
#define __S5P_TVOUT_FB_H_ __FILE__

#include <linux/fb.h>
#include "../../../video/samsung/s3cfb.h"

extern struct s3cfb_fimd_desc *fbfimd;

extern int s5p_tvout_fb_alloc_framebuffer(struct device *dev_fb);
extern int s5p_tvout_fb_register_framebuffer(struct device *dev_fb);
extern int s5p_tvout_fb_setup_framebuffer(struct device *dev_fb);
extern int s5p_tvout_fb_ctrl_enable(bool enable);

#endif /* __S5P_TVOUT_FB_H_ */
