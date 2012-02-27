/* linux/drivers/media/video/samsung/fimg2d/fimg2d3x_hw.c
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

#include <linux/io.h>
#include <mach/map.h>
#include <mach/regs-fimg2d.h>

#include "fimg2d.h"

#if defined(CONFIG_S5P_SYSMMU_FIMG2D)
#ifdef CONFIG_S5P_VMEM
extern void *s5p_getaddress(unsigned int cookie);
#endif

#ifdef CONFIG_UMP_VCM_ALLOC
#include "ump_kernel_interface_ref_drv.h"
#include "ump_kernel_interface_vcm.h"
#endif
#endif


/**
 * memid_to_virt - convert cookie/secure_id to virtual address
 * @p: parameter info
*/
static inline void memid_to_virt(struct fimg2d_image *p)
{
#ifdef CONFIG_UMP_VCM_ALLOC
	struct vcm_res *vcm_res;
#endif

#if defined(CONFIG_S5P_SYSMMU_FIMG2D)
	unsigned long addr = 0;

	if (p->addr_type == FIMG2D_ADDR_SECUID) {
		/* (device) virtual address */
#ifdef CONFIG_UMP_VCM_ALLOC
		vcm_res = (struct vcm_res *)
		ump_dd_meminfo_get(p->addr, (void*)VCM_DEV_G2D);
		if(vcm_res)
			addr = vcm_res->start;
		else
			addr = 0;
#endif
		if (!addr) {
			printk(KERN_ERR "[%s]: unknown secure_id(0x%x) daddr(0x%x)\n",
					__func__, (unsigned int)p->addr, (unsigned int)addr);
			p->addr = addr;
			return;
		}
		fimg2d_debug("secure_id(0x%x) --> daddr(0x%x)\n", (unsigned int)p->addr, (unsigned int)addr);
		p->addr = addr;
		p->addr_type = FIMG2D_ADDR_VCMM;

	} else if (p->addr_type == FIMG2D_ADDR_COOKIE) {
		/* (kernel) virtual address */
#ifdef CONFIG_S5P_VMEM
		addr = (unsigned long)s5p_getaddress(p->addr);
#endif
		if (!addr) {
			printk(KERN_ERR "[%s]: unknown cookie(0x%x) kaddr(0x%x)\n",
					__func__, (unsigned int)p->addr, (unsigned int)addr);
			p->addr = addr;
			return;
		}
		fimg2d_debug("cookie(0x%x) --> kaddr(0x%x)\n", (unsigned int)p->addr, (unsigned int)addr);
		p->addr = addr;
		p->addr_type = FIMG2D_ADDR_KERN;
	}
#endif
}

/**
 * fimg2d3x_reset - [3.x] Software reset
 * @info: controller info
*/
void fimg2d3x_reset(struct fimg2d_control *info)
{
	writel(FIMG2D_SOFT_RESET, info->regs + FIMG2D_SOFT_RESET_REG);
}

/**
 * fimg2d3x_enable_irq - [3.x] Enables interrupt
 * @info: controller info
*/
void fimg2d3x_enable_irq(struct fimg2d_control *info)
{
	writel(FIMG2D_INT_EN, info->regs + FIMG2D_INTEN_REG);
}

/**
 * fimg2d3x_disable_irq - [3.x] Disables interrupt
 * @info: controller info
*/
void fimg2d3x_disable_irq(struct fimg2d_control *info)
{
	writel(0, info->regs + FIMG2D_INTEN_REG);
}

/**
 * fimg2d3x_clear_irq - [3.x] Clears pending interrupts
 * @info: controller info
*/
void fimg2d3x_clear_irq(struct fimg2d_control *info)
{
	writel(FIMG2D_INTP_CMD_FIN, info->regs + FIMG2D_INTC_PEND_REG);
}

/**
 * fimg2d3x_clear_cache - [3.x] Clears 2D hardware caches
 * @info: controller info
*/
void fimg2d3x_clear_cache(struct fimg2d_control *info)
{
	unsigned long cfg;

	cfg = (FIMG2D_PATCACHE_CLEAR | FIMG2D_SRCBUFFER_CLEAR |
			FIMG2D_MASKBUFFER_CLEAR);
	writel(cfg, info->regs + FIMG2D_CACHECTL_REG);
}

