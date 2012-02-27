/* linux/drivers/media/video/samsung/fimg2d/fimg2d_drv.c
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>
#include <linux/pm_runtime.h>
#include <asm/atomic.h>
#include <asm/cacheflush.h>
#include <plat/cpu.h>
#include <plat/fimg2d.h>

#ifdef CONFIG_PM_RUNTIME
#include <linux/pm_runtime.h>
#endif

#ifdef CONFIG_S5P_SYSMMU_FIMG2D
#include <plat/sysmmu.h>
#ifdef CONFIG_VCM
#include <plat/s5p-vcm.h>
#endif /* CONFIG_VCM */
#endif /* CONFIG_S5P_SYSMMU_FIMG2D */

#include "fimg2d.h"

#if defined(CONFIG_S5P_SYSMMU_FIMG2D) && defined(CONFIG_S5P_VMEM)
extern void *s5p_getaddress(unsigned int cookie);
extern void s5p_vmem_dmac_map_area(const void *start_addr, unsigned long size, int dir);
extern struct page *s5p_vmem_va2page(const void *virt_addr);
#endif

static struct fimg2d_control *info;

#if defined(CONFIG_S5P_SYSMMU_FIMG2D) && defined(CONFIG_VCM)
struct vcm *s5pvcm = NULL;

static void fimg2d_tlb_invalidate(enum vcm_dev_id id)
{
	if (atomic_read(&info->power) == 1) {
		spin_lock(&info->power_lock);
		fimg2d_debug("sysmmu_tlb_invalidate\n");
		sysmmu_tlb_invalidate(id);
		spin_unlock(&info->power_lock);
	}
}

static void fimg2d_set_pagetable(enum vcm_dev_id id, unsigned long pgd_base)
{
	if (atomic_read(&info->power) == 1) {
		spin_lock(&info->power_lock);
		fimg2d_debug("sysmmu_set_tablebase_pgd\n");
		sysmmu_set_tablebase_pgd(id, pgd_base);
		spin_unlock(&info->power_lock);
	}
}

static const struct s5p_vcm_driver fimg2d_vcm_driver = {
       .tlb_invalidator = &fimg2d_tlb_invalidate,
       .pgd_base_specifier = &fimg2d_set_pagetable,
       .phys_alloc = NULL,
       .phys_free = NULL,
};
#endif

/**
 *
 */
static inline void fimg2d_power_on(void)
{
	spin_lock(&info->power_lock);
#ifndef CONFIG_S5PV310_FPGA
	clk_enable(info->clock);
	fimg2d_debug("clock enable\n");
#endif
#ifdef CONFIG_S5P_SYSMMU_FIMG2D
	sysmmu_on(SYSMMU_G2D);
	fimg2d_debug("sysmmu on\n");
#endif

	atomic_set(&info->power, 1);

	spin_unlock(&info->power_lock);
}

/**
 *
 */
static inline void fimg2d_power_off(void)
{
	spin_lock(&info->power_lock);
#ifdef CONFIG_S5P_SYSMMU_FIMG2D

	atomic_set(&info->power, 0);

	sysmmu_off(SYSMMU_G2D);
	fimg2d_debug("sysmmu off\n");
#endif
#ifndef CONFIG_S5PV310_FPGA
	clk_disable(info->clock);
	fimg2d_debug("clock disable\n");
#endif
	spin_unlock(&info->power_lock);
}

/**
 *
 */
static void fimg2d_worker(struct work_struct *work)
{
	info->bitblt(info);
}

static DECLARE_WORK(fimg2d_work, fimg2d_worker);

/**
 * fimg2d_irq - [GENERIC] irq service routine
 * @irq: irq number
 * @dev_id: pointer to private data
 *
 * ISR finalizes current bitblt and calls fimg2d_do_next_bitblt() to
 * render next context
*/
static irqreturn_t fimg2d_irq(int irq, void *dev_id)
{
	fimg2d_debug("irq\n");
	info->stop(info);

	return IRQ_HANDLED;
}

