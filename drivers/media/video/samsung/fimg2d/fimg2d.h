/* linux/drivers/media/video/samsung/fimg2d/fimg2d.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *	http://www.samsung.com/
 *
 * Samsung Graphics 2D driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _FIMG2D_H_
#define _FIMG2D_H_ __FILE__

#include <linux/clk.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>

/* driver helpers */
#define to_fimg2d_plat(d)		(to_platform_device(d)->dev.platform_data)

/* generic */
#define FIMG2D_MINOR			(240)
#define FIMG2D_DIR_NEGATIVE		(-1)

/* debug macro */
#ifdef CONFIG_VIDEO_FIMG2D_DEBUG
#define fimg2d_debug(fmt, arg...)	printk(KERN_DEBUG "[%s] " fmt, __FUNCTION__, ## arg)
#else
#define fimg2d_debug(fmt, arg...)	do { } while (0)
#endif

/* ioctls */
#define FIMG2D_IOCTL_MAGIC		'F'
#define FIMG2D_BITBLT_CONFIG		_IOWR(FIMG2D_IOCTL_MAGIC, 0, struct fimg2d_user_context)
#define FIMG2D_BITBLT_UPDATE		_IOWR(FIMG2D_IOCTL_MAGIC, 1, struct fimg2d_user_region)
#define FIMG2D_BITBLT_CLOSE		_IO(FIMG2D_IOCTL_MAGIC, 2)
#define FIMG2D_BITBLT_START		_IO(FIMG2D_IOCTL_MAGIC, 3)
#define FIMG2D_BITBLT_WAIT		_IO(FIMG2D_IOCTL_MAGIC, 4)
#define FIMG2D_DMA_CACHE_INVAL		_IOWR(FIMG2D_IOCTL_MAGIC, 5, struct fimg2d_dma_info)
#define FIMG2D_DMA_CACHE_CLEAN		_IOWR(FIMG2D_IOCTL_MAGIC, 6, struct fimg2d_dma_info)
#define FIMG2D_DMA_CACHE_FLUSH		_IOWR(FIMG2D_IOCTL_MAGIC, 7, struct fimg2d_dma_info)
#define FIMG2D_DMA_CACHE_FLUSH_ALL	_IO(FIMG2D_IOCTL_MAGIC, 8)


typedef int FIMG2D_ADDR_TYPE_T;
#define	FIMG2D_ADDR_NONE	0x0000	/* address is not set */
#define	FIMG2D_ADDR_PHYS	0x0001	/* physical address */
#define	FIMG2D_ADDR_KERN	0x0002	/* kernel virtual address */
#define	FIMG2D_ADDR_USER	0x0004	/* user virtual address */
#define	FIMG2D_ADDR_VCMM	0x0008	/* device virtual address */
#define	FIMG2D_ADDR_COOKIE	0x0100	/* key to kernel virtual address (for vmem) */
#define	FIMG2D_ADDR_SECUID	0x0200	/* key to device virtual address (for vcm) */

typedef enum img_t {
	NORMAL,
	PATTERN,
	FGCOLOR,
	BGCOLOR,
} FIMG2D_IMG_T;

typedef enum rgb_order_t {
	AX_RGB = 0,
	RGB_AX,
	AX_BGR,
	BGR_AX,
} FIMG2D_RGB_ORDER_T;

typedef enum rgb_format_t {
	XRGB8888 = 0,
	ARGB8888,
	RGB565,
	XRGB1555,
	ARGB1555,
	XRGB4444,
	ARGB4444,
	RGB888,
} FIMG2D_RGB_FORMAT_T;

typedef enum rotation_t {
	ORIGIN,
	ROT90,
	ROT180,
	ROT270,
	XFLIP,
	YFLIP,
} FIMG2D_ROTATION_T;

typedef enum nonpreblend_alpha_t {
	DISABLED,
	PERPIXEL,
	CONSTANT,
} FIMG2D_NONPRE_ALPHA_T;