/**
 * fimg2d3x_is_finish - [3.x] Checks if finished
 * @info: controller info
*/
int fimg2d3x_is_finish(struct fimg2d_control *info)
{
	return (readl(info->regs + FIMG2D_INTC_PEND_REG) & FIMG2D_INTP_CMD_FIN);
}

/**
 * fimg2d3x_start_bitblt - [3.x] Starts bitblt
 * @info: controller info
*/
void fimg2d3x_start_bitblt(struct fimg2d_control *info)
{
	writel(FIMG2D_START_BITBLT, info->regs + FIMG2D_BITBLT_START_REG);
}

/**
 * fimg2d3x_enable_alpha - [3.x] Enables alpha blending
 * @info: controller info
 * @a: alpha info
*/
void fimg2d3x_enable_alpha(struct fimg2d_control *info, struct fimg2d_alpha *a)
{
	unsigned long cfg;

	cfg = readl(info->regs + FIMG2D_BITBLT_COMMAND_REG);

	cfg &= ~(FIMG2D_ALPHA_MODE_MASK | FIMG2D_NONPREBLEND_MASK);
	cfg |= FIMG2D_ALPHA_MODE_ALPHA;

	if (a->nonpre_type == PERPIXEL)
		cfg |= FIMG2D_NONPREBLEND_PERPIXEL;
	else if (a->nonpre_type == CONSTANT)
		cfg |= FIMG2D_NONPREBLEND_CONSTANT;

	writel(cfg, info->regs + FIMG2D_BITBLT_COMMAND_REG);

	cfg = readl(info->regs + FIMG2D_ALPHA_REG);
	cfg &= ~FIMG2D_ALPHA_VALUE_MASK;
	cfg |= (a->value << FIMG2D_ALPHA_VALUE_SHIFT);

	writel(cfg, info->regs + FIMG2D_ALPHA_REG);
}

/**
 * fimg2d3x_enable_rot90 - [3.x] Enables 90 degree rotation
 * @info: controller info
*/
void fimg2d3x_enable_rot90(struct fimg2d_control *info)
{
	writel(FIMG2D_ROTATE_90_ENABLE, info->regs + FIMG2D_ROTATE_REG);
}

/**
 * fimg2d3x_disable_rot90 - [3.x] Disables 90 degree rotation
 * @info: controller info
*/
void fimg2d3x_disable_rot90(struct fimg2d_control *info)
{
	writel(0, info->regs + FIMG2D_ROTATE_REG);
}

/**
 * fimg2d3x_set_src_type - [3.x] Selects source type (memory, fgcolor, bgcolor)
 * @info: controller info
 * @type: image type
 *
 * This function sets source type using 3rd operand info
*/
void fimg2d3x_set_src_type(struct fimg2d_control *info, FIMG2D_IMG_T type)
{
	unsigned long cfg;

	if (type == NORMAL)
		cfg = FIMG2D_IMG_TYPE_MEMORY;
	else if (type == BGCOLOR)
		cfg = FIMG2D_IMG_TYPE_BGCOLOR;
	else
		cfg = FIMG2D_IMG_TYPE_FGCOLOR;

	writel(cfg, info->regs + FIMG2D_SRC_SELECT_REG);
}

/**
 * fimg2d3x_set_src_param - [3.x] Configures several parameters of source
 * @info: controller info
 * @p: parameter info
 *
 * This function sets several parameters such as direction, addr, stride and
 * color mode of source
*/
void fimg2d3x_set_src_param(struct fimg2d_control *info, struct fimg2d_image *p)
{
	unsigned long cfg;

#if defined(CONFIG_S5P_SYSMMU_FIMG2D)
	memid_to_virt(p);
#endif

	fimg2d_debug("addr(0x%x) addr_type(0x%x)\n", (unsigned int)p->addr, p->addr_type);
	writel(FIMG2D_ADDR(p->addr), info->regs + FIMG2D_SRC_BASE_ADDR_REG);

	writel(FIMG2D_STRIDE(p->stride), info->regs + FIMG2D_SRC_STRIDE_REG);

	/* color mode */
	cfg = p->order << FIMG2D_CHANNEL_ORDER_SHIFT;
	cfg |= p->fmt << FIMG2D_COLOR_FORMAT_SHIFT;
	writel(cfg, info->regs + FIMG2D_SRC_COLOR_MODE_REG);
}

