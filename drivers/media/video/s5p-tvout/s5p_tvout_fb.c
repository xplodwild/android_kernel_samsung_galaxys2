/*
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Frame buffer for Samsung S5P TVOUT driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/fb.h>
#include <linux/dma-mapping.h>
#include <linux/uaccess.h>

#include "s5p_tvout_common_lib.h"
#include "s5p_tvout_ctrl.h"
#include "s5p_tvout_v4l2.h"
#include "s5p_tvout_fb.h"

#define S5PTVFB_NAME		"s5ptvfb"

#define S5PTV_FB_LAYER0_MINOR	10
#define S5PTV_FB_LAYER1_MINOR	11

#define FB_INDEX(id)	(id - S5PTV_FB_LAYER0_MINOR)

#define S5PTVFB_CHROMA(r, g, b)	\
	(((r & 0xff) << 16) | ((g & 0xff) << 8) | ((b & 0xff) << 0))

#define S5PTVFB_WIN_POSITION		\
			_IOW('F', 213, struct s5ptvfb_user_window)
#define S5PTVFB_WIN_SET_PLANE_ALPHA	\
			_IOW('F', 214, struct s5ptvfb_user_plane_alpha)
#define S5PTVFB_WIN_SET_CHROMA		\
			_IOW('F', 215, struct s5ptvfb_user_chroma)
#define S5PTVFB_WAITFORVSYNC		_IO('F', 32)
#define S5PTVFB_SET_VSYNC_INT		_IOW('F', 216, u32)
#define S5PTVFB_WIN_SET_ADDR		_IOW('F', 219, u32)
#define S5PTVFB_SCALING			\
			_IOW('F', 222, struct s5ptvfb_user_scaling)
/* HDMI Display width and height */
#define HDMI_DISPLAY_WIDTH	1920
#define HDMI_DISPLAY_HEIGHT	1080
/* FB bits/pixel */
#define DEF_FB_BPP 32

/* Kernel cmd line parameter: s5p_tvout_fb.hdmi_lcd_display=X */
static int hdmi_lcd_display = 0;
module_param (hdmi_lcd_display, int, S_IRUGO);
MODULE_PARM_DESC (hdmi_lcd_display, "LCD display routed over HDMI @1024x600");

struct s5ptvfb_window {
	int				id;
	struct device			*dev_fb;
	int				enabled;
	atomic_t			in_use;
	int				x;
	int				y;
	enum s5ptvfb_data_path_t	path;
	int				local_channel;
	int				dma_burst;
	unsigned int			pseudo_pal[16];
	struct s5ptvfb_alpha		alpha;
	struct s5ptvfb_chroma		chroma;
	int				(*suspend_fifo)(void);
	int				(*resume_fifo)(void);
};

struct s5ptvfb_lcd_timing {
	int	h_fp;
	int	h_bp;
	int	h_sw;
	int	v_fp;
	int	v_fpe;
	int	v_bp;
	int	v_bpe;
	int	v_sw;
};

struct s5ptvfb_lcd_polarity {
	int	rise_vclk;
	int	inv_hsync;
	int	inv_vsync;
	int	inv_vden;
};

struct s5ptvfb_lcd {
	int				width;
	int				height;
	int				bpp;
	int				freq;
	struct s5ptvfb_lcd_timing	timing;
	struct s5ptvfb_lcd_polarity	polarity;

	void				(*init_ldi)(void);
};

static struct mutex fb_lock;

static struct fb_info *fb[S5PTV_FB_CNT];
static struct s5ptvfb_lcd lcd = {
	.width = 1920,
	.height = 1080,
	.bpp = 32,
	.freq = 60,

	.timing = {
		.h_fp = 49,
		.h_bp = 17,
		.h_sw = 33,
		.v_fp = 4,
		.v_fpe = 1,
		.v_bp = 15,
		.v_bpe = 1,
		.v_sw = 6,
	},

	.polarity = {
		.rise_vclk = 0,
		.inv_hsync = 1,
		.inv_vsync = 1,
		.inv_vden = 0,
	},
};