typedef enum bitblt_t {
	OP_SOLID_FILL,
	OP_SRC_COPY,
	OP_PAT_COPY,
	OP_PAT_BLEND,
	OP_CLEAR,
	OP_SRC,
	OP_DST,
	OP_OVER,
	OP_OVER_REV,
	OP_IN,
	OP_IN_REV,
	OP_OUT,
	OP_OUT_REV,
	OP_ATOP,
	OP_ATOP_REV,
	OP_XOR,
	OP_ADD,
	OP_SATURATE,
} FIMG2D_BITBLT_T;

/**
 * struct fimg2d_dma_info - dma info
 * @addr: physical address
 * @size: size
*/
struct fimg2d_dma_info {
	unsigned long addr;
	size_t size;
	FIMG2D_ADDR_TYPE_T addr_type;
};

/**
 * struct fimg2d_rect - region structure
 * @x1: left top x
 * @y1: left top y
 * @x2: right bottom x
 * @y2: right bottom y
*/
struct fimg2d_rect {
	int x1;
	int y1;
	int x2;
	int y2;
};

/**
 * struct fimg2d_alpha - alpha info
 * @enabled: 1 if enabled
 * @nonpre_type: non-preblend alpha type (DISABLE, PERPIXEL, CONSTANT)
 * @value: value for constant blending
*/
struct fimg2d_alpha {
	int enabled;
	FIMG2D_NONPRE_ALPHA_T nonpre_type;
	int value;
};

/**
 * struct fimg2d_image - src and dst info
 * @type: image type
 * @addr: address with addr_type
 * @addr_type: physical, kernel virtual or user virtual address type
 * @width: image width
 * @height: image height
 * @stride:
 * @order: RGB order
 * @fmt: RGB format
 * @rect: region info
*/
struct fimg2d_image {
	FIMG2D_IMG_T type;
	unsigned long addr;
	FIMG2D_ADDR_TYPE_T addr_type;
	int width;
	int height;
	int stride;
	FIMG2D_RGB_ORDER_T order;
	FIMG2D_RGB_FORMAT_T fmt;
	struct fimg2d_rect rect;
};

/**
 * struct fimg2d_param - param info
 * @op: bitblt operation type
 * @color: solid fill color
 * @dx: -1 if negative direction of x
 * @dy: -1 if negative direction of y
 * @alpha: alpha info
 * @rot: rotation info
 * @scale: would enable stretch bitblt, if 1
 * @clip: would enable clip region, if 1
*/
struct fimg2d_param {
	FIMG2D_BITBLT_T op;
	unsigned long color;
	int dx;
	int dy;
	struct fimg2d_alpha alpha;
	FIMG2D_ROTATION_T rot;
	int scale;
};

/**
 * struct fimg2d_user_region - region for user
 * @src: src region
 * @dst: dst region
 * @dst_clip: dst clip region
*/
struct fimg2d_user_region {
	struct fimg2d_rect *src;
	struct fimg2d_rect *dst;
	struct fimg2d_rect *dst_clip;
};

/**
 * struct fimg2d_user_context - context for user
 * @src: src info
 * @dst: dst info
*/
struct fimg2d_user_context {
	struct fimg2d_param *param;
	struct fimg2d_image *src;
	struct fimg2d_image *dst;
};

/**
 * struct fimg2d_region - region info
 * @src: src region
 * @dst: dst region
 * @clip: 1, clip region set
 * @dst_clip: dst clip region
 * @node: list head for region queue
*/
struct fimg2d_region {
	struct fimg2d_rect src;
	struct fimg2d_rect dst;
	int clip;
	struct fimg2d_rect dst_clip;
	struct list_head node;
};

