/*
 * Copyright (c) 2010-2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * EXYNOS4 - CPU frequency scaling support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/cpufreq.h>
#include <linux/suspend.h>
#include <linux/reboot.h>

#include <mach/map.h>
#include <mach/regs-clock.h>
#include <mach/regs-mem.h>
#include <mach/cpufreq.h>

#include <plat/clock.h>
#include <plat/pm.h>

static bool exynos4_cpufreq_init_done;
static DEFINE_MUTEX(set_freq_lock);
static DEFINE_MUTEX(set_cpu_freq_lock);

static struct clk *cpu_clk;
static struct clk *moutcore;
static struct clk *mout_mpll;
static struct clk *mout_apll;

static struct regulator *arm_regulator;

static struct cpufreq_freqs freqs;

struct cpufreq_clkdiv {
	unsigned int clkdiv;
};

enum cpufreq_level_index {
	L0, L1, L2, L3, L4, CPUFREQ_LEVEL_END,
};

static struct cpufreq_clkdiv exynos4_clkdiv_table[CPUFREQ_LEVEL_END];

static struct cpufreq_frequency_table exynos4_freq_table[] = {
	{L0, 1200*1000},
	{L1, 1000*1000},
	{L2, 800*1000},
	{L3, 500*1000},
	{L4, 200*1000},
	{0, CPUFREQ_TABLE_END},
};

/* This defines are for cpufreq lock */
#define CPUFREQ_MIN_LEVEL	(CPUFREQ_LEVEL_END - 1)
unsigned int cpufreq_lock_id;
unsigned int cpufreq_lock_val[DVFS_LOCK_ID_END];
unsigned int cpufreq_lock_level = CPUFREQ_MIN_LEVEL;

static unsigned int clkdiv_cpu0[CPUFREQ_LEVEL_END][7] = {
	/*
	 * Clock divider value for following
	 * { DIVCORE, DIVCOREM0, DIVCOREM1, DIVPERIPH,
	 *		DIVATB, DIVPCLK_DBG, DIVAPLL }
	 */

	/* ARM L0: 1200MHz */
	{ 0, 3, 7, 3, 4, 1, 7 },

	/* ARM L1: 1000MHz */
	{ 0, 3, 7, 3, 4, 1, 7 },

	/* ARM L2: 800MHz */
	{ 0, 3, 7, 3, 3, 1, 7 },

	/* ARM L3: 500MHz */
	{ 0, 3, 7, 3, 3, 1, 7 },

	/* ARM L4: 200MHz */
	{ 0, 1, 3, 1, 3, 1, 0 },
};

static unsigned int clkdiv_cpu1[CPUFREQ_LEVEL_END][2] = {
	/*
	 * Clock divider value for following
	 * { DIVCOPY, DIVHPM }
	 */

	 /* ARM L0: 1200MHz */
	{ 5, 0 },

	/* ARM L1: 1000MHz */
	{ 4, 0 },

	/* ARM L2: 800MHz */
	{ 3, 0 },

	/* ARM L3: 500MHz */
	{ 3, 0 },

	/* ARM L4: 200MHz */
	{ 3, 0 },
};

struct cpufreq_voltage_table {
	unsigned int	index;		/* any */
	unsigned int	arm_volt;	/* uV */
};

static struct cpufreq_voltage_table exynos4_volt_table[CPUFREQ_LEVEL_END] = {
	{
		.index		= L0,
		.arm_volt	= 1450000,
	}, {
		.index		= L1,
		.arm_volt	= 1300000,
	}, {
		.index		= L2,
		.arm_volt	= 1200000,
	}, {
		.index		= L3,
		.arm_volt	= 1100000,
	}, {
		.index		= L4,
		.arm_volt	= 1100000,
	},
};

static unsigned int exynos4_apll_pms_table[CPUFREQ_LEVEL_END] = {
	/* APLL FOUT L0: 1200MHz */
	((150 << 16) | (3 << 8) | 1),

	/* APLL FOUT L1: 1000MHz */
	((250 << 16) | (6 << 8) | 1),

	/* APLL FOUT L2: 800MHz */
	((200 << 16) | (6 << 8) | 1),

	/* APLL FOUT L3: 500MHz */
	((250 << 16) | (6 << 8) | 2),

	/* APLL FOUT L4: 200MHz */
	((200 << 16) | (6 << 8) | 3),
};

