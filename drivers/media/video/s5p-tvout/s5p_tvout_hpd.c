/*
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * HPD(Hot-Plug Detection) Interface for Samsung S5P TVOUT driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/miscdevice.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/poll.h>
#include <linux/sched.h>

#include <plat/tvout.h>

#include "hw_if/hw_if.h"
#include "s5p_tvout_common_lib.h"

#undef tvout_dbg

#ifdef CONFIG_HPD_DEBUG
#define tvout_dbg(fmt, ...)				\
		printk(KERN_INFO "\t[HPD] %s(): " fmt,	\
			__func__, ##__VA_ARGS__)
#else
#define tvout_dbg(fmt, ...)
#endif

/* /dev/hpd (Major 10, Minor 243) */
#define HPD_MINOR	243

#define HPD_LO		0
#define HPD_HI		1

#define HDMI_ON		1
#define HDMI_OFF	0

struct hpd_struct {
	spinlock_t lock;
	wait_queue_head_t waitq;
	atomic_t state;
	void (*int_src_hdmi_hpd)(void);
	void (*int_src_ext_hpd)(void);
	int (*read_gpio)(void);
	int irq_n;
};

static struct hpd_struct hpd_struct;

static int last_hpd_state;
atomic_t hdmi_status;
atomic_t poll_state;

static struct kobject *hpd_tvout_kobj, *hpd_video_kobj;

static void s5p_hpd_kobject_uevent(void)
{
	int hpd_state = atomic_read(&hpd_struct.state);

	if (hpd_state) {
		tvout_err("Event] Send UEvent = %d\n", hpd_state);
		kobject_uevent(hpd_tvout_kobj, KOBJ_ONLINE);
		kobject_uevent(hpd_video_kobj, KOBJ_ONLINE);
	} else {
		tvout_err("Event] Send UEvent = %d\n", hpd_state);
		kobject_uevent(hpd_tvout_kobj, KOBJ_OFFLINE);
		kobject_uevent(hpd_video_kobj, KOBJ_OFFLINE);
	}
}

static DECLARE_WORK(hpd_work, (void *)s5p_hpd_kobject_uevent);

void s5p_hpd_set_kobj(struct kobject *tvout_kobj, struct kobject *video_kobj)
{
	hpd_tvout_kobj = tvout_kobj;
	hpd_video_kobj = video_kobj;
}

static int s5p_hpd_open(struct inode *inode, struct file *file)
{
	atomic_set(&poll_state, 1);

	return 0;
}

static int s5p_hpd_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t s5p_hpd_read(struct file *file, char __user *buffer,
			    size_t count, loff_t *ppos)
{
	ssize_t retval;

	spin_lock_irq(&hpd_struct.lock);

	retval = put_user(atomic_read(&hpd_struct.state),
		(unsigned int __user *) buffer);

	atomic_set(&poll_state, -1);

	spin_unlock_irq(&hpd_struct.lock);

	return retval;
}

static unsigned int s5p_hpd_poll(struct file *file, poll_table *wait)
{
	poll_wait(file, &hpd_struct.waitq, wait);

	if (atomic_read(&poll_state) != -1)
		return POLLIN | POLLRDNORM;

	return 0;
}

static const struct file_operations hpd_fops = {
	.owner		= THIS_MODULE,
	.open		= s5p_hpd_open,
	.release	= s5p_hpd_release,
	.read		= s5p_hpd_read,
	.poll		= s5p_hpd_poll,
};

static struct miscdevice hpd_misc_device = {
	.minor		= HPD_MINOR,
	.name		= "HPD",
	.fops		= &hpd_fops,
};

int s5p_hpd_set_hdmiint(void)
{
	/* EINT -> HDMI */

	irq_set_irq_type(hpd_struct.irq_n, IRQ_TYPE_NONE);

	if (last_hpd_state)
		s5p_hdmi_reg_intc_enable(HDMI_IRQ_HPD_UNPLUG, 0);
	else
		s5p_hdmi_reg_intc_enable(HDMI_IRQ_HPD_PLUG, 0);

	atomic_set(&hdmi_status, HDMI_ON);

	hpd_struct.int_src_hdmi_hpd();

	s5p_hdmi_reg_hpd_gen();

	if (s5p_hdmi_reg_get_hpd_status())
		s5p_hdmi_reg_intc_enable(HDMI_IRQ_HPD_UNPLUG, 1);
	else
		s5p_hdmi_reg_intc_enable(HDMI_IRQ_HPD_PLUG, 1);

	return 0;
}

