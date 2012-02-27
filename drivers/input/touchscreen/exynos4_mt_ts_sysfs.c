/* 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/sysfs.h>

#include "exynos4_mt_ts_gpio_i2c.h"
#include "exynos4_mt_ts.h"

/* touch threshold control (range 0 - 10) : default 3 */
#define THRESHOLD_MAX 10

/*  sysfs function prototype define */
/*  screen hold control (on -> hold, off -> normal mode) */
static ssize_t show_hold_state(struct device *dev,
		struct device_attribute *attr, char *buf);
static ssize_t set_hold_state(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
static DEVICE_ATTR(hold_state, S_IRWXUGO, show_hold_state, set_hold_state);

/* touch sampling rate control (5, 10, 20 : unit msec) */
static ssize_t show_sampling_rate(struct device *dev,
		struct device_attribute *attr, char *buf);
static ssize_t set_sampling_rate(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
static DEVICE_ATTR(sampling_rate, S_IRWXUGO, show_sampling_rate,
		set_sampling_rate);

static ssize_t show_threshold_x(struct device *dev,\
		struct device_attribute *attr, char *buf);
static ssize_t set_threshold_x(struct device *dev,\
		struct device_attribute *attr, const char *buf, size_t count);
static DEVICE_ATTR(threshold_x, S_IRWXUGO, show_threshold_x, set_threshold_x);

static ssize_t show_threshold_y(struct device *dev,\
		struct device_attribute *attr, char *buf);
static ssize_t set_threshold_y(struct device *dev,\
		struct device_attribute *attr, const char *buf, size_t count);
static DEVICE_ATTR(threshold_y, S_IRWXUGO, show_threshold_y, set_threshold_y);

/* touch calibration */
#if defined(CONFIG_TOUCHSCREEN_EXYNOS4_MT)
static ssize_t set_ts_cal(struct device *dev, struct device_attribute *attr,\
		const char *buf, size_t count);
static DEVICE_ATTR(ts_cal, S_IRUGO, NULL, set_ts_cal);
#endif

static struct attribute *exynos4_ts_sysfs_entries[] = {
	&dev_attr_hold_state.attr,
	&dev_attr_sampling_rate.attr,
	&dev_attr_threshold_x.attr,
	&dev_attr_threshold_y.attr,
#if defined(CONFIG_TOUCHSCREEN_EXYNOS4_MT)
	&dev_attr_ts_cal.attr,
#endif
	NULL
};

static struct attribute_group exynos4_ts_attr_group = {
	.name   = NULL,
	.attrs  = exynos4_ts_sysfs_entries,
};
static ssize_t show_hold_state(struct device *dev,\
	struct device_attribute *attr, char *buf)
{
	if (exynos4_ts.hold_status)
		return sprintf(buf, "on\n");
	else
		return sprintf(buf, "off\n");
}
static ssize_t set_hold_state(struct device *dev,\
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long flags;
	unsigned char wdata;

	local_irq_save(flags);

	if (!strcmp(buf, "on\n"))
		exynos4_ts.hold_status = 1;
	else {
#if defined(CONFIG_TOUCHSCREEN_EXYNOS4_MT)
		/* INT_mode : disable interrupt, low-active, finger moving */
		wdata = 0x01;
		exynos4_ts_write(MODULE_INTMODE, &wdata, 1);
		mdelay(10);
		/* INT_mode : enable interrupt, low-active, finger moving */
		wdata = 0x09;
		exynos4_ts_write(MODULE_INTMODE, &wdata, 1);
		mdelay(10);
#endif
		exynos4_ts.hold_status = 0;
	}

	local_irq_restore(flags);

	return count;
}
static ssize_t show_sampling_rate(struct device *dev,\
	struct device_attribute *attr, char *buf)
{
	switch (exynos4_ts.sampling_rate) {
	default:
		exynos4_ts.sampling_rate = 0;
	case	0:
		return sprintf(buf, "10 msec\n");
	case	1:
		return sprintf(buf, "20 msec\n");
	case	2:
		return sprintf(buf, "50 msec\n");
	}
}

static ssize_t set_sampling_rate(struct device *dev,\
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long flags;
	unsigned int val;

	if (!(sscanf(buf, "%u\n", &val)))
		return -EINVAL;

	local_irq_save(flags);
	if (val > 20)
		exynos4_ts.sampling_rate = 2;
	else if (val > 10)
		exynos4_ts.sampling_rate = 1;
	else
		exynos4_ts.sampling_rate = 0;

	local_irq_restore(flags);

	return count;
}
static ssize_t show_threshold_x(struct device *dev,\
	struct device_attribute *attr, char *buf)
{
	if (exynos4_ts.threshold_x > THRESHOLD_MAX)
		exynos4_ts.threshold_x = THRESHOLD_MAX;

	return sprintf(buf, "%d\n", exynos4_ts.threshold_x);
}

static ssize_t set_threshold_x(struct device *dev,\
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long flags;
	unsigned int val;

	if (!(sscanf(buf, "%u\n", &val)))
		return -EINVAL;

	local_irq_save(flags);

	if (val < 0)
		val *= (-1);

	if (val > THRESHOLD_MAX)
		val = THRESHOLD_MAX;

	exynos4_ts.threshold_x = val;

	local_irq_restore(flags);

	return count;
}
static ssize_t show_threshold_y(struct device *dev,\
	struct device_attribute *attr, char *buf)
{
	if (exynos4_ts.threshold_y > THRESHOLD_MAX)
		exynos4_ts.threshold_y = THRESHOLD_MAX;

	return sprintf(buf, "%d\n", exynos4_ts.threshold_y);
}
static ssize_t set_threshold_y(struct device *dev,\
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long flags;
	unsigned int val;

	if (!(sscanf(buf, "%u\n", &val)))
		return -EINVAL;

	local_irq_save(flags);

	if (val < 0)
		val *= (-1);

	if (val > THRESHOLD_MAX)
		val = THRESHOLD_MAX;

	exynos4_ts.threshold_y = val;

	local_irq_restore(flags);

	return count;
}

#if defined(CONFIG_TOUCHSCREEN_EXYNOS4_MT)
static ssize_t set_ts_cal(struct device *dev, struct device_attribute *attr,\
	const char *buf, size_t count)
{

	unsigned char wdata;
	unsigned long flags;

	local_irq_save(flags);

	/* INT_mode : disable interrupt */
	wdata = 0x00;
	exynos4_ts_write(MODULE_INTMODE, &wdata, 1);

	/* touch calibration */

	wdata = 0x03;
	exynos4_ts_write(MODULE_CALIBRATION, &wdata, 1); /* set mode */

	mdelay(500);

	/* INT_mode : enable interrupt, low-active, periodically*/

	wdata = 0x09;
	exynos4_ts_write(MODULE_INTMODE, &wdata, 1);

	local_irq_restore(flags);

	return count;
}
#endif

int exynos4_ts_sysfs_create(struct platform_device *pdev)
{
	/* variable init */
	exynos4_ts.hold_status = 0;

	/* 5 msec sampling */
	exynos4_ts.sampling_rate = 0;

	/* x data threshold (0~10) */
	exynos4_ts.threshold_x = TS_X_THRESHOLD;

	/* y data threshold (0~10) */
	exynos4_ts.threshold_y = TS_Y_THRESHOLD;

	return sysfs_create_group(&pdev->dev.kobj, &exynos4_ts_attr_group);
}
void exynos4_ts_sysfs_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &exynos4_ts_attr_group);
}
