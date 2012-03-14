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
#include <linux/i2c-gpio.h>
#include <linux/i2c/mcs.h>
#include <linux/gpio_keys.h>
#include <linux/delay.h>
#include <linux/rfkill-gpio.h>
#include <linux/ath6kl.h>
#include <linux/power_supply.h>
#if defined(CONFIG_S5P_MEM_CMA)
#include <linux/cma.h>
#endif
#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
#include <linux/power/max17042_battery.h>
#include <asm/mach/arch.h>
#include <asm/mach-types.h>

#include <plat/regs-serial.h>
#include <plat/exynos4.h>
#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/sdhci.h>
#include <plat/mshci.h>
#include <plat/iic.h>
#include <plat/bootmem.h>
#include <plat/fb.h>
#include <plat/fimg2d.h>
#include <plat/fimc.h>
#include <plat/pd.h>
#include <plat/pm.h>
#include <plat/gpio-cfg.h>
#include <plat/backlight.h>
#include <linux/i2c/atmel_mxt_ts.h>
#include <plat/otg.h>
#include <plat/ohci.h>
#include <plat/ehci.h>
#include <plat/clock.h>
#include <plat/tvout.h>
#include <mach/map.h>
#include <mach/bootmem.h>
#include <mach/regs-clock.h>
#include <mach/regs-mem.h>
#include <mach/pm-core.h>
#include <mach/sec_battery.h>
#include <mach/gpio-smdk4210-rev02.h>


struct max8922_platform_data {
	int	(*topoff_cb)(void);
	int	(*cfg_gpio)(void);
	int	gpio_chg_en;
	int	gpio_chg_ing;
	int	gpio_ta_nconnected;
};

/* Extern init setup functions */
extern void c1_config_gpio_table(void);
extern void c1_config_sleep_gpio_table(void);

enum i2c_bus_ids {
	I2C_GPIO_BUS_TOUCHKEY = 8,
	I2C_GPIO_BUS_GAUGE = 9,
	I2C_GPIO_BUS_PROX = 11,
	I2C_GPIO_BUS_USB,
	I2C_GPIO_BUS_MHL,
	I2C_GPIO_BUS_FM,
};


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


/*********** 
 * TSP 
 */
static struct mxt_platform_data mxt_pdata = {
	.x_line			= 19,
	.y_line			= 11,
	.x_size			= 480,
	.y_size			= 800,
	.blen			= 0x1,
	.threshold		= 0x18,
	.voltage		= 2800000,		/* 2.8V */
	.orient			= MXT_DIAGONAL,
	.irqflags		= IRQF_TRIGGER_FALLING,
};

static struct i2c_board_info i2c_devs3[] __initdata = {
	{
		I2C_BOARD_INFO("mXT224", 0x4a),
		.platform_data = &mxt_pdata,
		.irq		= IRQ_EINT(4),
	},
};

static void __init smdk4210_init_tsp(void) {
	gpio_request(GPIO_TSP_INT, "TOUCH_INT");
	s5p_register_gpio_interrupt(GPIO_TSP_INT);
	s3c_gpio_cfgpin(GPIO_TSP_INT, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_TSP_INT, S3C_GPIO_PULL_UP);
	i2c_devs3[0].irq = gpio_to_irq(GPIO_TSP_INT);
	gpio_request(GPIO_TSP_LDO_ON, "TOUCH_LDO");
	s3c_gpio_cfgpin(GPIO_TSP_LDO_ON, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TSP_LDO_ON, S3C_GPIO_PULL_NONE);
	gpio_direction_output(GPIO_TSP_LDO_ON, 1);
	msleep(100);
}


/******
 * MAX17042 
 */
static struct i2c_gpio_platform_data i2c_gpio_gauge_data = {
	.sda_pin	= GPIO_FUEL_SDA,
	.scl_pin	= GPIO_FUEL_SCL,
};

