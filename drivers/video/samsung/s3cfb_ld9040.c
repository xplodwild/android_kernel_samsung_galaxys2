/* linux/drivers/video/samsung/s3cfb_ld9040.c
 *
 * Copyright (c) 2012 Team Hacksung
 *		http://www.teamhacksung.org/
 *
 * LD9040 AMOLED Panel module driver for the SMDK
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "s3cfb.h"

static struct s3cfb_lcd ld9040 = {
	.width	= 480,
	.height	= 800,
	.bpp	= 24,
	.freq	= 60,

	.timing = {
		.h_fp = 16,
		.h_bp = 14,
		.h_sw = 2,

		.v_fp = 10,
		.v_fpe = 1,
		.v_bp = 4,
		.v_bpe = 1,
		.v_sw = 2,
	},

	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 1,
		.inv_vsync = 1,
		.inv_vden = 1,
	},
};

/* name should be fixed as 's3cfb_set_lcd_info' */
void s3cfb_set_lcd_info(struct s3cfb_global *ctrl)
{
	ld9040.init_ldi = NULL;
	ctrl->lcd = &ld9040;
}