int exynos4_verify_speed(struct cpufreq_policy *policy)
{
	return cpufreq_frequency_table_verify(policy, exynos4_freq_table);
}

unsigned int exynos4_getspeed(unsigned int cpu)
{
	return clk_get_rate(cpu_clk) / 1000;
}

void exynos4_set_clkdiv(unsigned int div_index)
{
	unsigned int tmp;

	/* Change Divider - CPU0 */

	tmp = exynos4_clkdiv_table[div_index].clkdiv;

	__raw_writel(tmp, S5P_CLKDIV_CPU);

	do {
		tmp = __raw_readl(S5P_CLKDIV_STATCPU);
	} while (tmp & 0x1111111);

	/* Change Divider - CPU1 */

	tmp = __raw_readl(S5P_CLKDIV_CPU1);

	tmp &= ~((0x7 << 4) | 0x7);

	tmp |= ((clkdiv_cpu1[div_index][0] << 4) |
		(clkdiv_cpu1[div_index][1] << 0));

	__raw_writel(tmp, S5P_CLKDIV_CPU1);

	do {
		tmp = __raw_readl(S5P_CLKDIV_STATCPU1);
	} while (tmp & 0x11);
}

static void exynos4_set_apll(unsigned int index)
{
	unsigned int tmp;

	/* 1. MUX_CORE_SEL = MPLL, ARMCLK uses MPLL for lock time */
	clk_set_parent(moutcore, mout_mpll);

	do {
		tmp = (__raw_readl(S5P_CLKMUX_STATCPU)
			>> S5P_CLKSRC_CPU_MUXCORE_SHIFT);
		tmp &= 0x7;
	} while (tmp != 0x2);

	/* 2. Set APLL Lock time */
	__raw_writel(S5P_APLL_LOCKTIME, S5P_APLL_LOCK);

	/* 3. Change PLL PMS values */
	tmp = __raw_readl(S5P_APLL_CON0);
	tmp &= ~((0x3ff << 16) | (0x3f << 8) | (0x7 << 0));
	tmp |= exynos4_apll_pms_table[index];
	__raw_writel(tmp, S5P_APLL_CON0);

	/* 4. wait_lock_time */
	do {
		tmp = __raw_readl(S5P_APLL_CON0);
	} while (!(tmp & (0x1 << S5P_APLLCON0_LOCKED_SHIFT)));

	/* 5. MUX_CORE_SEL = APLL */
	clk_set_parent(moutcore, mout_apll);

	do {
		tmp = __raw_readl(S5P_CLKMUX_STATCPU);
		tmp &= S5P_CLKMUX_STATCPU_MUXCORE_MASK;
	} while (tmp != (0x1 << S5P_CLKSRC_CPU_MUXCORE_SHIFT));
}

static void exynos4_set_frequency(unsigned int old_index, unsigned int new_index)
{
	unsigned int tmp;

	if (old_index > new_index) {
		/*
		 * L1/L3, L2/L4 Level change require
		 * to only change s divider value
		 */
		if (((old_index == L3) && (new_index == L1)) ||
				((old_index == L4) && (new_index == L2))) {
			/* 1. Change the system clock divider values */
			exynos4_set_clkdiv(new_index);

			/* 2. Change just s value in apll m,p,s value */
			tmp = __raw_readl(S5P_APLL_CON0);
			tmp &= ~(0x7 << 0);
			tmp |= (exynos4_apll_pms_table[new_index] & 0x7);
			__raw_writel(tmp, S5P_APLL_CON0);
		} else {
			/* Clock Configuration Procedure */
			/* 1. Change the system clock divider values */
			exynos4_set_clkdiv(new_index);
			/* 2. Change the apll m,p,s value */
			exynos4_set_apll(new_index);
		}
	} else if (old_index < new_index) {
		/*
		 * L1/L3, L2/L4 Level change require
		 * to only change s divider value
		 */
		if (((old_index == L1) && (new_index == L3)) ||
				((old_index == L2) && (new_index == L4))) {
			/* 1. Change just s value in apll m,p,s value */
			tmp = __raw_readl(S5P_APLL_CON0);
			tmp &= ~(0x7 << 0);
			tmp |= (exynos4_apll_pms_table[new_index] & 0x7);
			__raw_writel(tmp, S5P_APLL_CON0);

			/* 2. Change the system clock divider values */
			exynos4_set_clkdiv(new_index);
		} else {
			/* Clock Configuration Procedure */
			/* 1. Change the apll m,p,s value */
			exynos4_set_apll(new_index);
			/* 2. Change the system clock divider values */
			exynos4_set_clkdiv(new_index);
		}
	}
	
	printk("EXYNOS4: CPU SPEED CHANGED: CPU0 : %d / CPU1 : %d\n", exynos4_getspeed(0), exynos4_getspeed(1));
}

