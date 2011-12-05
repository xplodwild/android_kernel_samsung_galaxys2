/* include/video/platform_lcd.h
 *
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * Generic platform-device LCD power control interface.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

struct plat_lcd_data;
struct fb_info;

/**
 * struct plat_lcd_data - platform data for platform_lcd driver.
 * @min_uV: Minimum required voltage output from the regulator.
 * @max_uV: Maximum acceptable voltage output from the regulator.
 */
struct plat_lcd_data {
	void	(*set_power)(struct plat_lcd_data *, unsigned int power);
	int	(*match_fb)(struct plat_lcd_data *, struct fb_info *);
	int	min_uV;
	int	max_uV;
};