static int s5p_tvout_fb_wait_for_vsync(void)
{
	sleep_on_timeout(&s5ptv_wq, HZ / 10);

	return 0;
}

static inline unsigned int s5p_tvout_fb_chan_to_field(unsigned int chan,
						      struct fb_bitfield bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf.length;

	return chan << bf.offset;
}

static int s5p_tvout_fb_set_alpha_info(struct fb_var_screeninfo *var,
				       struct s5ptvfb_window *win)
{
	if (var->transp.length > 0)
		win->alpha.mode = TVFB_PIXEL_BLENDING;
	else
		win->alpha.mode = TVFB_NONE_BLENDING;

	return 0;
}

static int s5p_tvout_fb_set_bitfield(struct fb_var_screeninfo *var)
{
	switch (var->bits_per_pixel) {
	case 16:
		if (var->transp.length == 1) {
			var->red.offset = 10;
			var->red.length = 5;
			var->green.offset = 5;
			var->green.length = 5;
			var->blue.offset = 0;
			var->blue.length = 5;
			var->transp.offset = 15;
		} else if (var->transp.length == 4) {
			var->red.offset = 8;
			var->red.length = 4;
			var->green.offset = 4;
			var->green.length = 4;
			var->blue.offset = 0;
			var->blue.length = 4;
			var->transp.offset = 12;
		} else {
			var->red.offset = 11;
			var->red.length = 5;
			var->green.offset = 5;
			var->green.length = 6;
			var->blue.offset = 0;
			var->blue.length = 5;
			var->transp.offset = 0;
		}
		break;

	case 24:
		var->red.offset = 16;
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
		var->blue.offset = 0;
		var->blue.length = 8;
		var->transp.offset = 0;
		var->transp.length = 0;
		break;

	case 32:
		var->red.offset = 16;
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
		var->blue.offset = 0;
		var->blue.length = 8;
		var->transp.offset = 24;
		break;
	}

	return 0;
}

static int s5p_tvout_fb_setcolreg(unsigned int regno, unsigned int red,
				  unsigned int green, unsigned int blue,
				  unsigned int transp, struct fb_info *fb)
{
	unsigned int *pal = (unsigned int *) fb->pseudo_palette;
	unsigned int val = 0;

	if (regno < 16) {
		/* fake palette of 16 colors */
		val |= s5p_tvout_fb_chan_to_field(red, fb->var.red);
		val |= s5p_tvout_fb_chan_to_field(green, fb->var.green);
		val |= s5p_tvout_fb_chan_to_field(blue, fb->var.blue);
		val |= s5p_tvout_fb_chan_to_field(transp, fb->var.transp);

		pal[regno] = val;
	}

	return 0;
}

static int s5p_tvout_fb_pan_display(struct fb_var_screeninfo *var,
				    struct fb_info *fb)
{
	dma_addr_t start_addr;
	enum s5p_mixer_layer layer;
	struct fb_fix_screeninfo *fix = &fb->fix;

	if (var->yoffset + var->yres > var->yres_virtual) {
		tvout_err("invalid y offset value\n");
		return -1;
	}

	fb->var.yoffset = var->yoffset;

	switch (fb->node) {
	case S5PTV_FB_LAYER0_MINOR:
		layer = MIXER_GPR0_LAYER;
		break;

	case S5PTV_FB_LAYER1_MINOR:
		layer = MIXER_GPR1_LAYER;
		break;

	default:
		tvout_err("invalid layer\n");
		return -1;
	}

	start_addr = fix->smem_start + (var->xres_virtual * \
			(var->bits_per_pixel / 8) * var->yoffset);

	s5p_mixer_ctrl_set_buffer_address(layer, start_addr);

	return 0;
}

