/* linux/arch/arm/plat-s3c/dev-usb.c
 *
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S5P series device definition for USB host and device
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/platform_device.h>
#include <mach/irqs.h>
#include <mach/map.h>
#include <plat/devs.h>
#include <plat/otg.h>
#include <plat/usb-phy.h>

/* USB Device (Gadget)*/
static struct resource s3c_usbgadget_resource[] = {
	[0] = {
		.start	= S5P_PA_HSDEVICE,
		.end	= S5P_PA_HSDEVICE + SZ_64K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_USB_HSOTG,
		.end	= IRQ_USB_HSOTG,
		.flags	= IORESOURCE_IRQ,
	}
};

struct platform_device s3c_device_usbgadget = {
	.name		= "s3c-usbgadget",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_usbgadget_resource),
	.resource	= s3c_usbgadget_resource,
};
EXPORT_SYMBOL(s3c_device_usbgadget);

void __init s5p_otg_set_platdata(struct s5p_otg_platdata *pd)
{
	struct s5p_otg_platdata *npd;

	npd = s3c_set_platdata(pd, sizeof(struct s5p_otg_platdata),
		&s3c_device_usbgadget);

	if (!npd->phy_init)
		npd->phy_init = s5p_usb_phy_init;
	if (!npd->phy_exit)
		npd->phy_exit = s5p_usb_phy_exit;
}
