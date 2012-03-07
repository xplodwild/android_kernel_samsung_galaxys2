/* linux/arch/arm/plat-s5p/devs.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Base S5P platform device definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>
#include <mach/hardware.h>
#include <mach/map.h>
#include <mach/dma.h>

#include <plat/devs.h>
#include <plat/gpio-cfg.h>
#include <plat/irqs.h>
#include <plat/fb.h>
#include <plat/fimc.h>
#include <plat/fimg2d.h>

/* OneNAND Controller */
#ifdef CONFIG_FB_S3C
static struct resource s3cfb_resource[] = {
	[0] = {
		.start	= S5P_PA_LCD0,
		.end	= S5P_PA_LCD0 + S5P_SZ_LCD0 - 1,
		.flags	= IORESOURCE_MEM,
	},
	/* Frame sync interrupt for LCD0 */
	[1] = {
		.start	= IRQ_LCD0_0,
		.end	= IRQ_LCD0_0,
		.flags	= IORESOURCE_IRQ,
	},
	/* FIFO level sync interrupt for LCD0 */
	[2] = {
		.start	= IRQ_LCD0_1,
		.end	= IRQ_LCD0_1,
		.flags	= IORESOURCE_IRQ,
	},
};

static u64 fb_dma_mask = 0xffffffffUL;

struct platform_device s3c_device_fb = {
	.name		= "s3cfb",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3cfb_resource),
	.resource	= s3cfb_resource,
	.dev		= {
		.dma_mask		= &fb_dma_mask,
		.coherent_dma_mask	= 0xffffffffUL
	}
};

static struct s3c_platform_fb default_fb_data __initdata = {
#if defined(CONFIG_ARCH_EXYNOS4)
	.hw_ver	= 0x70,
#else
	.hw_ver	= 0x62,
#endif
	.nr_wins	= 5,
#if defined(CONFIG_FB_S3C_DEFAULT_WINDOW)
	.default_win	= CONFIG_FB_S3C_DEFAULT_WINDOW,
#else
	.default_win	= 0,
#endif
	.swap		= FB_SWAP_WORD | FB_SWAP_HWORD,
};

void __init s3cfb_set_platdata(struct s3c_platform_fb *pd)
{
	struct s3c_platform_fb *npd;
	int i;

	if (!pd)
		pd = &default_fb_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fb), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		for (i = 0; i < npd->nr_wins; i++)
			npd->nr_buffers[i] = 1;

#if defined(CONFIG_FB_S3C_NR_BUFFERS)
		npd->nr_buffers[npd->default_win] = CONFIG_FB_S3C_NR_BUFFERS;
#else
		npd->nr_buffers[npd->default_win] = 1;
#endif

		s3cfb_get_clk_name(npd->clk_name);
#if defined(CONFIG_FB_S3C_WA101S) || defined(CONFIG_FB_S3C_AMS369FG06) \
		|| defined(CONFIG_FB_S3C_LTE480WV) || defined(CONFIG_FB_S3C_LD9040)
		npd->cfg_gpio = s3cfb_cfg_gpio;
#else
		printk(KERN_ALERT "panel not defined");
#endif
		npd->backlight_on = s3cfb_backlight_on;
		npd->backlight_off = s3cfb_backlight_off;
		npd->lcd_on = s3cfb_lcd_on;
		npd->lcd_off = s3cfb_lcd_off;
		npd->clk_on = s3cfb_clk_on;
		npd->clk_off = s3cfb_clk_off;

		s3c_device_fb.dev.platform_data = npd;
	}
}
#endif
#ifdef CONFIG_VIDEO_FIMC
static struct resource s3c_fimc0_resource[] = {
	[0] = {
		.start	= S5P_PA_FIMC0,
		.end	= S5P_PA_FIMC0 + S5P_SZ_FIMC0 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_FIMC0,
		.end	= IRQ_FIMC0,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_fimc0 = {
	.name		= "s3c-fimc",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s3c_fimc0_resource),
	.resource	= s3c_fimc0_resource,
};

static struct s3c_platform_fimc default_fimc0_data __initdata = {
	.default_cam	= CAMERA_PAR_A,
};

void __init s3c_fimc0_set_platdata(struct s3c_platform_fimc *pd)
{
	struct s3c_platform_fimc *npd;

	if (!pd)
		pd = &default_fimc0_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fimc), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		if (!npd->cfg_gpio)
			npd->cfg_gpio = s3c_fimc0_cfg_gpio;

		if (!npd->clk_on)
			npd->clk_on = s3c_fimc_clk_on;

		if (!npd->clk_off)
			npd->clk_off = s3c_fimc_clk_off;
		
		s3c_device_fimc0.dev.platform_data = npd;
	}
}