static int s5p_tvout_fb_blank(int blank_mode, struct fb_info *fb)
{
	enum s5p_mixer_layer layer = MIXER_GPR0_LAYER;

	tvout_dbg("change blank mode\n");

	switch (fb->node) {
	case S5PTV_FB_LAYER0_MINOR:
		layer = MIXER_GPR0_LAYER;
		break;

	case S5PTV_FB_LAYER1_MINOR:
		layer = MIXER_GPR1_LAYER;
		break;

	default:
		tvout_err("not supported layer\n");
		return -1;
	}

	switch (blank_mode) {
	default:
	case FB_BLANK_UNBLANK:
		if (fb->fix.smem_start)
			s5p_mixer_ctrl_enable_layer(layer);
		else
			tvout_dbg("[fb%d] no alloc memory for unblank\n", \
								fb->node);
		break;

	case FB_BLANK_POWERDOWN:
		s5p_mixer_ctrl_disable_layer(layer);
		break;

		//		tvout_err("not supported blank mode\n");
		//return -1;
	}

	return 0;
}

static int __devinit s5p_tvout_fb_map_video_memory(int id)
{
        enum s5p_mixer_layer layer;
        struct s5ptvfb_window *win = fb[FB_INDEX(id)]->par;
        struct fb_fix_screeninfo *fix = &fb[FB_INDEX(id)]->fix;

	printk("s5p_tvout_fb_map_video_memory called \n");

        if (win->path == TVFB_DATA_PATH_FIFO)
                return 0;
	
	printk("smem_length = %d\n", fix->smem_len);

        fb[FB_INDEX(id)]->screen_base = dma_alloc_writecombine(win->dev_fb,
                        PAGE_ALIGN(fix->smem_len),
                        (unsigned int *) &fix->smem_start, GFP_KERNEL);

        switch (id) {
        case S5PTV_FB_LAYER0_MINOR:
                layer = MIXER_GPR0_LAYER;
                break;
        case S5PTV_FB_LAYER1_MINOR:
                layer = MIXER_GPR1_LAYER;
                break;
        default:
                tvout_err("invalid layer\n");
                return -1;
        }
        s5p_mixer_ctrl_init_fb_addr_phy(layer, fix->smem_start);

        if (!fb[FB_INDEX(id)]->screen_base)
	{
		printk("s5p_tvout_fb_map_video_memory error \n");
                return -1;
	}
        else
                tvout_dbg("[fb%d] dma: 0x%08x, cpu: 0x%08x,size: 0x%08x\n",
                        win->id, (unsigned int) fix->smem_start,
                        (unsigned int) fb[FB_INDEX(id)]->screen_base,
                        fix->smem_len);

        memset(fb[FB_INDEX(id)]->screen_base, 0, fix->smem_len);

        return 0;
}

static int s5p_tvout_fb_set_par(struct fb_info *fb)
{
	u32 bpp, trans_len;
	u32 src_x, src_y, w, h;
	struct s5ptvfb_window *win = fb->par;
	enum s5p_mixer_layer layer = MIXER_GPR0_LAYER;

	tvout_dbg("[fb%d] set_par\n", win->id);

	if (!fb->fix.smem_start)
	  {
	    printk(KERN_INFO " The frame buffer is allocated here\n");
	    s5p_tvout_fb_map_video_memory(win->id);
	  }

	bpp = fb->var.bits_per_pixel;
	trans_len = fb->var.transp.length;
	w = fb->var.xres;
	h = fb->var.yres;
	src_x = fb->var.xoffset;
	src_y = fb->var.yoffset;

	switch (fb->node) {
	case S5PTV_FB_LAYER0_MINOR:
		layer = MIXER_GPR0_LAYER;
		break;

	case S5PTV_FB_LAYER1_MINOR:
		layer = MIXER_GPR1_LAYER;
		break;

	default:
		break;
	}

	s5p_mixer_ctrl_init_grp_layer(layer);
	s5p_mixer_ctrl_set_pixel_format(layer, bpp, trans_len);
	s5p_mixer_ctrl_set_src_win_pos(layer, src_x, src_y, w, h);
	s5p_mixer_ctrl_set_alpha_blending(layer, win->alpha.mode,
					  win->alpha.value);
	return 0;
}