/**
 * fimg2d_open - [GENERIC] open
 * @inode: pointer to inode
 * @file: pointer to file
*/
static int fimg2d_open(struct inode *inode, struct file *file)
{
	struct fimg2d_context *ctx;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx) {
		printk(KERN_ERR "not enough memory for context\n");
		return -ENOMEM;
	}

	fimg2d_debug("context: %p\n", ctx);

	INIT_LIST_HEAD(&ctx->node);
	INIT_LIST_HEAD(&ctx->reg_q);
	init_waitqueue_head(&ctx->wq);

	fimg2d_enqueue(info, &ctx->node, &info->ctx_q);

	file->private_data = ctx;
	atomic_inc(&info->ref_count);

	if (atomic_read(&info->ref_count) == 1) {
#ifndef CONFIG_PM_RUNTIME
		fimg2d_power_on();
#if defined(CONFIG_S5P_SYSMMU_FIMG2D) && defined(CONFIG_VCM)
		vcm_set_pgtable_base(VCM_DEV_G2D);
		fimg2d_debug("vcm_set_pgtable_base\n");
#endif /* CONFIG_S5P_SYSMMU_FIMG2D && CONFIG_VCM */
#endif /* !CONFIG_PM_RUNTIME */
	}

#if defined(CONFIG_S5P_SYSMMU_FIMG2D) && defined(CONFIG_S5P_VMEM)
	ctx->pgd = __pa(current->mm->pgd);
	fimg2d_debug("ctx pgd(va:0x%x pa:0x%x)\n",
			(unsigned int)current->mm->pgd,
			(unsigned int)ctx->pgd);
#endif

	return 0;
}

/**
 * fimg2d_wait - [INTERNAL] waits for done
 * @info: controller info
 * @ctx: context info
 *
 * This function waits until previous rendering is done for this context
*/
static void fimg2d_wait(struct fimg2d_control *info, struct fimg2d_context *ctx)
{
	struct fimg2d_context *active = NULL;
	int ret = 0;

	spin_lock(&info->lock);
	active = info->active;
	fimg2d_debug("active: %p\n", active);
	spin_unlock(&info->lock);

	/* wait if hardware is still working for
	 * previous rendering of current context.
	*/
	if (ctx == active) {
		fimg2d_debug("ctx(%p) waiting for bitblt complete\n", ctx);
wait:
		ret = wait_event_timeout(ctx->wq, ctx != info->active, 5000);
		if (ret == 0) {
			if (!info->fatal && atomic_read(&ctx->nreg) > 0) {
				fimg2d_debug("ctx(%p) bitblt not finished\n", ctx);
				goto wait;
			}
			printk(KERN_ERR "ctx(%p) bitblt wait timeout\n", ctx);
		}
	}
}

/**
 * fimg2d_release - [GENERIC] release
 * @inode: pointer to inode
 * @file: pointer to file
*/
static int fimg2d_release(struct inode *inode, struct file *file)
{
	struct fimg2d_context *ctx = file->private_data;

	fimg2d_debug("context: %p\n", ctx);

	fimg2d_wait(info, ctx);

	fimg2d_debug("done, it's time to release\n");

	fimg2d_debug("free context: %p\n", ctx);

	fimg2d_dequeue(info, &ctx->node);
	kfree(ctx);
	atomic_dec(&info->ref_count);

	if (atomic_read(&info->ref_count) == 0) {
#ifndef CONFIG_PM_RUNTIME
		fimg2d_power_off();
#endif
	}
	return 0;
}

/**
 * fimg2d_mmap - [GENERIC] mmap
 * @file: pointer to file
 * @vma: vm_area_struct
*/
static int fimg2d_mmap(struct file *file, struct vm_area_struct *vma)
{
	return 0;
}

/**
 * fimg2d_poll - [GENERIC] poll
 * @file: pointer to file
 * @wait: poll_table_struct
*/
static unsigned int fimg2d_poll(struct file *file, struct poll_table_struct *wait)
{
	return 0;
}

#ifdef CONFIG_OUTER_CACHE
/*
 * @sta_addr: virtual address for s5p-vmem, physical address for others
 */
