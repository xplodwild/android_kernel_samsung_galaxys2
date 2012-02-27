/* linux/arch/arm/mach-exynos4/mach-smdkv310.c
 *
 * Copyright (c) 2010-2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/serial_core.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/mmc/host.h>
#include <linux/platform_device.h>
#include <linux/smsc911x.h>
#include <linux/io.h>
#include <linux/i2c.h>
#if defined(CONFIG_S5P_MEM_CMA)
#include <linux/cma.h>
#endif
#include <linux/input.h>
#include <linux/pwm_backlight.h>
#include <linux/clk.h>
#include <plat/fimg2d.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/max8649.h>
#include <linux/regulator/fixed.h>
#include <linux/mfd/wm8994/pdata.h>

#include <asm/mach/arch.h>
#include <asm/mach-types.h>

#include <plat/regs-serial.h>
#include <plat/regs-srom.h>
#include <plat/exynos4.h>
#include <plat/cpu.h>
#include <plat/fb.h>
#include <plat/devs.h>
#include <plat/keypad.h>
#include <plat/sdhci.h>
#include <plat/iic.h>
#include <plat/pd.h>
#include <plat/gpio-cfg.h>
#include <plat/backlight.h>
#include <plat/bootmem.h>
#include <plat/s3c64xx-spi.h>
#include <plat/gpio-cfg.h>
#include <plat/ts.h>
#include <plat/fimc.h>
#include <plat/otg.h>
#include <plat/ohci.h>
#include <plat/clock.h>
#include <plat/ehci.h>
#include <plat/tvout.h>

#include <mach/regs-gpio.h>
#include <mach/regs-mem.h>
#include <mach/gpio.h>
#include <mach/spi-clocks.h>
#include <mach/map.h>
#include <mach/bootmem.h>
#include <media/s5k4ba_platform.h>

/* Following are default values for UCON, ULCON and UFCON UART registers */
#define SMDKV310_UCON_DEFAULT	(S3C2410_UCON_TXILEVEL |	\
				 S3C2410_UCON_RXILEVEL |	\
				 S3C2410_UCON_TXIRQMODE |	\
				 S3C2410_UCON_RXIRQMODE |	\
				 S3C2410_UCON_RXFIFO_TOI |	\
				 S3C2443_UCON_RXERR_IRQEN)

#define SMDKV310_ULCON_DEFAULT	S3C2410_LCON_CS8

#define SMDKV310_UFCON_DEFAULT	(S3C2410_UFCON_FIFOMODE |	\
				 S5PV210_UFCON_TXTRIG4 |	\
				 S5PV210_UFCON_RXTRIG4)

static struct s3c2410_uartcfg smdkv310_uartcfgs[] __initdata = {
	[0] = {
		.hwport		= 0,
		.flags		= 0,
		.ucon		= SMDKV310_UCON_DEFAULT,
		.ulcon		= SMDKV310_ULCON_DEFAULT,
		.ufcon		= SMDKV310_UFCON_DEFAULT,
	},
	[1] = {
		.hwport		= 1,
		.flags		= 0,
		.ucon		= SMDKV310_UCON_DEFAULT,
		.ulcon		= SMDKV310_ULCON_DEFAULT,
		.ufcon		= SMDKV310_UFCON_DEFAULT,
	},
	[2] = {
		.hwport		= 2,
		.flags		= 0,
		.ucon		= SMDKV310_UCON_DEFAULT,
		.ulcon		= SMDKV310_ULCON_DEFAULT,
		.ufcon		= SMDKV310_UFCON_DEFAULT,
	},
	[3] = {
		.hwport		= 3,
		.flags		= 0,
		.ucon		= SMDKV310_UCON_DEFAULT,
		.ulcon		= SMDKV310_ULCON_DEFAULT,
		.ufcon		= SMDKV310_UFCON_DEFAULT,
	},
};
#ifdef CONFIG_VIDEO_FIMC
/*
 * External camera reset
 * Because the most of cameras take i2c bus signal, so that
 * you have to reset at the boot time for other i2c slave devices.
 * This function also called at fimc_init_camera()
 * Do optimization for cameras on your platform.
*/
#ifdef CONFIG_ITU_A
static int exynos4_cam0_reset(int dummy)
{
	int err;
	/* Camera A */
	err = gpio_request(EXYNOS4_GPX1(2), "GPX1");
	if (err)
		printk(KERN_ERR "smdkv310_cam0_reset():failed to request GPX1_2");

	s3c_gpio_setpull(EXYNOS4_GPX1(2), S3C_GPIO_PULL_NONE);
	gpio_direction_output(EXYNOS4_GPX1(2), 0);
	gpio_direction_output(EXYNOS4_GPX1(2), 1);
	gpio_free(EXYNOS4_GPX1(2));
	printk("\n CAM0 RESET done \n");

	return 0;
}
#endif
#ifdef CONFIG_VIDEO_S5K4BA
static struct s5k4ba_platform_data s5k4ba_plat = {
	.default_width = 800,
	.default_height = 600,
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	.is_mipi = 0,
};

