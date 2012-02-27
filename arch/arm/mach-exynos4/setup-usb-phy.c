/*
 * Copyright (C) 2011 Samsung Electronics Co.Ltd
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/usb/ch9.h>
#include <mach/regs-pmu.h>
#include <mach/regs-usb-phy.h>
#include <plat/cpu.h>
#include <plat/usb-phy.h>
#include <plat/regs-otg.h>

static int host_phy_initialized;

int s5p_usb_phy_init(struct platform_device *pdev, int type)
{
	struct clk *otg_clk;
	struct clk *xusbxti_clk;
	u32 phyclk;
	u32 rstcon;
	int err;
/*
	if(type == S5P_USB_PHY_HOST && host_phy_initialized != 0) {
		host_phy_initialized++;
		return 0;
	}
*/
	otg_clk = clk_get(&pdev->dev, "otg");
	if (IS_ERR(otg_clk)) {
		dev_err(&pdev->dev, "Failed to get otg clock\n");
		return PTR_ERR(otg_clk);
	}

	err = clk_enable(otg_clk);
	if (err) {
		clk_put(otg_clk);
		return err;
	}

	/* set clock frequency for PLL */
	phyclk = readl(EXYNOS4_PHYCLK) & ~CLKSEL_MASK;

	xusbxti_clk = clk_get(&pdev->dev, "xusbxti");
	if (xusbxti_clk && !IS_ERR(xusbxti_clk)) {
		switch (clk_get_rate(xusbxti_clk)) {
		case 12 * MHZ:
			phyclk |= CLKSEL_12M;
			break;
		case 24 * MHZ:
			phyclk |= CLKSEL_24M;
			break;
		default:
		case 48 * MHZ:
			/* default reference clock */
			break;
		}
		clk_put(xusbxti_clk);
	}

	writel(phyclk, EXYNOS4_PHYCLK);

	if (type == S5P_USB_PHY_HOST) {
		writel(readl(S5P_USBHOST_PHY_CONTROL) | S5P_USBHOST_PHY_ENABLE,
			S5P_USBHOST_PHY_CONTROL);

		writel((readl(EXYNOS4_PHY1CON) | FPENABLEN), EXYNOS4_PHY1CON);

		/* set to normal HSIC 0 and 1 of PHY1 */
		writel((readl(EXYNOS4_PHYPWR) & ~PHY1_HSIC_NORMAL_MASK),
			EXYNOS4_PHYPWR);

		/* set to normal standard USB of PHY1 */
		writel((readl(EXYNOS4_PHYPWR) & ~PHY1_STD_NORMAL_MASK),
			EXYNOS4_PHYPWR);

		/* reset all ports of both PHY and Link */
		rstcon = readl(EXYNOS4_RSTCON) | HOST_LINK_PORT_SWRST_MASK |
			PHY1_SWRST_MASK;

		writel(rstcon, EXYNOS4_RSTCON);
		udelay(10);

		rstcon &= ~(HOST_LINK_PORT_SWRST_MASK | PHY1_SWRST_MASK);
		writel(rstcon, EXYNOS4_RSTCON);
//		host_phy_initialized++;
	} else if (type == S5P_USB_PHY_DEVICE) {
		writel(readl(S5P_USBDEVICE_PHY_CONTROL) | (0x1<<0),
				S5P_USBDEVICE_PHY_CONTROL);
		writel((readl(EXYNOS4_PHYPWR) & ~(0x7<<3)&~(0x1<<0)),
				EXYNOS4_PHYPWR);
		writel((readl(EXYNOS4_PHYCLK) & ~(0x5<<2))|(0x3<<0),
				EXYNOS4_PHYCLK);
		writel((readl(EXYNOS4_RSTCON) & ~(0x3<<1))|(0x1<<0),
				EXYNOS4_RSTCON);
		udelay(10);
		writel(readl(EXYNOS4_RSTCON) & ~(0x7<<0),
				EXYNOS4_RSTCON);
	}
	udelay(80);

	clk_disable(otg_clk);
	clk_put(otg_clk);

	return 0;
}

int s5p_usb_phy_exit(struct platform_device *pdev, int type)
{
	struct clk *otg_clk;
	int err;

	otg_clk = clk_get(&pdev->dev, "otg");
	if (IS_ERR(otg_clk)) {
		dev_err(&pdev->dev, "Failed to get otg clock\n");
		return PTR_ERR(otg_clk);
	}

	err = clk_enable(otg_clk);
	if (err) {
		clk_put(otg_clk);
		return err;
	}

	if (type == S5P_USB_PHY_HOST) {
/*		host_phy_initialized--;
		if(host_phy_initialized !=0)
			goto end;
*/
		writel((readl(EXYNOS4_PHYPWR) | PHY1_STD_ANALOG_POWERDOWN),
				EXYNOS4_PHYPWR);

		writel(readl(S5P_USBHOST_PHY_CONTROL) & ~S5P_USBHOST_PHY_ENABLE,
				S5P_USBHOST_PHY_CONTROL);

	} else if (type == S5P_USB_PHY_DEVICE) {
		writel(readl(EXYNOS4_PHYPWR) | (0x3<<3),
				EXYNOS4_PHYPWR);

		writel(readl(S5P_USBDEVICE_PHY_CONTROL) & ~(1<<0),
				S5P_USBDEVICE_PHY_CONTROL);
	}
end:
	clk_disable(otg_clk);
	clk_put(otg_clk);
	return 0;
}

/* USB Control request data struct must be located here for DMA transfer */
struct usb_ctrlrequest usb_ctrl __attribute__((aligned(8)));
EXPORT_SYMBOL(usb_ctrl);