static void outer_cache_opr(unsigned long sta_addr, int size, int opr)
{
	unsigned long phy_addr;
	unsigned long cur_addr;
	unsigned long end_addr;

	cur_addr = sta_addr & PAGE_MASK;
	end_addr = cur_addr + PAGE_ALIGN(size);

	if (opr == DMA_TO_DEVICE) {
		while (cur_addr < end_addr) {
#if defined(CONFIG_S5P_SYSMMU_FIMG2D) && defined(CONFIG_S5P_VMEM)
			phy_addr = (unsigned long)page_to_pfn(s5p_vmem_va2page((void *)cur_addr));
			phy_addr <<= PAGE_SHIFT;
#else
			phy_addr = cur_addr;
#endif
			outer_clean_range(phy_addr, phy_addr + PAGE_SIZE);
			cur_addr += PAGE_SIZE;
		}
	}
	else if (opr == DMA_FROM_DEVICE) {
		while (cur_addr < end_addr) {
#if defined(CONFIG_S5P_SYSMMU_FIMG2D) && defined(CONFIG_S5P_VMEM)
			phy_addr = (unsigned long)page_to_pfn(s5p_vmem_va2page((void *)cur_addr));
			phy_addr <<= PAGE_SHIFT;
#else
			phy_addr = cur_addr;
#endif
			outer_inv_range(phy_addr, phy_addr + PAGE_SIZE);
			cur_addr += PAGE_SIZE;
		}
	}
	else { // DMA_BIDIRECTIONAL
		while (cur_addr < end_addr) {
#if defined(CONFIG_S5P_SYSMMU_FIMG2D) && defined(CONFIG_S5P_VMEM)
			phy_addr = (unsigned long)page_to_pfn(s5p_vmem_va2page((void *)cur_addr));
			phy_addr <<= PAGE_SHIFT;
#else
			phy_addr = cur_addr;
#endif
			outer_flush_range(phy_addr, phy_addr + PAGE_SIZE);
			cur_addr += PAGE_SIZE;
		}
	}
}
#endif

/**
 * fimg2d_do_cache_op - [INTERNAL] performs cache operation
 * @cmd: ioctl command
 * @arg: argument for command
*/
static int fimg2d_do_cache_op(unsigned int cmd, unsigned long arg)
{
	struct fimg2d_dma_info dma;
	void *vaddr;
	int opr;

	fimg2d_debug("do cache op\n");

	if (copy_from_user(&dma, (struct fimg2d_dma_info *)arg, sizeof(dma)))
		return -EFAULT;

	switch (dma.addr_type) {
	case FIMG2D_ADDR_PHYS:
		vaddr = phys_to_virt(dma.addr);
		break;
#if defined(CONFIG_S5P_SYSMMU_FIMG2D) && defined(CONFIG_S5P_VMEM)
	case FIMG2D_ADDR_KERN:
		vaddr = (void *)dma.addr;
		break;
	case FIMG2D_ADDR_COOKIE:
		vaddr = s5p_getaddress(dma.addr);
		break;
#endif
	default:
		fimg2d_debug("invalid address type\n");
		return -1;
	}

	if (cmd == FIMG2D_DMA_CACHE_CLEAN)
		opr = DMA_TO_DEVICE;
	else if (cmd == FIMG2D_DMA_CACHE_INVAL)
		opr = DMA_FROM_DEVICE;
	else if (cmd == FIMG2D_DMA_CACHE_FLUSH)
		opr = DMA_BIDIRECTIONAL;
	else if (cmd == FIMG2D_DMA_CACHE_FLUSH_ALL) {
		__cpuc_flush_kern_all();
		fimg2d_debug("currently, does not support flush all for outer cache\n");
		return 0;
	}
	else {
		fimg2d_debug("invalid cmd\n");
		return -1;
	}

	if (opr == DMA_TO_DEVICE || opr == DMA_FROM_DEVICE)
		dmac_map_area(vaddr, dma.size, opr);
	else  // DMA_BIDIRECTIONAL
		dmac_flush_range(vaddr, vaddr + dma.size);

#ifdef CONFIG_OUTER_CACHE
	/* outer L2 cache */
#if defined(CONFIG_S5P_SYSMMU_FIMG2D) && defined(CONFIG_S5P_VMEM)
	outer_cache_opr((unsigned long)vaddr, dma.size, opr);
#else
	outer_cache_opr(dma.addr, dma.size, opr);
#endif
#endif

	return 0;
}

/**
 *
 */