static int s5p_tvout_fb_check_var(struct fb_var_screeninfo *var,
				  struct fb_info *fb)
{
	struct fb_fix_screeninfo *fix = &fb->fix;
	struct s5ptvfb_window *win = fb->par;

	tvout_dbg("[fb%d] check_var\n", win->id);

	if (var->bits_per_pixel != 16 && var->bits_per_pixel != 24 &&
		var->bits_per_pixel != 32) {
		tvout_err("invalid bits per pixel\n");
		return -1;
	}

	if (var->xres > lcd.width)
		var->xres = lcd.width;

	if (var->yres > lcd.height)
		var->yres = lcd.height;

	if (var->xres_virtual != var->xres)
		var->xres_virtual = var->xres;

	if (var->yres_virtual > var->yres * (fb->fix.ypanstep + 1))
		var->yres_virtual = var->yres * (fb->fix.ypanstep + 1);

	if (var->xoffset != 0)
		var->xoffset = 0;

	if (var->yoffset + var->yres > var->yres_virtual)
		var->yoffset = var->yres_virtual - var->yres;

	if (win->x + var->xres > lcd.width)
		win->x = lcd.width - var->xres;

	if (win->y + var->yres > lcd.height)
		win->y = lcd.height - var->yres;

	/* modify the fix info */
	fix->line_length = var->xres_virtual * var->bits_per_pixel / 8;
	fix->smem_len = fix->line_length * var->yres_virtual;

	s5p_tvout_fb_set_bitfield(var);
	s5p_tvout_fb_set_alpha_info(var, win);

	return 0;
}

static int s5p_tvout_fb_release(struct fb_info *fb, int user)
{
	struct s5ptvfb_window *win = fb->par;

	atomic_dec(&win->in_use);

	return 0;
}

static int s5p_tvout_fb_ioctl(struct fb_info *fb, unsigned int cmd,
			      unsigned long arg)
{
	dma_addr_t start_addr;
	enum s5p_mixer_layer layer;
	struct fb_var_screeninfo *var = &fb->var;
	struct s5ptvfb_window *win = fb->par;
	int ret = 0;
	void *argp = (void *) arg;

	union {
		struct s5ptvfb_user_window user_window;
		struct s5ptvfb_user_plane_alpha user_alpha;
		struct s5ptvfb_user_chroma user_chroma;
		struct s5ptvfb_user_scaling user_scaling;
		int vsync;
	} p;

	switch (fb->node) {
	case S5PTV_FB_LAYER0_MINOR:
		layer = MIXER_GPR0_LAYER;
		break;
	case S5PTV_FB_LAYER1_MINOR:
		layer = MIXER_GPR1_LAYER;
		break;
	default:
		printk(KERN_ERR "[Error] invalid layer\n");
		return -1;
	}

	switch (cmd) {
	case S5PTVFB_WIN_POSITION:
		if (copy_from_user(&p.user_window, \
			(struct s5ptvfb_user_window __user *) arg, \
					sizeof(p.user_window))) {
			ret = -EFAULT;
		} else {
			s5p_mixer_ctrl_set_dst_win_pos(layer,
				p.user_window.x, p.user_window.y,
				var->xres, var->yres);
		}
		break;

	case S5PTVFB_WIN_SET_PLANE_ALPHA:
		if (copy_from_user(&p.user_alpha, \
			(struct s5ptvfb_user_plane_alpha __user *) arg, \
					sizeof(p.user_alpha))) {
			ret = -EFAULT;
		} else {
			win->alpha.mode = TVFB_LAYER_BLENDING;
			win->alpha.value = p.user_alpha.alpha;
			s5p_mixer_ctrl_set_alpha_blending(layer,
				win->alpha.mode, win->alpha.value);
		}
		break;

	case S5PTVFB_WIN_SET_CHROMA:
		if (copy_from_user(&p.user_chroma, \
			(struct s5ptvfb_user_chroma __user *) arg, \
					sizeof(p.user_chroma))) {
			ret = -EFAULT;
		} else {
			win->chroma.enabled = p.user_chroma.enabled;
			win->chroma.key = S5PTVFB_CHROMA(p.user_chroma.red, \
				p.user_chroma.green, p.user_chroma.blue);
			s5p_mixer_ctrl_set_chroma_key(layer, win->chroma);
		}
		break;

	case S5PTVFB_SET_VSYNC_INT:
		s5p_mixer_ctrl_set_vsync_interrupt((int)argp);
		break;

	case S5PTVFB_WAITFORVSYNC:
		s5p_tvout_fb_wait_for_vsync();
		break;

	case S5PTVFB_WIN_SET_ADDR:
		fb->fix.smem_start = (unsigned long)argp;
		start_addr = fb->fix.smem_start + (var->xres_virtual * \
				(var->bits_per_pixel / 8) * var->yoffset);
		s5p_mixer_ctrl_set_buffer_address(layer, start_addr);
		break;

	case S5PTVFB_SCALING:
		if (copy_from_user(&p.user_scaling, \
			(struct s5ptvfb_user_scaling __user *) arg, \
					sizeof(p.user_scaling)))
			ret = -EFAULT;
		else
			s5p_mixer_ctrl_scaling(layer, p.user_scaling);
		break;
	}

	return 0;
}

