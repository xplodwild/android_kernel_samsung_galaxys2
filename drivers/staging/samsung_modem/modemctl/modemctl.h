/*
 * Modem control driver
 *
 * Copyright (C) 2012 Alexander Tarasikov <alexander.tarasikov@gmail.com>
 *
 * Copyright (C) 2010 Samsung Electronics Co.Ltd
 * Author: Suchang Woo <suchang.woo@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#ifndef __LINUX__SGS_MODEMCTL__
#define __LINUX__SGS_MODEMCTL__

#define MC_SUCCESS 0
#define MC_HOST_HIGH 1
#define MC_HOST_TIMEOUT 2

enum modem_state {
	MODEM_ON = BIT(0),
	MODEM_ACTIVE = BIT(1),
	MODEM_ERROR = BIT(2),
} modem_state;

struct modemctl_platform_data {
	int (*init)(struct device *dev);
	void (*exit)(struct device *dev);
	int (*reset)(struct device *dev);
	int (*set_power)(struct device *dev, bool enabled);
	enum modem_state (*get_state)(struct device *dev);
	int (*suspend)(struct device *dev);
	int (*resume)(struct device *dev);
};

#endif /* __LINUX__SGS_MODEMCTL__ */

