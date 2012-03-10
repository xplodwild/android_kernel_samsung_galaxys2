/* linux/arch/arm/mach-exynos4/mach-smdk4210.c
 *
 * Copyright (c) 2011 Insignal Co., Ltd.
 *		http://www.insignal.co.kr/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/serial_core.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <linux/mmc/host.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/input.h>
#include <linux/pwm_backlight.h>
#include <linux/gpio_keys.h>
#include <linux/i2c.h>
#include <linux/i2c/atmel_mxt_ts.h>
#include <linux/regulator/machine.h>
#include <linux/mfd/max8997.h>
#include <linux/lcd.h>
#include <linux/rfkill-gpio.h>
#include <linux/ath6kl.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>

#include <asm/mach/arch.h>
#include <asm/hardware/gic.h>
#include <asm/mach-types.h>

#include <video/platform_lcd.h>

#include <plat/regs-serial.h>
#include <plat/regs-fb-v4.h>
#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/sdhci.h>
#include <plat/iic.h>
#include <plat/ehci.h>
#include <plat/clock.h>
#include <plat/gpio-cfg.h>
#include <plat/backlight.h>
#include <plat/pd.h>
#include <plat/fb.h>
#include <plat/mfc.h>
#include <plat/udc-hs.h>
#include <plat/sysmmu.h>

#include <mach/ohci.h>
#include <mach/regs-clock.h>
#include <mach/map.h>

#include "common.h"

/* Following are default values for UCON, ULCON and UFCON UART registers */
#define SMDK4210_UCON_DEFAULT	(S3C2410_UCON_TXILEVEL |	\
				 S3C2410_UCON_RXILEVEL |	\
				 S3C2410_UCON_TXIRQMODE |	\
				 S3C2410_UCON_RXIRQMODE |	\
				 S3C2410_UCON_RXFIFO_TOI |	\
				 S3C2443_UCON_RXERR_IRQEN)

#define SMDK4210_ULCON_DEFAULT	S3C2410_LCON_CS8

#define SMDK4210_UFCON_DEFAULT	(S3C2410_UFCON_FIFOMODE |	\
				 S5PV210_UFCON_TXTRIG4 |	\
				 S5PV210_UFCON_RXTRIG4)

static struct s3c2410_uartcfg smdk4210_uartcfgs[] __initdata = {
	[0] = {
		.hwport		= 0,
		.flags		= 0,
		.ucon		= SMDK4210_UCON_DEFAULT,
		.ulcon		= SMDK4210_ULCON_DEFAULT,
		.ufcon		= SMDK4210_UFCON_DEFAULT,
	},
	[1] = {
		.hwport		= 1,
		.flags		= 0,
		.ucon		= SMDK4210_UCON_DEFAULT,
		.ulcon		= SMDK4210_ULCON_DEFAULT,
		.ufcon		= SMDK4210_UFCON_DEFAULT,
	},
	[2] = {
		.hwport		= 2,
		.flags		= 0,
		.ucon		= SMDK4210_UCON_DEFAULT,
		.ulcon		= SMDK4210_ULCON_DEFAULT,
		.ufcon		= SMDK4210_UFCON_DEFAULT,
	},
	[3] = {
		.hwport		= 3,
		.flags		= 0,
		.ucon		= SMDK4210_UCON_DEFAULT,
		.ulcon		= SMDK4210_ULCON_DEFAULT,
		.ufcon		= SMDK4210_UFCON_DEFAULT,
	},
};

static struct regulator_consumer_supply __initdata ldo3_consumer[] = {
	REGULATOR_SUPPLY("vdd11", "s5p-mipi-csis.0"), /* MIPI */
	REGULATOR_SUPPLY("vdd", "exynos4-hdmi"), /* HDMI */
	REGULATOR_SUPPLY("vdd_pll", "exynos4-hdmi"), /* HDMI */
};
static struct regulator_consumer_supply __initdata ldo6_consumer[] = {
	REGULATOR_SUPPLY("vdd18", "s5p-mipi-csis.0"), /* MIPI */
};
static struct regulator_consumer_supply __initdata ldo7_consumer[] = {
	REGULATOR_SUPPLY("avdd", "alc5625"), /* Realtek ALC5625 */
};
static struct regulator_consumer_supply __initdata ldo8_consumer[] = {
	REGULATOR_SUPPLY("vdd", "s5p-adc"), /* ADC */
	REGULATOR_SUPPLY("vdd_osc", "exynos4-hdmi"), /* HDMI */
};
static struct regulator_consumer_supply __initdata ldo9_consumer[] = {
	REGULATOR_SUPPLY("dvdd", "swb-a31"), /* AR6003 WLAN & CSR 8810 BT */
};
static struct regulator_consumer_supply __initdata ldo11_consumer[] = {
	REGULATOR_SUPPLY("dvdd", "alc5625"), /* Realtek ALC5625 */
};
static struct regulator_consumer_supply __initdata ldo14_consumer[] = {
	REGULATOR_SUPPLY("avdd18", "swb-a31"), /* AR6003 WLAN & CSR 8810 BT */
};
static struct regulator_consumer_supply __initdata ldo17_consumer[] = {
	REGULATOR_SUPPLY("vdd33", "swb-a31"), /* AR6003 WLAN & CSR 8810 BT */
};
static struct regulator_consumer_supply __initdata buck1_consumer[] = {
	REGULATOR_SUPPLY("vdd_arm", NULL), /* CPUFREQ */
};
static struct regulator_consumer_supply __initdata buck2_consumer[] = {
	REGULATOR_SUPPLY("vdd_int", NULL), /* CPUFREQ */
};
static struct regulator_consumer_supply __initdata buck3_consumer[] = {
	REGULATOR_SUPPLY("vdd_g3d", "mali_drm"), /* G3D */
};
static struct regulator_consumer_supply __initdata buck7_consumer[] = {
	REGULATOR_SUPPLY("vcc_lcd", "platform-lcd.0"), /* LCD */
};

