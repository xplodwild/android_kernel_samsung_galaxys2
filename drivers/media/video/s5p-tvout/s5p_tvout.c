/*
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * S5P TVOUT driver main
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/clk.h>
#include <linux/io.h>
#include <linux/mm.h>

#include "s5p_tvout_common_lib.h"
#include "s5p_tvout_ctrl.h"
#include "s5p_tvout_fb.h"
#include "s5p_tvout_v4l2.h"

#define TV_CLK_GET_WITH_ERR_CHECK(clk, pdev, clk_name)			\
		do {							\
			clk = clk_get(&pdev->dev, clk_name);		\
			if (IS_ERR(clk)) {				\
				printk(KERN_ERR				\
				"failed to find clock %s\n", clk_name);	\
				return -ENOENT;				\
			}						\
		} while (0);

struct s5p_tvout_status s5ptv_status;

static int __devinit s5p_tvout_clk_get(struct platform_device *pdev,
				       struct s5p_tvout_status *ctrl)
{
	struct clk *ext_xtal_clk, *mout_vpll_src, *fout_vpll, *mout_vpll;

	TV_CLK_GET_WITH_ERR_CHECK(ctrl->i2c_phy_clk,	pdev, "i2c-hdmiphy");

	TV_CLK_GET_WITH_ERR_CHECK(ctrl->sclk_dac,	pdev, "sclk_dac");
	TV_CLK_GET_WITH_ERR_CHECK(ctrl->sclk_hdmi,	pdev, "sclk_hdmi");

	TV_CLK_GET_WITH_ERR_CHECK(ctrl->sclk_pixel,	pdev, "sclk_pixel");
	TV_CLK_GET_WITH_ERR_CHECK(ctrl->sclk_hdmiphy,	pdev, "sclk_hdmiphy");

	TV_CLK_GET_WITH_ERR_CHECK(ext_xtal_clk,		pdev, "ext_xtal");
	TV_CLK_GET_WITH_ERR_CHECK(mout_vpll_src,	pdev, "vpll_src");
	TV_CLK_GET_WITH_ERR_CHECK(fout_vpll,		pdev, "fout_vpll");
	TV_CLK_GET_WITH_ERR_CHECK(mout_vpll,		pdev, "sclk_vpll");

	if (clk_set_rate(fout_vpll, 54000000) < 0)
		return -1;

	if (clk_set_parent(mout_vpll_src, ext_xtal_clk) < 0)
		return -1;

	if (clk_set_parent(mout_vpll, fout_vpll) < 0)
		return -1;

	/* sclk_dac's parent is fixed as mout_vpll */
	if (clk_set_parent(ctrl->sclk_dac, mout_vpll) < 0)
		return -1;

	/* It'll be moved in the future */
	if (clk_enable(ctrl->i2c_phy_clk) < 0)
		return -1;

	if (clk_enable(mout_vpll_src) < 0)
		return -1;

	if (clk_enable(fout_vpll) < 0)
		return -1;

	if (clk_enable(mout_vpll) < 0)
		return -1;

	clk_put(ext_xtal_clk);
	clk_put(mout_vpll_src);
	clk_put(fout_vpll);
	clk_put(mout_vpll);

	return 0;
}