struct platform_device i2c_gpio_gauge = {
	.name = "i2c-gpio",
	.id = I2C_GPIO_BUS_GAUGE,
	.dev.platform_data = &i2c_gpio_gauge_data,
};

static struct max17042_reg_data smdk4210_max17042_regs[] = {
	{
		.addr = MAX17042_CGAIN,
		.data = 0,
	},
	{
		.addr = MAX17042_MiscCFG,
		.data = 0x0003,
	},
	{
		.addr = MAX17042_LearnCFG,
		.data = 0x0007,
	},
	{
		.addr = MAX17042_RCOMP0,
		.data = 0x0050,
	},
	{
		.addr = MAX17042_SALRT_Th,
		.data = 0xff02,
	},
	{
		.addr = MAX17042_VALRT_Th,
		.data = 0xff00,
	},
	{
		.addr = MAX17042_TALRT_Th,
		.data = 0x7f80,
	}
};

static struct max17042_platform_data smdk4210_max17042_data = {
	.init_data = smdk4210_max17042_regs,
	.num_init_data = ARRAY_SIZE(smdk4210_max17042_regs),
};

static struct i2c_board_info i2c_gpio_gauge_devs[] __initdata = {
	{
		I2C_BOARD_INFO("max17042", 0x36),
		.platform_data = &smdk4210_max17042_data,
		.irq = IRQ_EINT(19),
	},
};

static void __init smdk4210_init_battery_gauge(void)
{
	gpio_request(GPIO_FUEL_ALERT, "FUEL_ALERT");
	s5p_register_gpio_interrupt(GPIO_FUEL_ALERT);
	s3c_gpio_cfgpin(GPIO_FUEL_ALERT, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_FUEL_ALERT, S3C_GPIO_PULL_UP);
	i2c_gpio_gauge_devs[0].irq = gpio_to_irq(GPIO_FUEL_ALERT);
	
	i2c_register_board_info(I2C_GPIO_BUS_GAUGE,
	i2c_gpio_gauge_devs, ARRAY_SIZE(i2c_gpio_gauge_devs));
}

/***
 * touch keys
 */
static struct i2c_gpio_platform_data i2c_gpio_touchkey_data = {
	.sda_pin	= GPIO_3_TOUCH_SDA,
	.scl_pin	= GPIO_3_TOUCH_SCL,
	.udelay		= 2,
};

static struct platform_device i2c_gpio_touchkey = {
	.name		= "i2c-gpio",
	.id		= I2C_GPIO_BUS_TOUCHKEY,
	.dev		= {
		.platform_data	= &i2c_gpio_touchkey_data,
	},
};

static void smdk4210_touchkey_power(bool on) {
	struct regulator *regulator;
	regulator = regulator_get(NULL, "touch");
	if (IS_ERR(regulator)) {
		pr_err("%s: failed to get regulator\n", __func__);
		return;
	}

	if (on) {
		printk("MCS TOUCHKEY: Regulator ENABLE\n");
		regulator_enable(regulator);
	} else {
		printk("MCS TOUCHKEY: Regulator DISABLE\n");
		regulator_disable(regulator);
	}

	regulator_put(regulator);
}

static struct i2c_board_info i2c_gpio_touchkey_devs[] __initdata = {
	{
		I2C_BOARD_INFO("melfas_touchkey", 0x20),
	},
};

static void __init smdk4210_init_touchkey(void)
{
	gpio_request(GPIO_3_TOUCH_INT, "3_TOUCH_INT");
	s5p_register_gpio_interrupt(GPIO_3_TOUCH_INT);
	s3c_gpio_cfgpin(GPIO_3_TOUCH_INT, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_3_TOUCH_INT, S3C_GPIO_PULL_UP);
	i2c_gpio_touchkey_devs[0].irq = gpio_to_irq(GPIO_3_TOUCH_INT);
	smdk4210_touchkey_power(true);
	printk("MCS TOUCHKEY: They are initialized!\n");
	i2c_register_board_info(I2C_GPIO_BUS_TOUCHKEY, i2c_gpio_touchkey_devs, ARRAY_SIZE(i2c_gpio_touchkey_devs));
}