static int exynos4_target(struct cpufreq_policy *policy,
			  unsigned int target_freq,
			  unsigned int relation)
{
	unsigned int index, old_index;
	unsigned int arm_volt;
	int ret = 0;

	mutex_lock(&set_freq_lock);

	freqs.old = exynos4_getspeed(policy->cpu);

	if (cpufreq_frequency_table_target(policy, exynos4_freq_table,
					   freqs.old, relation, &old_index)) {
		ret = -EINVAL;
		goto out;
	}

	if (cpufreq_frequency_table_target(policy, exynos4_freq_table,
					   target_freq, relation, &index)) {
		ret = -EINVAL;
		goto out;
	}

	freqs.new = exynos4_freq_table[index].frequency;
	freqs.cpu = policy->cpu;

	if (freqs.new == freqs.old) {
		ret = -EINVAL;
		goto out;
	}

	/* get the voltage value */
	arm_volt = exynos4_volt_table[index].arm_volt;

	cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);

	/* control regulator */
	if (freqs.new > freqs.old)
		/* Voltage up */
		regulator_set_voltage(arm_regulator, arm_volt, arm_volt);

	/* Clock Configuration Procedure */
	exynos4_set_frequency(old_index, index);

	cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);

	/* control regulator */
	if (freqs.new < freqs.old)
		/* Voltage down */
		regulator_set_voltage(arm_regulator, arm_volt, arm_volt);

out:
	mutex_unlock(&set_freq_lock);

	return ret;
}

atomic_t exynos4_cpufreq_lock_count;

int exynos4_cpufreq_lock(unsigned int id,
		enum cpufreq_level_request cpufreq_level)
{
	int i, old_idx = 0;
	unsigned int freq_old, freq_new, arm_volt;

	if (!exynos4_cpufreq_init_done)
		return 0;

	if (cpufreq_lock_id & (1 << id)) {
		printk(KERN_ERR "%s:Device [%d] already locked cpufreq\n",
				__func__,  id);
		return 0;
	}
	mutex_lock(&set_cpu_freq_lock);
	cpufreq_lock_id |= (1 << id);
	cpufreq_lock_val[id] = cpufreq_level;

	/* If the requested cpufreq is higher than current min frequency */
	if (cpufreq_level < cpufreq_lock_level)
		cpufreq_lock_level = cpufreq_level;

	mutex_unlock(&set_cpu_freq_lock);

	/*
	 * If current frequency is lower than requested freq,
	 * it needs to update
	 */
	mutex_lock(&set_freq_lock);
	freq_old = exynos4_getspeed(0);
	freq_new = exynos4_freq_table[cpufreq_level].frequency;
	if (freq_old < freq_new) {
		/* Find out current level index */
		for (i = 0 ; i < CPUFREQ_LEVEL_END ; i++) {
			if (freq_old == exynos4_freq_table[i].frequency) {
				old_idx = exynos4_freq_table[i].index;
				break;
			} else if (i == (CPUFREQ_LEVEL_END - 1)) {
				printk(KERN_ERR "%s: Level not found\n",
						__func__);
				mutex_unlock(&set_freq_lock);
				return -EINVAL;
			} else {
				continue;
			}
		}
		freqs.old = freq_old;
		freqs.new = freq_new;
		cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);

