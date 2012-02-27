/* linux/drivers/media/video/samsung/fimc_capture.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *	http://www.samsung.com/
 *
 * V4L2 Capture device support file for Samsung Camera Interface (FIMC) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/slab.h>
#include <linux/bootmem.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/videodev2.h>
#include <linux/videodev2_samsung.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <plat/bootmem.h>
#include <plat/clock.h>
#include <plat/fimc.h>

#include "fimc.h"

/* subdev handling macro */
#define subdev_call(ctrl, o, f, args...) \
	v4l2_subdev_call(ctrl->cam->sd, o, f, ##args)

const static struct v4l2_fmtdesc capture_fmts[] = {
	{
		.index		= 0,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PACKED,
		.description	= "RGB-5-6-5",
		.pixelformat	= V4L2_PIX_FMT_RGB565,
	}, {
		.index		= 1,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PACKED,
		.description	= "RGB-8-8-8, unpacked 24 bpp",
		.pixelformat	= V4L2_PIX_FMT_RGB32,
	}, {
		.index		= 2,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PACKED,
		.description	= "YUV 4:2:2 packed, YCbYCr",
		.pixelformat	= V4L2_PIX_FMT_YUYV,
	}, {
		.index		= 3,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PACKED,
		.description	= "YUV 4:2:2 packed, CbYCrY",
		.pixelformat	= V4L2_PIX_FMT_UYVY,
	}, {
		.index		= 4,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PACKED,
		.description	= "YUV 4:2:2 packed, CrYCbY",
		.pixelformat	= V4L2_PIX_FMT_VYUY,
	}, {
		.index		= 5,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PACKED,
		.description	= "YUV 4:2:2 packed, YCrYCb",
		.pixelformat	= V4L2_PIX_FMT_YVYU,
	}, {
		.index		= 6,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PLANAR,
		.description	= "YUV 4:2:2 planar, Y/Cb/Cr",
		.pixelformat	= V4L2_PIX_FMT_YUV422P,
	}, {
		.index		= 7,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PLANAR,
		.description	= "YUV 4:2:0 planar, Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_NV12,
	}, {
		.index		= 8,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PLANAR,
		.description	= "YUV 4:2:0 planar, Y/CbCr, Tiled",
		.pixelformat	= V4L2_PIX_FMT_NV12T,
	}, {
		.index		= 9,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PLANAR,
		.description	= "YUV 4:2:0 planar, Y/CrCb",
		.pixelformat	= V4L2_PIX_FMT_NV21,
	}, {
		.index		= 10,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PLANAR,
		.description	= "YUV 4:2:2 planar, Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_NV16,
	}, {
		.index		= 11,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PLANAR,
		.description	= "YUV 4:2:2 planar, Y/CrCb",
		.pixelformat	= V4L2_PIX_FMT_NV61,
	}, {
		.index		= 12,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PLANAR,
		.description	= "YUV 4:2:0 planar, Y/Cb/Cr",
		.pixelformat	= V4L2_PIX_FMT_YUV420,
	}, {
		.index		= 13,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.description	= "JPEG encoded data",
		.pixelformat	= V4L2_PIX_FMT_JPEG,
	}, {
		.index		= 14,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PACKED,
		.description	= "RGB-1-5-5-5",
		.pixelformat	= V4L2_PIX_FMT_RGB555,
	}, {
		.index		= 15,
		.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.flags		= FORMAT_FLAGS_PACKED,
		.description	= "RGB-4-4-4-4",
		.pixelformat	= V4L2_PIX_FMT_RGB444,
	},
};

const static struct v4l2_queryctrl fimc_controls[] = {
	{
		.id = V4L2_CID_ROTATION,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Roataion",
		.minimum = 0,
		.maximum = 270,
		.step = 90,
		.default_value = 0,
	}, {
		.id = V4L2_CID_HFLIP,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Horizontal Flip",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
	}, {
		.id = V4L2_CID_VFLIP,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Vertical Flip",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
	}, {
		.id = V4L2_CID_PADDR_Y,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Physical address Y",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
		.flags = V4L2_CTRL_FLAG_READ_ONLY,
	}, {
		.id = V4L2_CID_PADDR_CB,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Physical address Cb",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
		.flags = V4L2_CTRL_FLAG_READ_ONLY,
	}, {
		.id = V4L2_CID_PADDR_CR,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Physical address Cr",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
		.flags = V4L2_CTRL_FLAG_READ_ONLY,
	}, {
		.id = V4L2_CID_PADDR_CBCR,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Physical address CbCr",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
		.flags = V4L2_CTRL_FLAG_READ_ONLY,
	},
};

#ifndef CONFIG_VIDEO_FIMC_MIPI
void s3c_csis_start(int csis_id, int lanes, int settle, \
	int align, int width, int height, int pixel_format) {}
void s3c_csis_stop(int csis_id) {}
#endif

static int fimc_init_camera(struct fimc_control *ctrl)
{
	struct fimc_global *fimc = get_fimc_dev();
	struct s3c_platform_fimc *pdata;
	struct s3c_platform_camera *cam;

	fimc_info1("%s", __func__);
	pdata = to_fimc_plat(ctrl->dev);
	if (pdata->default_cam >= FIMC_MAXCAMS) {
		fimc_err("%s: invalid camera index\n", __func__);
		return -EINVAL;
	}

	if (!fimc->camera[pdata->default_cam]) {
		fimc_err("no external camera device\n");
		return -ENODEV;
	}

	/*
	 * ctrl->cam may not be null if already s_input called,
	 * otherwise, that should be default_cam if ctrl->cam is null.
	*/
	if (!ctrl->cam)
		ctrl->cam = fimc->camera[pdata->default_cam];

	cam = ctrl->cam;


	/* do nothing if already initialized */
	if (cam->initialized)
		return 0;

	/*
	 * WriteBack mode doesn't need to set clock and power,
	 * but it needs to set source width, height depend on LCD resolution.
	*/
	if ((cam->id == CAMERA_WB) || (cam->id == CAMERA_WB_B)) {
		s3cfb_direct_ioctl(0, S3CFB_GET_LCD_WIDTH, \
					(unsigned long)&cam->width);
		s3cfb_direct_ioctl(0, S3CFB_GET_LCD_HEIGHT, \
					(unsigned long)&cam->height);
		cam->window.width = cam->width;
		cam->window.height = cam->height;
		cam->initialized = 1;
		return 0;
	}
	/* set rate for mclk */
	if (clk_get_rate(cam->clk)) {
		clk_set_rate(cam->clk, cam->clk_rate);
		clk_enable(cam->clk);
		fimc_info1("clock enabled for camera: %d\n", cam->clk_rate);
	}
	else
	{
		fimc_err("clock not enabled for camera %s", cam->clk->name);
	}

	/* enable camera power (if needed??) */
	if (cam->cam_power)
		cam->cam_power(1);

	cam->initialized = 1;

	return 0;
}

static int fimc_capture_scaler_info(struct fimc_control *ctrl)
{
	struct fimc_scaler *sc = &ctrl->sc;
	struct v4l2_rect *window = &ctrl->cam->window;
	int tx, ty, sx, sy;
	struct s3c_platform_fimc *pdata = to_fimc_plat(ctrl->dev);

	fimc_info1("%s", __func__);
	sx = window->width;
	sy = window->height;
	tx = ctrl->cap->fmt.width;
	ty = ctrl->cap->fmt.height;

	sc->real_width = sx;
	sc->real_height = sy;

	if (sx <= 0 || sy <= 0) {
		fimc_err("%s: invalid source size\n", __func__);
		return -EINVAL;
	}

	if (tx <= 0 || ty <= 0) {
		fimc_err("%s: invalid target size\n", __func__);
		return -EINVAL;
	}

	fimc_get_scaler_factor(sx, tx, &sc->pre_hratio, &sc->hfactor);
	fimc_get_scaler_factor(sy, ty, &sc->pre_vratio, &sc->vfactor);

	sc->pre_dst_width = sx / sc->pre_hratio;
	sc->pre_dst_height = sy / sc->pre_vratio;

	if (pdata->hw_ver >= 0x50) {
		sc->main_hratio = (sx << 14) / (tx << sc->hfactor);
		sc->main_vratio = (sy << 14) / (ty << sc->vfactor);
	} else {
		sc->main_hratio = (sx << 8) / (tx << sc->hfactor);
		sc->main_vratio = (sy << 8) / (ty << sc->vfactor);
	}

	sc->scaleup_h = (tx >= sx) ? 1 : 0;
	sc->scaleup_v = (ty >= sy) ? 1 : 0;

	return 0;
}

static int fimc_add_inqueue(struct fimc_control *ctrl, int i)
{
	struct fimc_capinfo *cap = ctrl->cap;
	struct fimc_buf_set *tmp_buf;
	struct list_head *count;
	
	fimc_info1("%s", __func__);

	/* PINGPONG_2ADDR_MODE Only */
	list_for_each(count, &cap->inq) {
		tmp_buf = list_entry(count, struct fimc_buf_set, list);
		/* skip list_add_tail if already buffer is in cap->inq list*/
		if (tmp_buf->id == i)
			return 0;
	}
	list_add_tail(&cap->bufs[i].list, &cap->inq);

	return 0;
}

static int fimc_add_outqueue(struct fimc_control *ctrl, int i)
{
	struct fimc_capinfo *cap = ctrl->cap;
	struct fimc_buf_set *buf;
	unsigned int mask = 0x2;

	/* PINGPONG_2ADDR_MODE Only */
	/* pair_buf_index stands for pair index of i. (0<->2) (1<->3) */
	int pair_buf_index = (i ^ mask);

	fimc_info1("%s", __func__);

	/* FIMC have 4 h/w registers */
	if (i < 0 || i >= FIMC_PHYBUFS) {
		fimc_err("%s: invalid queue index : %d\n", __func__, i);
		return -ENOENT;
	}

	if (list_empty(&cap->inq))
		return -ENOENT;

	buf = list_first_entry(&cap->inq, struct fimc_buf_set, list);

	/* pair index buffer should be allocated first */
	cap->outq[pair_buf_index] = buf->id;
	fimc_hwset_output_address(ctrl, buf, pair_buf_index);

	cap->outq[i] = buf->id;
	fimc_hwset_output_address(ctrl, buf, i);

	list_del(&buf->list);

	return 0;
}

int fimc_g_parm(struct file *file, void *fh, struct v4l2_streamparm *a)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;
	int ret;

	fimc_dbg("%s\n", __func__);

	/* WriteBack doesn't have subdev_call */

	if ((ctrl->cam->id == CAMERA_WB) || (ctrl->cam->id == CAMERA_WB_B))
		return 0;

	mutex_lock(&ctrl->v4l2_lock);
	if (!ctrl->cam) {
			fimc_err("%s : ctrl->cam is null\n", __func__);
			mutex_unlock(&ctrl->v4l2_lock);
			return -EINVAL;
	}
	ret = subdev_call(ctrl, video, g_parm, a);
	mutex_unlock(&ctrl->v4l2_lock);

	return ret;
}