static inline void fimg2d_request_bitblt(struct fimg2d_context *ctx)
{
	struct fimg2d_context *active = NULL;

	spin_lock(&info->lock);
	active = info->active;
	spin_unlock(&info->lock);

	/* start kernel thread if hardware is idle */
	if (!active) {
		spin_lock(&info->lock);
		info->active = ctx;
		spin_unlock(&info->lock);
		fimg2d_debug("dispatch ctx to kernel thread\n");
		queue_work(info->workqueue, &fimg2d_work);
	}
}

/**
 * fimg2d_ioctl - [GENERIC] ioctl
 * @inode: pointer to inode
 * @file: pointer to file
 * @cmd: ioctl command
 * @arg: argument for command
 *
 * FIMG2D_BITBLT_CONFIG: configures for new rendering context
 * FIMG2D_BITBLT_UPDATE: updates for existing context
 *   (usually changes coordinate values)
 *
 * FIMG2D_BITBLT_CLOSE: closes for existing context
 * FIMG2D_BITBLT_WAIT: waits for done of previous rendering
 * FIMG2D_DMA_XXX: performs cache operation
*/
static long fimg2d_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct fimg2d_context *ctx = NULL;
	union {
		struct fimg2d_user_context *u_ctx;
		struct fimg2d_user_region *u_reg;
	} p;

	ctx = file->private_data;
	if (!ctx) {
		printk(KERN_ERR "fatal error: missing context\n");
		return -EFAULT;
	}

	switch (cmd) {
	case FIMG2D_BITBLT_CONFIG:
		fimg2d_debug("FIMG2D_BITBLT_CONFIG: %p\n", ctx);
		p.u_ctx = (struct fimg2d_user_context *)arg;
		ret = fimg2d_set_context(info, ctx, p.u_ctx);
		break;

	case FIMG2D_BITBLT_UPDATE:
		fimg2d_debug("FIMG2D_BITBLT_UPDATE: %p\n", ctx);
		p.u_reg = (struct fimg2d_user_region *)arg;
		ret = fimg2d_add_region(info, ctx, p.u_reg);

		if (atomic_read(&ctx->nreg) > 100)
			fimg2d_request_bitblt(ctx);
		break;

	case FIMG2D_BITBLT_CLOSE:
		fimg2d_debug("FIMG2D_BITBLT_CLOSE: %p\n", ctx);
		ret = fimg2d_close_bitblt(info, ctx);

		fimg2d_request_bitblt(ctx);
		fimg2d_wait(info, ctx);
		break;

	case FIMG2D_BITBLT_START:
		fimg2d_debug("FIMG2D_BITBLT_START: %p\n", ctx);

		if (atomic_read(&ctx->nreg) > 0)
			fimg2d_request_bitblt(ctx);
		break;

	case FIMG2D_BITBLT_WAIT:
		fimg2d_debug("FIMG2D_BITBLT_WAIT: %p\n", ctx);
		fimg2d_wait(info, ctx);
		break;

	case FIMG2D_DMA_CACHE_INVAL:	/* fall through */
	case FIMG2D_DMA_CACHE_CLEAN:	/* fall through */
	case FIMG2D_DMA_CACHE_FLUSH:	/* fall through */
	case FIMG2D_DMA_CACHE_FLUSH_ALL:
		ret = fimg2d_do_cache_op(cmd, arg);
		break;

	default:
		printk(KERN_ERR "unknown ioctl\n");
		ret = -EFAULT;
		break;
	}

	return ret;
}

/* fops */
static const struct file_operations fimg2d_fops = {
	.owner          = THIS_MODULE,
	.open           = fimg2d_open,
	.release        = fimg2d_release,
	.mmap           = fimg2d_mmap,
	.poll           = fimg2d_poll,
	.unlocked_ioctl = fimg2d_ioctl,
};

/* miscdev */
static struct miscdevice fimg2d_dev = {
	.minor		= FIMG2D_MINOR,
	.name		= "fimg2d",
	.fops		= &fimg2d_fops,
};

/**
 * fimg2d_setup_controller - controller information
 * @info: allocated controller header
*/
static int fimg2d_setup_controller(struct fimg2d_control *info)
{
	atomic_set(&info->power, 0);
	spin_lock_init(&info->power_lock);
	spin_lock_init(&info->lock);
	atomic_set(&info->ref_count, 0);
	atomic_set(&info->busy, 0);
	init_waitqueue_head(&info->wq);
	INIT_LIST_HEAD(&info->ctx_q);
	fimg2d_register_ops(info);

	info->workqueue = create_singlethread_workqueue("kfimg2dd");
	if (!info->workqueue)
		return -ENOMEM;

	return 0;
}

