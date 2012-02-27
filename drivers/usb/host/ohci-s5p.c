/* ohci-s5pv210.c - Driver for USB HOST on Samsung S5PV210 processor
 *
 * Bus Glue for SAMSUNG S5PV210 USB HOST OHCI Controller
 *
 * (C) Copyright 1999 Roman Weissgaerber <weissg@vienna.at>
 * (C) Copyright 2000-2002 David Brownell <dbrownell@users.sourceforge.net>
 * (C) Copyright 2002 Hewlett-Packard Company
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * Author: Jingoo Han <jg1.han@samsung.com>
 *
 * Based on "ohci-au1xxx.c" by Matt Porter <mporter@kernel.crashing.org>
 * Modified for SAMSUNG s5pv210 OHCI by Jingoo Han <jg1.han@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <plat/ohci.h>
#include <plat/usb-phy.h>

#define INSNREG00 0x12580090
#define APP_START_CLK (1<<21)

struct s5p_ohci_hcd {
        struct device *dev;
        struct usb_hcd *hcd;
        struct clk *clk;
};

static void ohci_s5pv210_set_clock(int start_clk);
#ifdef CONFIG_PM
static int ohci_hcd_s5pv210_drv_suspend(
	struct platform_device *pdev,
	pm_message_t message
){
	struct s5p_ohci_platdata *pdata = pdev->dev.platform_data;
	struct s5p_ohci_hcd *s5p_ohci = platform_get_drvdata(pdev);
        struct usb_hcd *hcd = s5p_ohci->hcd;
	struct ohci_hcd	*ohci = hcd_to_ohci(hcd);
	unsigned long flags;
	int rc = 0;

	
	if (!pdata) {
		dev_err(&pdev->dev, "No platform data defined\n");
		return -EINVAL;
	}

	/* Root hub was already suspended. Disable irq emission and
	 * mark HW unaccessible, bail out if RH has been resumed. Use
	 * the spinlock to properly synchronize with possible pending
	 * RH suspend or resume activity.
	 *
	 * This is still racy as hcd->state is manipulated outside of
	 * any locks =P But that will be a different fix.
	 */
	spin_lock_irqsave(&ohci->lock, flags);
	if (hcd->state != HC_STATE_SUSPENDED) {
		rc = -EINVAL;
		goto bail;
	}

	/* make sure snapshot being resumed re-enumerates everything */
	if (message.event == PM_EVENT_PRETHAW)
		ohci_usb_reset(ohci);

	clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	pdata->phy_exit(pdev, S5P_USB_PHY_HOST);
	clk_disable(s5p_ohci->clk);
bail:
	spin_unlock_irqrestore(&ohci->lock, flags);

	return rc;
}
static int ohci_hcd_s5pv210_drv_resume(struct platform_device *pdev)
{
	struct s5p_ohci_platdata *pdata = pdev->dev.platform_data;
        struct s5p_ohci_hcd *s5p_ohci = platform_get_drvdata(pdev);
        struct usb_hcd *hcd = s5p_ohci->hcd;
	int rc = 0;

	
	if (!pdata) {
		dev_err(&pdev->dev, "No platform data defined\n");
		return -EINVAL;
	}

	rc = clk_enable(s5p_ohci->clk);
	if(rc)
	  return rc;	
	if (pdata->phy_init)
		pdata->phy_init(pdev, S5P_USB_PHY_HOST);

	/* Mark hardware accessible again as we are out of D3 state by now */
	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	ohci_finish_controller_resume(hcd);

	return rc;
}
static int ohci_s5pv210_bus_suspend(struct usb_hcd *hcd)
{
	int ret;
	ohci_s5pv210_set_clock(1);
	ret = ohci_bus_suspend(hcd);
	ohci_s5pv210_set_clock(0);
	return ret;
}
static int ohci_s5pv210_bus_resume(struct usb_hcd *hcd)
{
	int ret;
	ohci_s5pv210_set_clock(1);
	ret = ohci_bus_resume(hcd);
	ohci_s5pv210_set_clock(0);
	return ret;
}
#else
#define ohci_hcd_s5pv210_drv_suspend NULL
#define ohci_hcd_s5pv210_drv_resume NULL
#endif
static int ohci_s5pv210_init(struct usb_hcd *hcd)
{
	dev_dbg(hcd->self.controller, "starting OHCI controller\n");

	return ohci_init(hcd_to_ohci(hcd));
}
static int __devinit ohci_s5pv210_start(struct usb_hcd *hcd)
{
	struct ohci_hcd	*ohci = hcd_to_ohci(hcd);
	int ret;

	ohci_dbg(ohci, "ohci_s5pv210_start, ohci:%p", ohci);

	ohci->hc_control = OHCI_CTRL_RWC;
	writel(OHCI_CTRL_RWC, &ohci->regs->control);

	ret = ohci_run(ohci);
	if (ret < 0) {
		err("can't start %s", hcd->self.bus_name);
		ohci_stop(hcd);
	}

	return ret;
}

static const struct hc_driver ohci_s5pv210_hc_driver = {
	.description		= hcd_name,
	.product_desc		= "Samsung OHCI Host Controller",
	.hcd_priv_size		= sizeof(struct ohci_hcd),

	.irq			= ohci_irq,
	.flags 			= HCD_USB11 | HCD_MEMORY,

	.reset			= ohci_s5pv210_init,
	.start			= ohci_s5pv210_start,
	.stop			= ohci_stop,
	.shutdown		= ohci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =		ohci_urb_enqueue,
	.urb_dequeue =		ohci_urb_dequeue,
	.endpoint_disable =	ohci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number =	ohci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data	= ohci_hub_status_data,
	.hub_control		= ohci_hub_control,
#ifdef	CONFIG_PM
	.bus_suspend		= ohci_s5pv210_bus_suspend,
	.bus_resume		= ohci_s5pv210_bus_resume,
#endif
	.start_port_reset	= ohci_start_port_reset,
};