int s5p_hpd_set_eint(void)
{
	/* HDMI -> EINT */

	atomic_set(&hdmi_status, HDMI_OFF);

	s5p_hdmi_reg_intc_clear_pending(HDMI_IRQ_HPD_PLUG);
	s5p_hdmi_reg_intc_clear_pending(HDMI_IRQ_HPD_UNPLUG);

	s5p_hdmi_reg_intc_enable(HDMI_IRQ_HPD_PLUG, 0);
	s5p_hdmi_reg_intc_enable(HDMI_IRQ_HPD_UNPLUG, 0);

	hpd_struct.int_src_ext_hpd();

	return 0;
}

static int s5p_hdp_irq_eint(int irq)
{
	if (hpd_struct.read_gpio()) {
		irq_set_irq_type(hpd_struct.irq_n, IRQ_TYPE_LEVEL_LOW);

		if (atomic_read(&hpd_struct.state) == HPD_HI)
			return IRQ_HANDLED;

		atomic_set(&hpd_struct.state, HPD_HI);
		atomic_set(&poll_state, 1);

		last_hpd_state = HPD_HI;
		wake_up_interruptible(&hpd_struct.waitq);
	} else {
		irq_set_irq_type(hpd_struct.irq_n, IRQ_TYPE_LEVEL_HIGH);

		if (atomic_read(&hpd_struct.state) == HPD_LO)
			return IRQ_HANDLED;

		atomic_set(&hpd_struct.state, HPD_LO);
		atomic_set(&poll_state, 1);

		last_hpd_state = HPD_LO;
		wake_up_interruptible(&hpd_struct.waitq);
	}
	schedule_work(&hpd_work);

	tvout_dbg("%s\n", atomic_read(&hpd_struct.state) == HPD_HI ?
		"HPD HI" : "HPD LO");

	return IRQ_HANDLED;
}

static int s5p_hpd_irq_hdmi(int irq)
{
	u8 flag;
	int ret = IRQ_HANDLED;

	/* read flag register */
	flag = s5p_hdmi_reg_intc_status();

	if (s5p_hdmi_reg_get_hpd_status())
		s5p_hdmi_reg_intc_clear_pending(HDMI_IRQ_HPD_PLUG);
	else
		s5p_hdmi_reg_intc_clear_pending(HDMI_IRQ_HPD_UNPLUG);

	s5p_hdmi_reg_intc_enable(HDMI_IRQ_HPD_UNPLUG, 0);
	s5p_hdmi_reg_intc_enable(HDMI_IRQ_HPD_PLUG, 0);

	/* is this our interrupt? */
	if (!(flag & ((1 << HDMI_IRQ_HPD_PLUG) | (1 << HDMI_IRQ_HPD_UNPLUG)))) {
		ret = IRQ_NONE;

		goto out;
	}

	if (flag == ((1 << HDMI_IRQ_HPD_PLUG) | (1 << HDMI_IRQ_HPD_UNPLUG))) {
		tvout_dbg("HPD_HI && HPD_LO\n");

		if (last_hpd_state == HPD_HI && s5p_hdmi_reg_get_hpd_status())
			flag = 1 << HDMI_IRQ_HPD_UNPLUG;
		else
			flag = 1 << HDMI_IRQ_HPD_PLUG;
	}

	if (flag & (1 << HDMI_IRQ_HPD_PLUG)) {
		s5p_hdmi_reg_intc_enable(HDMI_IRQ_HPD_UNPLUG, 1);

		atomic_set(&hpd_struct.state, HPD_HI);
		atomic_set(&poll_state, 1);

		last_hpd_state = HPD_HI;
		wake_up_interruptible(&hpd_struct.waitq);

		s5p_hdcp_encrypt_stop(true);

		tvout_dbg("HPD_HI\n");

	} else if (flag & (1 << HDMI_IRQ_HPD_UNPLUG)) {
		s5p_hdcp_encrypt_stop(false);

		s5p_hdmi_reg_intc_enable(HDMI_IRQ_HPD_PLUG, 1);

		atomic_set(&hpd_struct.state, HPD_LO);
		atomic_set(&poll_state, 1);

		last_hpd_state = HPD_LO;
		wake_up_interruptible(&hpd_struct.waitq);

		tvout_dbg("HPD_LO\n");
	}
	schedule_work(&hpd_work);

out:
	return IRQ_HANDLED;
}

/*
 * HPD interrupt handler
 *
 * Handles interrupt requests from HPD hardware.
 * Handler changes value of internal variable and notifies waiting thread.
 */