static struct regulator_init_data __initdata max8997_ldo1_data = {
	.constraints	= {
		.name		= "VDD_ABB_3.3V",
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.apply_uV	= 1,
		.state_mem	= {
			.disabled	= 1,
		},
	},
};

static struct regulator_init_data __initdata max8997_ldo2_data	= {
	.constraints	= {
		.name		= "VDD_ALIVE_1.1V",
		.min_uV		= 1100000,
		.max_uV		= 1100000,
		.apply_uV	= 1,
		.always_on	= 1,
		.state_mem	= {
			.enabled	= 1,
		},
	},
};

static struct regulator_init_data __initdata max8997_ldo3_data = {
	.constraints	= {
		.name		= "VMIPI_1.1V",
		.min_uV		= 1100000,
		.max_uV		= 1100000,
		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo3_consumer),
	.consumer_supplies	= ldo3_consumer,
};

static struct regulator_init_data __initdata max8997_ldo4_data = {
	.constraints	= {
		.name		= "VDD_RTC_1.8V",
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.apply_uV	= 1,
		.always_on	= 1,
		.state_mem	= {
			.disabled	= 1,
		},
	},
};

static struct regulator_init_data __initdata max8997_ldo6_data = {
	.constraints	= {
		.name		= "VMIPI_1.8V",
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo6_consumer),
	.consumer_supplies	= ldo6_consumer,
};

static struct regulator_init_data __initdata max8997_ldo7_data = {
	.constraints	= {
		.name		= "VDD_AUD_1.8V",
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.apply_uV	= 1,
		.always_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo7_consumer),
	.consumer_supplies	= ldo7_consumer,
};

static struct regulator_init_data __initdata max8997_ldo8_data = {
	.constraints	= {
		.name		= "VADC_3.3V",
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo8_consumer),
	.consumer_supplies	= ldo8_consumer,
};

static struct regulator_init_data __initdata max8997_ldo9_data = {
	.constraints	= {
		.name		= "DVDD_SWB_2.8V",
		.min_uV		= 2800000,
		.max_uV		= 2800000,
		.apply_uV	= 1,
		.always_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo9_consumer),
	.consumer_supplies	= ldo9_consumer,
};

static struct regulator_init_data __initdata max8997_ldo10_data = {
	.constraints	= {
		.name		= "VDD_PLL_1.1V",
		.min_uV		= 1100000,
		.max_uV		= 1100000,
		.apply_uV	= 1,
		.always_on	= 1,
		.state_mem	= {
			.disabled	= 1,
		},
	},
};

static struct regulator_init_data __initdata max8997_ldo11_data = {
	.constraints	= {
		.name		= "VDD_AUD_3V",
		.min_uV		= 3000000,
		.max_uV		= 3000000,
		.apply_uV	= 1,
		.always_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo11_consumer),
	.consumer_supplies	= ldo11_consumer,
};

static struct regulator_init_data __initdata max8997_ldo14_data = {
	.constraints	= {
		.name		= "AVDD18_SWB_1.8V",
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.apply_uV	= 1,
		.always_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo14_consumer),
	.consumer_supplies	= ldo14_consumer,
};

static struct regulator_init_data __initdata max8997_ldo17_data = {
	.constraints	= {
		.name		= "VDD_SWB_3.3V",
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.apply_uV	= 1,
		.always_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo17_consumer),
	.consumer_supplies	= ldo17_consumer,
};

static struct regulator_init_data __initdata max8997_ldo21_data = {
	.constraints	= {
		.name		= "VDD_MIF_1.2V",
		.min_uV		= 1200000,
		.max_uV		= 1200000,
		.apply_uV	= 1,
		.always_on	= 1,
		.state_mem	= {
			.disabled	= 1,
		},
	},
};

