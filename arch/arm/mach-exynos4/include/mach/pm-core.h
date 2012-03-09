/* linux/arch/arm/mach-exynos4/include/mach/pm-core.h
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Based on arch/arm/mach-s3c2410/include/mach/pm-core.h,
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * EXYNOS4210 - PM core support for arch/arm/plat-s5p/pm.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_PM_CORE_H
#define __ASM_ARCH_PM_CORE_H __FILE__

#include <mach/regs-pmu.h>
#include <mach/regs-gpio.h>

static inline void s3c_pm_debug_init_uart(void)
{
	/* nothing here yet */
}

static inline void s3c_pm_arch_prepare_irqs(void)
{
#if 0
	unsigned int tmp;
	tmp = __raw_readl(S5P_WAKEUP_MASK);
	tmp &= ~(1 << 31);
	__raw_writel(tmp, S5P_WAKEUP_MASK);

	__raw_writel(s3c_irqwake_intmask, S5P_WAKEUP_MASK);
	__raw_writel(s3c_irqwake_eintmask, S5P_EINT_WAKEUP_MASK);
#endif

	/* Set the reseverd bits to 0 */
	s3c_irqwake_intmask &= ~(0xFFF << 20);
	
	__raw_writel(s3c_irqwake_eintmask, S5P_EINT_WAKEUP_MASK);
	__raw_writel(s3c_irqwake_intmask, S5P_WAKEUP_MASK);
}

static inline void s3c_pm_arch_stop_clocks(void)
{
	/* nothing here yet */
}

static inline void s3c_pm_arch_show_resume_irqs(void)
{
	pr_info("WAKEUP_STAT: 0x%x\n", __raw_readl(S5P_WAKEUP_STAT));
	pr_info("WAKUP_INT0_PEND: 0x%x\n", __raw_readl(S5P_EINT_PEND(0)));
	pr_info("WAKUP_INT1_PEND: 0x%x\n", __raw_readl(S5P_EINT_PEND(1)));
	pr_info("WAKUP_INT2_PEND: 0x%x\n", __raw_readl(S5P_EINT_PEND(2)));
	pr_info("WAKUP_INT3_PEND: 0x%x\n", __raw_readl(S5P_EINT_PEND(3)));
}

static inline void s3c_pm_arch_update_uart(void __iomem *regs,
					   struct pm_uart_save *save)
{
	/* nothing here yet */
}

extern void (*s3c_config_sleep_gpio_table)(void);

#endif /* __ASM_ARCH_PM_CORE_H */