static struct i2c_board_info  s5k4ba_i2c_info = {
	I2C_BOARD_INFO("S5K4BA", 0x2d),
	.platform_data = &s5k4ba_plat,
};

static struct s3c_platform_camera s5k4ba = {
#ifdef CONFIG_ITU_A
	.id		= CAMERA_PAR_A,
	.clk_name	= "sclk_cam0",
	.i2c_busnum	= 0,
	.cam_power	= exynos4_cam0_reset,
#endif
#ifdef CONFIG_ITU_B
	.id		= CAMERA_PAR_B,
	.clk_name	= "sclk_cam1",
	.i2c_busnum	= 1,
	.cam_power	= exynos4_cam1_reset,
#endif
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.info		= &s5k4ba_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "xusbxti",
	.clk_rate	= 24000000,
	.line_length	= 1920,
	.width		= 1600,
	.height		= 1200,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 800,
		.height	= 600,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,

	.initialized	= 0,
};
#endif
/* Interface setting */
static struct s3c_platform_fimc fimc_plat = {
#ifdef CONFIG_ITU_A
	.default_cam	= CAMERA_PAR_A,
#endif
	.camera		= {
#ifdef CONFIG_VIDEO_S5K4BA
		&s5k4ba,
#endif
	},
#ifdef CONFIG_CPU_S5PV310_EVT1
	.hw_ver		= 0x52,
#else
 	.hw_ver		= 0x51,
#endif
};
#endif

static struct s3c_sdhci_platdata smdkv310_hsmmc0_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_INTERNAL,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
#ifdef CONFIG_EXYNOS4_SDHCI_CH0_8BIT
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA,
#endif
};

static struct s3c_sdhci_platdata smdkv310_hsmmc1_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_GPIO,
	.ext_cd_gpio		= EXYNOS4_GPK0(2),
	.ext_cd_gpio_invert	= 1,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
};

static struct s3c_sdhci_platdata smdkv310_hsmmc2_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_INTERNAL,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
#ifdef CONFIG_EXYNOS4_SDHCI_CH2_8BIT
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA,
#endif
};

static struct s3c_sdhci_platdata smdkv310_hsmmc3_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_GPIO,
	.ext_cd_gpio		= EXYNOS4_GPK2(2),
	.ext_cd_gpio_invert	= 1,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
};

#ifdef CONFIG_TOUCHSCREEN_EXYNOS4
static struct s3c_ts_mach_info s3c_ts_platform __initdata = {
	.delay                  = 10000,
	.presc                  = 49,
	.oversampling_shift     = 2,
	.resol_bit              = 12,
	.s3c_adc_con            = ADC_TYPE_2,
};
#endif

static struct resource smdkv310_smsc911x_resources[] = {
	[0] = {
		.start	= EXYNOS4_PA_SROM_BANK(1),
		.end	= EXYNOS4_PA_SROM_BANK(1) + SZ_64K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_EINT(5),
		.end	= IRQ_EINT(5),
		.flags	= IORESOURCE_IRQ | IRQF_TRIGGER_LOW,
	},
};