static struct regulator_init_data __initdata max8997_buck1_data = {
	.constraints	= {
		.name		= "VDD_ARM_1.2V",
		.min_uV		= 950000,
		.max_uV		= 1350000,
		.always_on	= 1,
		.boot_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
		.state_mem	= {
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(buck1_consumer),
	.consumer_supplies	= buck1_consumer,
};

static struct regulator_init_data __initdata max8997_buck2_data = {
	.constraints	= {
		.name		= "VDD_INT_1.1V",
		.min_uV		= 900000,
		.max_uV		= 1100000,
		.always_on	= 1,
		.boot_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
		.state_mem	= {
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(buck2_consumer),
	.consumer_supplies	= buck2_consumer,
};

static struct regulator_init_data __initdata max8997_buck3_data = {
	.constraints	= {
		.name		= "VDD_G3D_1.1V",
		.min_uV		= 900000,
		.max_uV		= 1100000,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
					REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(buck3_consumer),
	.consumer_supplies	= buck3_consumer,
};

static struct regulator_init_data __initdata max8997_buck5_data = {
	.constraints	= {
		.name		= "VDDQ_M1M2_1.2V",
		.min_uV		= 1200000,
		.max_uV		= 1200000,
		.apply_uV	= 1,
		.always_on	= 1,
		.state_mem	= {
			.disabled	= 1,
		},
	},
};

static struct regulator_init_data __initdata max8997_buck7_data = {
	.constraints	= {
		.name		= "VDD_LCD_3.3V",
		.min_uV		= 750000,
		.max_uV		= 3900000,
		.boot_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_STATUS |
					REGULATOR_CHANGE_VOLTAGE,
		.state_mem	= {
			.disabled	= 1
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(buck7_consumer),
	.consumer_supplies	= buck7_consumer,
};

static struct max8997_regulator_data __initdata smdk4210_max8997_regulators[] = {
	{ MAX8997_LDO1,		&max8997_ldo1_data },
	{ MAX8997_LDO2,		&max8997_ldo2_data },
	{ MAX8997_LDO3,		&max8997_ldo3_data },
	{ MAX8997_LDO4,		&max8997_ldo4_data },
	{ MAX8997_LDO6,		&max8997_ldo6_data },
	{ MAX8997_LDO7,		&max8997_ldo7_data },
	{ MAX8997_LDO8,		&max8997_ldo8_data },
	{ MAX8997_LDO9,		&max8997_ldo9_data },
	{ MAX8997_LDO10,	&max8997_ldo10_data },
	{ MAX8997_LDO11,	&max8997_ldo11_data },
	{ MAX8997_LDO14,	&max8997_ldo14_data },
	{ MAX8997_LDO17,	&max8997_ldo17_data },
	{ MAX8997_LDO21,	&max8997_ldo21_data },
	{ MAX8997_BUCK1,	&max8997_buck1_data },
	{ MAX8997_BUCK2,	&max8997_buck2_data },
	{ MAX8997_BUCK3,	&max8997_buck3_data },
	{ MAX8997_BUCK5,	&max8997_buck5_data },
	{ MAX8997_BUCK7,	&max8997_buck7_data },
};

static struct max8997_platform_data __initdata smdk4210_max8997_pdata = {
	.num_regulators = ARRAY_SIZE(smdk4210_max8997_regulators),
	.regulators	= smdk4210_max8997_regulators,

	.wakeup	= true,
	.buck1_gpiodvs	= false,
	.buck2_gpiodvs	= false,
	.buck5_gpiodvs	= false,
	.irq_base	= IRQ_GPIO_END + 1,

	.ignore_gpiodvs_side_effect = true,
	.buck125_default_idx = 0x0,

	.buck125_gpios[0]	= EXYNOS4_GPX0(0),
	.buck125_gpios[1]	= EXYNOS4_GPX0(1),
	.buck125_gpios[2]	= EXYNOS4_GPX0(2),

	.buck1_voltage[0]	= 1350000,
	.buck1_voltage[1]	= 1300000,
	.buck1_voltage[2]	= 1250000,
	.buck1_voltage[3]	= 1200000,
	.buck1_voltage[4]	= 1150000,
	.buck1_voltage[5]	= 1100000,
	.buck1_voltage[6]	= 1000000,
	.buck1_voltage[7]	= 950000,

	.buck2_voltage[0]	= 1100000,
	.buck2_voltage[1]	= 1100000,
	.buck2_voltage[2]	= 1100000,
	.buck2_voltage[3]	= 1100000,
	.buck2_voltage[4]	= 1000000,
	.buck2_voltage[5]	= 1000000,
	.buck2_voltage[6]	= 1000000,
	.buck2_voltage[7]	= 1000000,

	.buck5_voltage[0]	= 1200000,
	.buck5_voltage[1]	= 1200000,
	.buck5_voltage[2]	= 1200000,
	.buck5_voltage[3]	= 1200000,
	.buck5_voltage[4]	= 1200000,
	.buck5_voltage[5]	= 1200000,
	.buck5_voltage[6]	= 1200000,
	.buck5_voltage[7]	= 1200000,
};

/* I2C0 */
static struct i2c_board_info i2c0_devs[] __initdata = {
	{
		I2C_BOARD_INFO("max8997", (0xCC >> 1)),
		.platform_data	= &smdk4210_max8997_pdata,
		.irq		= IRQ_EINT(4),
	},
#ifdef CONFIG_TOUCHSCREEN_UNIDISPLAY_TS
	{
		I2C_BOARD_INFO("unidisplay_ts", 0x41),
		.irq = IRQ_TS,
	},
#endif
};

/* I2C1 */
static struct i2c_board_info i2c1_devs[] __initdata = {
	{
		I2C_BOARD_INFO("alc5625", 0x1E),
	},
};

/******************************************************************************
 * sdhci
 ******************************************************************************/
static struct s3c_sdhci_platdata smdk4210_hsmmc0_pdata __initdata = {
	.max_width		= 8,
	.host_caps		= (MMC_CAP_8_BIT_DATA | MMC_CAP_4_BIT_DATA |
				MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED |
				MMC_CAP_DISABLE | MMC_CAP_ERASE),
	.cd_type		= S3C_SDHCI_CD_PERMANENT,
	//.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
	.cfg_gpio = exynos4_setup_sdhci0_cfg_gpio,
};

static struct s3c_sdhci_platdata smdk4210_hsmmc2_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_GPIO,
	//.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
	.ext_cd_gpio	= GPIO_HSMMC2_CD,
	.ext_cd_gpio_invert = 1,
	.host_caps = MMC_CAP_4_BIT_DATA |
				MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED |
				MMC_CAP_DISABLE,
	.max_width		= 4,
	.cfg_gpio = exynos4_setup_sdhci2_cfg_gpio,
};

static struct s3c_sdhci_platdata smdk4210_hsmmc3_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_PERMANENT,
	//.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
	.max_width		= 4,
	.host_caps		= MMC_CAP_4_BIT_DATA |
				MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED,
	.cfg_gpio = exynos4_setup_sdhci3_cfg_gpio,
};


/*
 * WLAN: SDIO Host will call this func at booting time
 */
static int smdk4210_wifi_status_register(void (*notify_func)
		(struct platform_device *, int state));

/* WLAN: MMC3-SDIO */
/*static struct s3c_sdhci_platdata smdk4210_hsmmc3_pdata __initdata = {
	.max_width		= 4,
	.host_caps		= MMC_CAP_4_BIT_DATA |
			MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED,
	.cd_type		= S3C_SDHCI_CD_EXTERNAL,
	.ext_cd_init		= smdk4210_wifi_status_register,
};*/

/*
 * WLAN: Save SDIO Card detect func into this pointer
 */
static void (*wifi_status_cb)(struct platform_device *, int state);

static int smdk4210_wifi_status_register(void (*notify_func)
		(struct platform_device *, int state))
{
	if (!notify_func)
		return -EAGAIN;
	else
		wifi_status_cb = notify_func;

	return 0;
}

#define smdk4210_WLAN_WOW EXYNOS4_GPX2(3)
#define smdk4210_WLAN_RESET EXYNOS4_GPX2(4)


static void smdk4210_wlan_setup_power(bool val)
{
	int err;

	if (val) {
		err = gpio_request_one(smdk4210_WLAN_RESET,
				GPIOF_OUT_INIT_LOW, "GPX2_4");
		if (err) {
			pr_warning("smdk4210: Not obtain WIFI gpios\n");
			return;
		}
		s3c_gpio_cfgpin(smdk4210_WLAN_RESET, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(smdk4210_WLAN_RESET,
						S3C_GPIO_PULL_NONE);
		/* VDD33,I/O Supply must be done */
		gpio_set_value(smdk4210_WLAN_RESET, 0);
		udelay(30);	/*Tb */
		gpio_direction_output(smdk4210_WLAN_RESET, 1);
	} else {
		gpio_direction_output(smdk4210_WLAN_RESET, 0);
		gpio_free(smdk4210_WLAN_RESET);
	}

	mdelay(100);

	return;
}

/*
 * This will be called at init time of WLAN driver
 */
static int smdk4210_wifi_set_detect(bool val)
{
	if (!wifi_status_cb) {
		printk(KERN_WARNING "WLAN: Nobody to notify\n");
		return -EAGAIN;
	}
	if (true == val) {
		smdk4210_wlan_setup_power(true);
		wifi_status_cb(&s3c_device_hsmmc3, 1);
	} else {
		smdk4210_wlan_setup_power(false);
		wifi_status_cb(&s3c_device_hsmmc3, 0);
	}

	return 0;
}

struct ath6kl_platform_data smdk4210_wlan_data  __initdata = {
	.setup_power = smdk4210_wifi_set_detect,
};


/* USB EHCI */
static struct s5p_ehci_platdata smdk4210_ehci_pdata;

static void __init smdk4210_ehci_init(void)
{
	struct s5p_ehci_platdata *pdata = &smdk4210_ehci_pdata;

	s5p_ehci_set_platdata(pdata);
}

/* USB OHCI */
static struct exynos4_ohci_platdata smdk4210_ohci_pdata;

static void __init smdk4210_ohci_init(void)
{
	struct exynos4_ohci_platdata *pdata = &smdk4210_ohci_pdata;

	exynos4_ohci_set_platdata(pdata);
}

/* USB OTG */
static struct s3c_hsotg_plat smdk4210_hsotg_pdata;

static struct gpio_led smdk4210_gpio_leds[] = {
	{
		.name			= "smdk4210::status1",
		.default_trigger	= "heartbeat",
		.gpio			= EXYNOS4_GPX1(3),
		.active_low		= 1,
	},
	{
		.name			= "smdk4210::status2",
		.default_trigger	= "mmc0",
		.gpio			= EXYNOS4_GPX1(4),
		.active_low		= 1,
	},
};

static struct gpio_led_platform_data smdk4210_gpio_led_info = {
	.leds		= smdk4210_gpio_leds,
	.num_leds	= ARRAY_SIZE(smdk4210_gpio_leds),
};

static struct platform_device smdk4210_leds_gpio = {
	.name	= "leds-gpio",
	.id	= -1,
	.dev	= {
		.platform_data	= &smdk4210_gpio_led_info,
	},
};

/******************************************************************************
 * gpio keys
 ******************************************************************************/
static struct gpio_keys_button smdk4210_gpio_keys[] = {
	{
		.code = KEY_VOLUMEUP,
		.gpio = GPIO_VOL_UP,
		.active_low = 1,
		.type = EV_KEY,
		.wakeup = 1,
		.can_disable = 1,
	}, {
		.code = KEY_VOLUMEDOWN,
		.gpio = GPIO_VOL_DOWN,
		.active_low = 1,
		.type = EV_KEY,
		.wakeup = 1,
		.can_disable = 1,
	}, {
		.code = KEY_POWER,
		.gpio = GPIO_nPOWER,
		.active_low = 1,
		.type = EV_KEY,
		.wakeup = 1,
	}, {
		.code = KEY_HOME,
		.gpio = GPIO_OK_KEY,
		.active_low = 1,
		.type = EV_KEY,
		.wakeup = 1,
	}
};

static struct gpio_keys_platform_data smdk4210_gpio_keys_data = {
	.buttons	= smdk4210_gpio_keys,
	.nbuttons	= ARRAY_SIZE(smdk4210_gpio_keys),
};

static struct platform_device smdk4210_device_gpio_keys = {
	.name		= "gpio-keys",
	.dev		= {
		.platform_data	= &smdk4210_gpio_keys_data,
	},
};

/******************************************************************************
 * gpio table
 ******************************************************************************/
typedef enum {
	SGS_GPIO_SETPIN_ZERO,
	SGS_GPIO_SETPIN_ONE,
	SGS_GPIO_SETPIN_NONE
} sgs_gpio_initval;

static struct {
	unsigned int num;
	unsigned int cfg;
	sgs_gpio_initval val;
	samsung_gpio_pull_t pull;
	s5p_gpio_drvstr_t drv;
} smdk4210_init_gpios[] = {
	{
		.num	= GPIO_FM_RST,
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
	 {
		.num	= GPIO_TDMB_RST_N,
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
	{
		.num	= EXYNOS4_GPC1(3),	/* CODEC_SDA_1.8V */
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_NONE,
		.pull	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPC1(4),	/* CODEC_SCL_1.8V */
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_NONE,
		.pull	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPD0(2),	/* MSENSOR_MHL_SDA_2.8V */
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_NONE,
		.pull	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPD0(3),	/* MSENSOR_MHL_SCL_2.8V */
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_NONE,
		.pull	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPD1(0),	/* 8M_CAM_SDA_2.8V */
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_NONE,
		.pull	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPD1(1),	/* 8M_CAM_SCL_2.8V */
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_NONE,
		.pull	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPD1(2),	/* SENSE_SDA_2.8V */
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_NONE,
		.pull	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPD1(3),	/* SENSE_SCL_2.8V */
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_NONE,
		.pull	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
	{
		.num	= EXYNOS4_GPK1(1),
		.cfg	= S3C_GPIO_OUTPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV2,
	}, {
		.num	= EXYNOS4_GPK2(2),	/* PS_ALS_SDA_2.8V */
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_NONE,
		.pull	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
	{
		.num	= EXYNOS4_GPK3(1),	/* WLAN_SDIO_CMD */
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_NONE,
		.pull	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPK3(2),	/* PS_ALS_SCL_2.8V */
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_NONE,
		.pull	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPK3(3),	/* WLAN_SDIO_D(0) */
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_NONE,
		.pull	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPK3(4),	/* WLAN_SDIO_D(1) */
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_NONE,
		.pull	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPK3(5),	/* WLAN_SDIO_D(2) */
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_NONE,
		.pull	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPK3(6),	/* WLAN_SDIO_D(3) */
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_NONE,
		.pull	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPX0(1),	/* VOL_UP */
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_NONE,
		.pull	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
	{
		.num	= EXYNOS4_GPX0(2),	/* VOL_DOWN */
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_NONE,
		.pull	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPX0(3),	/* GPIO_BOOT_MODE */
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_NONE,
		.pull	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPX2(3),	/* GPIO_FUEL_ALERT */
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_NONE,
		.pull	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPX3(1),
		.cfg	= S3C_GPIO_OUTPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPX3(2),	/* GPIO_DET_35 */
		.cfg	= S3C_GPIO_SFN(GPIO_DET_35_AF),
		.val	= SGS_GPIO_SETPIN_NONE,
		.pull	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
	{
		.num	= EXYNOS4_GPX3(3),
		.cfg	= S3C_GPIO_OUTPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
	{
		.num	= EXYNOS4_GPX3(4),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_NONE,
		.pull	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
	{	/*GPY0 */
		.num	= EXYNOS4_GPY0(2),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY0(3),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY0(4),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY0(5),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
	{	/*GPY1 */
		.num	= EXYNOS4_GPY1(0),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY1(1),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
	{
		.num	= EXYNOS4_GPY1(2),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY1(3),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {	/*GPY2 */
		.num	= EXYNOS4_GPY2(0),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY2(1),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY2(2),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY2(3),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY2(4),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY2(5),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {	/*GPY4 */
		.num	= EXYNOS4_GPY4(4),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {	/*GPY5 */
		.num	= EXYNOS4_GPY5(0),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY5(1),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY5(2),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY5(3),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY5(4),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY5(5),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY5(6),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY5(7),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {	/* GPY6 */
		.num	= EXYNOS4_GPY6(0),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY6(1),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY6(2),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY6(3),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY6(4),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY6(5),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY6(6),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY6(7),
		.cfg	= S3C_GPIO_INPUT,
		.val	= SGS_GPIO_SETPIN_ZERO,
		.pull	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
};

static void smdk4210_config_gpio_table(void)
{
	u32 i, gpio;

	for (i = 0; i < ARRAY_SIZE(smdk4210_init_gpios); i++) {
		gpio = smdk4210_init_gpios[i].num;
		s3c_gpio_cfgpin(gpio, smdk4210_init_gpios[i].cfg);
		s3c_gpio_setpull(gpio, smdk4210_init_gpios[i].pull);

		if (smdk4210_init_gpios[i].val != SGS_GPIO_SETPIN_NONE)
			gpio_set_value(gpio, smdk4210_init_gpios[i].val);

		s5p_gpio_set_drvstr(gpio, smdk4210_init_gpios[i].drv);
	}
}


/******************************************************************************
 * framebuffer
 *******************************************************************************/
static void smdk4210_fimd0_gpio_setup(void)
{
	unsigned int reg;

	s3c_gpio_cfgrange_nopull(EXYNOS4_GPF0(0), 8, S3C_GPIO_SFN(2));
	s3c_gpio_cfgrange_nopull(EXYNOS4_GPF1(0), 8, S3C_GPIO_SFN(2));
	s3c_gpio_cfgrange_nopull(EXYNOS4_GPF2(0), 8, S3C_GPIO_SFN(2));
	s3c_gpio_cfgrange_nopull(EXYNOS4_GPF3(0), 4, S3C_GPIO_SFN(2));

	/*
	 * Set DISPLAY_CONTROL register for Display path selection.
	 *
	 * DISPLAY_CONTROL[1:0]
	 * ---------------------
	 *  00 | MIE
	 *  01 | MDNIE
	 *  10 | FIMD
	 *  11 | FIMD
	 *  TODO: fix s3c-fb driver to disable MDNIE
	 */
	reg = __raw_readl(S3C_VA_SYS + 0x0210);
	reg &= (1 << 1);
	reg |= 3;
	reg &= ~(1 << 13);
	reg &= ~(3 << 10);
	reg &= ~(1 << 12);
	__raw_writel(reg, S3C_VA_SYS + 0x0210);
}

 static struct s3c_fb_pd_win smdk4210_fb_win0 = {
	.win_mode = {
		.left_margin	= 16,
		.right_margin	= 14,
		.upper_margin	= 4,
		.lower_margin	= 10,
		.hsync_len	= 2,
		.vsync_len	= 2,
		.xres		= 480,
		.yres		= 800,
		.refresh	= 55,
	},
	.max_bpp	= 24,
	.default_bpp = 16,
	.virtual_x	= 480,
	.virtual_y	= 800,
};

static struct s3c_fb_platdata smdk4210_fb_pdata __initdata = {
	.win[0]		= &smdk4210_fb_win0,
	.vidcon0	= VIDCON0_VIDOUT_RGB | VIDCON0_PNRMODE_RGB |
		VIDCON0_CLKSEL_LCD,
	.vidcon1	= VIDCON1_INV_HSYNC | VIDCON1_INV_VSYNC |
		VIDCON1_INV_VDEN | VIDCON1_INV_VCLK,
	.setup_gpio	= smdk4210_fimd0_gpio_setup,
};

static int ld9040_reset(struct lcd_device *ld) {
	gpio_direction_output(GPIO_LCD_RESET, 0);
	mdelay(10);
	gpio_direction_output(GPIO_LCD_RESET, 1);
	mdelay(10);

	return 0;
}

static int ld9040_power(struct lcd_device *ld, int enable) {
	struct regulator *regulator;
	int rc = 0;

	if (ld == NULL) {
		printk(KERN_ERR "lcd device object is NULL.\n");
		rc = -EINVAL;
		goto fail;
	}

	regulator = regulator_get(NULL, "vlcd_3.0v");
	if (IS_ERR(regulator)) {
		rc = -ENODEV;
		goto fail;
	}
	
	if (enable) {
		regulator_enable(regulator);
	} else {
		regulator_disable(regulator);
	}
	
	regulator_put(regulator);
fail:
	return rc;
}

static struct lcd_platform_data ld9040_platform_data = {
	.power_on	= ld9040_power,
	.reset	= ld9040_reset,

	.lcd_enabled	= 0,
	.reset_delay	= 20,
	.power_on_delay	= 50,
	.power_off_delay	= 300,
};

static struct spi_gpio_platform_data lcd_spi_gpio_pdata = {
	.sck	= GPIO_LCD_SPI_SCK,
	.mosi	= GPIO_LCD_SPI_MOSI,
	.miso	= SPI_GPIO_NO_MISO,
	.num_chipselect	= 1,
};

static struct platform_device lcd_spi_gpio = {
	.name	= "spi_gpio",
	.id = 3, ///SDMK4210_SPI_LCD_BUS,
	.dev = {
		.platform_data = &lcd_spi_gpio_pdata,
	}
};

static struct spi_board_info spi_board_info[] __initdata = {
	{
		.modalias = "ld9040",
		.max_speed_hz	= 1200000,
		.bus_num	= SMDK4210_SPI_LCD_BUS,
		.chip_select	= 0,
		.mode	= SPI_MODE_3,
		.controller_data = (void*)GPIO_LCD_SPI_CS,
		.platform_data	= &ld9040_platform_data,
	}
};

static void __init ld9040_cfg_gpio(void)
{
	s3c_gpio_cfgpin(GPIO_LCD_RESET, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_LCD_RESET, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_LCD_SPI_CS, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_LCD_SPI_CS, S3C_GPIO_PULL_NONE);
	
	s3c_gpio_cfgpin(GPIO_LCD_SPI_SCK, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_LCD_SPI_SCK, S3C_GPIO_PULL_NONE);
	
	s3c_gpio_cfgpin(GPIO_LCD_SPI_MOSI, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_LCD_SPI_MOSI, S3C_GPIO_PULL_NONE);
}

static void __init smdk4210_init_fb(void) {
	if (gpio_request(GPIO_LCD_RESET, "LCD Reset")) {
		pr_err("%s: failed to request LCD Reset gpio\n", __func__);
	}
	ld9040_cfg_gpio();
	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
	s5p_fimd0_set_platdata(&smdk4210_fb_pdata);
}


/* Bluetooth rfkill gpio platform data */
struct rfkill_gpio_platform_data smdk4210_bt_pdata = {
	.reset_gpio	= EXYNOS4_GPX2(2),
	.shutdown_gpio	= -1,
	.type		= RFKILL_TYPE_BLUETOOTH,
	.name		= "smdk4210-bt",
};

/* Bluetooth Platform device */
static struct platform_device smdk4210_device_bluetooth = {
	.name		= "rfkill_gpio",
	.id		= -1,
	.dev		= {
		.platform_data	= &smdk4210_bt_pdata,
	},
};

static struct platform_device *smdk4210_devices[] __initdata = {
	&exynos4_device_sysmmu,
	&s3c_device_i2c0,
	&s3c_device_i2c1,
	&s3c_device_rtc,
	&s3c_device_wdt,
	&s3c_device_usb_hsotg,
	&s3c_device_hsmmc0,
	&s3c_device_hsmmc2,
	&s3c_device_hsmmc3,
	&s5p_device_ehci,
	&s5p_device_fimc0,
	&s5p_device_fimc1,
	&s5p_device_fimc2,
	&s5p_device_fimc3,
	&s5p_device_fimc_md,
	&s5p_device_fimd0,
	&s5p_device_g2d,
	&s5p_device_hdmi,
	&s5p_device_i2c_hdmiphy,
	&s5p_device_jpeg,
	&s5p_device_mfc,
	&s5p_device_mfc_l,
	&s5p_device_mfc_r,
	&s5p_device_mixer,
	&samsung_asoc_dma,
	&exynos4_device_i2s0,
	&exynos4_device_ohci,
	&exynos4_device_pd[PD_LCD0],
	&exynos4_device_pd[PD_TV],
	&exynos4_device_pd[PD_G3D],
	&exynos4_device_pd[PD_LCD1],
	&exynos4_device_pd[PD_CAM],
	&exynos4_device_pd[PD_GPS],
	&exynos4_device_pd[PD_MFC],
	&smdk4210_device_gpio_keys,
	&lcd_spi_gpio,
	&smdk4210_leds_gpio,
	&smdk4210_device_bluetooth,
	&exynos4_device_tmu,
};

/* LCD Backlight data */
static struct samsung_bl_gpio_info smdk4210_bl_gpio_info = {
	.no		= EXYNOS4_GPD0(0),
	.func		= S3C_GPIO_SFN(2),
};

static struct platform_pwm_backlight_data smdk4210_bl_data = {
	.pwm_id		= 0,
	.pwm_period_ns	= 1000,
};

static void __init smdk4210_bt_setup(void)
{
	gpio_request(EXYNOS4_GPA0(0), "GPIO BT_UART");
	/* 4 UART Pins configuration */
	s3c_gpio_cfgrange_nopull(EXYNOS4_GPA0(0), 4, S3C_GPIO_SFN(2));
	/* Setup BT Reset, this gpio will be requesed by rfkill-gpio */
	s3c_gpio_cfgpin(EXYNOS4_GPX2(2), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(EXYNOS4_GPX2(2), S3C_GPIO_PULL_NONE);
}

static void s5p_tv_setup(void)
{
	/* Direct HPD to HDMI chip */
	gpio_request_one(EXYNOS4_GPX3(7), GPIOF_IN, "hpd-plug");
	s3c_gpio_cfgpin(EXYNOS4_GPX3(7), S3C_GPIO_SFN(0x3));
	s3c_gpio_setpull(EXYNOS4_GPX3(7), S3C_GPIO_PULL_NONE);
}

static void __init smdk4210_map_io(void)
{
	exynos_init_io(NULL, 0);
	s3c24xx_init_clocks(24000000);
	s3c24xx_init_uarts(smdk4210_uartcfgs, ARRAY_SIZE(smdk4210_uartcfgs));
}

static void __init smdk4210_power_init(void)
{
	gpio_request(EXYNOS4_GPX0(4), "PMIC_IRQ");
	s3c_gpio_cfgpin(EXYNOS4_GPX0(4), S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(EXYNOS4_GPX0(4), S3C_GPIO_PULL_NONE);
}

static void __init smdk4210_reserve(void)
{
	s5p_mfc_reserve_mem(0x43000000, 8 << 20, 0x51000000, 8 << 20);
}

static void __init smdk4210_machine_init(void)
{
	smdk4210_power_init();
	smdk4210_config_gpio_table();
	
	s3c_sdhci0_set_platdata(&smdk4210_hsmmc0_pdata);
	s3c_sdhci2_set_platdata(&smdk4210_hsmmc2_pdata);
	s3c_sdhci3_set_platdata(&smdk4210_hsmmc3_pdata);
	
	s3c_i2c0_set_platdata(NULL);
	i2c_register_board_info(0, i2c0_devs, ARRAY_SIZE(i2c0_devs));

	s3c_i2c1_set_platdata(NULL);
	i2c_register_board_info(1, i2c1_devs, ARRAY_SIZE(i2c1_devs));
	
	s3c_gpio_cfgpin(GPIO_MASSMEM_EN, S3C_GPIO_OUTPUT);
	gpio_set_value(GPIO_MASSMEM_EN, GPIO_MASSMEM_EN_LEVEL);

	/* 400 kHz for initialization of MMC Card  */
	__raw_writel((__raw_readl(S5P_CLKDIV_FSYS3) & 0xfffffff0)
         | 0x9, S5P_CLKDIV_FSYS3);
	__raw_writel((__raw_readl(S5P_CLKDIV_FSYS2) & 0xfff0fff0)
         | 0x80008, S5P_CLKDIV_FSYS2);
	__raw_writel((__raw_readl(S5P_CLKDIV_FSYS1) & 0xfff0fff0)
         | 0x90009, S5P_CLKDIV_FSYS1);

		
	smdk4210_ehci_init();
	smdk4210_ohci_init();
	s3c_hsotg_set_platdata(&smdk4210_hsotg_pdata);
	clk_xusbxti.rate = 24000000;

	s5p_tv_setup();
	s5p_i2c_hdmiphy_set_platdata(NULL);

	//s5p_fimd0_set_platdata(&smdk4210_lcd_pdata);
	
	smdk4210_init_fb();


	platform_add_devices(smdk4210_devices, ARRAY_SIZE(smdk4210_devices));

	s5p_device_fimd0.dev.parent = &exynos4_device_pd[PD_LCD0].dev;

	s5p_device_hdmi.dev.parent = &exynos4_device_pd[PD_TV].dev;
	s5p_device_mixer.dev.parent = &exynos4_device_pd[PD_TV].dev;

	s5p_device_mfc.dev.parent = &exynos4_device_pd[PD_MFC].dev;

	samsung_bl_set(&smdk4210_bl_gpio_info, &smdk4210_bl_data);

	smdk4210_bt_setup();

	ath6kl_set_platform_data(&smdk4210_wlan_data);
}

MACHINE_START(SMDK4210, "SMDK4210")
	/* Maintainer: XpLoDWilD <xplodgui@gmail.com> */
	.atag_offset	= 0x100,
	.init_irq	= exynos4_init_irq,
	.map_io		= smdk4210_map_io,
	.handle_irq	= gic_handle_irq,
	.init_machine	= smdk4210_machine_init,
	.timer		= &exynos4_timer,
	.reserve	= &smdk4210_reserve,
	.restart	= exynos4_restart,
MACHINE_END
