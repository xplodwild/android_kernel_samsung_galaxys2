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
	REGULATOR_SUPPLY("vdd_abb", NULL),
};
static struct regulator_consumer_supply ldo2_consumer[] = {
	REGULATOR_SUPPLY("vdd_11on", NULL),
};
static struct regulator_consumer_supply ldo3_consumer[] = {
	REGULATOR_SUPPLY("vdd_11off", NULL),
};
static struct regulator_consumer_supply ldo4_consumer[] = {
	REGULATOR_SUPPLY("vdd_18on", NULL),
};
static struct regulator_consumer_supply ldo6_consumer[] = {
	REGULATOR_SUPPLY("vdd_18off", NULL),
};
static struct regulator_consumer_supply ldo7_consumer[] = {
	REGULATOR_SUPPLY("vdd_18aud", NULL),
};
static struct regulator_consumer_supply ldo8_consumer[] = {
	REGULATOR_SUPPLY("vdd_33off", NULL),
};
static struct regulator_consumer_supply ldo9_consumer[] = {
	REGULATOR_SUPPLY("vdd_33on", NULL),
};
static struct regulator_consumer_supply ldo10_consumer[] = {
	REGULATOR_SUPPLY("vdd_pll", NULL),
};
static struct regulator_consumer_supply ldo11_consumer[] = {
	REGULATOR_SUPPLY("vdd_aud", NULL),
};
static struct regulator_consumer_supply ldo14_consumer[] = {
	REGULATOR_SUPPLY("vdd_18_swb", NULL),
};
static struct regulator_consumer_supply ldo15_consumer[] = {
	REGULATOR_SUPPLY("vdd_lan", NULL),
};
static struct regulator_consumer_supply ldo16_consumer[] = {
	REGULATOR_SUPPLY("vdd_33off_ext1", NULL),
};
static struct regulator_consumer_supply ldo17_consumer[] = {
	REGULATOR_SUPPLY("vdd_33_swb", NULL),
};
static struct regulator_consumer_supply ldo18_consumer[] = {
	REGULATOR_SUPPLY("vdd_33off_ext2", NULL),
};
static struct regulator_consumer_supply ldo21_consumer[] = {
	REGULATOR_SUPPLY("vdd_mif", NULL),
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
#if 0
static struct regulator_consumer_supply buck4_consumer[] = {
	REGULATOR_SUPPLY("vdd_12off_ext1", NULL),
};
static struct regulator_consumer_supply buck5_consumer[] = {
	REGULATOR_SUPPLY("vdd_mem", NULL),
};
#endif
static struct regulator_consumer_supply buck7_consumer[] = {
	REGULATOR_SUPPLY("vdd_33off_ext3", NULL),
};
static struct regulator_init_data max8997_ldo1_data = {
	.constraints	= {
		.name		= "VDD_33_ABB",
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.apply_uV	= 1,
		.always_on	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.uV		= 3300000,
			.disabled = 1,
		},
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies  = &ldo1_consumer[0],
}; /* default 3.3v, output on with normal mode */

static struct regulator_init_data max8997_ldo2_data = {
	.constraints	= {
		.name		= "VDD_11ON",
		.min_uV		= 1100000,
		.max_uV		= 1100000,
		.apply_uV	= 1,
		.always_on	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.uV		= 1100000,
			.disabled = 1,
		},
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies  = &ldo2_consumer[0],
}; /* default 3.3v, output on with normal mode */

static struct regulator_init_data max8997_ldo3_data = {
	.constraints	= {
		.name		= "VDD_11OFF",
		.min_uV		= 1100000,
		.max_uV		= 1100000,
		.apply_uV	= 1,
		.always_on	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.uV		= 1100000,
			.disabled = 1,
		},
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies  = &ldo3_consumer[0],
}; /* default 1.1v, output on with normal mode */

static struct regulator_init_data max8997_ldo4_data = {
	.constraints	= {
		.name		= "VDD_18ON",
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.apply_uV	= 1,
		.always_on	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.uV		= 1800000,
			.disabled = 1,
		},
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies  = &ldo4_consumer[0],
}; /* default 1.8v, output on with normal mode */

static struct regulator_init_data max8997_ldo6_data = {
	.constraints	= {
		.name		= "VDD_18OFF",
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.apply_uV	= 1,
		.always_on	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	 = {
			.uV		= 1800000,
			.disabled = 1,
		},
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies  = &ldo6_consumer[0],
}; /* default 1.8v, output on with normal mode */

static struct regulator_init_data max8997_ldo7_data = {
	.constraints	= {
		.name		= "VDD_18AUD",
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.apply_uV	= 1,
		.always_on	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.uV		= 1800000,
			.disabled = 1,
		},
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies  = &ldo7_consumer[0],
}; /* default 1.8v, output on with normal mode */

static struct regulator_init_data max8997_ldo8_data = {
	.constraints	= {
		.name		= "VDD_33OFF",
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.apply_uV	= 1,
		.always_on	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.uV		= 3300000,
			.disabled = 1,
		},
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies  = &ldo8_consumer[0],
}; /* default 3.3v, output on with normal mode */

static struct regulator_init_data max8997_ldo9_data = {
	.constraints	= {
		.name		= "VDD_33ON",
		.min_uV		= 3000000,
		.max_uV		= 3000000,
		.apply_uV	= 1,
		.always_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
		.state_mem	= {
			.uV		= 3000000,
			.disabled = 1,
		},
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies  = &ldo9_consumer[0],
}; /* default 2.8v, output on with normal mode */

static struct regulator_init_data max8997_ldo10_data = {
	.constraints	= {
		.name		= "VDD_11ON_PLL",
		.min_uV		= 1100000,
		.max_uV		= 1100000,
		.apply_uV	= 1,
		.always_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
		.state_mem	= {
			.uV		= 1100000,
			.disabled = 1,
		},
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies  = &ldo10_consumer[0],
}; /* default 1.1v, output on with normal mode */

static struct regulator_init_data max8997_ldo11_data = {
	.constraints	= {
		.name		= "VDD_AUD",
		.min_uV		= 3000000,
		.max_uV		= 3000000,
		.apply_uV	= 1,
		.always_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE | \
				  REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.uV		= 3000000,
			.disabled = 1,
		},
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies  = &ldo11_consumer[0],
}; /* default 2.8v, output off */


static struct regulator_init_data max8997_ldo14_data = {
	.constraints	= {
		.name		= "AVDD18_SWB",
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.apply_uV	= 1,
		.always_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE | \
				  REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.uV		= 1800000,
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &ldo14_consumer[0],
}; /* default 1.8v, output off */

static struct regulator_init_data max8997_ldo15_data = {
	.constraints	= {
		.name		= "VDD_33ON_LAN",
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.apply_uV	= 1,
		.always_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE | \
				  REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.uV		= 3300000,
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &ldo15_consumer[0],
}; /* default 2.8v, output off */

static struct regulator_init_data max8997_ldo16_data = {
	.constraints	= {
		.name		= "VDD_33OFF_EXT1",
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.apply_uV	= 1,
		.always_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE | \
				  REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.uV		= 3300000,
			.disabled = 1,
		},
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies  = &ldo16_consumer[0],
}; /* default 3.3v, output off */

static struct regulator_init_data max8997_ldo17_data = {
	.constraints	= {
		.name		= "VDD33_SWB",
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.apply_uV	= 1,
		.always_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE | \
				  REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.uV		= 3300000,
			.disabled = 1,
		},
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies  = &ldo17_consumer[0],
}; /* default 3.3v, output off */

static struct regulator_init_data max8997_ldo18_data = {
	.constraints	= {
		.name		= "VDD_33OFF_EXT2",
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.apply_uV	= 1,
		.always_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE | \
				  REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.uV		= 3300000,
			.disabled = 1,
		},
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies  = &ldo18_consumer[0],
}; /* default 3.3v, output off */

static struct regulator_init_data max8997_ldo21_data = {
	.constraints	= {
		.name		= "VDD_MIF",
		.min_uV		= 1200000,
		.max_uV		= 1200000,
		.apply_uV	= 1,
		.always_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE | \
				  REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.uV		= 1200000,
			.disabled = 1,
		},
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies  = &ldo21_consumer[0],
}; /* default 1.2v, output on with normal mode */

static struct regulator_init_data max8997_buck1_data = {
	.constraints	= {
		.name		= "VDD_ARM",
		.min_uV		= 1250000,
		.max_uV		= 1250000,
		.always_on	= 1,
		.boot_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
		.state_mem	= {
			.uV		= 1250000,
			.disabled	= 1,
		},
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &buck1_consumer[0],
}; /* default 1.25v, turn on, dvs */

static struct regulator_init_data max8997_buck2_data = {
	.constraints	= {
		.name		= "VDD_INT",
		.min_uV		= 750000,
		.max_uV		= 1100000,
		.always_on	= 1,
		.boot_on	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
		.state_mem	= {
			.uV		= 1100000,
			.disabled	= 1,
		},
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies  = &buck2_consumer[0],
}; /* default 1.1v, turn on, dvs */

static struct regulator_init_data max8997_buck3_data = {
	.constraints	= {
		.name		= "VDD_G3D",
		.min_uV		= 900000,
		.max_uV		= 1200000,
		.always_on	= 1,
		.boot_on	= 0,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
			REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.uV		= 1200000,
			.disabled	= 1,
		},
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies  = &buck3_consumer[0],
}; /* default 1.2v, turn on */

#if 0
static struct regulator_init_data max8997_buck4_data = {
	.constraints	= {
		.name		= "VDD_12OFF_EXT1",
		.min_uV		= 1200000,
		.max_uV		= 1200000,
		.apply_uV	= 1,
		.always_on	= 1,
		.state_mem	= {
			.uV	= 1200000,
			.mode	= REGULATOR_MODE_NORMAL,
			.disabled = 1,
		},
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies  = &buck4_consumer[0],
}; /* default 1.2v, turn off */
#endif

static struct regulator_init_data max8997_buck5_data = {
	.constraints	= {
		.name		= "VDD_M15OFF",
		.min_uV		= 1500000,
		.max_uV		= 1500000,
		.apply_uV	= 1,
		.always_on	= 1,
		.state_mem	= {
			.uV	= 1500000,
			.mode	= REGULATOR_MODE_NORMAL,
			.disabled = 1,
		},
	},
#if 0
	.num_consumer_supplies  = 1,
	.consumer_supplies  = &buck5_consumer[0],
#endif
}; /* default 1.1v, turn on, dvs */

static struct regulator_init_data max8997_buck7_data = {
	.constraints	= {
		.name		= "VDD_33OFF_EXT3",
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.always_on	= 1,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.uV	= 3300000,
			.mode	= REGULATOR_MODE_NORMAL,
			.disabled = 1
		},
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies  = &buck7_consumer[0],
}; /* default 2.0v, turn on */


static struct max8997_regulator_data max8997_regulators[] = {
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
	{ MAX8997_LDO15,	&max8997_ldo15_data },
	{ MAX8997_LDO16,	&max8997_ldo16_data },
	{ MAX8997_LDO17,	&max8997_ldo17_data },
	{ MAX8997_LDO18,	&max8997_ldo18_data },
	{ MAX8997_LDO21,	&max8997_ldo21_data },
	{ MAX8997_BUCK1,	&max8997_buck1_data },
	{ MAX8997_BUCK2,	&max8997_buck2_data },
	{ MAX8997_BUCK3,	&max8997_buck3_data },
	{ MAX8997_BUCK5,	&max8997_buck5_data },
	{ MAX8997_BUCK7,	&max8997_buck7_data },
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

	.buck1_voltage[0]	= 1250000,
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