static int s5p_tvout_fb_open(struct fb_info *fb, int user)
{
	struct s5ptvfb_window *win = fb->par;
	int ret = 0;

	mutex_lock(&fb_lock);

	if (atomic_read(&win->in_use)) {
		tvout_dbg("do not allow multiple open for tvout framebuffer\n");
		ret = -EBUSY;
	} else
		atomic_inc(&win->in_use);

	mutex_unlock(&fb_lock);

	return ret;
}

struct fb_ops s5ptvfb_ops = {
	.owner			= THIS_MODULE,
	.fb_fillrect		= cfb_fillrect,
	.fb_copyarea		= cfb_copyarea,
	.fb_imageblit		= cfb_imageblit,
	.fb_check_var		= s5p_tvout_fb_check_var,
	.fb_set_par		= s5p_tvout_fb_set_par,
	.fb_blank		= s5p_tvout_fb_blank,
	.fb_pan_display		= s5p_tvout_fb_pan_display,
	.fb_setcolreg		= s5p_tvout_fb_setcolreg,
	.fb_ioctl		= s5p_tvout_fb_ioctl,
	.fb_open		= s5p_tvout_fb_open,
	.fb_release		= s5p_tvout_fb_release,
};

static int s5p_tvout_fb_init_fbinfo(int id, struct device *dev_fb)
{
	struct fb_fix_screeninfo *fix = &fb[FB_INDEX(id)]->fix;
	struct fb_var_screeninfo *var = &fb[FB_INDEX(id)]->var;
	struct s5ptvfb_window *win = fb[FB_INDEX(id)]->par;
	struct s5ptvfb_alpha *alpha = &win->alpha;

	memset(win, 0, sizeof(struct s5ptvfb_window));

	platform_set_drvdata(to_platform_device(dev_fb), fb[FB_INDEX(id)]);

	strcpy(fix->id, S5PTVFB_NAME);

	/* fimd specific */
	win->id = id;
	win->path = TVFB_DATA_PATH_DMA;
	win->dma_burst = 16;
	win->dev_fb = dev_fb;
	alpha->mode = TVFB_LAYER_BLENDING;
	alpha->value = 0xff;

	/* fbinfo */
	fb[FB_INDEX(id)]->fbops = &s5ptvfb_ops;
	fb[FB_INDEX(id)]->flags = FBINFO_FLAG_DEFAULT;
	fb[FB_INDEX(id)]->pseudo_palette = &win->pseudo_pal;
	fix->xpanstep = 0;
	fix->ypanstep = 0;
	fix->type = FB_TYPE_PACKED_PIXELS;
	fix->accel = FB_ACCEL_NONE;
	fix->visual = FB_VISUAL_TRUECOLOR;

	if (hdmi_lcd_display) {
		struct s3cfb_global *fbdev;
		fbdev = fbfimd->fbdev[0];
		var->xres = fbdev->lcd->width;
		var->yres = fbdev->lcd->height;
	} else {
		var->xres = lcd.width;
		var->yres = lcd.height;
	}

	var->xres_virtual = var->xres;
	var->yres_virtual = var->yres + (var->yres * fix->ypanstep);
	var->bits_per_pixel = 32;
	var->xoffset = 0;
	var->yoffset = 0;
	var->width = 0;
	var->height = 0;
	var->transp.length = 0;

	fix->line_length = var->xres_virtual * var->bits_per_pixel / 8;
	fix->smem_len = fix->line_length * var->yres_virtual;

	var->nonstd = 0;
	var->activate = FB_ACTIVATE_NOW;
	var->vmode = FB_VMODE_NONINTERLACED;
	var->hsync_len = lcd.timing.h_sw;
	var->vsync_len = lcd.timing.v_sw;
	var->left_margin = lcd.timing.h_fp;
	var->right_margin = lcd.timing.h_bp;
	var->upper_margin = lcd.timing.v_fp;
	var->lower_margin = lcd.timing.v_bp;

	var->pixclock = lcd.freq * (var->left_margin + \
		var->right_margin + var->hsync_len + var->xres)
			* (var->upper_margin + var->lower_margin \
				+ var->vsync_len + var->yres);

	tvout_dbg("pixclock: %d\n", var->pixclock);

	s5p_tvout_fb_set_bitfield(var);
	s5p_tvout_fb_set_alpha_info(var, win);

	mutex_init(&fb_lock);

	return 0;
}