/**
 * fimg2d3x_set_src_coordinate - [3.x] Changes coordinate value of source
 * @info: controller info
 * @r: coordinate value via struct fimg2d_rect
*/
void fimg2d3x_set_src_coordinate(struct fimg2d_control *info, struct fimg2d_rect *r)
{
	writel(FIMG2D_OFFSET(r->x1, r->y1), info->regs + FIMG2D_SRC_LEFT_TOP_REG);
	writel(FIMG2D_OFFSET(r->x2, r->y2), info->regs + FIMG2D_SRC_RIGHT_BOTTOM_REG);
}

/**
 * fimg2d3x_set_dst_type - [3.x] Selects destination type (memory, fgcolor, bgcolor)
 * @info: controller info
 * @type: 3rd operand type
 *
 * This function sets destination type using 3rd operand info
*/
void fimg2d3x_set_dst_type(struct fimg2d_control *info, FIMG2D_IMG_T type)
{
	unsigned long cfg;

	if (type == FGCOLOR)
		cfg = FIMG2D_IMG_TYPE_FGCOLOR;
	else if (type == BGCOLOR)
		cfg = FIMG2D_IMG_TYPE_BGCOLOR;
	else
		cfg = FIMG2D_IMG_TYPE_MEMORY;

	writel(cfg, info->regs + FIMG2D_DST_SELECT_REG);
}

/**
 * fimg2d3x_set_dst_param - [3.x] Configures several parameters of destination
 * @info: controller info
 * @p: parameter info
 *
 * This function sets several parameters such as direction, addr, stride and
 * color mode of destination
*/
void fimg2d3x_set_dst_param(struct fimg2d_control *info, struct fimg2d_image *p)
{
	unsigned long cfg;

#if defined(CONFIG_S5P_SYSMMU_FIMG2D)
	memid_to_virt(p);
#endif

	fimg2d_debug("addr(0x%x) addr_type(0x%x)\n", (unsigned int)p->addr, p->addr_type);
	writel(FIMG2D_ADDR(p->addr), info->regs + FIMG2D_DST_BASE_ADDR_REG);

	writel(FIMG2D_STRIDE(p->stride), info->regs + FIMG2D_DST_STRIDE_REG);

	/* color mode */
	cfg = p->order << FIMG2D_CHANNEL_ORDER_SHIFT;
	cfg |= p->fmt << FIMG2D_COLOR_FORMAT_SHIFT;
	writel(cfg, info->regs + FIMG2D_DST_COLOR_MODE_REG);
}

/**
 * fimg2d3x_set_dst_coordinate - [3.x] Changes coordinate value of destination
 * @info: controller info
 * @r: coordinate value via struct fimg2d_rect
*/
void fimg2d3x_set_dst_coordinate(struct fimg2d_control *info, struct fimg2d_rect *r)
{
	writel(FIMG2D_OFFSET(r->x1, r->y1), info->regs + FIMG2D_DST_LEFT_TOP_REG);
	writel(FIMG2D_OFFSET(r->x2, r->y2), info->regs + FIMG2D_DST_RIGHT_BOTTOM_REG);
}

/**
 * fimg2d3x_set_pat_param - [3.x] Configures several parameters of pattern image
 * @info: controller info
 * @p: parameter info
 *
 * This function sets several parameters such as direction, addr, stride and
 * color mode of pattern image
*/
void fimg2d3x_set_pat_param(struct fimg2d_control *info, struct fimg2d_image *p)
{
	unsigned long cfg;

#if defined(CONFIG_S5P_SYSMMU_FIMG2D)
	memid_to_virt(p);
#endif

	fimg2d_debug("addr(0x%x) addr_type(0x%x)\n", (unsigned int)p->addr, p->addr_type);
	writel(FIMG2D_ADDR(p->addr), info->regs + FIMG2D_PAT_BASE_ADDR_REG);

	/* width & height */
	writel(FIMG2D_SIZE(p->width, p->height), info->regs + FIMG2D_PAT_SIZE_REG);

	/* color mode */
	cfg = p->order << FIMG2D_CHANNEL_ORDER_SHIFT;
	cfg |= p->fmt << FIMG2D_COLOR_FORMAT_SHIFT;
	writel(cfg, info->regs + FIMG2D_PAT_COLOR_MODE_REG);

	writel(FIMG2D_STRIDE(p->stride), info->regs + FIMG2D_PAT_STRIDE_REG);
}

/**
 * fimg2d3x_set_pat_offset - [3.x] Changes offset value of pattern image
 * @info: controller info
 * @ctx: context info
 * @r: coordinate values via struct fimg2d_region
*/
void fimg2d3x_set_pat_offset(struct fimg2d_control *info,
			struct fimg2d_context *ctx, struct fimg2d_region *r)
{
	int x, y;

