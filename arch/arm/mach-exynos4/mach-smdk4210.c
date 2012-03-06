/* linux/arch/arm/mach-exynos4/mach-smdk4210.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/serial_core.h>
#include <linux/gpio.h>
#include <linux/mmc/host.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/input.h>
#include <linux/pwm_backlight.h>
#include <linux/i2c.h>
#include <linux/gpio_keys.h>
#include <linux/delay.h>
#include <linux/rfkill-gpio.h>
#include <linux/ath6kl.h>
#if defined(CONFIG_S5P_MEM_CMA)
#include <linux/cma.h>
#endif
#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
#include <asm/mach/arch.h>
#include <asm/mach-types.h>

#include <plat/regs-serial.h>
#include <plat/exynos4.h>
#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/sdhci.h>
#include <plat/iic.h>
#include <plat/bootmem.h>
#include <plat/fb.h>
#include <plat/fimg2d.h>
#include <plat/fimc.h>
#include <plat/pd.h>
#include <plat/gpio-cfg.h>
#include <plat/backlight.h>
#include <plat/otg.h>
#include <plat/ohci.h>
#include <plat/ehci.h>
#include <plat/clock.h>
#include <plat/tvout.h>
#include <mach/map.h>
#include <mach/bootmem.h>

extern struct max8997_platform_data max8997_pdata;

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
		/* this should be used for bt wake: .wake_peer	= c1_bt_uart_wake_peer, */
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
	[4] = {
                .hwport         = 4,
                .flags          = 0,
                .ucon           = SMDK4210_UCON_DEFAULT,
                .ulcon          = SMDK4210_ULCON_DEFAULT,
                .ufcon          = SMDK4210_UFCON_DEFAULT,
        },
};

/* I2C0 */
static struct i2c_board_info i2c_devs0[] __initdata = {
	{
		/* The address is 0xCC used since SRAD = 0 */
		I2C_BOARD_INFO("max8997", (0xCC >> 1)),
		.platform_data = &max8997_pdata,
	},
#ifdef CONFIG_TOUCHSCREEN_UNIDISPLAY_TS
	{
		I2C_BOARD_INFO("unidisplay_ts", 0x41),
		.irq = EINT_NUMBER(25),
	},
#endif
};

/* I2C1 */
static struct i2c_board_info i2c_devs1[] __initdata = {
	{
		I2C_BOARD_INFO("rt5625", 0x1e),
	},
};
static struct i2c_board_info i2c_devs6[] __initdata = {
	{
		I2C_BOARD_INFO("s5p_ddc", (0x74 >> 1)),
	},
};

static struct s3c_sdhci_platdata smdk4210_hsmmc0_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_INTERNAL,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
#ifdef CONFIG_EXYNOS4_SDHCI_CH0_8BIT
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA,
#endif
};

static struct s3c_sdhci_platdata smdk4210_hsmmc2_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_INTERNAL,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
#ifdef CONFIG_EXYNOS4_SDHCI_CH2_8BIT
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA,
#endif
};

/*
 * WLAN: SDIO Host will call this func at booting time
 */
static int smdk4210_wifi_status_register(void (*notify_func)
		(struct platform_device *, int state));

/* WLAN: MMC3-SDIO */
static struct s3c_sdhci_platdata smdk4210_hsmmc3_pdata __initdata = {
	.max_width		= 4,
	.host_caps		= MMC_CAP_4_BIT_DATA |
			MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED,
	.cd_type		= S3C_SDHCI_CD_EXTERNAL,
	.ext_cd_init		= smdk4210_wifi_status_register,
};

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

#define SMDK4210_WLAN_WOW EXYNOS4_GPX2(3)
#define SMDK4210_WLAN_RESET EXYNOS4_GPX2(4)