		/* get the voltage value */
		arm_volt = exynos4_volt_table[cpufreq_level].arm_volt;
		regulator_set_voltage(arm_regulator, arm_volt,
				arm_volt);

		exynos4_set_frequency(old_idx, cpufreq_level);
		cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
	}
	mutex_unlock(&set_freq_lock);

	return 0;
}
EXPORT_SYMBOL_GPL(exynos4_cpufreq_lock);

void exynos4_cpufreq_lock_free(unsigned int id)
{
	int i;

	if (!exynos4_cpufreq_init_done)
		return;

	mutex_lock(&set_cpu_freq_lock);
	cpufreq_lock_id &= ~(1 << id);
	cpufreq_lock_val[id] = CPUFREQ_MIN_LEVEL;
	cpufreq_lock_level = CPUFREQ_MIN_LEVEL;
	if (cpufreq_lock_id) {
		for (i = 0; i < DVFS_LOCK_ID_END; i++) {
			if (cpufreq_lock_val[i] < cpufreq_lock_level)
				cpufreq_lock_level = cpufreq_lock_val[i];
		}
	}
	mutex_unlock(&set_cpu_freq_lock);
}
EXPORT_SYMBOL_GPL(exynos4_cpufreq_lock_free);

#ifdef CONFIG_PM
static int exynos4_cpufreq_suspend(struct cpufreq_policy *policy)
{
	return 0;
}

static int exynos4_cpufreq_resume(struct cpufreq_policy *policy)
{
	return 0;
}
#endif

static int exynos4_cpufreq_notifier_event(struct notifier_block *this,
		unsigned long event, void *ptr)
{
	switch (event) {
	case PM_SUSPEND_PREPARE:
		if (exynos4_cpufreq_lock(DVFS_LOCK_ID_PM, CPU_L0))
			return NOTIFY_BAD;
		pr_debug("PM_SUSPEND_PREPARE for CPUFREQ\n");
		return NOTIFY_OK;
	case PM_POST_RESTORE:
	case PM_POST_SUSPEND:
		pr_debug("PM_POST_SUSPEND for CPUFREQ\n");
		exynos4_cpufreq_lock_free(DVFS_LOCK_ID_PM);
		return NOTIFY_OK;
	}
	return NOTIFY_DONE;
}

static struct notifier_block exynos4_cpufreq_notifier = {
	.notifier_call = exynos4_cpufreq_notifier_event,
};

static int exynos4_cpufreq_cpu_init(struct cpufreq_policy *policy)
{
	policy->cur = policy->min = policy->max = exynos4_getspeed(policy->cpu);

	cpufreq_frequency_table_get_attr(exynos4_freq_table, policy->cpu);

	/* set the transition latency value */
	policy->cpuinfo.transition_latency = 100000;

	/*
	 * EXYNOS4 multi-core processors has 2 cores
	 * that the frequency cannot be set independently.
	 * Each cpu is bound to the same speed.
	 * So the affected cpu is all of the cpus.
	 */
	if (!cpu_online(1)) {
		cpumask_copy(policy->related_cpus, cpu_possible_mask);
		cpumask_copy(policy->cpus, cpu_online_mask);
	} else {
		cpumask_setall(policy->cpus);
	}

	return cpufreq_frequency_table_cpuinfo(policy, exynos4_freq_table);
}

static int exynos4_cpufreq_reboot_notifier_call(struct notifier_block *this,
		unsigned long code, void *_cmd)
{
	if (exynos4_cpufreq_lock(DVFS_LOCK_ID_PM, CPU_L0))
		return NOTIFY_BAD;

	printk(KERN_INFO "REBOOT Notifier for CPUFREQ\n");
	return NOTIFY_DONE;
}

static struct notifier_block exynos4_cpufreq_reboot_notifier = {
	.notifier_call = exynos4_cpufreq_reboot_notifier_call,
};

