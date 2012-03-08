/* linux/arch/arm/mach-exynos4/dev-pmic.c
 *
 * MAX8997 PMIC platform data.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/regulator/machine.h>
#include <linux/mfd/max8997.h>

#include <mach/gpio.h>

static struct regulator_consumer_supply ldo1_consumer[] = {
	REGULATOR_SUPPLY("vadc_3.3v", NULL),
};
static struct regulator_consumer_supply ldo3_consumer[] = {
	REGULATOR_SUPPLY("vusb_1.1v", "s5p-ehci"),
	REGULATOR_SUPPLY("vusb_1.1v", "s3c-usbgadget"),
	REGULATOR_SUPPLY("vmipi_1.1v", "s3c-fimc.0"),
	REGULATOR_SUPPLY("vmipi_1.1v", NULL),
};
static struct regulator_consumer_supply ldo4_consumer[] = {
	REGULATOR_SUPPLY("vmipi_1.8v", NULL),
};
static struct regulator_consumer_supply ldo5_consumer[] = {
	REGULATOR_SUPPLY("vhsic", NULL),
};
static struct regulator_consumer_supply ldo7_consumer[] = {
	REGULATOR_SUPPLY("cam_isp", NULL),
};
static struct regulator_consumer_supply ldo8_consumer[] = {
	REGULATOR_SUPPLY("vusb_3.3v", NULL),
};
static struct regulator_consumer_supply ldo10_consumer[] = {
	REGULATOR_SUPPLY("vpll_1.1v", NULL),
};
static struct regulator_consumer_supply ldo11_consumer[] = {
	REGULATOR_SUPPLY("touch", NULL),
};
static struct regulator_consumer_supply ldo12_consumer[] = {
	REGULATOR_SUPPLY("vt_cam_1.8v", NULL),
};
static struct regulator_consumer_supply ldo13_consumer[] = {
	REGULATOR_SUPPLY("vlcd_3.0v", NULL),
};
static struct regulator_consumer_supply ldo14_consumer[] = {
	REGULATOR_SUPPLY("vmotor", NULL),
};
static struct regulator_consumer_supply ldo15_consumer[] = {
	REGULATOR_SUPPLY("vled", NULL),
};
static struct regulator_consumer_supply ldo16_consumer[] = {
	REGULATOR_SUPPLY("cam_sensor_io", NULL),
};
static struct regulator_consumer_supply ldo17_consumer[] = {
	REGULATOR_SUPPLY("vt_cam_core_1.8v", NULL),
};
static struct regulator_consumer_supply ldo17_rev04_consumer[] = {
	REGULATOR_SUPPLY("vtf_2.8v", NULL),
};
static struct regulator_consumer_supply ldo18_consumer[] = {
	REGULATOR_SUPPLY("touch_led", NULL),
};
static struct regulator_consumer_supply ldo21_consumer[] = {
	REGULATOR_SUPPLY("vddq_m1m2", NULL),
};
static struct regulator_consumer_supply buck1_consumer[] = {
	REGULATOR_SUPPLY("vdd_arm", NULL),
};
static struct regulator_consumer_supply buck2_consumer[] = {
	REGULATOR_SUPPLY("vdd_int", NULL),
};
static struct regulator_consumer_supply buck3_consumer[] = {
	REGULATOR_SUPPLY("vdd_g3d", NULL),
};
static struct regulator_consumer_supply buck4_consumer[] = {
	REGULATOR_SUPPLY("cam_isp_core", NULL),
};
static struct regulator_consumer_supply buck7_consumer[] = {
	REGULATOR_SUPPLY("vcc_sub", NULL),
};
static struct regulator_consumer_supply safeout1_consumer[] = {
	REGULATOR_SUPPLY("safeout1", NULL),
};
static struct regulator_consumer_supply safeout2_consumer[] = {
	REGULATOR_SUPPLY("safeout2", NULL),
};
static struct regulator_consumer_supply led_flash_consumer[] = {
	REGULATOR_SUPPLY("led_flash", NULL),
};
static struct regulator_consumer_supply led_movie_consumer[] = {
	REGULATOR_SUPPLY("led_movie", NULL),
};

#define REGULATOR_INIT(_ldo,_name, _min_uV, _max_uV, _always_on, _ops_mask,\
						_disabled) \
		static struct regulator_init_data _ldo##_data = {				\
			.constraints = {												\
				.name	= _name,											\
				.min_uV = _min_uV,											\
				.max_uV = _max_uV,											\
				.always_on = _always_on,									\
				.boot_on = _always_on,										\
				.apply_uV = 1,												\
				.valid_ops_mask = _ops_mask,								\
				.state_mem = {												\
					.disabled = _disabled,									\
					.enabled = !(_disabled),								\
				}															\
			},																\
			.num_consumer_supplies = ARRAY_SIZE(_ldo##_consumer),				\
			.consumer_supplies = &_ldo##_consumer[0],							\
		};


REGULATOR_INIT(ldo1, "VADC_3.3V_C210", 3300000, 3300000, 1,
				REGULATOR_CHANGE_STATUS, 1);

REGULATOR_INIT(ldo3, "VUSB_1.1V", 1100000, 1100000, 1,
				REGULATOR_CHANGE_STATUS, 1);

REGULATOR_INIT(ldo4, "VMIPI_1.8V", 1800000, 1800000, 1,
				REGULATOR_CHANGE_STATUS, 1);

REGULATOR_INIT(ldo5, "VHSIC_1.2V", 1200000, 1200000, 1,
				REGULATOR_CHANGE_STATUS, 1);

REGULATOR_INIT(ldo7, "CAM_ISP_1.8V", 1800000, 1800000, 0,
				REGULATOR_CHANGE_STATUS, 1);

REGULATOR_INIT(ldo8, "VUSB_3.3V", 3300000, 3300000, 1,
				REGULATOR_CHANGE_STATUS, 1);

REGULATOR_INIT(ldo10, "VPLL_1.1V", 1100000, 1100000, 1,
				REGULATOR_CHANGE_STATUS, 1);

REGULATOR_INIT(ldo11, "TOUCH_2.8V", 2800000, 2800000, 0,
				REGULATOR_CHANGE_STATUS, 1);

REGULATOR_INIT(ldo12, "VT_CAM_1.8V", 1800000, 1800000, 0,
				REGULATOR_CHANGE_STATUS, 1);

REGULATOR_INIT(ldo13, "VCC_3.0V_LCD", 3000000, 3000000, 1,
				REGULATOR_CHANGE_STATUS, 1);

REGULATOR_INIT(ldo14, "VCC_2.8V_MOTOR", 2800000, 2800000, 0,
				REGULATOR_CHANGE_STATUS, 1);

REGULATOR_INIT(ldo15, "LED_A_2.8V", 2800000, 2800000, 0,
				REGULATOR_CHANGE_STATUS, 1);

REGULATOR_INIT(ldo16, "CAM_SENSOR_IO_1.8V", 1800000, 1800000, 0,
				REGULATOR_CHANGE_STATUS, 1);

REGULATOR_INIT(ldo17, "VT_CAM_CORE_1.8V", 1800000, 1800000, 0,
				REGULATOR_CHANGE_STATUS, 1);

REGULATOR_INIT(ldo17_rev04, "VTF_2.8V", 2800000, 2800000, 0,
				REGULATOR_CHANGE_STATUS, 1);

REGULATOR_INIT(ldo18, "TOUCH_LED_3.3V", 3000000, 3000000, 0,
				REGULATOR_CHANGE_STATUS | REGULATOR_CHANGE_VOLTAGE, 1);

REGULATOR_INIT(ldo21, "VDDQ_M1M2_1.2V", 1200000, 1200000, 1,
				REGULATOR_CHANGE_STATUS, 1);


static struct regulator_init_data max8997_buck1_data = {
	.constraints	= {
		.name		= "vdd_arm range",
		.min_uV		= 650000,
		.max_uV		= 2225000,
		.always_on	= 1,
		.boot_on	= 1,
		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.mode		= REGULATOR_MODE_NORMAL,
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &buck1_consumer[0],
};

static struct regulator_init_data max8997_buck2_data = {
	.constraints	= {
		.name		= "vdd_int range",
		.min_uV		= 650000,
		.max_uV		= 2225000,
		.always_on	= 1,
		.boot_on	= 1,
		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.mode	= REGULATOR_MODE_NORMAL,
			.disabled	= 1,
		},
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies  = &buck2_consumer[0],
}; 

static struct regulator_init_data max8997_buck3_data = {
	.constraints	= {
		.name		= "G3D_1.1V",
		.min_uV		= 900000,
		.max_uV		= 1200000,
		.always_on	= 0,
		.boot_on	= 0,
		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
			REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.mode		= REGULATOR_MODE_NORMAL,
			.disabled	= 1,
		},
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies  = &buck3_consumer[0],
}; 

static struct regulator_init_data max8997_buck4_data = {
	.constraints	= {
		.name		= "CAM_ISP_CORE_1.2V",
		.min_uV		= 1200000,
		.max_uV		= 1200000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies  = &buck4_consumer[0],
};

static struct regulator_init_data max8997_buck5_data = {
	.constraints	= {
		.name		= "VMEM_1.2V",
		.min_uV		= 1200000,
		.max_uV		= 1200000,
		.apply_uV	= 1,
		.always_on	= 1,
		.state_mem	= {
			.mode	= REGULATOR_MODE_NORMAL,
			.enabled = 1,
		},
	},
};

static struct regulator_init_data max8997_buck7_data = {
	.constraints	= {
		.name		= "VCC_SUB_2.0V",
		.min_uV		= 2000000,
		.max_uV		= 2000000,
		.always_on	= 1,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.enabled = 1,
		},
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies  = &buck7_consumer[0],
};

static struct regulator_init_data max8997_safeout1_data = {
	.constraints	= {
		.name		= "safeout1 range",
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.always_on	= 0,
		.boot_on	= 1,
		.state_mem	= {
			.enabled = 1,
		},
	},
	.num_consumer_supplies  = ARRAY_SIZE(safeout1_consumer),
	.consumer_supplies  = safeout1_consumer,
};

static struct regulator_init_data max8997_safeout2_data = {
	.constraints	= {
		.name		= "safeout2 range",
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.always_on	= 0,
		.boot_on	= 0,
		.state_mem	= {
			.enabled = 1,
		},
	},
	.num_consumer_supplies  = ARRAY_SIZE(safeout2_consumer),
	.consumer_supplies  = safeout2_consumer,
};

static struct regulator_init_data max8997_led_flash_data = {
	.constraints	= {
		.name		= "FLASH_CUR",
		.min_uA = 23440,
		.max_uA = 750080,
		.valid_ops_mask = REGULATOR_CHANGE_CURRENT | REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies  = &led_flash_consumer[0],
};

static struct regulator_init_data max8997_led_movie_data = {
	.constraints	= {
		.name		= "MOVIE_CUR",
		.min_uA = 15625,
		.max_uA = 250000,
		.valid_ops_mask = REGULATOR_CHANGE_CURRENT | REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies  = &led_movie_consumer[0],
};


static struct max8997_regulator_data max8997_regulators[] = {
	{ MAX8997_LDO1,		&ldo1_data },
	{ MAX8997_LDO3,		&ldo3_data },
	{ MAX8997_LDO4,		&ldo4_data },
	{ MAX8997_LDO5,		&ldo5_data },
	{ MAX8997_LDO7,		&ldo7_data },
	{ MAX8997_LDO8,		&ldo8_data },
	{ MAX8997_LDO10,		&ldo10_data },
	{ MAX8997_LDO11,		&ldo11_data },
	{ MAX8997_LDO12,	&ldo12_data },
	{ MAX8997_LDO13,	&ldo13_data },
	{ MAX8997_LDO14,	&ldo14_data },
	{ MAX8997_LDO15,	&ldo15_data },
	{ MAX8997_LDO16,	&ldo16_data },
	{ MAX8997_LDO17,	&ldo17_data },
	//{ MAX8997_LDO17,	&ldo17_revO4_data },
	{ MAX8997_LDO18,	&ldo18_data },
	{ MAX8997_LDO21,	&ldo21_data },
	{ MAX8997_BUCK1,	&max8997_buck1_data },
	{ MAX8997_BUCK2,	&max8997_buck2_data },
	{ MAX8997_BUCK3,	&max8997_buck3_data },
	{ MAX8997_BUCK4,	&max8997_buck4_data },
	{ MAX8997_BUCK5,	&max8997_buck5_data },
	{ MAX8997_BUCK7,	&max8997_buck7_data },
	{ MAX8997_ESAFEOUT1,	&max8997_safeout1_data },
	{ MAX8997_ESAFEOUT2,	&max8997_safeout2_data },
	{ MAX8997_FLASH_CUR,	&max8997_led_flash_data },
	{ MAX8997_MOVIE_CUR,	&max8997_led_movie_data },
};

struct max8997_platform_data max8997_pdata = {
	.num_regulators	= ARRAY_SIZE(max8997_regulators),
	.regulators	= max8997_regulators,

	.wakeup = true,
	.buck1_gpiodvs	= false,
	.buck2_gpiodvs	= false,
	.buck5_gpiodvs	= false,

	.ignore_gpiodvs_side_effect = true,

	.buck125_default_idx = 0x0,

	.buck125_gpios[0]	= EXYNOS4_GPX1(6),
	.buck125_gpios[1]	= EXYNOS4_GPX1(7),
	.buck125_gpios[2]	= EXYNOS4_GPX0(4),

	.buck1_voltage[0]	= 1200000,
	.buck1_voltage[1]	= 1200000,
	.buck1_voltage[2]	= 1150000,
	.buck1_voltage[3]	= 1100000,
	.buck1_voltage[4]	= 1050000,
	.buck1_voltage[5]	= 1000000,
	.buck1_voltage[6]	= 950000,
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

