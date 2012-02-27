/* linux/drivers/media/video/samsung/fimg2d/fimg2d3x_blt.c
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

#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>

#include "fimg2d.h"
#include "fimg2d3x.h"

#ifdef CONFIG_S5P_SYSMMU_FIMG2D
#include <asm/pgtable.h>
#include <plat/sysmmu.h>
#ifdef CONFIG_VCM
#include <plat/s5p-vcm.h>
#endif
#endif

#ifdef CONFIG_PM_RUNTIME
#include <plat/devs.h>
#include <linux/pm_runtime.h>
#endif

// bitblt time measure
#undef PERF

/* for pattern blend */
static void *scratch_buf;
static int pat_height;
static int org_width;
static int org_height;
static struct fimg2d_rect org_dst;
static unsigned long org_addr;
static FIMG2D_ADDR_TYPE_T org_type;

/**
 * fimg2d3x_is_valid_region - [3.x] check validity of src region
 * @ctx: context info
 * @reg: region info
*/
static inline int fimg2d3x_is_valid_region(struct fimg2d_control *info,
		struct fimg2d_context *ctx, struct fimg2d_region *reg)
{
	struct fimg2d_rect *src, *dst, *clp;

	/* for tracing bitblt timeout */
	if (info->fatal)
		return -1;

	src = &reg->src;
	dst = &reg->dst;
	clp = &reg->dst_clip;

	if (ctx->src.addr_type && ctx->param.op != OP_PAT_COPY) {
		if (src->x1 < 0 || src->x2 > ctx->src.width)
			goto err_src;

		if (src->y1 < 0 || src->y2 > ctx->src.height)
			goto err_src;
	}

	if (!reg->clip) {
		if (dst->x1 < 0 || dst->x1 >= ctx->dst.width)
			goto err_dst;

		if (dst->y1 < 0 || dst->y1 >= ctx->dst.height)
			goto err_dst;

		/* set auto dest clipping */
		if (dst->x2 > ctx->dst.width || dst->y2 > ctx->dst.height) {
			clp->x1 = 0;
			clp->y1 = 0;
			clp->x2 = ctx->dst.width;
			clp->y2 = ctx->dst.height;
			reg->clip = 1;

			printk(KERN_ERR "auto clipping dst(%d,%d => %d,%d) clp(%d,%d => %d,%d)\n",
					dst->x1, dst->y1, dst->x2, dst->y2,
					clp->x1, clp->y1, clp->x2, clp->y2);
		}
	} else {
		if (clp->x1 < 0 || clp->x1 >= ctx->dst.width)
			goto err_clp;

		if (clp->y1 < 0 || clp->y1 >= ctx->dst.height)
			goto err_clp;

		if (clp->x2 > ctx->dst.width || clp->y2 > ctx->dst.height) {
			if (clp->x2 > ctx->dst.width)
				clp->x2 = ctx->dst.width;
			if (clp->y2 > ctx->dst.height)
				clp->y2 = ctx->dst.height;

			printk(KERN_ERR "fixing clip reg(%d,%d => %d,%d)\n",
					clp->x1, clp->y1, clp->x2, clp->y2);
		}
	}

	return 0;

err_src:
	printk(KERN_ERR "invalid src(%d,%d => %d,%d) dst(%d,%d => %d,%d) ctx src(w:%d, h:%d) op(%d)\n",
			src->x1, src->y1, src->x2, src->y2,
			dst->x1, dst->y1, dst->x2, dst->y2,
			ctx->src.width, ctx->src.height, ctx->param.op);
	return -1;

err_dst:
	printk(KERN_ERR "invalid dst(%d,%d => %d,%d) ctx dst(w:%d, h:%d)\n",
			dst->x1, dst->y1, dst->x2, dst->y2,
			ctx->dst.width, ctx->dst.height);
	return -1;

err_clp:
	printk(KERN_ERR "invalid clip(%d,%d => %d,%d) ctx dst(w:%d, h:%d)\n",
			clp->x1, clp->y1, clp->x2, clp->y2,
			ctx->dst.width, ctx->dst.height);
	return -1;
}

/**
 * fimg2d3x_pattern_blend_pre - [3.x] Do pre-work for pattern blending
 * @ctx: context info
 * @reg: region info
*/
static inline void fimg2d3x_pattern_blend_pre(struct fimg2d_context *ctx,
					struct fimg2d_region *reg)
{
	/* backup originals */
	pat_height = ctx->src.height;
	org_addr = ctx->dst.addr;
	org_type = ctx->dst.addr_type;
	org_width = ctx->dst.width;
	org_height = ctx->dst.height;
	memcpy(&org_dst, &reg->dst, sizeof(org_dst));