static struct smsc911x_platform_config smsc9215_config = {
	.irq_polarity	= SMSC911X_IRQ_POLARITY_ACTIVE_LOW,
	.irq_type	= SMSC911X_IRQ_TYPE_PUSH_PULL,
	.flags		= SMSC911X_USE_32BIT | SMSC911X_FORCE_INTERNAL_PHY,
	.phy_interface	= PHY_INTERFACE_MODE_MII,
	.mac		= {0x00, 0x80, 0x00, 0x23, 0x45, 0x67},
};

static struct platform_device smdkv310_smsc911x = {
	.name		= "smsc911x",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(smdkv310_smsc911x_resources),
	.resource	= smdkv310_smsc911x_resources,
	.dev		= {
		.platform_data	= &smsc9215_config,
	},
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

#if defined(CONFIG_SND_SOC_WM8994) || defined(CONFIG_SND_SOC_WM8994_MODULE)
static struct wm8994_pdata wm8994_platform_data = {
	/* configure gpio1 function: 0x0001(Logic level input/output) */
	.gpio_defaults[0] = 0x0001,
	/* configure gpio3/4/5/7 function for AIF2 voice */
	.gpio_defaults[2] = 0x8100,/*BCLK2 in*/
	.gpio_defaults[3] = 0x8100,/*LRCLK2 in*/
	.gpio_defaults[4] = 0x8100,/*DACDAT2 in*/
	/* configure gpio6 function: 0x0001(Logic level input/output) */
	.gpio_defaults[5] = 0x0001,
	.gpio_defaults[6] = 0x0100,/*ADCDAT2 out*/
};
#endif

static struct regulator_consumer_supply max8952_supply[] = {
	REGULATOR_SUPPLY("vdd_arm", NULL),
};

static struct regulator_consumer_supply max8649a_supply[] = {
	REGULATOR_SUPPLY("vdd_g3d", NULL),
};

static struct regulator_init_data max8952_init_data = {
	.constraints	= {
		.name		= "vdd_arm range",
		.min_uV		= 770000,
		.max_uV		= 1400000,
		.always_on	= 1,
		.boot_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
		.state_mem	= {
			.uV		= 1200000,
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &max8952_supply[0],
};

static struct regulator_init_data max8649a_init_data = {
	.constraints	= {
		.name		= "vdd_g3d range",
		.min_uV		= 750000,
		.max_uV		= 1380000,
		.always_on	= 0,
		.boot_on	= 0,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
		.state_mem	= {
			.uV		= 1200000,
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &max8649a_supply[0],
};

static struct max8649_platform_data smdkv310_max8952_info = {
	.mode		= 3,	/* VID1 = 1, VID0 = 1 */
	.extclk		= 0,
	.ramp_timing	= MAX8649_RAMP_32MV,
	.regulator	= &max8952_init_data,
};

static struct max8649_platform_data smdkv310_max8649a_info = {
	.mode		= 2,	/* VID1 = 1, VID0 = 0 */
	.extclk		= 0,
	.ramp_timing	= MAX8649_RAMP_32MV,
	.regulator	= &max8649a_init_data,
};
#ifdef CONFIG_I2C_S3C2410
/* I2C0 */
static struct i2c_board_info i2c_devs0[] __initdata = {
	{ I2C_BOARD_INFO("24c128", 0x50), },	 /* Samsung S524AD0XD1 */
	{ I2C_BOARD_INFO("24c128", 0x52), },	 /* Samsung S524AD0XD1 */
	{
		I2C_BOARD_INFO("max8952", 0x60),
		.platform_data	= &smdkv310_max8952_info,
	}, {
		I2C_BOARD_INFO("max8649", 0x62),
		.platform_data	= &smdkv310_max8649a_info,
	},
};
#endif
#ifdef CONFIG_FB_S3C_AMS369FG06

static struct s3c_platform_fb ams369fg06_data __initdata = {
	.hw_ver = 0x70,
	.clk_name = "sclk_lcd",
	.nr_wins = 5,
	.default_win = CONFIG_FB_S3C_DEFAULT_WINDOW,
	.swap = FB_SWAP_HWORD | FB_SWAP_WORD,
};

#define         LCD_BUS_NUM     1
static struct s3c64xx_spi_csinfo spi1_csi[] = {
	[0] = {
		.line = EXYNOS4_GPB(5),
		.set_level = gpio_set_value,
	},
};

static struct spi_board_info spi_board_info[] __initdata = {
	{
		.modalias       = "ams369fg06",
		.platform_data  = NULL,
		.max_speed_hz   = 1200000,
		.bus_num        = LCD_BUS_NUM,
		.chip_select    = 0,
		.mode           = SPI_MODE_3,
		.controller_data = &spi1_csi[0],
	}
};
#endif

static uint32_t smdkv310_keymap[] __initdata = {
	/* KEY(row, col, keycode) */
	KEY(0, 3, KEY_1), KEY(0, 4, KEY_2), KEY(0, 5, KEY_3),
	KEY(0, 6, KEY_4), KEY(0, 7, KEY_5),
	KEY(1, 3, KEY_A), KEY(1, 4, KEY_B), KEY(1, 5, KEY_C),
	KEY(1, 6, KEY_D), KEY(1, 7, KEY_E)
};

static struct matrix_keymap_data smdkv310_keymap_data __initdata = {
	.keymap		= smdkv310_keymap,
	.keymap_size	= ARRAY_SIZE(smdkv310_keymap),
};

static struct samsung_keypad_platdata smdkv310_keypad_data __initdata = {
	.keymap_data	= &smdkv310_keymap_data,
	.rows		= 2,
	.cols		= 8,
};

static struct i2c_board_info i2c_devs1[] __initdata = {
	{I2C_BOARD_INFO("wm8994", 0x1a),
#if defined(CONFIG_SND_SOC_WM8994) || defined(CONFIG_SND_SOC_WM8994_MODULE)
		.platform_data  = &wm8994_platform_data,
#endif
	},
#ifdef CONFIG_VIDEO_SAMSUNG_TVOUT
	{
		I2C_BOARD_INFO("s5p_ddc", (0x74 >> 1)),
	},
#endif

};

static struct s5p_otg_platdata smdkv310_otg_pdata;

static void __init smdkv310_otg_init(void)
{
	struct s5p_otg_platdata *pdata = &smdkv310_otg_pdata;

	s5p_otg_set_platdata(pdata);
}

/*USB OHCI*/
static struct s5p_ohci_platdata smdkv310_ohci_pdata;

static void __init smdkv310_ohci_init(void)
{
	struct s5p_ohci_platdata *pdata = &smdkv310_ohci_pdata;

	s5p_ohci_set_platdata(pdata);
}

/* USB EHCI */
static struct s5p_ehci_platdata smdkv310_ehci_pdata;

static void __init smdkv310_ehci_init(void)
{
	struct s5p_ehci_platdata *pdata = &smdkv310_ehci_pdata;

	s5p_ehci_set_platdata(pdata);
}

static struct platform_device *smdkv310_devices[] __initdata = {
#ifdef CONFIG_FB_S3C
	&s3c_device_fb,
#endif
#ifdef CONFIG_I2C_S3C2410
	&s3c_device_i2c0,
#endif
#ifdef CONFIG_FB_S3C_AMS369FG06
	&exynos4_device_spi1,
#endif
#ifdef CONFIG_TOUCHSCREEN_EXYNOS4
	&s3c_device_ts,
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_TVOUT
	&s5p_device_tvout,
	&s5p_device_cec,
	&s5p_device_hpd,
#endif
	&s3c_device_hsmmc0,
	&s3c_device_hsmmc1,
	&s3c_device_hsmmc2,
	&s3c_device_hsmmc3,
	&s3c_device_i2c1,
	&s3c_device_rtc,
	&s3c_device_wdt,
	&s5p_device_ohci,
	&s5p_device_ehci,
	&exynos4_device_ac97,
	&exynos4_device_i2s0,
	&exynos4_device_pcm0,
	&samsung_device_keypad,
#ifdef CONFIG_SND_SOC_SAMSUNG_SMDK_SPDIF
	&exynos4_device_spdif,
#endif
	&exynos4_device_pd[PD_MFC],
	&exynos4_device_pd[PD_G3D],
	&exynos4_device_pd[PD_LCD0],
	&exynos4_device_pd[PD_LCD1],
	&exynos4_device_pd[PD_CAM],
	&exynos4_device_pd[PD_TV],
	&exynos4_device_pd[PD_GPS],
	&exynos4_device_sysmmu,
	&samsung_asoc_dma,
	&smdkv310_smsc911x,
#ifdef CONFIG_VIDEO_FIMC
	&s3c_device_fimc0,
	&s3c_device_fimc1,
	&s3c_device_fimc2,
#endif
#ifdef CONFIG_VIDEO_JPEG
	&s5p_device_jpeg,
#endif
#ifdef CONFIG_VIDEO_FIMG2D

	&s5p_device_fimg2d,
#endif
	&exynos4_device_ahci,
#ifdef CONFIG_USB_GADGET_S3C_OTGD
	&s3c_device_usbgadget,
#endif
};

static void __init smdkv310_smsc911x_init(void)
{
	u32 cs1;

	/* configure nCS1 width to 16 bits */
	cs1 = __raw_readl(S5P_SROM_BW) &
		~(S5P_SROM_BW__CS_MASK << S5P_SROM_BW__NCS1__SHIFT);
	cs1 |= ((1 << S5P_SROM_BW__DATAWIDTH__SHIFT) |
		(1 << S5P_SROM_BW__WAITENABLE__SHIFT) |
		(1 << S5P_SROM_BW__BYTEENABLE__SHIFT)) <<
		S5P_SROM_BW__NCS1__SHIFT;
	__raw_writel(cs1, S5P_SROM_BW);

	/* set timing for nCS1 suitable for ethernet chip */
	__raw_writel((0x1 << S5P_SROM_BCX__PMC__SHIFT) |
		     (0x9 << S5P_SROM_BCX__TACP__SHIFT) |
		     (0xc << S5P_SROM_BCX__TCAH__SHIFT) |
		     (0x1 << S5P_SROM_BCX__TCOH__SHIFT) |
		     (0x6 << S5P_SROM_BCX__TACC__SHIFT) |
		     (0x1 << S5P_SROM_BCX__TCOS__SHIFT) |
		     (0x1 << S5P_SROM_BCX__TACS__SHIFT), S5P_SROM_BC1);
}

/* LCD Backlight data */
static struct samsung_bl_gpio_info smdkv310_bl_gpio_info = {
	.no = EXYNOS4_GPD0(1),
	.func = S3C_GPIO_SFN(2),
};

static struct platform_pwm_backlight_data smdkv310_bl_data = {
	.pwm_id = 1,
	.pwm_period_ns = 1000,
};

static void __init smdkv310_map_io(void)
{
	s5p_init_io(NULL, 0, S5P_VA_CHIPID);
	s3c24xx_init_clocks(24000000);
	s3c24xx_init_uarts(smdkv310_uartcfgs, ARRAY_SIZE(smdkv310_uartcfgs));
}

#if defined(CONFIG_VIDEO_SAMSUNG_TVOUT)
static struct s5p_platform_hpd hdmi_hpd_data __initdata = {

};
static struct s5p_platform_cec hdmi_cec_data __initdata = {

};
#endif

static void __init smdkv310_machine_init(void)
{
#ifdef CONFIG_FB_S3C_AMS369FG06
	struct clk *sclk = NULL;
	struct clk *prnt = NULL;
	struct device *spi_dev = &exynos4_device_spi1.dev;
#endif
#ifdef CONFIG_I2C_S3C2410
	s3c_i2c0_set_platdata(NULL);
	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
#endif

	s3c_i2c1_set_platdata(NULL);
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));

	smdkv310_smsc911x_init();
#ifdef CONFIG_VIDEO_FIMG2D
	s5p_fimg2d_set_platdata(&fimg2d_data);
#endif

#ifdef CONFIG_TOUCHSCREEN_EXYNOS4
	s3c_ts_set_platdata(&s3c_ts_platform);
#endif

#if defined(CONFIG_VIDEO_SAMSUNG_TVOUT)
	s5p_hdmi_hpd_set_platdata(&hdmi_hpd_data);
	s5p_hdmi_cec_set_platdata(&hdmi_cec_data);
#endif
	s3c_sdhci0_set_platdata(&smdkv310_hsmmc0_pdata);
	s3c_sdhci1_set_platdata(&smdkv310_hsmmc1_pdata);
	s3c_sdhci2_set_platdata(&smdkv310_hsmmc2_pdata);
	s3c_sdhci3_set_platdata(&smdkv310_hsmmc3_pdata);

	samsung_keypad_set_platdata(&smdkv310_keypad_data);
	samsung_bl_set(&smdkv310_bl_gpio_info, &smdkv310_bl_data);

#ifdef CONFIG_VIDEO_FIMC
	/* fimc */
	s3c_fimc0_set_platdata(&fimc_plat);
	s3c_fimc1_set_platdata(&fimc_plat);
	s3c_fimc2_set_platdata(&fimc_plat);
#endif
#ifdef CONFIG_FB_S3C
#ifdef CONFIG_FB_S3C_AMS369FG06
#else
	s3cfb_set_platdata(NULL);
#endif
#endif
	smdkv310_ohci_init();

	clk_xusbxti.rate = 24000000;

	smdkv310_ehci_init();
#ifdef CONFIG_USB_GADGET_S3C_OTGD
	smdkv310_otg_init();
#endif

	platform_add_devices(smdkv310_devices, ARRAY_SIZE(smdkv310_devices));
#ifdef CONFIG_FB_S3C_AMS369FG06
	sclk = clk_get(spi_dev, "sclk_spi");
	if (IS_ERR(sclk))
		dev_err(spi_dev, "failed to get sclk for SPI-1\n");
	prnt = clk_get(spi_dev, "mout_mpll");
	if (IS_ERR(prnt))
		dev_err(spi_dev, "failed to get prnt\n");
	clk_set_parent(sclk, prnt);
	clk_put(prnt);

	if (!gpio_request(EXYNOS4_GPB(5), "LCD_CS")) {
		gpio_direction_output(EXYNOS4_GPB(5), 1);
		s3c_gpio_cfgpin(EXYNOS4_GPB(5), S3C_GPIO_SFN(1));
		s3c_gpio_setpull(EXYNOS4_GPB(5), S3C_GPIO_PULL_UP);
		exynos4_spi_set_info(LCD_BUS_NUM, EXYNOS4_SPI_SRCCLK_SCLK,
			ARRAY_SIZE(spi1_csi));
	}
	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
	s3cfb_set_platdata(&ams369fg06_data);
#endif
}

static char const *smdkv310_dt_compat[] __initdata = {
	"samsung,smdkv310",
	NULL
};

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
		{
			.name = "common",
			.size = CONFIG_CMA_COMMON_MEMORY_SIZE * SZ_1K,
			.start = 0
		},
		{}
	};
	static const char map[] __initconst = 
#if defined (CONFIG_VIDEO_FIMC)
		"s3c-fimc.0=fimc0;s3c-fimc.1=fimc1;"
		"s3c-fimc.2=fimc2;s3c-fimc.3=fimc3;"
#endif
#if defined(CONFIG_VIDEO_JPEG) && !defined(CONFIG_VIDEO_UMP)
	        "s5p-jpeg=jpeg;"
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

MACHINE_START(SMDKV310, "SMDKV310")
	/* Maintainer: Kukjin Kim <kgene.kim@samsung.com> */
	/* Maintainer: Changhwan Youn <chaos.youn@samsung.com> */
	.boot_params	= S5P_PA_SDRAM + 0x100,
	.init_irq	= exynos4_init_irq,
	.map_io		= smdkv310_map_io,
	.init_machine	= smdkv310_machine_init,
	.timer		= &exynos4_timer,
	.dt_compat	= smdkv310_dt_compat,
#if defined(CONFIG_S5P_MEM_CMA)
	.reserve    = &exynos4_reserve_cma,
#else
	.reserve	= &s5p_reserve_bootmem,
#endif
MACHINE_END