static int __devinit s5p_tvout_probe(struct platform_device *pdev)
{
	enum s5p_mixer_layer layer = MIXER_GPR0_LAYER;
	s5p_tvout_pm_runtime_enable(&pdev->dev);

	printk("[s5p_tvout.c] : s5p_tvout_probe()\n");
	/* The feature of System MMU will be turned on later */
	if (s5p_tvout_clk_get(pdev, &s5ptv_status) < 0) {
		printk(KERN_ERR "[s5p_tvout.c] s5p_tvout_clk_get() error\n");
		return -ENODEV;
	}
	if (s5p_vp_ctrl_constructor(pdev) < 0) {
		printk(KERN_ERR "[s5p_tvout.c] s5p_vp_ctrl_constructor error\n");
		return -ENODEV;
	}
	/*
	* s5p_mixer_ctrl_constructor() must be
	* called before s5p_tvif_ctrl_constructor
	*/
	if (s5p_mixer_ctrl_constructor(pdev) < 0) {
		printk(KERN_ERR "[s5p_tvout.c] s5p_mixer_ctrl_constructor error\n");
		return -ENODEV;
	}
	if (s5p_tvif_ctrl_constructor(pdev) < 0) {
		printk(KERN_ERR "[s5p_tvout.c] s5p_tvif_ctrl_constructor error\n");
		return -ENODEV;
	}
	if (s5p_tvout_v4l2_constructor(pdev) < 0) {
		printk(KERN_ERR "[s5p_tvout.c] s5p_tvout_v4l2_constructor error\n");
		return -ENODEV;
	}
	#ifndef CONFIG_USER_ALLOC_TVOUT
	if (s5p_tvif_ctrl_start(TVOUT_1080P_60, TVOUT_HDMI) < 0) {
		printk(KERN_ERR "[s5p_tvout.c] s5p_tvif_ctrl_start error\n");
		return -ENODEV;
	}
	#endif
	/* prepare memory */
	if (s5p_tvout_fb_alloc_framebuffer(&pdev->dev)) {
		printk(KERN_ERR "[s5p_tvout.c] s5p_tvout_fb_alloc_framebuffer error\n");
		return -ENODEV;
	}
	if (s5p_tvout_fb_register_framebuffer(&pdev->dev)) {
		printk(KERN_ERR "[s5p_tvout.c] s5p_tvout_fb_register_framebuffer error\n");
		return -ENODEV;
	}

	/* Associate LCD FB memory with the HDMI FB memory */
	if (s5p_tvout_fb_setup_framebuffer(&pdev->dev)) {
		printk(KERN_ERR "[S5P-TVOUT] Failed to setup FB memory\n");
		return -ENODEV;
	}

	/* Enable the HDMI/FB to start displaying */
	if (s5p_tvout_fb_ctrl_enable(1) < 0) {
		printk(KERN_ERR "[S5P-TVOUT] Failed to enable the HDMI display\n");
		return -ENODEV;
	}

	return 0;
}

static int s5p_tvout_remove(struct platform_device *pdev)
{
	s5p_vp_ctrl_destructor();
	s5p_tvif_ctrl_destructor();
	s5p_mixer_ctrl_destructor();

	s5p_tvout_v4l2_destructor();

	clk_disable(s5ptv_status.sclk_hdmi);

	clk_put(s5ptv_status.sclk_hdmi);
	clk_put(s5ptv_status.sclk_dac);
	clk_put(s5ptv_status.sclk_pixel);
	clk_put(s5ptv_status.sclk_hdmiphy);

	s5p_tvout_pm_runtime_disable(&pdev->dev);
	return 0;
}

#ifdef CONFIG_PM
static int s5p_tvout_suspend(struct device *dev)
{
	s5p_vp_ctrl_suspend();
	s5p_mixer_ctrl_suspend();
	s5p_tvif_ctrl_suspend();

	return 0;
}

static int s5p_tvout_resume(struct device *dev)
{
	s5p_tvif_ctrl_resume();
	s5p_mixer_ctrl_resume();
	s5p_vp_ctrl_resume();

	return 0;
}

static int s5p_tvout_runtime_suspend(struct device *dev)
{
	return 0;
}

static int s5p_tvout_runtime_resume(struct device *dev)
{
	return 0;
}

#else
#define s5p_tvout_suspend		NULL
#define s5p_tvout_resume		NULL
#define s5p_tvout_runtime_suspend	NULL
#define s5p_tvout_runtime_resume	NULL
#endif

static const struct dev_pm_ops s5p_tvout_pm_ops = {
	.suspend		= s5p_tvout_suspend,
	.resume			= s5p_tvout_resume,
	.runtime_suspend	= s5p_tvout_runtime_suspend,
	.runtime_resume		= s5p_tvout_runtime_resume
};

static struct platform_driver s5p_tvout_driver = {
	.probe		=  s5p_tvout_probe,
	.remove		=  s5p_tvout_remove,
	.driver		=  {
		.name		= "s5p-tvout",
		.owner		= THIS_MODULE,
		.pm		= &s5p_tvout_pm_ops
	},
};

static int __init s5p_tvout_init(void)
{
	int ret;
	printk(KERN_INFO "S5P TVOUT Driver, Copyright (c) 2011 Samsung Electronics Co., LTD.\n");
	ret = platform_driver_register(&s5p_tvout_driver);
	if (ret) {
		printk(KERN_ERR "Platform Device Register Failed %d\n", ret);
		return -1;
	}
	return 0;
}

static void __exit s5p_tvout_exit(void)
{
	platform_driver_unregister(&s5p_tvout_driver);
}
late_initcall(s5p_tvout_init);
module_exit(s5p_tvout_exit);

MODULE_AUTHOR("Jiun Yu <jiun.yu@xxxxxxxxxxx>");
MODULE_DESCRIPTION("Samsung S5P TVOUT driver");
MODULE_LICENSE("GPL");