/**
 * struct fimg2d_context - context info
 * @closed: not permitted to modify, if closed
 * @rop: ROP value
 * @op: bitblt operation type
 * @color: solid fill color
 * @rot: rotation info
 * @scale: would enable stretch bitblt, if 1
 * @alpha: alpha info
 * @src: src param
 * @dst: dst param
 * @node: list head for context queue
 * @reg_q: region queue
 * @wq: wait queue head
 * @nreg: count of listed regions
*/
struct fimg2d_context {
	atomic_t closed;
	unsigned char rop;
	struct fimg2d_param param;
	struct fimg2d_image src;
	struct fimg2d_image dst;
	struct list_head node;
	struct list_head reg_q;
	wait_queue_head_t wq;
	unsigned long pgd;
	atomic_t nreg;
};

/**
 * struct fimg2d_control - controller info
 * @power: power status for runtime-PM
 * @power_lock: spin lock for power and sysmmu
 * @clock: clock info
 * @irq: irq number
 * @mem: resource info of platform device
 * @regs: base address of hardware
 * @dev: pointer to device struct
 * @lock: spin lock
 * @ref_count: reference counter (= open counter)
 * @busy: 1 if hardware is running
 * @fatal: 1 if hardware is timed out while bitblt
 * @wq: wait queue head
 * @ctx_q: context queue head
 * @ctx: pointer to current context
 * @active: pointer to active context
 * @workqueue: workqueue_struct for kfimg2dd
 * @bitblt: function pointer to perform bitblt (h/w specific)
 * @decode: function pointer to op decode (h/w specific)
 * @configure: function pointer to configure (h/w specific)
 * @update: function pointer to update (h/w specific)
 * @run: function pointer to run (h/w specific)
 * @stop: function pointer to stop (h/w specific)
 * @finalize: function pointer to finalize (h/w specific)
 * @dump: function pointer to dump SFR (h/w specific)
*/
struct fimg2d_control {
	atomic_t power;
	spinlock_t power_lock;
	struct clk *clock;
	int irq;
	struct resource *mem;
	void __iomem *regs;
	struct device *dev;
	spinlock_t lock;
	atomic_t ref_count;
	atomic_t busy;
	int fatal;
	wait_queue_head_t wq;
	struct list_head ctx_q;
	struct fimg2d_context *active;
	struct workqueue_struct *workqueue;

	/* hardware specific ops */
	void (*bitblt)(struct fimg2d_control *info);
	unsigned char (*decode)(struct fimg2d_context *ctx);
	void (*configure)(struct fimg2d_control *info, struct fimg2d_context *ctx);
	void (*update)(struct fimg2d_control *info,
			struct fimg2d_context *ctx, struct fimg2d_region *reg);
	void (*run)(struct fimg2d_control *info);
	void (*stop)(struct fimg2d_control *info);
	void (*finalize)(struct fimg2d_control *info);
	void (*dump)(struct fimg2d_control *info,
			struct fimg2d_context *ctx, int width, int height);
};

/* externs */
extern inline int fimg2d_match_closed(struct fimg2d_context *ctx, void *data);
extern inline void fimg2d_enqueue(struct fimg2d_control *info,
		struct list_head *node, struct list_head *q);
extern void fimg2d_dequeue(struct fimg2d_control *info, struct list_head *node);

extern inline int fimg2d_queue_is_empty(struct list_head *q);
extern inline struct fimg2d_region *
	fimg2d_get_first_region(struct fimg2d_context *ctx);
extern struct fimg2d_context *fimg2d_find_context(struct fimg2d_control *info,
		void *data, int (*match)(struct fimg2d_context *ctx, void *data));
extern int fimg2d_set_context(struct fimg2d_control *info,
		struct fimg2d_context *ctx, struct fimg2d_user_context __user *c);
extern int fimg2d_update_context(struct fimg2d_control *info,
		struct fimg2d_context *ctx, struct fimg2d_param *p);
extern int fimg2d_add_region(struct fimg2d_control *info,
		struct fimg2d_context *ctx, struct fimg2d_user_region __user *r);
extern void fimg2d_do_bitblt(struct fimg2d_control *info);
extern int fimg2d_close_bitblt(struct fimg2d_control *info, struct fimg2d_context *ctx);
extern int fimg2d_register_ops(struct fimg2d_control *info);

#endif /* _FIMG2D_H_ */
