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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/gpio.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/wakelock.h>
#include <linux/earlysuspend.h>
#include <linux/suspend.h>
#endif

#include <asm/system.h>

#include <plat/gpio-cfg.h>

#include <mach/irqs.h>
#include <mach/regs-gpio.h>

#include "exynos4_mt_ts.h"
#include "exynos4_mt_ts_gpio_i2c.h"
#include "exynos4_mt_ts_sysfs.h"

struct exynos4_ts exynos4_ts;

static void exynos4_ts_process_data(struct touch_process_data *ts_data);
static void exynos4_ts_get_data(void);

static int exynos4_ts_open(struct input_dev *dev);
static void exynos4_ts_close(struct input_dev *dev);

static void exynos4_ts_release_device(struct device *dev);

#ifdef CONFIG_PM
static int exynos4_ts_resume(struct platform_device *dev);
static int exynos4_ts_suspend(struct platform_device *dev, pm_message_t state);
#else
static int exynos4_ts_resume(struct platform_device *dev)
{
	return 0;
}

static int exynos4_ts_suspend(struct platform_device *dev, pm_message_t state)
{
	return 0;
}
#endif

static void exynos4_ts_config(unsigned char state);

static int __devinit exynos4_ts_probe(struct platform_device *pdev);
static int __devexit exynos4_ts_remove(struct platform_device *pdev);

static int __init exynos4_ts_init(void);
static void __exit exynos4_ts_exit(void);

static struct platform_driver exynos4_ts_platform_device_driver = {
	.probe = exynos4_ts_probe,
	.remove = exynos4_ts_remove,
	.suspend = exynos4_ts_suspend,
	.resume = exynos4_ts_resume,
	.driver = {
		.owner = THIS_MODULE,
		.name = EXYNOS4_TS_DEVICE_NAME,
	},
};
static struct platform_device exynos4_ts_platform_device = {
	.name = EXYNOS4_TS_DEVICE_NAME,
	.id = -1,
	.num_resources = 0,
	.dev = {
		.release = exynos4_ts_release_device,
	},
};


static void exynos4_ts_process_data(struct touch_process_data *ts_data)
{
	/* read address setup */
	exynos4_ts_write(0x00, NULL, 0x00);

	/* Acc data read */
	write_seqlock(&exynos4_ts.lock);
	exynos4_ts_read(&exynos4_ts.rd[0], 10);

	write_sequnlock(&exynos4_ts.lock);

	ts_data->finger_cnt = exynos4_ts.rd[0] & 0x03;

	ts_data->x1 = ((exynos4_ts.rd[3] << 8) | exynos4_ts.rd[2]);
	if (ts_data->x1) {
		ts_data->x1 = (ts_data->x1 * 133) / 100;
#ifdef CONFIG_EXYNOS4_TS_FLIP
#else
		/* flip X & resize */
		ts_data->x1 = TS_ABS_MAX_X - ts_data->x1;
#endif
	}

	/* resize */
	ts_data->y1 = ((exynos4_ts.rd[5] << 8) | exynos4_ts.rd[4]);
	if (ts_data->y1) {
		ts_data->y1 = (ts_data->y1 * 128) / 100;
#ifdef CONFIG_EXYNOS4_TS_FLIP
		/* flip Y & resize */
		ts_data->y1 = TS_ABS_MAX_Y - ts_data->y1;
#else
#endif
	}
	if (ts_data->finger_cnt > 1) {
		/* flip X & resize */
		ts_data->x2 = ((exynos4_ts.rd[7] << 8) | exynos4_ts.rd[6]);
		if (ts_data->x2) {
			ts_data->x2 = (ts_data->x2 * 133) / 100;
#ifdef CONFIG_EXYNOS4_TS_FLIP
#else
			ts_data->x2 = TS_ABS_MAX_X - ts_data->x2;
#endif
		}
		/* resize */
		ts_data->y2 = ((exynos4_ts.rd[9] << 8) | exynos4_ts.rd[8]);
		if (ts_data->y2) {
			ts_data->y2 = (ts_data->y2 * 128) / 100;
#ifdef CONFIG_EXYNOS4_TS_FLIP
			/* flip Y & resize */
			ts_data->y2 = TS_ABS_MAX_Y - ts_data->y2;
#else
#endif

		}
	}
}