static int __devinit ohci_hcd_s5pv210_drv_probe(struct platform_device *pdev)
{
	struct s5p_ohci_platdata *pdata;
	struct s5p_ohci_hcd *s5p_ohci;
	struct usb_hcd  *hcd = NULL;
	struct resource *res;
	int retval = 0;
	int irq;

	if (usb_disabled())
		return -ENODEV;

	pdata = pdev->dev.platform_data;
	if (!pdata) {
		dev_err(&pdev->dev, "No platform data defined\n");
		return -EINVAL;
	}
	
	s5p_ohci = kzalloc(sizeof(struct s5p_ohci_hcd), GFP_KERNEL);
	if(!s5p_ohci)
		return -ENOMEM;
	
	s5p_ohci->dev = &pdev->dev;		

	hcd = usb_create_hcd(&ohci_s5pv210_hc_driver, &pdev->dev, dev_name(&pdev->dev));
	if (!hcd) {
		dev_err(&pdev->dev, "usb_create_hcd failed!\n");
		retval= -ENOMEM;
		goto fail_hcd;
	}
	
	s5p_ohci->hcd = hcd;
	s5p_ohci->clk = clk_get(&pdev->dev, "usbhost");

        if (IS_ERR(s5p_ohci->clk)) {
                dev_err(&pdev->dev, "Failed to get usbhost clock\n");
                retval = PTR_ERR(s5p_ohci->clk);
                goto fail_clk;
        }

        retval = clk_enable(s5p_ohci->clk);
        if (retval)
                goto fail_clken;
	
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        if (!res) {
                dev_err(&pdev->dev, "Failed to get I/O memory\n");
                retval = -ENXIO;
                goto fail_io;
        }

	hcd->rsrc_start = res->start;
	hcd->rsrc_len = resource_size(res);
	hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
	if (!hcd->regs) {
		dev_err(&pdev->dev, "ioremap failed!\n");
		retval = -ENOMEM;
		goto fail_io;
	}

	irq = platform_get_irq(pdev, 0);
        if (!irq) {
                dev_err(&pdev->dev, "Failed to get IRQ\n");
                retval = -ENODEV;
                goto fail;
        }
  	
	if (pdata->phy_init)
		pdata->phy_init(pdev, S5P_USB_PHY_HOST);


	ohci_hcd_init(hcd_to_ohci(hcd));

	/* IRQF_SHARED: Since it is shared with s5p-ehci */
	retval = usb_add_hcd(hcd, irq, IRQF_DISABLED | IRQF_SHARED);

	if (retval) {
		dev_err(&pdev->dev, "Failed to add USB HCD\n");
                goto fail;
	}

	platform_set_drvdata(pdev, s5p_ohci);
	
	return 0;
fail:
	iounmap(hcd->regs);
	
fail_io:
	clk_disable(s5p_ohci->clk);
fail_clken:
        clk_put(s5p_ohci->clk);
fail_clk:
        usb_put_hcd(hcd);
fail_hcd:
	kfree(s5p_ohci);

	return retval;
}

static int __devexit ohci_hcd_s5pv210_drv_remove(struct platform_device *pdev)
{
	struct s5p_ohci_platdata *pdata = pdev->dev.platform_data;
	struct s5p_ohci_hcd *s5p_ohci = platform_get_drvdata(pdev);
	struct usb_hcd *hcd = s5p_ohci->hcd;
	
	iounmap(hcd->regs);
	usb_remove_hcd(hcd);
	
	if(pdata && pdata->phy_exit)
		pdata->phy_exit(pdev, S5P_USB_PHY_HOST);

	
	
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	clk_disable(s5p_ohci->clk);
        clk_put(s5p_ohci->clk);

	usb_put_hcd(hcd);
	kfree(s5p_ohci);
	platform_set_drvdata(pdev, NULL);

	return 0;
}
static void ohci_s5pv210_set_clock(int start_clk)
{
	void __iomem* insnreg;
        insnreg = ioremap(INSNREG00,4);

	if(start_clk) {
		__raw_writel(APP_START_CLK,insnreg);
	}
	else {
		__raw_writel(~APP_START_CLK,insnreg);
	}		
}
static void ohci_hcd_s5pv210_shutdown(struct platform_device *pdev)
{
        struct s5p_ohci_hcd *s5p_ohci = platform_get_drvdata(pdev);
        struct usb_hcd *hcd = s5p_ohci->hcd;
	ohci_s5pv210_set_clock(1);
        if (hcd->driver->shutdown)
                hcd->driver->shutdown(hcd);
	ohci_s5pv210_set_clock(0);
}
static struct platform_driver  ohci_hcd_s5pv210_driver = {
	.probe		= ohci_hcd_s5pv210_drv_probe,
	.remove		= __devexit_p(ohci_hcd_s5pv210_drv_remove),
	.shutdown	= ohci_hcd_s5pv210_shutdown,
	.suspend	= ohci_hcd_s5pv210_drv_suspend,
	.resume		= ohci_hcd_s5pv210_drv_resume,
	.driver = {
		.name = "s5p-ohci",
		.owner = THIS_MODULE,
	}
};

MODULE_ALIAS("platform:s5p-ohci");