/******
 * MAX8997 
 */

extern struct max8997_platform_data max8997_pdata;

static struct i2c_board_info i2c_devs5[] __initdata = {
	{
		I2C_BOARD_INFO("max8997", (0xCC >> 1)),
		.platform_data = &max8997_pdata,
	},
};

static void __init smdk4210_init_pmic(void) {
	gpio_request(GPIO_PMIC_IRQ, "PMIC_IRQ");
	s5p_register_gpio_interrupt(GPIO_PMIC_IRQ);
	s3c_gpio_cfgpin(GPIO_PMIC_IRQ, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_PMIC_IRQ, S3C_GPIO_PULL_UP);
	i2c_devs5[0].irq = gpio_to_irq(GPIO_PMIC_IRQ);

}

/* I2C0 */


/* I2C1 */
/*static struct i2c_board_info i2c_devs1[] __initdata = {
	{
		I2C_BOARD_INFO("rt5625", 0x1e),
	},
};
*/

/* I2C2 */

/* To Do */

/* I2C3 */


/* I2C5 */


/* I2C6 */


/* I2C7 */
static struct i2c_board_info i2c_devs7[] __initdata = {
	{
		I2C_BOARD_INFO("s5p_ddc", (0x74 >> 1)), /* TVOUT */
	},
};


/* I2C9 */

/******
 * HSMMC def
 */
static struct s3c_sdhci_platdata smdk4210_hsmmc0_pdata __initdata = {
        .cd_type                = S3C_SDHCI_CD_PERMANENT,
        .clk_type               = S3C_SDHCI_CLK_DIV_EXTERNAL,
#ifdef CONFIG_EXYNOS4_SDHCI_CH0_8BIT
        .max_width              = 8,
        .host_caps              = MMC_CAP_8_BIT_DATA,
#endif
};

static struct s3c_sdhci_platdata smdk4210_hsmmc2_pdata __initdata = {
       .cd_type                = S3C_SDHCI_CD_GPIO,
       .ext_cd_gpio_invert		= true,
       .ext_cd_gpio				= EXYNOS4_GPX3(4),
#ifdef CONFIG_EXYNOS4_SDHCI_CH2_8BIT
        .max_width              = 8,
        .host_caps              = MMC_CAP_8_BIT_DATA,
#endif
};

static struct s3c_sdhci_platdata smdk4210_hsmmc3_pdata __initdata = {
	.cd_type					= S3C_SDHCI_CD_PERMANENT,
};

static struct s3c_mshci_platdata smdk4210_mshc_pdata __initdata = {
	.cd_type					= S3C_MSHCI_CD_PERMANENT,
	.max_width					= 8,
	.host_caps					= MMC_CAP_8_BIT_DATA | MMC_CAP_1_8V_DDR,
};


static struct s3c2410_platform_i2c i2c3_data __initdata = {
	.flags		= 0,
	.bus_num	= 3,
	.slave_addr	= 0x10,
	.frequency	= 400 * 1000,
	.sda_delay	= 100,
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
#ifdef CONFIG_CPU_EXYNOS4_EVT1
	.hw_ver		= 0x52,
#else
	.hw_ver		= 0x51,
#endif
};
#endif