static void smdk4210_wlan_setup_power(bool val)
{
	int err;

	if (val) {
		err = gpio_request_one(SMDK4210_WLAN_RESET,
				GPIOF_OUT_INIT_LOW, "GPX2_4");
		if (err) {
			pr_warning("SMDK4210: Not obtain WIFI gpios\n");
			return;
		}
		s3c_gpio_cfgpin(SMDK4210_WLAN_RESET, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(SMDK4210_WLAN_RESET,
						S3C_GPIO_PULL_NONE);
		/* VDD33,I/O Supply must be done */
		gpio_set_value(SMDK4210_WLAN_RESET, 0);
		udelay(30);	/*Tb */
		gpio_direction_output(SMDK4210_WLAN_RESET, 1);
	} else {
		gpio_direction_output(SMDK4210_WLAN_RESET, 0);
		gpio_free(SMDK4210_WLAN_RESET);
	}
	mdelay(100);

	return;
}

/*
 * This will be called at init time of WLAN driver
 */
int smdk4210_wifi_set_detect(bool val)
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


#ifdef CONFIG_VIDEO_FIMG2D
static struct fimg2d_platdata fimg2d_data __initdata = {
	.hw_ver = 30,
	.parent_clkname = "mout_mpll",
	.clkname = "sclk_fimg2d",
	.gate_clkname = "fimg2d",
	.clkrate = 250 * 1000000,
};
#endif

#ifdef CONFIG_VIDEO_FIMC
static struct s3c_platform_fimc fimc_plat = {
#ifdef CONFIG_ITU_A
	.default_cam	= CAMERA_PAR_A,
#endif
	.camera		= {
	},
#ifdef CONFIG_CPU_S5PV310_EVT1
	.hw_ver		= 0x52,
#else
	.hw_ver		= 0x51,
#endif
};
#endif

static struct gpio_keys_button smdk4210_gpio_keys_table[] = {
	{
		.code = KEY_MENU,
		.gpio = EXYNOS4_GPX1(5),
		.desc = "gpio-keys: KEY_MENU",
		.type = EV_KEY,
		.active_low = 1,
		.wakeup = 1,
		.debounce_interval = 1,
	}, {
		.code = KEY_HOME,
		.gpio = EXYNOS4_GPX1(6),
		.desc = "gpio-keys: KEY_HOME",
		.type = EV_KEY,
		.active_low = 1,
		.wakeup = 1,
		.debounce_interval = 1,
	}, {
		.code = KEY_BACK,
		.gpio = EXYNOS4_GPX1(7),
		.desc = "gpio-keys: KEY_BACK",
		.type = EV_KEY,
		.active_low = 1,
		.wakeup = 1,
		.debounce_interval = 1,
	}, {
		.code = KEY_UP,
		.gpio = EXYNOS4_GPX2(0),
		.desc = "gpio-keys: KEY_UP",
		.type = EV_KEY,
		.active_low = 1,
		.wakeup = 1,
		.debounce_interval = 1,
	}, {
		.code = KEY_DOWN,
		.gpio = EXYNOS4_GPX2(1),
		.desc = "gpio-keys: KEY_DOWN",
		.type = EV_KEY,
		.active_low = 1,
		.wakeup = 1,
		.debounce_interval = 1,
	},
};

static struct gpio_keys_platform_data smdk4210_gpio_keys_data = {
	.buttons	= smdk4210_gpio_keys_table,
	.nbuttons	= ARRAY_SIZE(smdk4210_gpio_keys_table),
};

static struct platform_device smdk4210_device_gpiokeys = {
	.name = "gpio-keys",
	.dev = {
		.platform_data = &smdk4210_gpio_keys_data,
	},
};

static struct s5p_otg_platdata smdk4210_otg_pdata;
static void __init smdk4210_otg_init(void)
{
	struct s5p_otg_platdata *pdata = &smdk4210_otg_pdata;
 
	s5p_otg_set_platdata(pdata);
}
/*USB OHCI*/
static struct s5p_ohci_platdata smdk4210_ohci_pdata;

static void __init smdk4210_ohci_init(void)
{
	struct s5p_ohci_platdata *pdata = &smdk4210_ohci_pdata;

	s5p_ohci_set_platdata(pdata);
}
/* USB EHCI */
static struct s5p_ehci_platdata smdk4210_ehci_pdata;

static void __init smdk4210_ehci_init(void)
{
	struct s5p_ehci_platdata *pdata = &smdk4210_ehci_pdata;

	s5p_ehci_set_platdata(pdata);
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
	&exynos4_device_pd[PD_MFC],
	&exynos4_device_pd[PD_G3D],
	&exynos4_device_pd[PD_LCD0],
	&exynos4_device_pd[PD_TV],
#ifdef CONFIG_FB_S3C
	&s3c_device_fb,
#endif
	&s3c_device_i2c0,
	&s3c_device_i2c1,
	&s3c_device_i2c6,
	&s3c_device_hsmmc2,
	&s3c_device_hsmmc0,
	&s3c_device_hsmmc3,
	&s3c_device_rtc,
	&s3c_device_wdt,
	&s5p_device_ohci,
	&s5p_device_ehci,
	&exynos4_device_i2s0,
	&samsung_asoc_dma,
	&exynos4_device_sysmmu,
#ifdef CONFIG_VIDEO_FIMG2D
	&s5p_device_fimg2d,
#endif
#ifdef CONFIG_VIDEO_JPEG
	&s5p_device_jpeg,
#endif
#ifdef CONFIG_VIDEO_FIMC
	&s3c_device_fimc0,
	&s3c_device_fimc1,
	&s3c_device_fimc2,
#endif

#ifdef CONFIG_VIDEO_SAMSUNG_TVOUT
	&s5p_device_tvout,
	&s5p_device_cec,
	&s5p_device_hpd,
#endif
#ifdef CONFIG_USB_GADGET_S3C_OTGD
	&s3c_device_usbgadget,
#endif

	&smdk4210_device_bluetooth,

	&smdk4210_device_gpiokeys,
};

/* LCD Backlight data */
static struct samsung_bl_gpio_info smdk4210_bl_gpio_info = {
	.no = EXYNOS4_GPD0(0),
	.func = S3C_GPIO_SFN(2),
};

static struct platform_pwm_backlight_data smdk4210_bl_data = {
	.pwm_id = 0,
	.pwm_period_ns = 1000,
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

static void __init smdk4210_map_io(void)
{
	s5p_init_io(NULL, 0, S5P_VA_CHIPID);
	s3c24xx_init_clocks(24000000);
	s3c24xx_init_uarts(smdk4210_uartcfgs, ARRAY_SIZE(smdk4210_uartcfgs));
}

#if defined(CONFIG_VIDEO_SAMSUNG_TVOUT)
static struct s5p_platform_hpd hdmi_hpd_data __initdata = {

};
static struct s5p_platform_cec hdmi_cec_data __initdata = {

};
#endif

static void __init smdk4210_machine_init(void)
{
	s3c_i2c0_set_platdata(NULL);
	s3c_i2c1_set_platdata(NULL);
	s3c_i2c6_set_platdata(NULL);
	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));
	i2c_register_board_info(2, i2c_devs6, ARRAY_SIZE(i2c_devs6));
#ifdef CONFIG_FB_S3C
	s3cfb_set_platdata(NULL);
	s3c_device_fb.dev.parent = &exynos4_device_pd[PD_LCD0].dev;
#endif
	s3c_sdhci2_set_platdata(&smdk4210_hsmmc2_pdata);
	s3c_sdhci0_set_platdata(&smdk4210_hsmmc0_pdata);
	s3c_sdhci3_set_platdata(&smdk4210_hsmmc3_pdata);
#ifdef CONFIG_VIDEO_FIMG2D
	s5p_fimg2d_set_platdata(&fimg2d_data);
	s5p_device_fimg2d.dev.parent = &exynos4_device_pd[PD_LCD0].dev;
#endif
#ifdef CONFIG_VIDEO_FIMC
	/* fimc */
	s3c_fimc0_set_platdata(&fimc_plat);
	s3c_fimc1_set_platdata(&fimc_plat);
	s3c_fimc2_set_platdata(&fimc_plat);
#endif
#if defined(CONFIG_VIDEO_SAMSUNG_TVOUT)
	s5p_hdmi_hpd_set_platdata(&hdmi_hpd_data);
	s5p_hdmi_cec_set_platdata(&hdmi_cec_data);
	s5p_device_tvout.dev.parent = &exynos4_device_pd[PD_TV].dev;
#endif
	platform_add_devices(smdk4210_devices, ARRAY_SIZE(smdk4210_devices));
#ifdef CONFIG_USB_GADGET_S3C_OTGD
	 smdk4210_otg_init();
#endif
	smdk4210_ohci_init();
	clk_xusbxti.rate = 24000000;
	smdk4210_ehci_init();

	samsung_bl_set(&smdk4210_bl_gpio_info, &smdk4210_bl_data);

	smdk4210_bt_setup();

	ath6kl_set_platform_data(&smdk4210_wlan_data);

}

