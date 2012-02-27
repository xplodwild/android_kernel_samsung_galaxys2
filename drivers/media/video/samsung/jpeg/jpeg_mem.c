/* linux/drivers/media/video/samsung/jpeg/jpeg_mem.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * http://www.samsung.com/
 *
 * Managent memory of the jpeg driver for encoder/docoder.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <asm/page.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>

#if defined(CONFIG_S5P_MEM_CMA)
#include <linux/cma.h>
#elif defined(CONFIG_S5P_MEM_BOOTMEM)
#include <mach/bootmem.h>
#include <plat/bootmem.h>
#endif

#include "jpeg_mem.h"
#include "jpeg_core.h"

#if defined(CONFIG_S5P_SYSMMU_JPEG) && !defined(CONFIG_S5P_VMEM)
unsigned int s_cookie;	/* for stream buffer */
unsigned int f_cookie;	/* for frame buffer */
#endif

#if defined(CONFIG_S5P_SYSMMU_JPEG) && defined(CONFIG_VIDEO_UMP)
#if defined(CONFIG_VCM_MMU)
unsigned int str_s_id;	/* secure id for stream buffer */
unsigned int fra_s_id;	/* secure id for frame buffer */
#endif /* CONFIG_VCM_MMU */
unsigned int str_base;	/* for stream buffer */
unsigned int fra_base;	/* for frame buffer */
#endif

int jpeg_init_mem(struct device *dev, unsigned int *base)
{
#ifdef CONFIG_S5P_MEM_CMA
	struct cma_info mem_info;
	int err;
	int size;
	char cma_name[8];
#endif
#if defined(CONFIG_S5P_SYSMMU_JPEG)
#if !defined(CONFIG_S5P_VMEM)
	unsigned char *addr;
	addr = vmalloc(JPEG_MEM_SIZE);
	if (addr == NULL)
		return -1;
	*base = (unsigned int)addr;
#endif /* CONFIG_S5P_VMEM */
#else
#ifdef CONFIG_S5P_MEM_CMA
	/* CMA */
	sprintf(cma_name, "jpeg");
	err = cma_info(&mem_info, dev, 0);
	jpeg_info("[cma_info] start_addr : 0x%x, end_addr : 0x%x, "
			"total_size : 0x%x, free_size : 0x%x\n",
			mem_info.lower_bound, mem_info.upper_bound,
			mem_info.total_size, mem_info.free_size);
	if (err) {
		printk("%s: get cma info failed\n", __func__);
		return -1;
	}
	size = mem_info.total_size;
	*base = (dma_addr_t)cma_alloc
		(dev, cma_name, (size_t)size, 0);
	jpeg_info("size = 0x%x\n", size);
	jpeg_info("*base = 0x%x\n", *base);
#else
	*base = s5p_get_media_memory_bank(S5P_MDEV_JPEG, 0);
#endif
#endif /* CONFIG_S5P_SYSMMU_JPEG */
	return 0;
}

int jpeg_mem_free(void)
{
#if defined(CONFIG_S5P_SYSMMU_JPEG) && !defined(CONFIG_S5P_VMEM)
	if (s_cookie != 0) {
		s5p_vfree(s_cookie);
		s_cookie = 0;
	}
	if (f_cookie != 0) {
		s5p_vfree(f_cookie);
		f_cookie = 0;
	}
#endif
	return 0;
}

unsigned long jpeg_get_stream_buf(unsigned long arg)
{
#if defined(CONFIG_S5P_SYSMMU_JPEG) && !defined(CONFIG_S5P_VMEM)
	arg = ((arg / PAGE_SIZE + 1) * PAGE_SIZE);
	s_cookie = (unsigned int)s5p_vmalloc(arg);
	if (s_cookie == 0)
		return -1;
	return (unsigned long)s_cookie;
#else
	return arg + JPEG_MAIN_START;
#endif
}

unsigned long jpeg_get_frame_buf(unsigned long arg)
{
#if defined(CONFIG_S5P_SYSMMU_JPEG) && !defined(CONFIG_S5P_VMEM)
	arg = ((arg / PAGE_SIZE + 1) * PAGE_SIZE);
	f_cookie = (unsigned int)s5p_vmalloc(arg);
	if (f_cookie == 0)
		return -1;
	return (unsigned long)f_cookie;
#else
	return arg + JPEG_S_BUF_SIZE;
#endif
}

void jpeg_set_stream_buf(unsigned int *str_buf, unsigned int base)
{
#if defined(CONFIG_S5P_SYSMMU_JPEG) && !defined(CONFIG_S5P_VMEM)
	*str_buf = (unsigned int)s5p_getaddress(s_cookie);
#elif defined(CONFIG_S5P_SYSMMU_JPEG) && defined(CONFIG_VIDEO_UMP)
#if !defined(CONFIG_VCM)
	sysmmu_set_tablebase_pgd(SYSMMU_JPEG, __pa(current->mm->pgd));
#endif
	*str_buf = str_base;
#else
	*str_buf = base;
#endif
}

void jpeg_set_frame_buf(unsigned int *fra_buf, unsigned int base)
{
#if defined(CONFIG_S5P_SYSMMU_JPEG) && !defined(CONFIG_S5P_VMEM)
	*fra_buf = (unsigned int)s5p_getaddress(f_cookie);
#elif defined(CONFIG_S5P_SYSMMU_JPEG) && defined(CONFIG_VIDEO_UMP)
#if !defined(CONFIG_VCM)
	sysmmu_set_tablebase_pgd(SYSMMU_JPEG, __pa(current->mm->pgd));
#endif
	*fra_buf = fra_base;
#else
	*fra_buf = base + JPEG_S_BUF_SIZE;
#endif
}

void jpeg_set_stream_base(unsigned int base)
{
#if defined(CONFIG_S5P_SYSMMU_JPEG) && defined(CONFIG_VIDEO_UMP)
#if defined(CONFIG_UMP_VCM_ALLOC)
	struct vcm_res *vcm_res;
	str_s_id = base;
	vcm_res = (struct vcm_res *)
		ump_dd_meminfo_get(str_s_id, (void *)VCM_DEV_JPEG);
	if (vcm_res)
		str_base = vcm_res->start;
	else
		str_base = 0;
#else
	str_base = base;
#endif /* CONFIG_UMP_VCM_ALLOC */
#endif
}

void jpeg_set_frame_base(unsigned int base)
{
#if defined(CONFIG_S5P_SYSMMU_JPEG) && defined(CONFIG_VIDEO_UMP)
#if defined(CONFIG_UMP_VCM_ALLOC)
	struct vcm_res *vcm_res;
	fra_s_id = base;
	vcm_res = (struct vcm_res *)
		ump_dd_meminfo_get(fra_s_id, (void *)VCM_DEV_JPEG);
	if (vcm_res)
		fra_base = vcm_res->start;
	else
		fra_base = 0;
#else
	fra_base = base;
#endif /* CONFIG_UMP_VCM_ALLOC */
#endif
}