static irqreturn_t s5p_hpd_irq_handler(int irq, void *dev_id)
{
	irqreturn_t ret = IRQ_HANDLED;

	/* check HDMI status */

	if (atomic_read(&hdmi_status)) {
		/* HDMI on */
		ret = s5p_hpd_irq_hdmi(irq);
		tvout_dbg("HDMI HPD interrupt\n");
	} else {
		/* HDMI off */
		ret = s5p_hdp_irq_eint(irq);
		tvout_dbg("EINT HPD interrupt\n");
	}

	return ret;
}

static int __init s5p_hpd_probe(struct platform_device *pdev)
{
	struct s5p_platform_hpd *pdata;

	if (misc_register(&hpd_misc_device)) {
		printk(KERN_WARNING " Couldn't register device 10, %d.\n", \
							HPD_MINOR);
		return -EBUSY;
	}

	init_waitqueue_head(&hpd_struct.waitq);

	spin_lock_init(&hpd_struct.lock);

	atomic_set(&hpd_struct.state, -1);

	atomic_set(&hdmi_status, HDMI_OFF);

	printk(KERN_INFO "initialised variables\n");

	pdata = to_tvout_plat(&pdev->dev);

	if (pdata->int_src_hdmi_hpd)
		hpd_struct.int_src_hdmi_hpd = \
			(void (*)(void))pdata->int_src_hdmi_hpd;

	if (pdata->int_src_ext_hpd)
		hpd_struct.int_src_ext_hpd = \
			(void (*)(void))pdata->int_src_ext_hpd;

	if (pdata->read_gpio)
		hpd_struct.read_gpio = \
				(int (*)(void))pdata->read_gpio;

	hpd_struct.irq_n = platform_get_irq(pdev, 0);

	hpd_struct.int_src_ext_hpd();
	if (hpd_struct.read_gpio()) {
		atomic_set(&hpd_struct.state, HPD_HI);
		last_hpd_state = HPD_HI;
	} else {
		atomic_set(&hpd_struct.state, HPD_LO);
		last_hpd_state = HPD_LO;
	}

	irq_set_irq_type(hpd_struct.irq_n, IRQ_TYPE_EDGE_BOTH);

	if (request_irq(hpd_struct.irq_n, (irq_handler_t)s5p_hpd_irq_handler,
				IRQF_DISABLED, "hpd", (void *)(&pdev->dev))) {
		printk(KERN_ERR  "failed to install hpd irq\n");
		return -EBUSY;
	}

	s5p_hdmi_reg_intc_set_isr(s5p_hpd_irq_handler, (u8)HDMI_IRQ_HPD_PLUG);
	s5p_hdmi_reg_intc_set_isr(s5p_hpd_irq_handler, \
						(u8)HDMI_IRQ_HPD_UNPLUG);

	return 0;
}

static int s5p_hpd_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_PM
static int s5p_hpd_suspend(struct platform_device *dev, pm_message_t state)
{
	hpd_struct.int_src_ext_hpd();
	return 0;
}

static int s5p_hpd_resume(struct platform_device *dev)
{
	if (atomic_read(&hdmi_status) == HDMI_ON)
		hpd_struct.int_src_hdmi_hpd();

	return 0;
}

#else
#define s5p_hpd_suspend NULL
#define s5p_hpd_resume NULL
#endif

static struct platform_driver s5p_hpd_driver = {
	.probe		= s5p_hpd_probe,
	.remove		= s5p_hpd_remove,
	.suspend	= s5p_hpd_suspend,
	.resume		= s5p_hpd_resume,
	.driver		= {
		.name		= "s5p-tvout-hpd",
		.owner		= THIS_MODULE,
	},
};

static int __init s5p_hpd_init(void)
{
	int ret;
	printk(KERN_INFO "S5P HPD for TVOUT Driver, Copyright (c) 2011 Samsung Electronics Co., LTD.\n");
	ret = platform_driver_register(&s5p_hpd_driver);
	if (ret) {
		printk(KERN_ERR \
		"Platform Device Register Failed - s5p_tvout_hpd %d\n", ret);
		return -1;
	}
	return 0;
}

static void __exit s5p_hpd_exit(void)
{
	misc_deregister(&hpd_misc_device);
}
module_init(s5p_hpd_init);
module_exit(s5p_hpd_exit);

MODULE_AUTHOR("Abhilash Kesavan <a.kesavan@xxxxxxxxxxx>");
MODULE_DESCRIPTION("Samsung S5P HPD(Hot-Plug Detection) driver for TVOUT");
MODULE_LICENSE("GPL");