	/* new width and height */
	ctx->dst.width = reg->dst.x2 - reg->dst.x1;
	ctx->dst.height = reg->dst.y2 - reg->dst.y1;

	/* forcely increase the height */
	ctx->dst.height *= 2;

	/* temporary buffer for pattern copy */
	scratch_buf = kmalloc(ctx->dst.width * ctx->dst.height * 4, GFP_KERNEL);
	if (!scratch_buf) {
		printk(KERN_ERR "[%s]: kmalloc failed\n", __func__);
		return;
	}

	/* new addr and region */
	ctx->dst.addr = (unsigned long)virt_to_phys(scratch_buf);
	ctx->dst.addr_type = FIMG2D_ADDR_PHYS;
	reg->src.x1 = 0;
	reg->src.y1 = 0;
	reg->src.x2 = ctx->src.width;
	reg->src.y2 = ctx->src.height;
	reg->dst.x1 = 0;
	reg->dst.y1 = 0;
	reg->dst.x2 = ctx->dst.width;
	reg->dst.y2 = ctx->dst.height;

	/* disable alpha */
	ctx->param.alpha.enabled = 0;
}

/**
 * fimg2d3x_pattern_blend_post - [3.x] Do post-work for pattern blending
 * @info: controller info
 * @ctx: context info
 * @reg: region info
*/
static inline void fimg2d3x_pattern_blend_post(struct fimg2d_control *info,
			struct fimg2d_context *ctx, struct fimg2d_region *reg)
{
	int ret = 0;

	ctx->src.type = NORMAL;
	ctx->dst.type = NORMAL;
	ctx->src.addr = (unsigned long)virt_to_phys(scratch_buf);
	ctx->src.addr_type = FIMG2D_ADDR_PHYS;
	ctx->dst.addr = org_addr;
	ctx->dst.addr_type = org_type;
	ctx->param.alpha.enabled = 1;
	ctx->param.alpha.value = 0xff;
	ctx->rop = 0xcc;
	ctx->src.width = ctx->dst.width;
	ctx->src.height = ctx->dst.height;
	ctx->dst.width = org_width;
	ctx->dst.height = org_height;
	reg->src.x1 = 0;
	reg->src.y1 = 0;
	reg->src.x2 = ctx->src.width;
	reg->src.y2 = ctx->src.height;
	memcpy(&reg->dst, &org_dst, sizeof(reg->dst));

	reg->src.y2 /= 2;

	info->finalize(info);
	info->configure(info, ctx);
	info->update(info, ctx, reg);

	if ((ctx->src.addr_type && !ctx->src.addr) ||
		(ctx->dst.addr_type && !ctx->dst.addr)) {
		/* avoid system hang-up */
		printk(KERN_ERR "[%s]: invalid addr: src.addr(0x%x) dst.addr(0x%x)\n",
				__func__, (unsigned int)ctx->src.addr, (unsigned int)ctx->dst.addr);
	}
	else {
		atomic_set(&info->busy, 1);

		info->run(info);

		ret = wait_event_timeout(info->wq, !atomic_read(&info->busy), 1000);
		if (ret == 0)
			printk(KERN_ERR "pattern: wait timeout\n");
	}

	kfree(scratch_buf);
}