int fimc_s_parm(struct file *file, void *fh, struct v4l2_streamparm *a)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;
	int ret;

	fimc_dbg("%s\n", __func__);

	/* WriteBack doesn't have subdev_call */
	if ((ctrl->cam->id == CAMERA_WB) || (ctrl->cam->id == CAMERA_WB_B))
		return 0;

	mutex_lock(&ctrl->v4l2_lock);
	if (!ctrl->cam) {
			fimc_err("%s : ctrl->cam is null\n", __func__);
			mutex_unlock(&ctrl->v4l2_lock);
			return -EINVAL;
	}
	ret = subdev_call(ctrl, video, s_parm, a);
	mutex_unlock(&ctrl->v4l2_lock);

	return ret;
}

/* Enumerate controls */
int fimc_queryctrl(struct file *file, void *fh, struct v4l2_queryctrl *qc)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;
	int i, ret;

	fimc_dbg("%s\n", __func__);

	for (i = 0; i < ARRAY_SIZE(fimc_controls); i++) {
		if (fimc_controls[i].id == qc->id) {
			memcpy(qc, &fimc_controls[i], \
				sizeof(struct v4l2_queryctrl));
			return 0;
		}
	}

	mutex_lock(&ctrl->v4l2_lock);
	if (!ctrl->cam) {
			fimc_err("%s : ctrl->cam is null\n", __func__);
			mutex_unlock(&ctrl->v4l2_lock);
			return -EINVAL;
	}
	ret = subdev_call(ctrl, core, queryctrl, qc);
	mutex_unlock(&ctrl->v4l2_lock);

	return ret;
}

/* Menu control items */
int fimc_querymenu(struct file *file, void *fh, struct v4l2_querymenu *qm)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;
	int ret;

	fimc_dbg("%s\n", __func__);

	mutex_lock(&ctrl->v4l2_lock);
	if (!ctrl->cam) {
			fimc_err("%s : ctrl->cam is null\n", __func__);
			mutex_unlock(&ctrl->v4l2_lock);
			return -EINVAL;
	}
	ret = subdev_call(ctrl, core, querymenu, qm);
	if (!ctrl->cam) {
			fimc_err("%s : ctrl->cam is null\n", __func__);
			mutex_unlock(&ctrl->v4l2_lock);
			return -EINVAL;
	}
	mutex_unlock(&ctrl->v4l2_lock);

	return ret;
}

