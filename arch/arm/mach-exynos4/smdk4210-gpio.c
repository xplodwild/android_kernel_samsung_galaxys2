#include <linux/gpio.h>
#include <linux/serial_core.h>
#include <plat/devs.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-serial.h>
#include <mach/gpio.h>


struct gpio_init_data {
	uint num;
	uint cfg;
	uint val;
	uint pud;
	uint drv;
};

static struct gpio_init_data c1_init_gpios[] = {
#if defined(CONFIG_TDMB)
	{
	/* TDMB_INT */
		.num	= EXYNOS4_GPB(4),
		.cfg	= S3C_GPIO_OUTPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
	/* TDMB_RST_N */
		.num	= EXYNOS4_GPB(5),
		.cfg	= S3C_GPIO_OUTPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
	/* TDMB_EN*/
		.num	= EXYNOS4_GPC0(1),
		.cfg	= S3C_GPIO_OUTPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
#elif defined(CONFIG_ISDBT_FC8100)
	{
	/* ISDBT_RST_N */
		.num	= EXYNOS4_GPE1(5),
		.cfg	= S3C_GPIO_OUTPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
	/* ISDBT_EN*/
		.num	= EXYNOS4_GPC0(1),
		.cfg	= S3C_GPIO_OUTPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
#else
#if defined(CONFIG_TARGET_LOCALE_NA)
	{
		.num	= EXYNOS4_GPB(1),
		.cfg	= S3C_GPIO_OUTPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV4,
	},
#else
	{
		.num	= EXYNOS4_GPB(1),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
#endif /* CONFIG_TARGET_LOCALE_NA */
	 {
		.num	= EXYNOS4_GPB(5),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
#endif
	{
		.num	= EXYNOS4_GPC1(3),	/* CODEC_SDA_1.8V */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPC1(4),	/* CODEC_SCL_1.8V */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPD0(2),	/* MSENSOR_MHL_SDA_2.8V */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPD0(3),	/* MSENSOR_MHL_SCL_2.8V */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPD1(0),	/* 8M_CAM_SDA_2.8V */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPD1(1),	/* 8M_CAM_SCL_2.8V */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPD1(2),	/* SENSE_SDA_2.8V */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPD1(3),	/* SENSE_SCL_2.8V */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
#if defined(CONFIG_TARGET_LOCALE_NA)
	{
		.num	= EXYNOS4_GPE1(6),	/*wimax EEP_SEL*/
		.cfg	= S3C_GPIO_OUTPUT,
		.val	= S3C_GPIO_SETPIN_ONE,
		.pud	= S3C_GPIO_PULL_UP,
		.drv	= S5P_GPIO_DRVSTR_LV4,
	}, {
		.num	= EXYNOS4_GPE2(2),	/* VT_CAM_1.5V_EN*/
		.cfg	= S3C_GPIO_OUTPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV4,
	}, {
		.num    = EXYNOS4_GPJ0(0),  /* GPIO_CAM_PCLK*/
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num    = EXYNOS4_GPJ0(1),  /* GPIO_CAM_VSYNC*/
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPJ0(2),	/* GPIO_CAM_HSYNC */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPJ0(3),	/* GPIO_CAM_D0 */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPJ0(4),	/* GPIO_CAM_D1 */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPJ0(5),	/* GPIO_CAM_D2 */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPJ0(6),	/* GPIO_CAM_D3 */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPJ0(7),	/* GPIO_CAM_D4 */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPJ1(0),	/* GPIO_CAM_D5 */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPJ1(1),	/* GPIO_CAM_D6 */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPJ1(2),	/* GPIO_CAM_D7 */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPJ1(3),	/* GPIO_CAM_MCLK */
		.cfg	= S3C_GPIO_OUTPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
#endif /* CONFIG_TARGET_LOCALE_NA */
	{
		.num	= EXYNOS4_GPK1(1),
		.cfg	= S3C_GPIO_OUTPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV2,
	}, {
		.num	= EXYNOS4_GPK2(2),	/* PS_ALS_SDA_2.8V */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
#if defined(CONFIG_TARGET_LOCALE_NA)
	{
		.num	= EXYNOS4_GPK3(2),	/* PS_ALS_SCL_2.8V */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPL0(4),
		.cfg	= S3C_GPIO_OUTPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV4,
	}, {
		.num    = EXYNOS4_GPL0(6),	/* NC */
		.cfg    = S3C_GPIO_INPUT,
		.val    = 0,
		.pud    = S3C_GPIO_PULL_DOWN,
		.drv    = S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPL1(0),
		.cfg	= S3C_GPIO_OUTPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV4,
	}, {
		.num	= EXYNOS4_GPL1(2),
		.cfg	= S3C_GPIO_OUTPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV4,
	}, {
		.num	= EXYNOS4_GPL2(4),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPL2(5),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPX0(1),	/* VOL_UP */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
#else
	{
		.num	= EXYNOS4_GPK3(1),	/* WLAN_SDIO_CMD */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPK3(2),	/* PS_ALS_SCL_2.8V */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPK3(3),	/* WLAN_SDIO_D(0) */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPK3(4),	/* WLAN_SDIO_D(1) */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPK3(5),	/* WLAN_SDIO_D(2) */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPK3(6),	/* WLAN_SDIO_D(3) */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPX0(1),	/* VOL_UP */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
#endif /* CONFIG_TARGET_LOCALE_NA */
	{
		.num	= EXYNOS4_GPX0(2),	/* VOL_DOWN */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPX0(3),	/* GPIO_BOOT_MODE */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		/* TODO: remove belows if max17042 would be ready */
		.num	= EXYNOS4_GPX2(3),	/* GPIO_FUEL_ALERT */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPX3(1),
		.cfg	= S3C_GPIO_OUTPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, /* {
		.num	= EXYNOS4_GPX3(2),	// GPIO_DET_35
		.cfg	= S3C_GPIO_SFN(GPIO_DET_35_AF),
		.val	= 2,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, */
#if defined(CONFIG_TARGET_LOCALE_NA)
	{
		.num	= EXYNOS4_GPX3(3),	/* NC */
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
#else
	{
		.num	= EXYNOS4_GPX3(3),
		.cfg	= S3C_GPIO_OUTPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
#endif /* CONFIG_TARGET_LOCALE_NA */
	{
		.num	= EXYNOS4_GPX3(4),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 2,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
#if defined(CONFIG_TARGET_LOCALE_NA)
	{
		.num	= EXYNOS4_GPX3(5),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
#endif /* CONFIG_TARGET_LOCALE_NA */
#if defined(CONFIG_TARGET_LOCALE_NTT)
	{	/*GPY0 */
		.num	= EXYNOS4_GPY0(0),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY0(1),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
#endif
	{	/*GPY0 */
		.num	= EXYNOS4_GPY0(2),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY0(3),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY0(4),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY0(5),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
#if defined(CONFIG_TARGET_LOCALE_NA)
	{	/*GPY1 */
		.num	= EXYNOS4_GPY1(0),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY1(1),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_NONE,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
#else
	{	/*GPY1 */
		.num	= EXYNOS4_GPY1(0),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY1(1),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
#endif /* CONFIG_TARGET_LOCALE_NA */
	{
		.num	= EXYNOS4_GPY1(2),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY1(3),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {	/*GPY2 */
		.num	= EXYNOS4_GPY2(0),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY2(1),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY2(2),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY2(3),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY2(4),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY2(5),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {	/*GPY4 */
		.num	= EXYNOS4_GPY4(4),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {	/*GPY5 */
		.num	= EXYNOS4_GPY5(0),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY5(1),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY5(2),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY5(3),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY5(4),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY5(5),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY5(6),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY5(7),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {	/* GPY6 */
		.num	= EXYNOS4_GPY6(0),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY6(1),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY6(2),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY6(3),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY6(4),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY6(5),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY6(6),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	}, {
		.num	= EXYNOS4_GPY6(7),
		.cfg	= S3C_GPIO_INPUT,
		.val	= 0,
		.pud	= S3C_GPIO_PULL_DOWN,
		.drv	= S5P_GPIO_DRVSTR_LV1,
	},
#if defined(CONFIG_TARGET_LOCALE_NA)
	{ /* WIMAX_EN */
		.num	= EXYNOS4_GPE1(7),
		.cfg	= S3C_GPIO_OUTPUT,
		.val    = 0,
		.pud    = S3C_GPIO_PULL_NONE,
		.drv    = S5P_GPIO_DRVSTR_LV4,
	}, { /* WLAN_WAKE */
		.num	= EXYNOS4_GPD0(0),
		.cfg	= S3C_GPIO_INPUT,
		.val    = 0,
		.pud    = S3C_GPIO_PULL_DOWN,
		.drv    = S5P_GPIO_DRVSTR_LV1,
	}, { /* WIMAX_SDIO_CLK */
		.num	= EXYNOS4_GPK3(0),
		.cfg	= S3C_GPIO_INPUT,
		.val    = 0,
		.pud    = S3C_GPIO_PULL_DOWN,
		.drv    = S5P_GPIO_DRVSTR_LV1,
	}, { /* WIMAX_SDIO_CMD */
		.num	= EXYNOS4_GPK3(1),
		.cfg	= S3C_GPIO_INPUT,
		.val    = 0,
		.pud    = S3C_GPIO_PULL_DOWN,
		.drv    = S5P_GPIO_DRVSTR_LV1,
	}, { /* WIMAX_SDIO_D(0) */
		.num	= EXYNOS4_GPK3(3),
		.cfg	= S3C_GPIO_INPUT,
		.val    = 0,
		.pud    = S3C_GPIO_PULL_DOWN,
		.drv    = S5P_GPIO_DRVSTR_LV1,
	}, { /* WIMAX_SDIO_D(1) */
		.num	= EXYNOS4_GPK3(4),
		.cfg	= S3C_GPIO_INPUT,
		.val    = 0,
		.pud    = S3C_GPIO_PULL_DOWN,
		.drv    = S5P_GPIO_DRVSTR_LV1,
	}, { /* WIMAX_SDIO_D(2) */
		.num	= EXYNOS4_GPK3(5),
		.cfg	= S3C_GPIO_INPUT,
		.val    = 0,
		.pud    = S3C_GPIO_PULL_DOWN,
		.drv    = S5P_GPIO_DRVSTR_LV1,
	}, { /* WIMAX_SDIO_D(3) */
		.num	= EXYNOS4_GPK3(6),
		.cfg	= S3C_GPIO_INPUT,
		.val    = 0,
		.pud    = S3C_GPIO_PULL_DOWN,
		.drv    = S5P_GPIO_DRVSTR_LV1,
	}, { /* WIMAX_EEP_SCL_1.8V */
		.num	= EXYNOS4_GPY0(0),
		.cfg	= S3C_GPIO_INPUT,
		.val    = 0,
		.pud    = S3C_GPIO_PULL_DOWN,
		.drv    = S5P_GPIO_DRVSTR_LV1,
	}, { /* WIMAX_EEP_SDA_1.8V */
		.num	= EXYNOS4_GPY0(1),
		.cfg	= S3C_GPIO_INPUT,
		.val    = 0,
		.pud    = S3C_GPIO_PULL_DOWN,
		.drv    = S5P_GPIO_DRVSTR_LV1,
	}, { /* WIMAX_MODE1 */
		.num	= EXYNOS4_GPY3(6),
		.cfg	= S3C_GPIO_OUTPUT,
		.val    = 0,
		.pud    = S3C_GPIO_PULL_NONE,
		.drv    = S5P_GPIO_DRVSTR_LV4,
	}, {  /* WIMAX_MODE0 */
		.num	= EXYNOS4_GPY3(5),
		.cfg	= S3C_GPIO_OUTPUT,
		.val    = 0,
		.pud    = S3C_GPIO_PULL_NONE,
		.drv    = S5P_GPIO_DRVSTR_LV4,
	}, { /* WIMAX_CON1 */
		.num	= EXYNOS4_GPL2(7),
		.cfg	= S3C_GPIO_OUTPUT,
		.val    = 0,
		.pud    = S3C_GPIO_PULL_NONE,
		.drv    = S5P_GPIO_DRVSTR_LV4,
	}, { /* WIMAX_CON0 */
		.num	= EXYNOS4_GPL2(6),
		.cfg	= S3C_GPIO_OUTPUT,
		.val    = 0,
		.pud    = S3C_GPIO_PULL_NONE,
		.drv    = S5P_GPIO_DRVSTR_LV4,
	}, { /* WIMAX_WAKEUP */
		.num	= EXYNOS4_GPX1(3),
		.cfg	= S3C_GPIO_OUTPUT,
		.val    = 0,
		.pud    = S3C_GPIO_PULL_NONE,
		.drv    = S5P_GPIO_DRVSTR_LV4,
	}, { /* WIMAX_CON2 */
		.num	= EXYNOS4_GPE2(3),
		.cfg	= S3C_GPIO_OUTPUT,
		.val    = 0,
		.pud    = S3C_GPIO_PULL_NONE,
		.drv    = S5P_GPIO_DRVSTR_LV4,
	}, { /* WIMAX_INT */
		.num	= EXYNOS4_GPX1(1),
		.cfg	= S3C_GPIO_OUTPUT,
		.val    = 2,
		.pud    = S3C_GPIO_PULL_NONE,
		.drv    = S5P_GPIO_DRVSTR_LV4,
	}, { /* WIMAX_RESET_N */
		.num	= EXYNOS4_GPA0(5),
		.cfg	= S3C_GPIO_OUTPUT,
		.val    = 0,
		.pud    = S3C_GPIO_PULL_NONE,
		.drv    = S5P_GPIO_DRVSTR_LV4,
	}, { /*WIMAX_DBGEN_2.8V */
		.num	= EXYNOS4_GPA0(4),
		.cfg	= S3C_GPIO_OUTPUT,
		.val    = 0,
		.pud    = S3C_GPIO_PULL_NONE,
		.drv    = S5P_GPIO_DRVSTR_LV4,
	},
#endif /* CONFIG_TARGET_LOCALE_NA */
};

void c1_config_gpio_table(void)
{
	u32 i, gpio;

	printk(KERN_DEBUG "c1_config_gpio_table.\n");

	for (i = 0; i < ARRAY_SIZE(c1_init_gpios); i++) {
		gpio = c1_init_gpios[i].num;
		if (gpio <= EXYNOS4_MP02(5)) {
			s3c_gpio_cfgpin(gpio, c1_init_gpios[i].cfg);
			s3c_gpio_setpull(gpio, c1_init_gpios[i].pud);

			if (c1_init_gpios[i].val != 2)
				gpio_set_value(gpio, c1_init_gpios[i].val);

			s5p_gpio_set_drvstr(gpio, c1_init_gpios[i].drv);
		}
	}
}