static void exynos4_ts_get_data(void)
{
	struct touch_process_data ts_data = {0};

	exynos4_ts_process_data(&ts_data);

	printk(KERN_DEBUG "x1: %d, y1: %d\n", ts_data.x1, ts_data.y1);
	printk(KERN_DEBUG "x2: %d, y2: %d\n", ts_data.x2, ts_data.y2);

	if (ts_data.finger_cnt > 0 && ts_data.finger_cnt < 3) {
		exynos4_ts.x = ts_data.x1;
		exynos4_ts.y = ts_data.y1;
		/* press */
		input_report_abs(exynos4_ts.driver, ABS_MT_TOUCH_MAJOR, 200);
		input_report_abs(exynos4_ts.driver, ABS_MT_WIDTH_MAJOR, 10);
		input_report_abs(exynos4_ts.driver, ABS_MT_POSITION_X,\
			exynos4_ts.x);
		input_report_abs(exynos4_ts.driver, ABS_MT_POSITION_Y,\
			exynos4_ts.y);
		input_mt_sync(exynos4_ts.driver);

		if (ts_data.finger_cnt > 1) {
			exynos4_ts.x = ts_data.x2;
			exynos4_ts.y = ts_data.y2;
			 /* press */
			input_report_abs(exynos4_ts.driver,\
				ABS_MT_TOUCH_MAJOR, 200);
			input_report_abs(exynos4_ts.driver,\
				ABS_MT_WIDTH_MAJOR, 10);
			input_report_abs(exynos4_ts.driver,\
				ABS_MT_POSITION_X, exynos4_ts.x);
			input_report_abs(exynos4_ts.driver,\
				ABS_MT_POSITION_Y, exynos4_ts.y);
			input_mt_sync(exynos4_ts.driver);
		}

		input_sync(exynos4_ts.driver);
	} else {
		 /* press */
		input_report_abs(exynos4_ts.driver, ABS_MT_TOUCH_MAJOR, 0);
		input_report_abs(exynos4_ts.driver, ABS_MT_WIDTH_MAJOR, 10);
		input_report_abs(exynos4_ts.driver, ABS_MT_POSITION_X,\
			exynos4_ts.x);
		input_report_abs(exynos4_ts.driver, ABS_MT_POSITION_Y,\
			exynos4_ts.y);
		input_mt_sync(exynos4_ts.driver);
		input_sync(exynos4_ts.driver);
	}
}

irqreturn_t exynos4_ts_irq(int irq, void *dev_id)
{
	unsigned long flags;

	local_irq_save(flags);
	local_irq_disable();
	exynos4_ts_get_data();
	local_irq_restore(flags);
	return IRQ_HANDLED;
}

static int exynos4_ts_open(struct input_dev *dev)
{
	printk(KERN_DEBUG "%s\n", __func__);

	return 0;
}

static void exynos4_ts_close(struct input_dev *dev)
{
	printk(KERN_DEBUG "%s\n", __func__);
}

static void exynos4_ts_release_device(struct device *dev)
{
	printk(KERN_DEBUG "%s\n", __func__);
}

#ifdef CONFIG_PM
static int exynos4_ts_resume(struct platform_device *dev)
{
	exynos4_ts_config(TOUCH_STATE_RESUME);

	/* interrupt enable */
	enable_irq(EXYNOS4_TS_IRQ);

	return 0;
}

static int exynos4_ts_suspend(struct platform_device *dev, pm_message_t state)
{
	unsigned char wdata;

	wdata = 0x00;
	exynos4_ts_write(MODULE_POWERMODE, &wdata, 1);
	mdelay(10);

	/* INT_mode : disable interrupt */
	wdata = 0x00;
	exynos4_ts_write(MODULE_INTMODE, &wdata, 1);
	mdelay(10);

	/* Touchscreen enter freeze mode : */
	wdata = 0x03;
	exynos4_ts_write(MODULE_POWERMODE, &wdata, 1);
	mdelay(10);

	/* interrupt disable */
	disable_irq(EXYNOS4_TS_IRQ);

	return 0;
}
#endif

static void exynos4_ts_config(unsigned char state)
{
	unsigned char wdata;

	/* exynos4_ts_reset(); */
	exynos4_ts_port_init();
	mdelay(500);

	/* Touchscreen Active mode */
	wdata = 0x00;
	exynos4_ts_write(MODULE_POWERMODE, &wdata, 1);
	mdelay(10);

	if (state == TOUCH_STATE_BOOT) {
		/* INT_mode : disable interrupt */
		wdata = 0x00;
		exynos4_ts_write(MODULE_INTMODE, &wdata, 1);

		if (!request_irq(EXYNOS4_TS_IRQ, exynos4_ts_irq, IRQF_DISABLED,\
			    "s5pc210-Touch IRQ", (void *)&exynos4_ts))
			printk(KERN_INFO "TOUCH request_irq = %d\r\n",
					EXYNOS4_TS_IRQ);
		else
			printk(KERN_ERR "TOUCH request_irq = %d error!! \r\n",
					EXYNOS4_TS_IRQ);

		if (gpio_is_valid(TS_ATTB))
			if (gpio_request(TS_ATTB, "TS_ATTB"))
				printk(KERN_ERR"failed to request GPH1 for TS_ATTB..\n");


		s3c_gpio_cfgpin(TS_ATTB, (0xf << 20));
		s3c_gpio_cfgpin((int)EINT_43CON, ~((0x7) << 20));
		s3c_gpio_setpull(TS_ATTB, S3C_GPIO_PULL_DOWN);
		/* s3c_gpio_setpull(TS_ATTB, S3C_GPIO_PULL_NONE); */

		irq_set_irq_type(EXYNOS4_TS_IRQ, IRQ_TYPE_EDGE_RISING);

		/* seqlock init */
		seqlock_init(&exynos4_ts.lock);

		 exynos4_ts.seq = 0;
	} else {
		/* INT_mode : disable interrupt, low-active, finger moving */
		wdata = 0x01;
		exynos4_ts_write(MODULE_INTMODE, &wdata, 1);
		mdelay(10);
		/* INT_mode : enable interrupt, low-active, finger moving */
		wdata = 0x09;
		/* wdata = 0x08; */
		exynos4_ts_write(MODULE_INTMODE, &wdata, 1);
		mdelay(10);
	}
}

