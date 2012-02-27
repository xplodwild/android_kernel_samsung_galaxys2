/*
 * Copyright (C) 2011 Samsung Electronics Co.Ltd
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __PLAT_SAMSUNG_EHCI_H
#define __PLAT_SAMSUNG_EHCI_H __FILE__

struct s5p_ehci_platdata {
	int (*phy_init)(struct platform_device *pdev, int type);
	int (*phy_exit)(struct platform_device *pdev, int type);
	int (*burst_enable)(struct platform_device *pdev, void __iomem *base);
};

extern void s5p_ehci_set_platdata(struct s5p_ehci_platdata *pd);
extern int s5p_ehci_burst_enable(struct platform_device *pdev,
	void __iomem *base);

/* EXYNOS */
#define EHCI_INSNREG00				0x90
#define EHCI_INSNREG00_ENABLE_INCR16		(1 << 25)
#define EHCI_INSNREG00_ENABLE_INCR8		(1 << 24)
#define EHCI_INSNREG00_ENABLE_INCR4		(1 << 23)
#define EHCI_INSNREG00_ENABLE_INCRX_ALIGN	(1 << 22)

#endif /* __PLAT_SAMSUNG_EHCI_H */