#if !defined(CONFIG_CPU_S5P6450)
static struct resource s3c_fimc1_resource[] = {
	[0] = {
		.start	= S5P_PA_FIMC1,
		.end	= S5P_PA_FIMC1 + S5P_SZ_FIMC1 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_FIMC1,
		.end	= IRQ_FIMC1,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_fimc1 = {
	.name		= "s3c-fimc",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(s3c_fimc1_resource),
	.resource	= s3c_fimc1_resource,
};

static struct s3c_platform_fimc default_fimc1_data __initdata = {
	.default_cam	= CAMERA_PAR_A,
};

void __init s3c_fimc1_set_platdata(struct s3c_platform_fimc *pd)
{
	struct s3c_platform_fimc *npd;

	if (!pd)
		pd = &default_fimc1_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fimc), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		if (!npd->cfg_gpio)
			npd->cfg_gpio = s3c_fimc1_cfg_gpio;

		if (!npd->clk_on)
			npd->clk_on = s3c_fimc_clk_on;

		if (!npd->clk_off)
			npd->clk_off = s3c_fimc_clk_off;

		s3c_device_fimc1.dev.platform_data = npd;
	}
}

static struct resource s3c_fimc2_resource[] = {
	[0] = {
		.start	= S5P_PA_FIMC2,
		.end	= S5P_PA_FIMC2 + S5P_SZ_FIMC2 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_FIMC2,
		.end	= IRQ_FIMC2,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_fimc2 = {
	.name		= "s3c-fimc",
	.id		= 2,
	.num_resources	= ARRAY_SIZE(s3c_fimc2_resource),
	.resource	= s3c_fimc2_resource,
};

static struct s3c_platform_fimc default_fimc2_data __initdata = {
	.default_cam	= CAMERA_PAR_A,
};

void __init s3c_fimc2_set_platdata(struct s3c_platform_fimc *pd)
{
	struct s3c_platform_fimc *npd;

	if (!pd)
		pd = &default_fimc2_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fimc), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		if (!npd->cfg_gpio)
			npd->cfg_gpio = s3c_fimc2_cfg_gpio;

		if (!npd->clk_on)
			npd->clk_on = s3c_fimc_clk_on;

		if (!npd->clk_off)
			npd->clk_off = s3c_fimc_clk_off;

		s3c_device_fimc2.dev.platform_data = npd;
	}
}

#endif
#if defined(CONFIG_CPU_EXYNOS4210)
static struct resource s3c_fimc3_resource[] = {
	[0] = {
		.start	= S5P_PA_FIMC3,
		.end	= S5P_PA_FIMC3 + S5P_SZ_FIMC3 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_FIMC3,
		.end	= IRQ_FIMC3,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_fimc3 = {
	.name		= "s3c-fimc",
	.id		= 3,
	.num_resources	= ARRAY_SIZE(s3c_fimc3_resource),
	.resource	= s3c_fimc3_resource,
};

static struct s3c_platform_fimc default_fimc3_data __initdata = {
	.default_cam	= CAMERA_PAR_A,
};

void __init s3c_fimc3_set_platdata(struct s3c_platform_fimc *pd)
{
	struct s3c_platform_fimc *npd;

	if (!pd)
		pd = &default_fimc3_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fimc), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		if (!npd->cfg_gpio)
			npd->cfg_gpio = s3c_fimc3_cfg_gpio;

		if (!npd->clk_on)
			npd->clk_on = s3c_fimc_clk_on;

		if (!npd->clk_off)
			npd->clk_off = s3c_fimc_clk_off;

		s3c_device_fimc3.dev.platform_data = npd;
	}
}
#endif



#endif
#ifdef CONFIG_VIDEO_JPEG
/* JPEG controller  */
static struct resource s5p_jpeg_resource[] = {
	[0] = {
		.start = S5P_PA_JPEG,
		.end   = S5P_PA_JPEG + S5P_SZ_JPEG - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_JPEG,
		.end   = IRQ_JPEG,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device s5p_device_jpeg = {
	.name             = "s5p-jpeg",
	.id               = -1,
	.num_resources    = ARRAY_SIZE(s5p_jpeg_resource),
	.resource         = s5p_jpeg_resource,
};
EXPORT_SYMBOL(s5p_device_jpeg);
#endif
#ifdef CONFIG_VIDEO_FIMG2D
static struct resource s5p_fimg2d_resource[] = {
	[0] = {
		.start	= S5P_PA_FIMG2D,
		.end	= S5P_PA_FIMG2D + S5P_SZ_FIMG2D - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_FIMG2D,
		.end	= IRQ_FIMG2D,
		.flags	= IORESOURCE_IRQ,
	}
};

struct platform_device s5p_device_fimg2d = {
	.name		= "s5p-fimg2d",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s5p_fimg2d_resource),
	.resource	= s5p_fimg2d_resource
};
EXPORT_SYMBOL(s5p_device_fimg2d);

static struct fimg2d_platdata default_fimg2d_data __initdata = {
	.parent_clkname = "mout_g2d0",
	.clkname = "sclk_fimg2d",
	.gate_clkname = "fimg2d",
	.clkrate = 250 * 1000000,
};
void __init s5p_fimg2d_set_platdata(struct fimg2d_platdata *pd)
{
	struct fimg2d_platdata *npd;

	if (!pd)
		pd = &default_fimg2d_data;

	npd = kmemdup(pd, sizeof(*pd), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "no memory for fimg2d platform data\n");
	else
		s5p_device_fimg2d.dev.platform_data = npd;
}
#endif