static struct gpio_keys_button smdk4210_gpio_keys_table[] = {
	{
		.code = KEY_VOLUMEUP,
		.gpio = GPIO_VOL_UP,
		.desc = "gpio-keys: KEY_VOLUMEUP",
		.type = EV_KEY,
		.active_low = 1,
		.wakeup = 1,
		.debounce_interval = 1,
	}, {
		.code = KEY_VOLUMEDOWN,
		.gpio = GPIO_VOL_DOWN,
		.desc = "gpio-keys: KEY_VOLUMEDOWN",
		.type = EV_KEY,
		.active_low = 1,
		.wakeup = 1,
		.debounce_interval = 1,
	}, {
		.code = KEY_POWER,
		.gpio = GPIO_nPOWER,
		.desc = "gpio-keys: KEY_POWER",
		.type = EV_KEY,
		.active_low = 1,
		.wakeup = 1,
		.debounce_interval = 1,
	}, {
		.code = KEY_HOME,
		.gpio = EXYNOS4_GPX3(5),
		.desc = "gpio-keys: KEYHOME",
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


static struct platform_device *smdk4210_devices[] __initdata = {
	&exynos4_device_sysmmu,
	&exynos4_device_pd[PD_MFC],
	&exynos4_device_pd[PD_G3D],
	&exynos4_device_pd[PD_LCD0],
	&exynos4_device_pd[PD_TV],
	&s3c_device_fb,
	&s3c_device_i2c5,
	&s3c_device_i2c0,
	&s3c_device_i2c1,
	&s3c_device_i2c3,
	&s3c_device_i2c6,
	/*&s3c_device_i2c7,*/
	&s3c_device_hsmmc0,
	&s3c_device_hsmmc2,
	&s3c_device_hsmmc3,
	&s3c_device_mshci,
	&s3c_device_rtc,
	&s3c_device_timer[1],
	&s3c_device_wdt,
	&s5p_device_ohci,
	&s5p_device_ehci,
	&exynos4_device_i2s0,
	&samsung_asoc_dma,
	&s5p_device_fimg2d,
	&s5p_device_jpeg,
	&s3c_device_fimc0,
	&s3c_device_fimc1,
	&s3c_device_fimc2,

#if 0 /* TVOUT */
	&s5p_device_tvout,
	&s5p_device_cec,
	&s5p_device_hpd,
#endif

	&s3c_device_usbgadget,

	&i2c_gpio_gauge,
	&i2c_gpio_touchkey,
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

#if 0 /* defined(CONFIG_VIDEO_SAMSUNG_TVOUT)*/
static struct s5p_platform_hpd hdmi_hpd_data __initdata = {

};
static struct s5p_platform_cec hdmi_cec_data __initdata = {

};
#endif


static void __init smdk4210_machine_init(void)
{
	c1_config_gpio_table();
	c1_config_sleep_gpio_table();
	
	s3c_pm_init();
	
	exynos4_pd_enable(&exynos4_device_pd[PD_MFC].dev);
	exynos4_pd_enable(&exynos4_device_pd[PD_G3D].dev);
	exynos4_pd_enable(&exynos4_device_pd[PD_LCD0].dev);
	exynos4_pd_enable(&exynos4_device_pd[PD_LCD1].dev);
	exynos4_pd_enable(&exynos4_device_pd[PD_CAM].dev);
	exynos4_pd_enable(&exynos4_device_pd[PD_TV].dev);
	
	/* SROMC Setup */
	/* TODO: Move me to a separate function */
	/*u32 tmp;

	tmp = __raw_readl(S5P_SROM_BW);
	tmp &= ~(0xffff);
	tmp |= (0x9999);
	__raw_writel(tmp, S5P_SROM_BW);

	__raw_writel(0xff1ffff1, S5P_SROM_BC1);

	tmp = __raw_readl(S5P_VA_GPIO + 0x120);
	tmp &= ~(0xffffff);
	tmp |= (0x221121);
	__raw_writel(tmp, (S5P_VA_GPIO + 0x120));

	__raw_writel(0x22222222, (S5P_VA_GPIO + 0x180));
	__raw_writel(0x22222222, (S5P_VA_GPIO + 0x1a0));
	__raw_writel(0x22222222, (S5P_VA_GPIO + 0x1c0));
	__raw_writel(0x22222222, (S5P_VA_GPIO + 0x1e0));	*/
	
	/* MMC Card init */
	s3c_gpio_cfgpin(GPIO_MASSMEM_EN, S3C_GPIO_OUTPUT);
	gpio_set_value(GPIO_MASSMEM_EN, GPIO_MASSMEM_EN_LEVEL);
	
	/* 400 kHz for initialization of MMC Card  */
	__raw_writel((__raw_readl(S5P_CLKDIV_FSYS3) & 0xfffffff0)
		     | 0x9, S5P_CLKDIV_FSYS3);
	__raw_writel((__raw_readl(S5P_CLKDIV_FSYS2) & 0xfff0fff0)
		     | 0x80008, S5P_CLKDIV_FSYS2);
	__raw_writel((__raw_readl(S5P_CLKDIV_FSYS1) & 0xfff0fff0)
		     | 0x90009, S5P_CLKDIV_FSYS1);

	/* PLATDATA init */
	s3c_i2c0_set_platdata(NULL);
	/*i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0))*/

	s3c_i2c1_set_platdata(NULL);
	/*i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));*/
	
	smdk4210_init_tsp();
	s3c_i2c3_set_platdata(&i2c3_data);
	i2c_register_board_info(3, i2c_devs3, ARRAY_SIZE(i2c_devs3)); /* TSP */
	
	smdk4210_init_pmic();
	s3c_i2c5_set_platdata(NULL);
	i2c_register_board_info(5, i2c_devs5, ARRAY_SIZE(i2c_devs5));
	
	s3c_i2c6_set_platdata(NULL);
	//i2c_register_board_info(6, i2c_devs6, ARRAY_SIZE(i2c_devs6));

	/*s3c_i2c7_set_platdata(NULL);											TVOUT
	i2c_register_board_info(7, i2c_devs7, ARRAY_SIZE(i2c_devs7));*/
	
	smdk4210_init_touchkey();
	i2c_register_board_info(9, i2c_gpio_gauge_devs, ARRAY_SIZE(i2c_gpio_gauge_devs));
	
	s3cfb_set_platdata(NULL);
	s3c_device_fb.dev.parent = &exynos4_device_pd[PD_LCD0].dev;
	
	s3c_sdhci2_set_platdata(&smdk4210_hsmmc2_pdata);
	s3c_sdhci0_set_platdata(&smdk4210_hsmmc0_pdata);
	s3c_sdhci3_set_platdata(&smdk4210_hsmmc3_pdata);
	s3c_mshci_set_platdata(&smdk4210_mshc_pdata);

	
	s5p_fimg2d_set_platdata(&fimg2d_data);
	s5p_device_fimg2d.dev.parent = &exynos4_device_pd[PD_LCD0].dev;
	
	/* fimc */
	s3c_fimc0_set_platdata(&fimc_plat);
	s3c_fimc1_set_platdata(&fimc_plat);
	s3c_fimc2_set_platdata(&fimc_plat);

#if 0 /* TVOUT - Will nebkat hax? */
	s5p_hdmi_hpd_set_platdata(&hdmi_hpd_data);
	s5p_hdmi_cec_set_platdata(&hdmi_cec_data);
	s5p_device_tvout.dev.parent = &exynos4_device_pd[PD_TV].dev;
#endif
	

	smdk4210_otg_init();
	smdk4210_ohci_init();
	clk_xusbxti.rate = 24000000;
	smdk4210_init_battery_gauge();
	smdk4210_ehci_init();
	
	platform_add_devices(smdk4210_devices, ARRAY_SIZE(smdk4210_devices));

	samsung_bl_set(&smdk4210_bl_gpio_info, &smdk4210_bl_data);
	/*smdk4210_bt_setup();*/
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
	mi->bank[0].size = 0x10000000; //512 * SZ_1M;

	mi->bank[1].start = 0x50000000;
	mi->bank[1].size = 0x10000000; //512 * SZ_1M;

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