int s5p_tvout_fb_alloc_framebuffer(struct device *dev_fb)
{
	int ret, i;

	/* alloc for each framebuffer */
	for (i = 0; i < S5PTV_FB_CNT; i++) {
		fb[i] = framebuffer_alloc(sizeof(struct s5ptvfb_window), \
						dev_fb);
		if (!fb[i]) {
			tvout_err("not enough memory\n");
			ret = -1;
			goto err_alloc_fb;
		}

		ret = s5p_tvout_fb_init_fbinfo(i + S5PTV_FB_LAYER0_MINOR, \
						dev_fb);
		if (ret) {
			tvout_err("fail to allocate memory for tv fb\n");
			ret = -1;
			goto err_alloc_fb;
		}
	}

	return 0;

err_alloc_fb:
	for (i = 0; i < S5PTV_FB_CNT; i++) {
		if (fb[i])
			framebuffer_release(fb[i]);
	}

	return ret;
}

int s5p_tvout_fb_register_framebuffer(struct device *dev_fb)
{
	int ret, i = 0;

	do {
		ret = register_framebuffer(fb[0]);
		if (ret) {
			tvout_err("fail to register framebuffer device\n");
			return -1;
		}
	} while (fb[0]->node < S5PTV_FB_LAYER0_MINOR);

	for (i = 1; i < S5PTV_FB_CNT; i++) {
		ret = register_framebuffer(fb[i]);
		if (ret) {
			tvout_err("fail to register framebuffer device\n");
			return -1;
		}
	}

	for (i = 0; i < S5PTV_FB_CNT; i++)
		tvout_dbg("fb[%d] = %d\n", i, fb[i]->node);

	for (i = 0; i < S5PTV_FB_CNT; i++) {
#ifndef CONFIG_FRAMEBUFFER_CONSOLE
#ifndef CONFIG_USER_ALLOC_TVOUT
		s5p_tvout_fb_check_var(&fb[i]->var, fb[i]);
		s5p_tvout_fb_set_par(fb[i]);
#endif
#endif
	}

	return 0;
}
/* making this below global for time being */
dma_addr_t start_addr;
enum s5p_mixer_layer layer;