int fimc_enum_input(struct file *file, void *fh, struct v4l2_input *inp)
{
	struct fimc_global *fimc = get_fimc_dev();
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;
	struct s3c_platform_camera *cam = NULL;
	int i, cam_count = 0;
	
	fimc_info1("%s", __func__);

	if (inp->index >= FIMC_MAXCAMS) {
		fimc_err("%s: invalid input index\n", __func__);
		return -EINVAL;
	}

	fimc_dbg("%s: index %d\n", __func__, inp->index);

	mutex_lock(&ctrl->v4l2_lock);

	/*
	 * External camera input devices are managed in fimc->camera[]
	 * but it aligned in the order of H/W camera interface's (A/B/C)
	 * Therefore it could be NULL if there is no actual camera to take
	 * place the index
	 * ie. if index is 1, that means that one camera has been selected
	 * before so choose second object it reaches
	 */
	for (i = 0; i < FIMC_MAXCAMS; i++) {
		/* increase index until get not NULL and upto FIMC_MAXCAMS */
		if (!fimc->camera[i])
			continue;
		if (fimc->camera[i]) {
			++cam_count;
			if (cam_count == inp->index + 1) {
				cam = fimc->camera[i];
				fimc_info1("%s:v4l2 input[%d] is %s",
						__func__, inp->index,
						fimc->camera[i]->info->type);
				break;
			} else
				continue;
		}
	}

	if (cam) {
		strcpy(inp->name, cam->info->type);
		inp->type = V4L2_INPUT_TYPE_CAMERA;
#ifdef SUPPORT_GSTREAMER
		/* Default cam setting because gstreamer did not use s_input
		 * function */
		if ((cam->id != CAMERA_WB) && (cam->id != CAMERA_WB_B)) {
			ctrl->cam = cam;
		}
#endif
	} else {
		fimc_err("%s: no more camera input\n", __func__);
		mutex_unlock(&ctrl->v4l2_lock);
		return -EINVAL;
	}

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_g_input(struct file *file, void *fh, unsigned int *i)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;

	fimc_info1("%s", __func__);

	/* In case of isueing g_input before s_input */
	if (!ctrl->cam) {
		dev_err(ctrl->dev,
				"no camera device selected yet!"
				"do VIDIOC_S_INPUT first\n");
		return -ENODEV;
	}

	*i = (unsigned int) ctrl->cam->id;

	fimc_dbg("%s: index %d\n", __func__, *i);

	return 0;
}