/**
 * fimg2d_probe - [GENERIC] probe
 * @pdev: pointer to platform device
*/
static int fimg2d_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct clk *parent, *sclk;
	struct fimg2d_platdata *pdata;
	int ret;

	pdata = to_fimg2d_plat(&pdev->dev);
	if (!pdata) {
		printk(KERN_ERR "no platform data\n");
		ret = -ENOMEM;
		goto err_plat;
	}

	/* global structure */
	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		printk(KERN_ERR "no memory for fimg2d info\n");
		ret = -ENOMEM;
		goto err_plat;
	}

	/* setup global info */
	ret = fimg2d_setup_controller(info);
	if (ret) {
		printk(KERN_ERR "failed to setup controller info\n");
		goto err_setup;
	}

	/* memory region */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		printk(KERN_ERR "failed to get resource\n");
		ret = -ENOENT;
		goto err_res;
	}

	info->mem = request_mem_region(res->start, resource_size(res),
					pdev->name);
	if (!info->mem) {
		printk(KERN_ERR "failed to request memory region\n");
		ret = -ENOMEM;
		goto err_region;
	}

	/* ioremap */
	info->regs = ioremap(res->start, resource_size(res));
	if (!info->regs) {
		printk(KERN_ERR "failed to ioremap\n");
		ret = -ENOENT;
		goto err_map;
	}

	/* irq */
	info->irq = platform_get_irq(pdev, 0);
	if (!info->irq) {
		printk(KERN_ERR "failed to get irq resource\n");
		ret = -ENOENT;
		goto err_map;
	}

	ret = request_irq(info->irq, fimg2d_irq, IRQF_DISABLED, pdev->name, info);
	if (ret) {
		printk(KERN_ERR "failed to request irq\n");
		ret = -ENOENT;
		goto err_irq;
	}

#ifndef CONFIG_S5PV310_FPGA
	/* clock for setting parent and rate */
	parent = clk_get(&pdev->dev, pdata->parent_clkname);
	if (IS_ERR(parent)) {
		printk(KERN_ERR "failed to get parent clock\n");
		ret = -ENOENT;
		goto err_clk1;
	}

	sclk = clk_get(&pdev->dev, pdata->clkname);
	if (IS_ERR(sclk)) {
		printk(KERN_ERR "failed to get sclk_g2d clock\n");
		ret = -ENOENT;
		goto err_clk2;
	}

	clk_set_parent(sclk, parent);
	clk_set_rate(sclk, pdata->clkrate);

	/* clock for gating */
	info->clock = clk_get(&pdev->dev, pdata->gate_clkname);
	if (IS_ERR(info->clock)) {
		printk(KERN_ERR "failed to get clock\n");
		ret = -ENOENT;
		goto err_clk3;
	}
#endif

	pm_runtime_enable(&pdev->dev);
	pm_runtime_get_sync(&pdev->dev);
	printk(KERN_INFO "FIMG2D PM runtime enable\n");

#if defined(CONFIG_S5P_SYSMMU_FIMG2D) && defined(CONFIG_VCM)
	s5pvcm = vcm_create_unified(1024 << 20, VCM_DEV_G2D, &fimg2d_vcm_driver);
	printk(KERN_INFO "FIMG2D VCM init 1024 MB\n");

	if (s5pvcm) {
		if (vcm_activate(s5pvcm)) {
			printk(KERN_ERR "FIMG2D failed to activate vcm\n");
		} else {
			fimg2d_debug("vcm activate\n");
		}
	}
#endif /* CONFIG_S5P_SYSMMU_FIMG2D && CONFIG_VCM */

	/* misc register */
	ret = misc_register(&fimg2d_dev);
	if (ret) {
		printk(KERN_ERR "FIMG2D failed to register misc driver\n");
		goto err_reg;
	}

	info->dev = &pdev->dev;
	printk(KERN_INFO "Samsung Graphics 2D driver, (c) 2010 Samsung Electronics\n");

	return 0;