#if defined(CONFIG_S5P_MEM_CMA)
static void __init exynos4_reserve_cma(void)
{
	static struct cma_region regions[] = {
#if defined(CONFIG_VIDEO_SAMSUNG_MEMSIZE_JPEG) && !defined(CONFIG_VIDEO_UMP)
		{
			.name = "jpeg",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_JPEG * SZ_1K,
			.start = 0
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC0
		{
			.name = "fimc0",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC0 * SZ_1K,
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC1
		{
			.name = "fimc1",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC1 * SZ_1K,
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC2
		{
			.name = "fimc2",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC2 * SZ_1K,
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC3
		{
			.name = "fimc3",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC3 * SZ_1K,
		},
#endif
#ifdef CONFIG_FB_S3C
		{
			.name = "fimd",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMD0 * SZ_1K,
		},
#endif
		{
			.name = "common",
			.size = CONFIG_CMA_COMMON_MEMORY_SIZE * SZ_1K,
			.start = 0
		},
		{}
	};
	static const char map[] __initconst =
#if defined(CONFIG_VIDEO_JPEG) && !defined(CONFIG_VIDEO_UMP)
		"s5p-jpeg=jpeg;"
#endif
#if defined(CONFIG_VIDEO_FIMC)
		"s3c-fimc.0=fimc0;s3c-fimc.1=fimc1;"
		"s3c-fimc.2=fimc2;s3c-fimc.3=fimc3;"
#endif
#ifdef CONFIG_FB_S3C
		"s3cfb=fimd;"
#endif
		"*=common";
	int i = 0;
	unsigned int bank0_end = meminfo.bank[0].start +
					meminfo.bank[0].size;
	unsigned int bank1_end = meminfo.bank[1].start +
					meminfo.bank[1].size;

	for (; i < ARRAY_SIZE(regions) ; i++) {
		if (regions[i].start == 0) {
			regions[i].start = bank0_end - regions[i].size;
			bank0_end = regions[i].start;
		} else if (regions[i].start == 1) {
			regions[i].start = bank1_end - regions[i].size;
			bank1_end = regions[i].start;
		}
		printk(KERN_ERR "CMA reserve : %s, addr is 0x%x, size is 0x%x\n",
			regions[i].name, regions[i].start, regions[i].size);
	}

	cma_set_defaults(regions, map);
	cma_early_regions_reserve(NULL);
}
#endif

static void __init smdk4210_fixup(struct machine_desc *desc,
				struct tag *tags, char **cmdline,
				struct meminfo *mi)
{
	mi->bank[0].start = 0x40000000;
	mi->bank[0].size = 512 * SZ_1M;

	mi->bank[1].start = 0x60000000;
	mi->bank[1].size = 512 * SZ_1M;

	mi->nr_banks = 2;
}

MACHINE_START(SMDK4210, "SMDK4210")
	/* Maintainer: XpLoDWilD <xplodgui@gmail.com> */
	.boot_params	= S5P_PA_SDRAM + 0x100,
	.init_irq	= exynos4_init_irq,
	.fixup		= smdk4210_fixup,
	.map_io		= smdk4210_map_io,
	.init_machine	= smdk4210_machine_init,
	.timer		= &exynos4_timer,
#if defined(CONFIG_S5P_MEM_CMA)
	.reserve	= &exynos4_reserve_cma,
#else
	.reserve	= &s5p_reserve_bootmem,
#endif
MACHINE_END