	x = ((ctx->src.width << 14) + (r->src.x1 - r->dst.x1)) % ctx->src.width;
	y = ((ctx->src.height << 14) + (r->src.y1 - r->dst.y1)) % ctx->src.height;

	writel(FIMG2D_OFFSET(x, y), info->regs + FIMG2D_PAT_OFFSET_REG);
}

/**
 * fimg2d3x_set_direction - [3.x] Configures directions
 * @info: controller info
 * @ctx: context info
*/
void fimg2d3x_set_direction(struct fimg2d_control *info, struct fimg2d_param *p)
{
	int sdx, sdy, ddx, ddy;
	int rot90 = 0;
	unsigned long cfg;

	/* forcely update to default value 1 for non-initialized values */
	sdx = ddx = !p->dx ? 1 : p->dx;
	sdy = ddy = !p->dy ? 1 : p->dy;

	/* not allowed rotation and flip when direction is negative */
	if (p->dx == FIMG2D_DIR_NEGATIVE && p->dy == FIMG2D_DIR_NEGATIVE)
		p->rot = ORIGIN;

	/* parse rotation */
	switch (p->rot) {
	default:
	case ORIGIN:
		break;

	case ROT90:
		rot90 = 1;
		fimg2d_debug("rot90\n");
		break;

	case ROT180:
		ddx = FIMG2D_DIR_NEGATIVE;
		ddy = FIMG2D_DIR_NEGATIVE;
		fimg2d_debug("rot180\n");
		break;

	case ROT270:
		ddx = FIMG2D_DIR_NEGATIVE;
		ddy = FIMG2D_DIR_NEGATIVE;
		rot90 = 1;
		fimg2d_debug("rot270\n");
		break;

	case XFLIP:
		ddy = FIMG2D_DIR_NEGATIVE;
		fimg2d_debug("x-flip\n");
		break;

	case YFLIP:
		ddx = FIMG2D_DIR_NEGATIVE;
		fimg2d_debug("y-flip\n");
		break;
	}

	/* src direction */
	cfg = readl(info->regs + FIMG2D_SRC_MSK_DIRECT_REG);
	cfg &= ~FIMG2D_SRC_DIR_MASK;

	if (sdx == FIMG2D_DIR_NEGATIVE)
		cfg |= FIMG2D_SRC_X_DIR_NEGATIVE;

	if (sdy == FIMG2D_DIR_NEGATIVE)
		cfg |= FIMG2D_SRC_Y_DIR_NEGATIVE;

	writel(cfg, info->regs + FIMG2D_SRC_MSK_DIRECT_REG);

	/* dst & pattern direction */
	cfg = readl(info->regs + FIMG2D_DST_PAT_DIRECT_REG);
	cfg &= ~FIMG2D_DST_DIR_MASK;

	if (ddx == FIMG2D_DIR_NEGATIVE)
		cfg |= FIMG2D_DST_X_DIR_NEGATIVE;

	if (ddy == FIMG2D_DIR_NEGATIVE)
		cfg |= FIMG2D_DST_Y_DIR_NEGATIVE;

	cfg &= ~FIMG2D_PAT_DIR_MASK;

	if (sdx == FIMG2D_DIR_NEGATIVE)
		cfg |= FIMG2D_PAT_X_DIR_NEGATIVE;

	if (sdy == FIMG2D_DIR_NEGATIVE)
		cfg |= FIMG2D_PAT_Y_DIR_NEGATIVE;

	writel(cfg, info->regs + FIMG2D_DST_PAT_DIRECT_REG);

	/* rotation 90 */
	if (rot90)
		writel(FIMG2D_ROTATE_90_ENABLE, info->regs + FIMG2D_ROTATE_REG);
}