/**
 * fimg2d3x_bitblt - [3.x] kernel thread for 3.0 bitblt
 * @info: controller info
*/
void fimg2d3x_bitblt(struct fimg2d_control *info)
{
	struct fimg2d_context *ctx = info->active;
	struct fimg2d_platdata *pdata;
	struct fimg2d_region *reg;
	atomic_t configured;
	int ret = 0;

	pdata = to_fimg2d_plat(info->dev);

	fimg2d_debug("start kernel thread\n");
	atomic_set(&configured, 0);

#ifdef CONFIG_PM_RUNTIME
	if (atomic_read(&info->power) == 0) {
		fimg2d_debug("pm_runtime_get_sync\n");
		pm_runtime_get_sync(&(s5p_device_fimg2d.dev));
	}
#endif

	while (ctx) {
		reg = fimg2d_get_first_region(ctx);
		if (!reg) {
			printk(KERN_ERR "region not found\n");
			goto noreg;
		}

		if (fimg2d3x_is_valid_region(info, ctx, reg))
			goto noblt;

#ifndef CONFIG_S5P_SYSMMU_FIMG2D
		/* pattern blend work around for 3.0 hardware */
		if (ctx->param.op == OP_PAT_BLEND)
			fimg2d3x_pattern_blend_pre(ctx, reg);
#endif

		if (!atomic_read(&configured)) {
			info->configure(info, ctx);
			atomic_set(&configured, 1);
		}

		info->update(info, ctx, reg);

		if ((ctx->src.addr_type && !ctx->src.addr) ||
			(ctx->dst.addr_type && !ctx->dst.addr)) {
			/* avoid system hang-up */
			printk(KERN_ERR "[%s]: invalid addr: src.addr(0x%x) dst.addr(0x%x)\n",
					__func__, (unsigned int)ctx->src.addr, (unsigned int)ctx->dst.addr);
		}
		else {
			atomic_set(&info->busy, 1);

#if defined(CONFIG_S5P_SYSMMU_FIMG2D)
			if (ctx->src.addr_type == FIMG2D_ADDR_VCMM ||
					ctx->dst.addr_type == FIMG2D_ADDR_VCMM) {
#ifdef CONFIG_PM_RUNTIME
				fimg2d_debug("vcm_set_pgtable_base\n");
				vcm_set_pgtable_base(VCM_DEV_G2D);
#else
				/* nop */
				fimg2d_debug("using vcm\n");
#endif

			} else if (ctx->src.addr_type == FIMG2D_ADDR_USER ||
					ctx->dst.addr_type == FIMG2D_ADDR_USER) {
				sysmmu_set_tablebase_pgd(SYSMMU_G2D, ctx->pgd);
				fimg2d_debug("set to ctx->pgd(0x%x)\n", (unsigned int)ctx->pgd);

			} else if (ctx->src.addr_type == FIMG2D_ADDR_KERN ||
					ctx->dst.addr_type == FIMG2D_ADDR_KERN) {
				sysmmu_set_tablebase_pgd(SYSMMU_G2D, __pa(swapper_pg_dir));
				fimg2d_debug("set to swapper_pg_dir(0x%x)\n",
						(unsigned int)__pa(swapper_pg_dir));
			}
#endif

#ifdef PERF
			struct timeval start, end;
			printk("[%s] start bitblt\n", __func__);
			do_gettimeofday(&start);
#endif

			info->run(info);

			ret = wait_event_timeout(info->wq, !atomic_read(&info->busy), 1000);
			if (ret == 0) {
				info->fatal = 1;
				printk(KERN_ERR "bitblt: wait timeout\n");

				/* for tracing bitblt timeout */
				printk(KERN_ERR "ctx src addr: 0x%08lx, width: %d, height: %d\n",
						ctx->src.addr, ctx->src.width, ctx->src.height);
				printk(KERN_ERR "ctx dst addr: 0x%08lx, width: %d, height: %d\n",
						ctx->dst.addr, ctx->dst.width, ctx->dst.height);
				printk(KERN_ERR "ctx op: %d\n", ctx->param.op);

				printk(KERN_ERR "reg src: (x1:%d, y1:%d) => (x2:%d, y2:%d)\n", reg->src.x1, reg->src.y1,
						reg->src.x2, reg->src.y2);
				printk(KERN_ERR "reg dst: (x1:%d, y1:%d) => (x2:%d, y2:%d)\n", reg->dst.x1, reg->dst.y1,
						reg->dst.x2, reg->dst.y2);
				if (reg->clip) {
					printk(KERN_ERR "reg dst clip: (x1:%d, y1:%d) => (x2:%d, y2:%d)\n",
							reg->dst_clip.x1, reg->dst_clip.y1,
							reg->dst_clip.x2, reg->dst_clip.y2);
				}
			}

#ifdef PERF
			do_gettimeofday(&end);
			long sec, usec, time;
			sec = end.tv_sec - start.tv_sec;
			if (end.tv_usec >= start.tv_usec) {
				usec = end.tv_usec - start.tv_usec;
			} else {
				usec = end.tv_usec + 1000000 - start.tv_usec;
				sec--;
			}
			time = sec * 1000000 + usec;
			printk("[%s] end bitblt: %ld usec elapsed\n", __func__, time);
#endif
		}

#ifndef CONFIG_S5P_SYSMMU_FIMG2D
		/* pattern blend work around for 3.0 hardware */
		if (ctx->param.op == OP_PAT_BLEND) {
			fimg2d3x_pattern_blend_post(info, ctx, reg);
			atomic_set(&configured, 0);
		}
#endif

noblt:
		fimg2d_dequeue(info, &reg->node);
		kfree(reg);
		atomic_dec(&ctx->nreg);

noreg:
		if (fimg2d_queue_is_empty(&ctx->reg_q)) {
			spin_lock(&info->lock);
			fimg2d_debug("done: %p\n", ctx);
			atomic_set(&ctx->closed, 0);
			info->finalize(info);

			fimg2d_debug("try to wake up for %p\n", ctx);
			info->active = NULL;
			wake_up(&ctx->wq);

			ctx = fimg2d_find_context(info, (void *)1, fimg2d_match_closed);
			spin_unlock(&info->lock);

			if (!ctx) {
				fimg2d_debug("no more jobs\n");
				break;
			}

			spin_lock(&info->lock);
			fimg2d_debug("restart with %p\n", ctx);
			info->active = ctx;
			spin_unlock(&info->lock);

			atomic_set(&configured, 0);
		}
	}

#ifdef CONFIG_PM_RUNTIME
	if (atomic_read(&info->power) == 1) {
		pm_runtime_put_sync(&(s5p_device_fimg2d.dev));
		fimg2d_debug("pm_runtime_put_sync\n");
	}
#endif

	fimg2d_debug("stop kernel thread\n");
}

