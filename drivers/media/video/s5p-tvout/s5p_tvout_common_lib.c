/*
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Common library for SAMSUNG S5P TVOUT driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/io.h>
#include <linux/slab.h>
#include <linux/err.h>

#include <linux/pm_runtime.h>

#include "s5p_tvout_common_lib.h"

int s5p_tvout_map_resource_mem(struct platform_device *pdev, char *name,
			       void __iomem **base, struct resource **res)
{
	size_t		size;
	void __iomem	*tmp_base;
	struct resource	*tmp_res;

	tmp_res = platform_get_resource_byname(pdev, IORESOURCE_MEM, name);

	if (!tmp_res)
		goto not_found;

	size = resource_size(tmp_res);

	tmp_res = request_mem_region(tmp_res->start, size, tmp_res->name);

	if (!tmp_res) {
		tvout_err("%s: fail to get memory region\n", __func__);
		goto err_on_request_mem_region;
	}

	tmp_base = ioremap(tmp_res->start, size);

	if (!tmp_base) {
		tvout_err("%s: fail to ioremap address region\n", __func__);
		goto err_on_ioremap;
	}

	*res = tmp_res;
	*base = tmp_base;
	return 0;

err_on_ioremap:
	release_resource(tmp_res);
	kfree(tmp_res);

err_on_request_mem_region:
	return -ENXIO;

not_found:
	tvout_err("%s: fail to get IORESOURCE_MEM for %s\n", __func__, name);
	return -ENODEV;
}

void s5p_tvout_unmap_resource_mem(void __iomem *base, struct resource *res)
{
	if (!base)
		iounmap(base);

	if (res != NULL) {
		release_resource(res);
		kfree(res);
	}
}

/* Libraries for runtime PM */

static struct device	*s5p_tvout_dev;

void s5p_tvout_pm_runtime_enable(struct device *dev)
{
	pm_runtime_enable(dev);

	s5p_tvout_dev = dev;
}

void s5p_tvout_pm_runtime_disable(struct device *dev)
{
	pm_runtime_disable(dev);
}

void s5p_tvout_pm_runtime_get(void)
{
	pm_runtime_get_sync(s5p_tvout_dev);
}

void s5p_tvout_pm_runtime_put(void)
{
	pm_runtime_put_sync(s5p_tvout_dev);
}
