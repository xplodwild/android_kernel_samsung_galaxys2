/* linux/arch/arm/mach-exynos4/bootmem-smdkv310.c
 * File copied from the AP kernel
 *
 * Copyright (c) 2009 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Bootmem helper functions for smdkv310
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/mm.h>
#include <linux/bootmem.h>
#include <linux/swap.h>
#include <linux/io.h>

#include <asm/setup.h>

#include <plat/bootmem.h>

#include <mach/memory.h>
#include <mach/bootmem.h>

struct s5p_media_device media_devs[] = {
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMD0
	{
		.id = S5P_MDEV_FIMD,
		.name = "fimd",
		.bank = 0,
		.memsize = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMD0 * SZ_1K,
		.paddr = 0,
	},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_JPEG
	{
		.id = S5P_MDEV_JPEG,
		.name = "jpeg",
		.bank = 0,
		.memsize = CONFIG_VIDEO_SAMSUNG_MEMSIZE_JPEG * SZ_1K,
		.paddr = 0,
	},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMG2D
    {
        .id = S5P_MDEV_FIMG2D,
        .name = "fimg2d",
        .bank = 0,
        .memsize = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMD0 * SZ_1K,
        .paddr = 0,
    },
#endif
};
int nr_media_devs = (sizeof(media_devs) / sizeof(media_devs[0]));