/**
 * fimg2d3x_decode - [3.x] decode context
 * @ctx: context info
 *
 * This function decodes bitblt type and generates ROP value.
 * Additionally, some more information like fgcolor and alpha would be fetched
 * from user space if needed.
*/
static unsigned char fimg2d3x_decode(struct fimg2d_context *ctx)
{
	unsigned char src = 0xcc;	/* source only */
	unsigned char opr3 = 0xf0;	/* 3rd opr only */
	unsigned char rop = 0;

	switch (ctx->param.op) {
	case OP_SOLID_FILL:
		/* solid fill uses fgcolor */
		ctx->src.type = FGCOLOR;
		ctx->dst.type = FGCOLOR;
		rop = opr3;	/* ROP must be 3rd opr only */
		fimg2d_debug("op(fill) src(fgcolor) dst(fgcolor) rop(0x%x)\n", rop);
		break;

	case OP_SRC_COPY:
		/* source copy is simple */
		ctx->src.type = NORMAL;
		ctx->dst.type = FGCOLOR;
		rop = src;
		fimg2d_debug("op(copy) src(normal) dst(fgcolor) rop(0x%x)\n", rop);
		break;

	case OP_PAT_COPY:	/* fall through */
	case OP_PAT_BLEND:
		/* pattern blend might be treated as 2 steps WA.  */
		ctx->src.type = PATTERN;
		ctx->dst.type = FGCOLOR;
		rop = opr3;
		fimg2d_debug("op(pat:%d) src(pattern) dst(fgcolor) rop(0x%x)\n", ctx->param.op, rop);
		break;

	case OP_OVER:
		ctx->src.type = NORMAL;
		ctx->dst.type = NORMAL;
		rop = src;
		fimg2d_debug("op(over) src(normal) dst(normal) rop(0x%x)\n", rop);
		break;

	default:
		break;
	}

	return rop;
}

/**
 * fimg2d3x_configure - [3.x] configure hardware
 * @info: controller info
 * @ctx: context info
 *
 * This function calls apropriate hardware interface functions according to
 * configured context information.
*/
static void fimg2d3x_configure(struct fimg2d_control *info, struct fimg2d_context *ctx)
{
	fimg2d_debug("src addr: 0x%08lx, width: %d, height: %d\n",
			ctx->src.addr, ctx->src.width, ctx->src.height);
	fimg2d_debug("dst addr: 0x%08lx, width: %d, height: %d\n",
			ctx->dst.addr, ctx->dst.width, ctx->dst.height);
	fimg2d_debug("rop: 0x%02x\n", ctx->rop);

	fimg2d3x_reset(info);

	fimg2d3x_set_src_type(info, ctx->src.type);

	if (ctx->src.type == NORMAL) {
		fimg2d3x_set_src_param(info, &ctx->src);
	} else {
		fimg2d_debug("3rd opr: %s\n", ctx->src.type == FGCOLOR ? "fgcolor" :
				(ctx->src.type == BGCOLOR ? "bgcolor" : "pattern"));

		fimg2d3x_set_opr3_type(info, ctx->src.type);

		if (ctx->src.type == PATTERN)
			fimg2d3x_set_pat_param(info, &ctx->src);
		else if (ctx->src.type == FGCOLOR)
			fimg2d3x_set_fgcolor(info, ctx->param.color);
		else if (ctx->src.type == BGCOLOR)
			fimg2d3x_set_bgcolor(info, ctx->param.color);

	}

	fimg2d3x_set_dst_type(info, ctx->dst.type);
	fimg2d3x_set_dst_param(info, &ctx->dst);

	if (ctx->param.alpha.enabled) {
		fimg2d_debug("alpha value:0x%x\n", ctx->param.alpha.value);
		fimg2d3x_enable_alpha(info, &ctx->param.alpha);
	}

	if (ctx->param.scale)
		fimg2d3x_enable_stretch(info);

	fimg2d3x_set_direction(info, &ctx->param);
	fimg2d3x_set_rop_value(info, ctx->rop);
}