err_reg:
	clk_put(info->clock);

err_clk3:
	clk_put(sclk);

err_clk2:
	clk_put(parent);

err_clk1:
	free_irq(info->irq, NULL);

err_irq:
	iounmap(info->regs);

err_map:
	kfree(info->mem);

err_region:
	release_resource(info->mem);

err_res:
	destroy_workqueue(info->workqueue);

err_setup:
	kfree(info);

err_plat:
	return ret;
}

/**
 * fimg2d_remove - [GENERIC] remove
 * @pdev: pointer to platform device
*/
static int fimg2d_remove(struct platform_device *pdev)
{
	free_irq(info->irq, NULL);

	if (info->mem) {
		iounmap(info->regs);
		release_resource(info->mem);
		kfree(info->mem);
	}

	destroy_workqueue(info->workqueue);
	misc_deregister(&fimg2d_dev);
	kfree(info);

#if defined(CONFIG_S5P_SYSMMU_FIMG2D) && defined(CONFIG_VCM)
	if (s5pvcm) {
		vcm_deactivate(s5pvcm);
		fimg2d_debug("vcm deactivate\n");
	}
#endif /* CONFIG_S5P_SYSMMU_FIMG2D && CONFIG_VCM */

	/* disable the power domain */
        pm_runtime_put(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	fimg2d_debug("pm_runtime_disable\n");

	return 0;
}

/**
 * fimg2d_suspend - [GENERIC] suspend
 * @pdev: pointer to platform device
 * @state: power state
*/
static int fimg2d_suspend(struct platform_device *pdev, pm_message_t state)
{
	if (atomic_read(&info->power) == 1) {
		fimg2d_power_off();
		/* disable the power domain */
        	fimg2d_debug("fimg2d: disable power domain\n");
	        pm_runtime_put(&pdev->dev);
	}
	return 0;
}

/**
 * fimg2d_resume - [GENERIC] resume
 * @pdev: pointer to platform device
*/
static int fimg2d_resume(struct platform_device *pdev)
{
	if (atomic_read(&info->power) == 0) {
		/* enable the power domain */
	        fimg2d_debug("fimg2d - enable power domain\n");
        	pm_runtime_get_sync(&pdev->dev);
		fimg2d_power_on();
#if defined(CONFIG_S5P_SYSMMU_FIMG2D) && defined(CONFIG_VCM)
		fimg2d_debug("vcm_set_pgtable_base\n");
		vcm_set_pgtable_base(VCM_DEV_G2D);
#endif
	}
	return 0;
}

#ifdef CONFIG_PM_RUNTIME
static int fimg2d_runtime_suspend(struct device *dev)
{
	fimg2d_power_off();
	return 0;
}

#if 0
static int fimg2d_runtime_idle(struct device *dev)
{
	return 0;
}
#endif

static int fimg2d_runtime_resume(struct device *dev)
{
	fimg2d_power_on();
	return 0;
}

static const struct dev_pm_ops fimg2d_pm_ops = {
	.runtime_suspend = fimg2d_runtime_suspend,
	.runtime_resume = fimg2d_runtime_resume,
};
#endif

/* pdev */
static struct platform_driver fimg2d_driver = {
	.probe		= fimg2d_probe,
	.remove		= fimg2d_remove,
	.suspend	= fimg2d_suspend,
	.resume		= fimg2d_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "s5p-fimg2d",
#ifdef CONFIG_PM_RUNTIME
		.pm     = &fimg2d_pm_ops,
#endif
	},
};

/**
 * fimg2d_register - [GENERIC] register platform driver
 * @pdev: pointer to platform device
*/
static int __init fimg2d_register(void)
{
	platform_driver_register(&fimg2d_driver);

	return 0;
}

/**
 * fimg2d_unregister - [GENERIC] unregister platform driver
 * @pdev: pointer to platform device
*/
static void __exit fimg2d_unregister(void)
{
	platform_driver_unregister(&fimg2d_driver);
}

module_init(fimg2d_register);
module_exit(fimg2d_unregister);

MODULE_AUTHOR("Jinsung Yang <jsgood.yang@samsung.com>");
MODULE_DESCRIPTION("Samsung Graphics 2D driver");
MODULE_LICENSE("GPL");