static struct cpufreq_driver exynos4_driver = {
	.flags		= CPUFREQ_STICKY,
	.verify		= exynos4_verify_speed,
	.target		= exynos4_target,
	.get		= exynos4_getspeed,
	.init		= exynos4_cpufreq_cpu_init,
	.name		= "exynos4_cpufreq",
#ifdef CONFIG_PM
	.suspend	= exynos4_cpufreq_suspend,
	.resume		= exynos4_cpufreq_resume,
#endif
};

static int __init exynos4_cpufreq_init(void)
{
	int i;
	unsigned int tmp;

	cpu_clk = clk_get(NULL, "armclk");
	if (IS_ERR(cpu_clk))
		return PTR_ERR(cpu_clk);

	moutcore = clk_get(NULL, "moutcore");
	if (IS_ERR(moutcore))
		goto err_moutcore;

	mout_mpll = clk_get(NULL, "mout_mpll");
	if (IS_ERR(mout_mpll))
		goto err_mout_mpll;

	mout_apll = clk_get(NULL, "mout_apll");
	if (IS_ERR(mout_apll))
		goto err_mout_apll;

	arm_regulator = regulator_get(NULL, "vdd_arm");
	if (IS_ERR(arm_regulator)) {
		printk(KERN_ERR "failed to get resource %s\n", "vdd_arm");
		goto err_vdd_arm;
	}

	register_pm_notifier(&exynos4_cpufreq_notifier);
	register_reboot_notifier(&exynos4_cpufreq_reboot_notifier);

	exynos4_cpufreq_init_done = true;

	tmp = __raw_readl(S5P_CLKDIV_CPU);

	for (i = L0; i <  CPUFREQ_LEVEL_END; i++) {
		tmp &= ~(S5P_CLKDIV_CPU0_CORE_MASK |
			 S5P_CLKDIV_CPU0_COREM0_MASK |
			 S5P_CLKDIV_CPU0_COREM1_MASK |
			 S5P_CLKDIV_CPU0_PERIPH_MASK |
			 S5P_CLKDIV_CPU0_ATB_MASK |
			 S5P_CLKDIV_CPU0_PCLKDBG_MASK |
			 S5P_CLKDIV_CPU0_APLL_MASK);

		tmp |= ((clkdiv_cpu0[i][0] << S5P_CLKDIV_CPU0_CORE_SHIFT) |
			(clkdiv_cpu0[i][1] << S5P_CLKDIV_CPU0_COREM0_SHIFT) |
			(clkdiv_cpu0[i][2] << S5P_CLKDIV_CPU0_COREM1_SHIFT) |
			(clkdiv_cpu0[i][3] << S5P_CLKDIV_CPU0_PERIPH_SHIFT) |
			(clkdiv_cpu0[i][4] << S5P_CLKDIV_CPU0_ATB_SHIFT) |
			(clkdiv_cpu0[i][5] << S5P_CLKDIV_CPU0_PCLKDBG_SHIFT) |
			(clkdiv_cpu0[i][6] << S5P_CLKDIV_CPU0_APLL_SHIFT));

		exynos4_clkdiv_table[i].clkdiv = tmp;
	}

	if (cpufreq_register_driver(&exynos4_driver)) {
		pr_err("failed to register cpufreq driver\n");
		goto err_cpufreq;
	}

	return 0;
err_cpufreq:
	unregister_reboot_notifier(&exynos4_cpufreq_reboot_notifier);
	unregister_pm_notifier(&exynos4_cpufreq_notifier);

	if (!IS_ERR(arm_regulator))
		regulator_put(arm_regulator);

err_vdd_arm:
	if (!IS_ERR(mout_apll))
		clk_put(mout_apll);

err_mout_apll:
	if (!IS_ERR(mout_mpll))
		clk_put(mout_mpll);

err_mout_mpll:
	if (!IS_ERR(moutcore))
		clk_put(moutcore);

err_moutcore:
	if (!IS_ERR(cpu_clk))
		clk_put(cpu_clk);

	pr_debug("%s: failed initialization\n", __func__);

	return -EINVAL;
}
late_initcall(exynos4_cpufreq_init);