static int __devinit exynos4_ts_probe(struct platform_device *pdev)
{
	int rc;
	unsigned char wdata;

	/* struct init */
	memset(&exynos4_ts, 0x00, sizeof(struct exynos4_ts));

	/* create sys_fs */
	rc = exynos4_ts_sysfs_create(pdev);
	if (rc) {
		printk(KERN_ERR "%s : sysfs_create_group fail!!\n", __func__);
		return rc;
	}

	exynos4_ts.driver = input_allocate_device();

	if (!(exynos4_ts.driver)) {
		printk(KERN_ERR "ERROR! : %s cdev_alloc()\
				error!!! no memory!!\n", __func__);
		exynos4_ts_sysfs_remove(pdev);
		return -ENOMEM;
	}

	exynos4_ts.driver->name = EXYNOS4_TS_DEVICE_NAME;
	exynos4_ts.driver->phys = "exynos4_ts/input1";
	exynos4_ts.driver->open = exynos4_ts_open;
	exynos4_ts.driver->close = exynos4_ts_close;

	exynos4_ts.driver->id.bustype = BUS_HOST;
	exynos4_ts.driver->id.vendor = 0x16B4;
	exynos4_ts.driver->id.product = 0x0702;
	exynos4_ts.driver->id.version = 0x0001;

	set_bit(EV_ABS, exynos4_ts.driver->evbit);

	/* multi touch */
	input_set_abs_params(exynos4_ts.driver, ABS_MT_POSITION_X,\
		TS_ABS_MIN_X, TS_ABS_MAX_X, 0, 0);
	input_set_abs_params(exynos4_ts.driver, ABS_MT_POSITION_Y,\
		TS_ABS_MIN_Y, TS_ABS_MAX_Y, 0, 0);
	input_set_abs_params(exynos4_ts.driver, ABS_MT_TOUCH_MAJOR,\
		0, 255, 2, 0);
	input_set_abs_params(exynos4_ts.driver, ABS_MT_WIDTH_MAJOR,\
		0,  15, 2, 0);
	input_set_abs_params(exynos4_ts.driver, ABS_X, 0, 0x3FF, 0, 0);
	input_set_abs_params(exynos4_ts.driver, ABS_Y, 0, 0x3FF, 0, 0);

	if (input_register_device(exynos4_ts.driver)) {
		printk(KERN_ERR "EXYNOS4 touch driver register failed.\n");

		exynos4_ts_sysfs_remove(pdev);
		input_free_device(exynos4_ts.driver);
		return -ENODEV;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	exynos4_ts.power.suspend = exynos4_ts_early_suspend;
	exynos4_ts.power.resume = exynos4_ts_late_resume;
	exynos4_ts.power.level = EARLY_SUSPEND_LEVEL_DISABLE_FB-1;

	register_early_suspend(&exynos4_ts.power);
#endif

	exynos4_ts_config(TOUCH_STATE_BOOT);


	/*  INT_mode : disable interrupt */
	wdata = 0x00;
	exynos4_ts_write(MODULE_INTMODE, &wdata, 1);

	/*  touch calibration */
	wdata = 0x03;
	/*  set mode */
	exynos4_ts_write(MODULE_CALIBRATION, &wdata, 1);
	mdelay(500);

	/*  INT_mode : enable interrupt, low-active, periodically*/
	wdata = 0x09;
	/* wdata = 0x08; */
	exynos4_ts_write(MODULE_INTMODE, &wdata, 1);

	printk(KERN_NOTICE "EXYNOS4 Multi-Touch driver initialized\n");

	return 0;
}

static int __devexit exynos4_ts_remove(struct platform_device *pdev)
{
	free_irq(EXYNOS4_TS_IRQ, (void *)&exynos4_ts);

	exynos4_ts_sysfs_remove(pdev);

	input_unregister_device(exynos4_ts.driver);

	return 0;
}

static int __init exynos4_ts_init(void)
{
	int ret = platform_driver_register(&exynos4_ts_platform_device_driver);

	if (!ret) {
		ret = platform_device_register(&exynos4_ts_platform_device);

		if (ret)
			platform_driver_unregister(
				&exynos4_ts_platform_device_driver
				);
	}
	return ret;
}

static void __exit exynos4_ts_exit(void)
{
	printk(KERN_DEBUG "%s\n", __func__);

	platform_device_unregister(&exynos4_ts_platform_device);
	platform_driver_unregister(&exynos4_ts_platform_device_driver);
}
module_init(exynos4_ts_init);
module_exit(exynos4_ts_exit);

MODULE_AUTHOR("Samsung AP");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("10.1 inch WXGA Multi-touch touchscreen driver");