int fimc_s_input(struct file *file, void *fh, unsigned int i)
{
	struct fimc_global *fimc = get_fimc_dev();
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;
	int index, dev_index = -1;
	
	fimc_info1("%s", __func__);

	if (i >= FIMC_MAXCAMS) {
		fimc_err("%s: invalid input index\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&ctrl->v4l2_lock);

	fimc_dbg("%s: index %d\n", __func__, i);

	/*
	 * Align mounted camera in v4l2 input order
	 * (handling NULL devices)
	 * dev_index represents the actual index number
	 */
	for (index = 0; index < FIMC_MAXCAMS; index++) {
		/* If there is no exact camera H/W for exact index */
		if (!fimc->camera[index])
			continue;

		/* Found actual device */
		if (fimc->camera[index]) {
			/* Count actual device number */
			++dev_index;
			/* If the index number matches the expecting input i */
			if (dev_index == i)
				ctrl->cam = fimc->camera[index];
			else
				continue;
		}
	}

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_enum_fmt_vid_capture(struct file *file, void *fh,
					struct v4l2_fmtdesc *f)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;
	int i = f->index;

	fimc_dbg("%s\n", __func__);

	mutex_lock(&ctrl->v4l2_lock);
	
	if (f->index >= ARRAY_SIZE(capture_fmts)) {
		fimc_err("%s : %d index is not suppoted\n", __func__, f->index);
		mutex_unlock(&ctrl->v4l2_lock);
		return -EINVAL;
	}
	memset(f, 0, sizeof(*f));
	memcpy(f, &capture_fmts[i], sizeof(*f));

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_g_fmt_vid_capture(struct file *file, void *fh, struct v4l2_format *f)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;

	fimc_dbg("%s\n", __func__);

	if (!ctrl->cap) {
		fimc_err("%s: no capture device info\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&ctrl->v4l2_lock);

	memset(&f->fmt.pix, 0, sizeof(f->fmt.pix));
	memcpy(&f->fmt.pix, &ctrl->cap->fmt, sizeof(f->fmt.pix));

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

/*
 * Check for whether the requested format
 * can be streamed out from FIMC
 * depends on FIMC node
 */
static int fimc_fmt_avail(struct fimc_control *ctrl,
		struct v4l2_pix_format *f)
{
	int i;
	
	fimc_info1("%s", __func__);

	/*
	 * TODO: check for which FIMC is used.
	 * Available fmt should be varied for each FIMC
	 */

	for (i = 0; i < ARRAY_SIZE(capture_fmts); i++) {
		if (capture_fmts[i].pixelformat == f->pixelformat)
			return 0;
	}

	fimc_info1("Not supported pixelformat requested\n");

	return -1;
}

/*
 * figures out the depth of requested format
 */
static int fimc_fmt_depth(struct fimc_control *ctrl, struct v4l2_pix_format *f)
{
	int err, depth = 0;
	
	fimc_info1("%s", __func__);

	/* First check for available format or not */
	err = fimc_fmt_avail(ctrl, f);
	if (err < 0)
		return -1;

	/* handles only supported pixelformats */
	switch (f->pixelformat) {
	case V4L2_PIX_FMT_RGB32:
		depth = 32;
		fimc_dbg("32bpp\n");
		break;
	case V4L2_PIX_FMT_RGB565:
	case V4L2_PIX_FMT_RGB555:
	case V4L2_PIX_FMT_RGB444:
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_VYUY:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_YUV422P:
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
		depth = 16;
		fimc_dbg("16bpp\n");
		break;
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV12T:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_YUV420:
		depth = 12;
		fimc_dbg("12bpp\n");
		break;
	case V4L2_PIX_FMT_JPEG:
		depth = 8;
		fimc_dbg("8bpp\n");
		break;
	default:
		fimc_dbg("why am I here?\n");
		break;
	}

	return depth;
}

int fimc_s_fmt_vid_capture(struct file *file, void *fh, struct v4l2_format *f)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;
	struct fimc_capinfo *cap = ctrl->cap;
	int ret = 0;

	fimc_dbg("%s\n", __func__);
	if (!cap) {
		cap = kzalloc(sizeof(*cap), GFP_KERNEL);
		if (!cap) {
			fimc_err("%s: no memory for "
				"capture device info\n", __func__);
			return -ENOMEM;
		}

		/* assign to ctrl */
		ctrl->cap = cap;
	}
	mutex_lock(&ctrl->v4l2_lock);

	memset(&cap->fmt, 0, sizeof(cap->fmt));
	memcpy(&cap->fmt, &f->fmt.pix, sizeof(cap->fmt));

	/*
	 * Note that expecting format only can be with
	 * available output format from FIMC
	 * Following items should be handled in driver
	 * bytesperline = width * depth / 8
	 * sizeimage = bytesperline * height
	 */
	cap->fmt.bytesperline = (cap->fmt.width * fimc_fmt_depth(ctrl, &f->fmt.pix)) >> 3;
	cap->fmt.sizeimage = (cap->fmt.bytesperline * cap->fmt.height);

#ifdef SUPPORT_GSTREAMER
	cap->fmt.field = V4L2_FIELD_ANY;
#endif
	if (cap->fmt.pixelformat == V4L2_PIX_FMT_JPEG) {
		ctrl->sc.bypass = 1;
		cap->lastirq = 1;
	}

	if (!ctrl->cam) {
			fimc_err("%s : ctrl->cam is null\n", __func__);
			mutex_unlock(&ctrl->v4l2_lock);
			return -EINVAL;
	}
	/* WriteBack doesn't have subdev_call */
	if ((ctrl->cam->id == CAMERA_WB) || (ctrl->cam->id == CAMERA_WB_B)) {
		mutex_unlock(&ctrl->v4l2_lock);
		return 0;
	}

	//ret = subdev_call(ctrl, video, s_fmt, f);

	mutex_unlock(&ctrl->v4l2_lock);

	return ret;
}

int fimc_try_fmt_vid_capture(struct file *file, void *fh, struct v4l2_format *f)
{
	/* Not implement */	
	return -ENOTTY;
}

#ifdef CONFIG_VIDEO_FIMC_UMP_VCM_CMA
void fimc_phys_free(struct vcm_phys *phys)
{
	BUG_ON(phys->count != 1);
	printk("%s\n", __func__);
	cma_free(phys->parts[0].start);
	kfree(phys);
}

int fimc_vcm_alloc(struct fimc_control *ctrl, int buf_num, int buf_size)
{
	unsigned long arg = 0;
	struct ump_vcm ump_vcm;
	struct vcm_phys *phys = NULL;
	dma_addr_t phys_addr;
	
	fimc_info1("%s", __func__);

	phys = kmalloc(sizeof(*phys) + sizeof(*phys->parts), GFP_KERNEL);
	memset(phys, 0, sizeof(*phys) + sizeof(*phys->parts));

	phys_addr = (dma_addr_t)cma_alloc(ctrl->dev, ctrl->cma_name, (size_t)buf_size, 0);
	fimc_info1("%s: phys_addr : 0x%x, ctrl->dev : 0x%x, ctrl->name : %s\n", __func__,
			phys_addr, (unsigned int)ctrl->dev, (char*)ctrl->name);
	phys->count = 1;
	phys->size = buf_size;
	phys->free = fimc_phys_free;
	phys->parts[0].start = phys_addr;
	phys->parts[0].size = buf_size;

	ctrl->dev_vcm_res[buf_num] = vcm_map(ctrl->dev_vcm, phys, 0);
	
	/* physical address */
	ctrl->mem.base = ctrl->dev_vcm_res[buf_num]->phys->parts->start;
	
	/* virtual address */
	ctrl->mem.vaddr_base = ctrl->dev_vcm_res[buf_num]->start;

	ctrl->mem.curr = ctrl->mem.base;
	ctrl->mem.vaddr_curr = ctrl->mem.vaddr_base;

	fimc_info1("%s : vaddr base : 0x%x\n", __func__, ctrl->mem.vaddr_base);
	ctrl->mem.size = buf_size;

	/* UMP */
	ctrl->ump_memory_description.addr = ctrl->mem.base;
	ctrl->ump_memory_description.size = ctrl->mem.size;

	ump_vcm.vcm = ctrl->dev_vcm;
	ump_vcm.vcm_res = ctrl->dev_vcm_res[buf_num];
	ump_vcm.dev_id = ctrl->vcm_id;
	arg = (unsigned int)&ump_vcm;

	ctrl->ump_wrapped_buffer[buf_num] =
		ump_dd_handle_create_from_phys_blocks( &ctrl->ump_memory_description, 1);

	if (UMP_DD_HANDLE_INVALID == ctrl->ump_wrapped_buffer[buf_num]) {
		fimc_err("%s : ump_wrapped_buffer is unhandled\n", __func__);
		return -ENOMEM;
	}
#ifdef CONFIG_UMP_VCM_ALLOC
	if (ump_dd_meminfo_set(ctrl->ump_wrapped_buffer[buf_num], (void*)arg))
		return -ENOMEM;
#endif
	return 0;
}
#endif
static int fimc_alloc_buffers(struct fimc_control *ctrl,
				int plane, int size, int align, int bpp)
{
	struct fimc_capinfo *cap = ctrl->cap;
	int i, j;
	int plane_length[3];
#ifdef CONFIG_VIDEO_FIMC_UMP_VCM_CMA
	int buf_size;
#endif
	if (plane < 1 || plane > 3)
		return -ENOMEM;
		
	fimc_info1("%s", __func__);

	switch (plane) {
	case 1:
		plane_length[0] = PAGE_ALIGN((size*bpp) >> 3);
		plane_length[1] = 0;
		plane_length[2] = 0;
		break;
	/* In case of 2, only NV12 and NV12T is supported. */
	case 2:
		plane_length[0] = PAGE_ALIGN((size*8) >> 3);
		plane_length[1] = PAGE_ALIGN((size*(bpp-8)) >> 3);
		plane_length[2] = 0;
		break;
	/* In case of 3
	 * YUV422 : 8 / 4 / 4 (bits)
	 * YUV420 : 8 / 2 / 2 (bits)
	 * 3rd plane have to consider page align for mmap */
	case 3:
		plane_length[0] = (size*8) >> 3;
		plane_length[1] = (size*((bpp-8)/2)) >> 3;
		plane_length[2] = PAGE_ALIGN((size*bpp)>>3) - plane_length[0] - plane_length[1];
		break;
	default:
		fimc_err("impossible!\n");
		return -ENOMEM;
	}
#ifdef CONFIG_VIDEO_FIMC_UMP_VCM_CMA
	/* set each buffer pointer in nr_bufs */
	if (!align)
		buf_size = plane_length[0] + plane_length[1] + plane_length[2];
	else
		buf_size = ALIGN(plane_length[0], align) + ALIGN(plane_length[1], align)
			 + ALIGN(plane_length[2], align);
#endif
	for (i = 0; i < cap->nr_bufs; i++) {
#ifdef CONFIG_VIDEO_FIMC_UMP_VCM_CMA
		fimc_vcm_alloc(ctrl, i, buf_size);
#endif
		for (j = 0; j < plane; j++) {
			cap->bufs[i].length[j] = plane_length[j];
			fimc_dma_alloc(ctrl, &cap->bufs[i], j, align);
			
			fimc_info1("%s : cap->bufs[%d].base[%d] : 0x%x\n", __func__,\
				i, j, cap->bufs[i].base[j]);
			fimc_info1("%s : cap->bufs[%d].vaddr_base[%d] : 0x%x\n", __func__,\
				i, j, cap->bufs[i].vaddr_base[j]);
			if (!cap->bufs[i].base[j])
				goto err_alloc;
		}
		cap->bufs[i].state = VIDEOBUF_PREPARED;
	}

	return 0;

err_alloc:
	for (i = 0; i < cap->nr_bufs; i++) {
		for (j = 0; j < plane; j++) {
			if (cap->bufs[i].base[j])
				fimc_dma_free(ctrl, &cap->bufs[i], j);
		}
		memset(&cap->bufs[i], 0, sizeof(cap->bufs[i]));
	}

	return -ENOMEM;
}


int fimc_reqbufs_capture(void *fh, struct v4l2_requestbuffers *b)
{
	struct fimc_control *ctrl = fh;
	struct fimc_capinfo *cap = ctrl->cap;
	struct s3c_platform_fimc *pdata = to_fimc_plat(ctrl->dev);
#if (defined(CONFIG_S5PV310_DEV_PD) && defined(CONFIG_PM_RUNTIME))
	struct platform_device *pdev = to_platform_device(ctrl->dev);
#endif
	int ret = 0, i;
	int bpp = 0;
	
	fimc_info1("%s", __func__);

	if (!cap) {
		fimc_err("%s: no capture device info\n", __func__);
		return -ENODEV;
	}

	mutex_lock(&ctrl->v4l2_lock);

	/*  A count value of zero frees all buffers */
	if ((b->count == 0) || (b->count >= FIMC_CAPBUFS)) {
		/* aborting or finishing any DMA in progress */
		if (ctrl->status == FIMC_STREAMON)
			fimc_streamoff_capture(fh);
		for (i = 0; i < FIMC_CAPBUFS; i++) {
			fimc_dma_free(ctrl, &ctrl->cap->bufs[i], 0);
			fimc_dma_free(ctrl, &ctrl->cap->bufs[i], 1);
			fimc_dma_free(ctrl, &ctrl->cap->bufs[i], 2);
		}

		mutex_unlock(&ctrl->v4l2_lock);
		return 0;
	}
	/* free previous buffers */
	if ((cap->nr_bufs >= 0) && (cap->nr_bufs < FIMC_CAPBUFS)) {
		fimc_info1("%s : remained previous buffer count is %d\n", __func__,
				cap->nr_bufs);
		for (i = 0; i < cap->nr_bufs; i++) {
			fimc_dma_free(ctrl, &cap->bufs[i], 0);
			fimc_dma_free(ctrl, &cap->bufs[i], 1);
			fimc_dma_free(ctrl, &cap->bufs[i], 2);
		}
	}

	cap->nr_bufs = b->count;
	if (pdata->hw_ver >= 0x51)
	{
#if (defined(CONFIG_S5PV310_DEV_PD) && defined(CONFIG_PM_RUNTIME))
		if (ctrl->power_status == FIMC_POWER_OFF) {
			pm_runtime_get_sync(&pdev->dev);
			vcm_set_pgtable_base(ctrl->vcm_id);
		}
#endif
		fimc_hw_reset_output_buf_sequence(ctrl);
		for (i = 0; i < cap->nr_bufs; i++) {
			fimc_hwset_output_buf_sequence(ctrl, i, 1);
			cap->bufs[i].id = i;
			cap->bufs[i].state = VIDEOBUF_NEEDS_INIT;

			/* initialize list */
			INIT_LIST_HEAD(&cap->bufs[i].list);
		}
		fimc_err("%s: requested %d buffers\n", __func__, b->count);
		fimc_err("%s : sequence[%d]\n", __func__, fimc_hwget_output_buf_sequence(ctrl));
		INIT_LIST_HEAD(&cap->outgoing_q);
	}
	if (pdata->hw_ver < 0x51) {
		INIT_LIST_HEAD(&cap->inq);
		for (i = 0; i < cap->nr_bufs; i++) {
			cap->bufs[i].id = i;
			cap->bufs[i].state = VIDEOBUF_NEEDS_INIT;

			/* initialize list */
			INIT_LIST_HEAD(&cap->bufs[i].list);
		}
	}

	bpp = fimc_fmt_depth(ctrl, &cap->fmt);
	fimc_info1("%s : bpp : %d\n", __func__, bpp);
	
	switch (cap->fmt.pixelformat) {
	case V4L2_PIX_FMT_JPEG:		/* fall through */
	case V4L2_PIX_FMT_RGB32:	/* fall through */
	case V4L2_PIX_FMT_RGB565:	/* fall through */
	case V4L2_PIX_FMT_RGB555:	/* fall through */
	case V4L2_PIX_FMT_RGB444:	/* fall through */
	case V4L2_PIX_FMT_YUYV:		/* fall through */
	case V4L2_PIX_FMT_UYVY:		/* fall through */
	case V4L2_PIX_FMT_VYUY:		/* fall through */
	case V4L2_PIX_FMT_YVYU:		/* fall through */
	case V4L2_PIX_FMT_NV16:		/* fall through */
	case V4L2_PIX_FMT_NV61:		/* fall through */
	case V4L2_PIX_FMT_NV21:
		fimc_info1("%s : 1plane\n", __func__);
		ret = fimc_alloc_buffers(ctrl, 1,
			cap->fmt.width * cap->fmt.height, 0, bpp);
		break;

	case V4L2_PIX_FMT_NV12:		/* fall through */
		fimc_info1("%s : 2plane for NV12\n", __func__);
		ret = fimc_alloc_buffers(ctrl, 2,
			cap->fmt.width * cap->fmt.height, SZ_64K, bpp);
		break;
	case V4L2_PIX_FMT_NV12T:
		fimc_info1("%s : 2plane for NV12T\n", __func__);
		ret = fimc_alloc_buffers(ctrl, 2,
			ALIGN(cap->fmt.width, 128) * ALIGN(cap->fmt.height, 32), SZ_64K, bpp);
		break;

	case V4L2_PIX_FMT_YUV422P:	/* fall through */
	case V4L2_PIX_FMT_YUV420:
		fimc_info1("%s : 3plane\n", __func__);
		ret = fimc_alloc_buffers(ctrl, 3,
			cap->fmt.width * cap->fmt.height, 0, bpp);
		break;
	default:
		break;
	}

	if (ret) {
		fimc_err("%s: no memory for capture buffer\n", __func__);
		mutex_unlock(&ctrl->v4l2_lock);
		return -ENOMEM;
	}

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_querybuf_capture(void *fh, struct v4l2_buffer *b)
{
	struct fimc_control *ctrl = fh;
	struct fimc_capinfo *cap = ctrl->cap;
	
	fimc_info1("%s", __func__);

	if (ctrl->status != FIMC_STREAMOFF) {
		fimc_err("fimc is running\n");
		return -EBUSY;
	}

	mutex_lock(&ctrl->v4l2_lock);

	switch (cap->fmt.pixelformat) {
	case V4L2_PIX_FMT_JPEG:		/* fall through */
	case V4L2_PIX_FMT_RGB32:	/* fall through */
	case V4L2_PIX_FMT_RGB565:	/* fall through */
	case V4L2_PIX_FMT_RGB555:	/* fall through */
	case V4L2_PIX_FMT_RGB444:	/* fall through */
	case V4L2_PIX_FMT_YUYV:		/* fall through */
	case V4L2_PIX_FMT_UYVY:		/* fall through */
	case V4L2_PIX_FMT_VYUY:		/* fall through */
	case V4L2_PIX_FMT_YVYU:		/* fall through */
	case V4L2_PIX_FMT_NV16:		/* fall through */
	case V4L2_PIX_FMT_NV61:		/* fall through */
	case V4L2_PIX_FMT_NV21:
		b->length = cap->bufs[b->index].length[0];
		break;

	case V4L2_PIX_FMT_NV12:		/* fall through */
	case V4L2_PIX_FMT_NV12T:
		b->length = ALIGN(ctrl->cap->bufs[b->index].length[0], SZ_64K)
			+ ALIGN(ctrl->cap->bufs[b->index].length[1], SZ_64K);
		break;
	case V4L2_PIX_FMT_YUV422P:	/* fall through */
	case V4L2_PIX_FMT_YUV420:
		b->length = ctrl->cap->bufs[b->index].length[0]
			+ ctrl->cap->bufs[b->index].length[1]
			+ ctrl->cap->bufs[b->index].length[2];
		break;

	default:
		b->length = cap->bufs[b->index].length[0];
		break;
	}
	b->m.offset = b->index * PAGE_SIZE;
	/* memory field should filled V4L2_MEMORY_MMAP */
	b->memory = V4L2_MEMORY_MMAP;

	ctrl->cap->bufs[b->index].state = VIDEOBUF_IDLE;

	fimc_dbg("%s: %d bytes with offset: %d\n",
		__func__, b->length, b->m.offset);

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_g_ctrl_capture(void *fh, struct v4l2_control *c)
{
	struct fimc_control *ctrl = fh;
	int ret = 0;

	fimc_dbg("%s\n", __func__);

	mutex_lock(&ctrl->v4l2_lock);

	switch (c->id) {
	case V4L2_CID_ROTATION:
		ctrl->cap->rotate = c->value;
		break;

	case V4L2_CID_HFLIP:	/* fall through */
	case V4L2_CID_VFLIP:
		ctrl->cap->flip = c->id;
		break;
	default:
		/* get ctrl supported by subdev */
		/* WriteBack doesn't have subdev_call */
		if ((ctrl->cam->id == CAMERA_WB) || (ctrl->cam->id == CAMERA_WB_B))
			break;
		if (!ctrl->cam) {
				fimc_err("%s : ctrl->cam is null\n", __func__);
				mutex_unlock(&ctrl->v4l2_lock);
				return -EINVAL;
		}
		ret = subdev_call(ctrl, core, g_ctrl, c);
		break;
	}

	mutex_unlock(&ctrl->v4l2_lock);

	return ret;
}

int fimc_s_ctrl_capture(void *fh, struct v4l2_control *c)
{
	struct fimc_control *ctrl = fh;
	int ret = 0;

	fimc_dbg("%s\n", __func__);

	mutex_lock(&ctrl->v4l2_lock);

	switch (c->id) {
	case V4L2_CID_ROTATION:
		ctrl->cap->rotate = c->value;
		break;

	case V4L2_CID_HFLIP:
		if (c->value)
			ctrl->cap->flip |= FIMC_XFLIP;
		else
			ctrl->cap->flip &= ~FIMC_XFLIP;
		break;

	case V4L2_CID_VFLIP:
		if (c->value)
			ctrl->cap->flip |= FIMC_YFLIP;
		else
			ctrl->cap->flip &= ~FIMC_YFLIP;
		break;

	case V4L2_CID_PADDR_Y:
		c->value = ctrl->cap->bufs[c->value].base[FIMC_ADDR_Y];
		break;

	case V4L2_CID_PADDR_CB:		/* fall through */
	case V4L2_CID_PADDR_CBCR:
		c->value = ctrl->cap->bufs[c->value].base[FIMC_ADDR_CB];
		break;

	case V4L2_CID_PADDR_CR:
		c->value = ctrl->cap->bufs[c->value].base[FIMC_ADDR_CR];
		break;
#ifdef CONFIG_VIDEO_FIMC_UMP_VCM_CMA
	case V4L2_CID_GET_UMP_SECURE_ID:
	{
		ump_secure_id secure_id = 
			ump_dd_secure_id_get(ctrl->ump_wrapped_buffer[c->value]);
		c->value = secure_id;
		fimc_info1("%s : ump_secure_id : %d\n", __func__, secure_id);

		break;
	}
#endif
	case V4L2_CID_RGB_ALPHA:
		fimc_hwset_output_rgb_alpha(ctrl, c->value);
		break;
	default:
		/* try on subdev */
		/* WriteBack doesn't have subdev_call */

		if ((ctrl->cam->id == CAMERA_WB) || (ctrl->cam->id == CAMERA_WB_B))
			break;
		if (!ctrl->cam) {
				fimc_err("%s : ctrl->cam is null\n", __func__);
				mutex_unlock(&ctrl->v4l2_lock);
				return -EINVAL;
		}
		ret = subdev_call(ctrl, core, s_ctrl, c);
		break;
	}

	mutex_unlock(&ctrl->v4l2_lock);

	return ret;
}

int fimc_cropcap_capture(void *fh, struct v4l2_cropcap *a)
{
	struct fimc_control *ctrl = fh;
	struct fimc_capinfo *cap = ctrl->cap;
	struct fimc_global *fimc = get_fimc_dev();
	struct s3c_platform_fimc *pdata;

	fimc_dbg("%s\n", __func__);

	mutex_lock(&ctrl->v4l2_lock);

	pdata = to_fimc_plat(ctrl->dev);
	if (!ctrl->cam) {
		ctrl->cam = fimc->camera[pdata->default_cam];
	}

	if (!cap) {
		cap = kzalloc(sizeof(*cap), GFP_KERNEL);
		if (!cap) {
			fimc_err("%s: no memory for "
				"capture device info\n", __func__);
			return -ENOMEM;
		}

		/* assign to ctrl */
		ctrl->cap = cap;
	}

	/* crop limitations */
	cap->cropcap.bounds.left = 0;
	cap->cropcap.bounds.top = 0;
	cap->cropcap.bounds.width = ctrl->cam->width;
	cap->cropcap.bounds.height = ctrl->cam->height;

	/* crop default values */
	cap->cropcap.defrect.left = 0;
	cap->cropcap.defrect.top = 0;
	cap->cropcap.defrect.width = ctrl->cam->width;
	cap->cropcap.defrect.height = ctrl->cam->height;

	a->bounds = cap->cropcap.bounds;
	a->defrect = cap->cropcap.defrect;

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_g_crop_capture(void *fh, struct v4l2_crop *a)
{
	struct fimc_control *ctrl = fh;

	fimc_dbg("%s\n", __func__);

	mutex_lock(&ctrl->v4l2_lock);
	a->c = ctrl->cap->crop;
	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_s_crop_capture(void *fh, struct v4l2_crop *a)
{
	struct fimc_control *ctrl = fh;

	fimc_dbg("%s\n", __func__);

	mutex_lock(&ctrl->v4l2_lock);
	ctrl->cap->crop = a->c;
	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_start_capture(struct fimc_control *ctrl)
{
	fimc_dbg("%s", __func__);

	if (!ctrl->sc.bypass)
		fimc_hwset_start_scaler(ctrl);

	fimc_hwset_enable_capture(ctrl, ctrl->sc.bypass);

	return 0;
}

int fimc_stop_capture(struct fimc_control *ctrl)
{
	fimc_dbg("%s\n", __func__);

	if (ctrl->cap->lastirq) {
		fimc_hwset_enable_lastirq(ctrl);
		fimc_hwset_disable_capture(ctrl);
		fimc_hwset_disable_lastirq(ctrl);
	} else {
		fimc_hwset_disable_capture(ctrl);
	}

	fimc_hwset_stop_scaler(ctrl);
	/* wait for stop hardware */
	fimc_hwget_frame_end(ctrl);

	return 0;
}

int fimc_streamon_capture(void *fh)
{
	struct fimc_control *ctrl = fh;
	struct fimc_capinfo *cap = ctrl->cap;
	int rot, i;
	int ret = 0;	
	struct s3c_platform_camera *cam = NULL;
	u32 pixelformat;
	
	struct s3c_platform_fimc *pdata = to_fimc_plat(ctrl->dev);

	fimc_dbg("%s\n", __func__);

	/* enable camera power if needed */
	if (ctrl->cam->cam_power)
		ctrl->cam->cam_power(1);

	if (pdata->hw_ver < 0x51)
		fimc_hw_reset_camera(ctrl);
#if (!defined(CONFIG_S5PV310_DEV_PD) && !defined(CONFIG_PM_RUNTIME))
	ctrl->status = FIMC_READY_ON;
#endif
	cap->irq = 0;

	fimc_hwset_enable_irq(ctrl, 0, 1);

	if (!ctrl->cam->initialized)
		fimc_init_camera(ctrl);

/* csi control position change because runtime pm */
	if (ctrl->cam)
		cam = ctrl->cam;
	/* subdev call for init */
	if (ctrl->cap->fmt.pixelformat == V4L2_PIX_FMT_JPEG) {
		ret = v4l2_subdev_call(cam->sd, core, init, 1);
		pixelformat = V4L2_PIX_FMT_JPEG;
	} else {
		ret = v4l2_subdev_call(cam->sd, core, init, 0);		// will invoke s5k4ba_init()
		pixelformat = cam->pixelformat;
	}

	if (ret == -ENOIOCTLCMD) {
		fimc_err("%s: init subdev api not supported\n", __func__);
		return ret;
	}

	if (cam->type == CAM_TYPE_MIPI) {
		/*
		 * subdev call for sleep/wakeup:
		 * no error although no s_stream api support
		*/
		v4l2_subdev_call(cam->sd, video, s_stream, 0);
		if (cam->id == CAMERA_CSI_C) {
			s3c_csis_start(CSI_CH_0, cam->mipi_lanes, cam->mipi_settle, \
				cam->mipi_align, cam->width, cam->height, pixelformat);
		} else {
			s3c_csis_start(CSI_CH_1, cam->mipi_lanes, cam->mipi_settle, \
				cam->mipi_align, cam->width, cam->height, pixelformat);
		}
		v4l2_subdev_call(cam->sd, video, s_stream, 1);
	}

	/* Set FIMD to write back */
	if ((ctrl->cam->id == CAMERA_WB) || (ctrl->cam->id == CAMERA_WB_B)) {
		if (ctrl->cam->id == CAMERA_WB)
			fimc_hwset_sysreg_camblk_fimd0_wb(ctrl);
		else
			fimc_hwset_sysreg_camblk_fimd1_wb(ctrl);

		s3cfb_direct_ioctl(0, S3CFB_SET_WRITEBACK, 1);
	}

	fimc_hwset_camera_source(ctrl);
	fimc_hwset_camera_offset(ctrl);
	fimc_hwset_camera_type(ctrl);
	fimc_hwset_camera_polarity(ctrl);
	fimc_capture_scaler_info(ctrl);
	fimc_hwset_prescaler(ctrl, &ctrl->sc);
	fimc_hwset_scaler(ctrl, &ctrl->sc);
	// Use case requests either RGB565/RGB888 as of now
	fimc_hwset_output_colorspace(ctrl, cap->fmt.pixelformat);
	fimc_hwset_output_addr_style(ctrl, cap->fmt.pixelformat);

	if (cap->fmt.pixelformat == V4L2_PIX_FMT_RGB32 ||
			cap->fmt.pixelformat == V4L2_PIX_FMT_RGB565 ||
			cap->fmt.pixelformat == V4L2_PIX_FMT_RGB555 ||
			cap->fmt.pixelformat == V4L2_PIX_FMT_RGB444) {
		fimc_info1("%s :  version : %x\n", __func__, pdata->hw_ver);
		fimc_hwset_output_rgb(ctrl, cap->fmt.pixelformat);
		if (pdata->hw_ver >= 0x52)
			fimc_hwset_output_rgb16b_fmt(ctrl, cap->fmt.pixelformat);
	}
	else
		fimc_hwset_output_yuv(ctrl, cap->fmt.pixelformat);

	fimc_hwset_output_size(ctrl, cap->fmt.width, cap->fmt.height);
	fimc_hwset_output_area(ctrl, cap->fmt.width, cap->fmt.height);
	fimc_hwset_output_scan(ctrl, &cap->fmt);

	fimc_hwset_output_rot_flip(ctrl, cap->rotate, cap->flip);
	rot = fimc_mapping_rot_flip(cap->rotate, cap->flip);

	if (rot & FIMC_ROT) {
		fimc_hwset_org_output_size(ctrl, cap->fmt.height, cap->fmt.width);
	} else {
		fimc_hwset_org_output_size(ctrl, cap->fmt.width, cap->fmt.height);
	}

	if (pdata->hw_ver >= 0x51) {
		for (i = 0; i < cap->nr_bufs; i++)
			fimc_hwset_output_address(ctrl, &cap->bufs[i], i);
	} else {
		for (i = 0; i < FIMC_PINGPONG; i++)
			fimc_add_outqueue(ctrl, i);
	}
	
	fimc_start_capture(ctrl);
	ctrl->status = FIMC_STREAMON;

	/* if available buffer did not remained */
	return 0;
}

int fimc_streamoff_capture(void *fh)
{
	struct fimc_control *ctrl = fh;
	struct fimc_capinfo *cap = ctrl->cap;

	struct s3c_platform_fimc *pdata = to_fimc_plat(ctrl->dev);
	
	fimc_dbg("%s\n", __func__);

	/* disable camera power */
	if (ctrl->cam->cam_power)
		ctrl->cam->cam_power(0);

	ctrl->status = FIMC_READY_OFF;
	fimc_stop_capture(ctrl);
	if (ctrl->cam->type == CAM_TYPE_MIPI) {
		if (ctrl->cam->id == CAMERA_CSI_C)
			s3c_csis_stop(CSI_CH_0);
		else
			s3c_csis_stop(CSI_CH_1);
	}

	if (pdata->hw_ver < 0x51)
		INIT_LIST_HEAD(&cap->inq);
/* Do not call init function in camera mode */
	if (ctrl->power_status != FIMC_POWER_SUSPEND)	
		ctrl->cam->initialized = 0;
	ctrl->status = FIMC_STREAMOFF;

	return 0;
}

int fimc_qbuf_capture(void *fh, struct v4l2_buffer *b)
{
	struct fimc_control *ctrl = fh;
	struct s3c_platform_fimc *pdata = to_fimc_plat(ctrl->dev);
	struct fimc_capinfo *cap = ctrl->cap;
#if (defined(CONFIG_S5PV310_DEV_PD) && defined(CONFIG_PM_RUNTIME))
	struct platform_device *pdev = to_platform_device(ctrl->dev);
	int ready_to_start = 0;
	/* This is for writeback mode */
	if (ctrl->power_status == FIMC_POWER_OFF) {
		pm_runtime_get_sync(&pdev->dev);
		vcm_set_pgtable_base(ctrl->vcm_id);
		ready_to_start = 1;
	}
#endif
	fimc_info1("%s", __func__);

	if (b->memory != V4L2_MEMORY_MMAP) {
		fimc_err("%s: invalid memory type\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&ctrl->v4l2_lock);
	if (pdata->hw_ver >= 0x51) {
		if (cap->bufs[b->index].state != VIDEOBUF_IDLE) {
			fimc_err("%s: invalid state\n", __func__);
			mutex_unlock(&ctrl->v4l2_lock);
			return -EINVAL;
		}
		else {
			fimc_info2("%s : b->index : %d\n", __func__, b->index);
			fimc_hwset_output_buf_sequence(ctrl, b->index, FIMC_FRAMECNT_SEQ_ENABLE);
			cap->bufs[b->index].state = VIDEOBUF_QUEUED;
#if (defined(CONFIG_S5PV310_DEV_PD) && defined(CONFIG_PM_RUNTIME))
			if(ready_to_start == 1) {
				fimc_info1("%s : wake up \n", __func__);
				fimc_streamon_capture(ctrl);
			}
#else
			if (ctrl->status == FIMC_BUFFER_STOP) {
				fimc_start_capture(ctrl);
				ctrl->status = FIMC_STREAMON;
			}
#endif
		}
	} else {
		fimc_add_inqueue(ctrl, b->index);
	}

	mutex_unlock(&ctrl->v4l2_lock);

	return 0;
}

int fimc_dqbuf_capture(void *fh, struct v4l2_buffer *b)
{
	struct fimc_control *ctrl = fh;
	struct fimc_capinfo *cap = ctrl->cap;
	struct fimc_buf_set *buf;
	int pp, ret = 0;

	struct s3c_platform_fimc *pdata = to_fimc_plat(ctrl->dev);
	
	fimc_info1("%s", __func__);

	if (b->memory != V4L2_MEMORY_MMAP) {
		fimc_err("%s: invalid memory type\n", __func__);
		mutex_unlock(&ctrl->v4l2_lock);
		return -EINVAL;
	}

	mutex_lock(&ctrl->v4l2_lock);
	if (pdata->hw_ver >= 0x51) {
		if (list_empty(&cap->outgoing_q)) {
			fimc_err("%s: outgoing_q is empty\n", __func__);
			mutex_unlock(&ctrl->v4l2_lock);
			return -EAGAIN;
		}
		else
		{
			buf = list_first_entry(&cap->outgoing_q, struct fimc_buf_set, list);
			fimc_info2("%s: buf->id : %d\n", __func__, buf->id);
			b->index = buf->id;
			buf->state = VIDEOBUF_IDLE; 
			
			list_del(&buf->list);
		}
			
	} else {
		pp = ((fimc_hwget_frame_count(ctrl) + 2) % 4);
		if (cap->fmt.field == V4L2_FIELD_INTERLACED_TB)
			pp &= ~0x1;
		b->index = cap->outq[pp];
		fimc_info2("%s: buffer(%d) outq[%d]\n", __func__, b->index, pp);
		ret = fimc_add_outqueue(ctrl, pp);
		if (ret) {
			b->index = -1;
			fimc_err("%s: no inqueue buffer\n", __func__);
		}
	}
#ifdef SUPPORT_GSTREAMER
/* This is contravention to V4L2 standard,
 * because capture type is bytesused field not used */
	b->bytesused = cap->fmt.sizeimage;
#endif
	mutex_unlock(&ctrl->v4l2_lock);

	return ret;
}

int fimc_enum_framesizes(struct file *filp, void *fh, struct v4l2_frmsizeenum *fsize)
{
	struct fimc_control *ctrl = ((struct fimc_prv_data *)fh)->ctrl;
	int i;
	u32 index = 0;
	fimc_info1("%s", __func__);
	for (i = 0; i < ARRAY_SIZE(capture_fmts); i++)
	{
		if (fsize->pixel_format != capture_fmts[i].pixelformat)
			continue;
		if (fsize->index == index) {
			fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
			/* this is camera sensor's width, height.
			 * originally this should be filled each file format
			 */
			fsize->discrete.width = ctrl->cam->width;
			fsize->discrete.height = ctrl->cam->height;

			return 0;
		}
		index++;
	}

	return -EINVAL;
}
int fimc_enum_frameintervals(struct file *filp, void *fh, struct v4l2_frmivalenum *fival)
{
	if (fival->index > 0)
		return -EINVAL;
	/* temporary only support 30fps */
	fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fival->discrete.numerator = 1000;
	fival->discrete.denominator = 30000;

	return 0;
}