/**
 * fimg2d3x_update - [3.x] update hardware configurations
 * @info: controller info
 * @ctx: context info
 * @reg: region info
 *
 * This function updates coordinate values with region info.
*/
static void fimg2d3x_update(struct fimg2d_control *info,
			struct fimg2d_context *ctx, struct fimg2d_region *reg)
{
	fimg2d_debug("context: %p\n", ctx);
	fimg2d_debug("src: (x1:%d, y1:%d) => (x2:%d, y2:%d)\n", reg->src.x1, reg->src.y1,
			reg->src.x2, reg->src.y2);
	fimg2d_debug("dst: (x1:%d, y1:%d) => (x2:%d, y2:%d)\n", reg->dst.x1, reg->dst.y1,
			reg->dst.x2, reg->dst.y2);
	if (reg->clip) {
		fimg2d_debug("dst clip: (x1:%d, y1:%d) => (x2:%d, y2:%d)\n", reg->dst_clip.x1, reg->dst_clip.y1,
				reg->dst_clip.x2, reg->dst_clip.y2);
	}

	if (ctx->src.type == NORMAL)
		fimg2d3x_set_src_coordinate(info, &reg->src);
	else if (ctx->src.type == PATTERN)
		fimg2d3x_set_pat_offset(info, ctx, reg);

	fimg2d3x_set_dst_coordinate(info, &reg->dst);

	if (reg->clip) {
		fimg2d3x_set_clip_coordinate(info, &reg->dst_clip);
		fimg2d3x_enable_clip(info);
	}
}

/**
 * fimg2d3x_run - [3.x] run the hardware
 * @info: controller info
 *
 * IRQ must be enabled before start.
*/
static void fimg2d3x_run(struct fimg2d_control *info)
{
	fimg2d3x_enable_irq(info);
	fimg2d3x_clear_irq(info);
	fimg2d3x_start_bitblt(info);
}

/**
 * fimg2d3x_stop - [3.x] stop the hardware
 * @info: controller info
 *
 * This function stops current hardware working.
*/
static void fimg2d3x_stop(struct fimg2d_control *info)
{
	if (fimg2d3x_is_finish(info)) {
		fimg2d3x_disable_irq(info);
		fimg2d3x_clear_irq(info);
		atomic_set(&info->busy, 0);
		wake_up(&info->wq);
	}
}

/**
 * fimg2d3x_finalize - [3.x] finalizes the hardware
 * @info: controller info
 *
 * This function finalizes current hardware working.
*/
static void fimg2d3x_finalize(struct fimg2d_control *info)
{
	fimg2d3x_clear_cache(info);
}

/**
 * fimg2d3x_dump - [3.x] Dump SFR
 * @info: controller info
 * @ctx: context info
 * @width: prints if width is larger than this
 * @height: prints if height is larger than this
*/
static void fimg2d3x_dump(struct fimg2d_control *info,
			struct fimg2d_context *ctx, int width, int height)
{
	if (ctx->dst.width >= width && ctx->dst.height >= height)
		fimg2d3x_dump_regs(info);
}

/**
 * fimg2d_register_ops - [GENERIC] register hardware specific functions
 * @info: controller info
*/
int fimg2d_register_ops(struct fimg2d_control *info)
{
	info->bitblt = fimg2d3x_bitblt;
	info->decode = fimg2d3x_decode;
	info->configure = fimg2d3x_configure;
	info->update = fimg2d3x_update;
	info->run = fimg2d3x_run;
	info->stop = fimg2d3x_stop;
	info->finalize = fimg2d3x_finalize;
	info->dump = fimg2d3x_dump;

	return 0;
}