int s5p_tvout_fb_setup_framebuffer(struct device *dev_fb)
{
	struct s3cfb_global *fbdev;
	struct fb_info *fb1;
	struct fb_var_screeninfo *var;

	fbdev = fbfimd->fbdev[0];
	fb1 = fb[0];
	var = &fb1->var;

	if (!hdmi_lcd_display) {
		struct s5ptvfb_window *win;
		win = fb1->par;
	}
	switch (fb1->node) {
	case S5PTV_FB_LAYER0_MINOR:
		layer = MIXER_GPR0_LAYER;
		break;
	case S5PTV_FB_LAYER1_MINOR:
		layer = MIXER_GPR1_LAYER;
		break;
	default:
		printk(KERN_ERR "[Error] invalid layer\n");
		return -1;
	}

	s5p_tvout_fb_check_var(&fb[0]->var, fb[0]);
	s5p_tvout_fb_set_par(fb[0]);

	if (hdmi_lcd_display) {
		fb1->fix.smem_start = fbdev->fb[0]->fix.smem_start;
		start_addr = fb1->fix.smem_start + \
			     (var->xres_virtual * \
			     (var->bits_per_pixel / 8) * var->yoffset);
	} else {
		start_addr = fb1->fix.smem_start;
	}

	return 0;
}

int s5p_tvout_fb_ctrl_enable(bool enable)
{
	struct fb_info *fb1 = fb[0];
	struct fb_var_screeninfo var;
	struct s5ptvfb_user_window user_window;
	enum s5p_mixer_layer layer;
	enum s5p_tvout_disp_mode        tv_std;
	enum s5p_tvout_o_mode           tv_if;
	struct s3cfb_global *fbdev;

	fbdev = fbfimd->fbdev[0];

	/* hard-coded values below for 1080p 60Hz */
	tv_std = TVOUT_1080P_60;
	tv_if = TVOUT_HDMI_RGB;

	switch (fb1->node) {
	case S5PTV_FB_LAYER0_MINOR:
		layer = MIXER_GPR0_LAYER;
		break;
	case S5PTV_FB_LAYER1_MINOR:
		layer = MIXER_GPR1_LAYER;
		break;
	default:
		printk(KERN_ERR"[Error] invalid layer\n");
		return -1;
	}

	if (s5p_tvif_ctrl_start(tv_std, tv_if) < 0) {
		printk(KERN_ERR"[S5P-TVOUT] s5p_tvif_ctrl_start failed\n");
		return -1;
	}

	if (hdmi_lcd_display)
		s5p_mixer_ctrl_set_buffer_address(layer, start_addr);
	else
		s5p_mixer_ctrl_set_buffer_address(layer, fb1->fix.smem_start);

	if (fb1->fix.smem_start)
		s5p_mixer_ctrl_enable_layer(layer);
	else
		printk(KERN_ERR"[S5P-TVOUT] FB unblanking failed\n");

	if (hdmi_lcd_display) {
		var.xres = fbdev->lcd->width;
		var.yres = fbdev->lcd->height;

		/* X and Y coordinate are calculated to center the display */
		user_window.x = (HDMI_DISPLAY_WIDTH/2) - ((fbdev->lcd->width)/2);
		user_window.y = (HDMI_DISPLAY_HEIGHT/2) - ((fbdev->lcd->height)/2);
	} else {
		var.xres = 1920;
		var.yres = 1080;

		/* X and Y coordinate are calculated to center the display */
		user_window.x = 0;
		user_window.y = 0;
	}
	var.bits_per_pixel = DEF_FB_BPP;
	var.xres_virtual = var.xres;
	var.yres_virtual = var.yres;
	var.xoffset = 0;
	var.yoffset = 0;
	var.width = 0;
	var.height = 0;
	var.transp.length = 0;
	var.activate = FB_ACTIVATE_FORCE;

	s5p_mixer_ctrl_set_dst_win_pos(layer, user_window.x, user_window.y, \
			var.xres, var.yres);

	return 0;
}