/**
 * fimg2d3x_set_opr3_type - [3.x] Sets 3rd operand type
 * @info: controller info
 * @type: 3rd operand type
 *
 * Only supports 'Unmasked' case because we don't use 1 bit mask operation.
 * 'Masked' type would be set to same value with 'Unmasked'
*/
void fimg2d3x_set_opr3_type(struct fimg2d_control *info, FIMG2D_IMG_T type)
{
	unsigned long cfg;

	cfg = readl(info->regs + FIMG2D_THIRD_OPERAND_REG);
	cfg &= ~(FIMG2D_MASKED_OPR3_MASK | FIMG2D_UNMASKED_OPR3_MASK);

	if (type == FGCOLOR)
		cfg |= (FIMG2D_MASKED_OPR3_FGCOLOR | FIMG2D_UNMASKED_OPR3_FGCOLOR);
	else if (type == BGCOLOR)
		cfg |= (FIMG2D_MASKED_OPR3_BGCOLOR | FIMG2D_UNMASKED_OPR3_BGCOLOR);
	else if (type == PATTERN)
		cfg |= (FIMG2D_MASKED_OPR3_PATTERN | FIMG2D_UNMASKED_OPR3_PATTERN);

	writel(cfg, info->regs + FIMG2D_THIRD_OPERAND_REG);
}

/**
 * fimg2d3x_set_rop_value - [3.x] Sets ROP4 value
 * @info: controller info
 * @value: pre-defined ROP value
 *
 * Only supports 'Unmasked' case because we don't use 1 bit mask operation.
 * 'Masked' type would be set to same value with 'Unmasked'
*/
void fimg2d3x_set_rop_value(struct fimg2d_control *info, unsigned long value)
{
	unsigned long cfg;

	cfg = (value << FIMG2D_MASKED_ROP3_SHIFT) |
		(value << FIMG2D_UNMASKED_ROP3_SHIFT);

	writel(cfg, info->regs + FIMG2D_ROP4_REG);
}

/**
 * fimg2d3x_set_fgcolor - [3.x] Sets foreground color
 * @info: controller info
 * @value: foreground color value
*/
void fimg2d3x_set_fgcolor(struct fimg2d_control *info, unsigned long value)
{
	writel(FIMG2D_COLOR(value), info->regs + FIMG2D_FG_COLOR_REG);
}

/**
 * fimg2d3x_set_bgcolor - [3.x] Sets background color
 * @info: controller info
 * @value: background color value
*/
void fimg2d3x_set_bgcolor(struct fimg2d_control *info, unsigned long value)
{
	writel(FIMG2D_COLOR(value), info->regs + FIMG2D_BG_COLOR_REG);
}

/**
 * fimg2d3x_enable_stretch - [3.x] Enables stretch bitblt
 * @info: controller info
*/
void fimg2d3x_enable_stretch(struct fimg2d_control *info)
{
	unsigned long cfg;

	cfg = readl(info->regs + FIMG2D_BITBLT_COMMAND_REG);
	cfg |= FIMG2D_ENABLE_STRETCH;

	writel(cfg, info->regs + FIMG2D_BITBLT_COMMAND_REG);
}

/**
 * fimg2d3x_enable_clip - [3.x] Enables clipping window
 * @info: controller info
*/
void fimg2d3x_enable_clip(struct fimg2d_control *info)
{
	unsigned long cfg;

	cfg = readl(info->regs + FIMG2D_BITBLT_COMMAND_REG);
	cfg |= FIMG2D_ENABLE_CW;

	writel(cfg, info->regs + FIMG2D_BITBLT_COMMAND_REG);
}

/**
 * fimg2d3x_set_clip_coordinate - [3.x] Changes coordinate value of clipping window
 * @info: controller info
 * @r: coordinate value via struct fimg2d_rect
*/
void fimg2d3x_set_clip_coordinate(struct fimg2d_control *info, struct fimg2d_rect *r)
{
	writel(FIMG2D_OFFSET(r->x1, r->y1), info->regs + FIMG2D_CW_LT_REG);
	writel(FIMG2D_OFFSET(r->x2, r->y2), info->regs + FIMG2D_CW_RB_REG);
}


/**
 * fimg2d3x_dump_regs - [3.x] Dump SFR
 * @info: controller info
*/
void fimg2d3x_dump_regs(struct fimg2d_control *info)
{
	int i, j, offset;
	unsigned long table[11][2] = {
		{0x004, 5}, {0x100, 2}, {0x200, 3}, {0x300, 6},
		{0x400, 6}, {0x500, 5}, {0x520, 2}, {0x600, 2},
		{0x610, 3}, {0x700, 3}, {0x710, 6},
	};

	for (i = 0; i < 11; i++) {
		for (j = 0; j < table[i][1]; j++) {
			offset = table[i][0] + (j * 4);
			fimg2d_debug("[0x%03x] 0x%08x\n", offset,
						readl(info->regs + offset));
		}
	}
}
