/* drivers/input/touchscreen/vpad_ts.c
 *
 * Copyright (C) 2010 Pixcir, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <mach/regs-gpio.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

#include "vpad_ts.h"

#define TOUCH_INT_PIN	EXYNOS4_GPX3(1)
#define TOUCH_RST_PIN	EXYNOS4_GPE3(5)

static struct workqueue_struct *pixcir_wq;

struct pixcir_i2c_ts_data {
	struct i2c_client *client;
	struct input_dev *input;
	struct delayed_work work;
	int irq;
};

static inline int pixcir_touch_read_gpio(void)
{
	int ret = 0;

	ret = gpio_get_value(TOUCH_INT_PIN);
	return ret;
}


static void pixcir_ts_poscheck(struct work_struct *work)
{
	struct pixcir_i2c_ts_data *tsdata = container_of(work,
						struct pixcir_i2c_ts_data,
						work.work);
	unsigned char Rdbuf[13], Wrbuf[1], msglenth;
	int posx1, posy1, posx2, posy2;
	unsigned char touching, oldtouching;
	int ret;
	int z = 50;
	int w = 15;

	if (pixcir_touch_read_gpio())
		goto out;


	memset(Wrbuf, 0, sizeof(Wrbuf));
	memset(Rdbuf, 0, sizeof(Rdbuf));

#ifdef NAS_7inch
	Wrbuf[0] = 0xf9;
	msglenth = 13;
#elif defined MUTTO_7inch
	Wrbuf[0] = 0x02;
	msglenth = 11;
#elif defined NAS_10inch
	Wrbuf[0] = 0x0;
	msglenth = 10;
#elif defined Unidisplay_7inch
	Wrbuf[0] = 0x10;
	msglenth = 9;
#elif defined Unidisplay_9_7inch
	Wrbuf[0] = 0x10;
	msglenth = 9;
#elif defined Touch_key
	Wrbuf[0] = 0x00;
	msglenth = 4;
#endif

	ret = i2c_master_send(tsdata->client, Wrbuf, 1);

	if (ret != 1) {
		dev_err(&tsdata->client->dev,\
				 "Unable to write to i2c touchscreen!\n");
		goto out;
	}

	ret = i2c_master_recv(tsdata->client, Rdbuf, msglenth);

	if (ret != msglenth) {
		dev_err(&tsdata->client->dev, "Unable to read i2c page!\n");
		goto out;
	}


#ifdef NAS_7inch
	touching = Rdbuf[3];
	oldtouching = 0;

	if (touching == 0xff)
		goto out;

	posy1 = ((Rdbuf[5] << 8) | Rdbuf[6]);
	posx1 = ((Rdbuf[7] << 8) | Rdbuf[8]);

	posy2 = ((Rdbuf[9] << 8) | Rdbuf[10]);
	posx2 = ((Rdbuf[11] << 8) | Rdbuf[12]);

	posx1 = posx1 & 0x0fff;
	posy1 = posy1 & 0x0fff;
	posx2 = posx2 & 0x0fff;
	posy2 = posy2 & 0x0fff;

	posy1 = TOUCHSCREEN_MAXY - posy1;
	posy2 = TOUCHSCREEN_MAXY - posy2;

#elif defined MUTTO_7inch
	touching = Rdbuf[0];
	oldtouching = 0;

	posx1 = ((Rdbuf[1] << 8) | Rdbuf[2]);
	posy1 = ((Rdbuf[3] << 8) | Rdbuf[4]);

	posx2 = ((Rdbuf[7] << 8) | Rdbuf[8]);
	posy2 = ((Rdbuf[9] << 8) | Rdbuf[10]);

	posx1 = posx1 & 0x0fff;
	posy1 = posy1 & 0x0fff;
	posx2 = posx2 & 0x0fff;
	posy2 = posy2 & 0x0fff;

#elif defined NAS_10inch
	touching = Rdbuf[0];
	oldtouching = Rdbuf[1];
	posx1 = ((Rdbuf[3] << 8) | Rdbuf[2]);
	posy1 = ((Rdbuf[5] << 8) | Rdbuf[4]);

	posx2 = ((Rdbuf[7] << 8) | Rdbuf[6]);
	posy2 = ((Rdbuf[9] << 8) | Rdbuf[8]);

#elif defined Unidisplay_7inch
	touching = Rdbuf[0];

	posx1 = ((Rdbuf[2] << 8) | Rdbuf[1]);
	posy1 = ((Rdbuf[4] << 8) | Rdbuf[3]);

	posx2 = ((Rdbuf[6] << 8) | Rdbuf[5]);
	posy2 = ((Rdbuf[8] << 8) | Rdbuf[7]);

#elif defined Unidisplay_9_7inch
	touching = Rdbuf[0]&0x03;
	posx1 = ((Rdbuf[2] << 8) | Rdbuf[1]);
	posy1 = ((Rdbuf[4] << 8) | Rdbuf[3]);
	posx2 = ((Rdbuf[6] << 8) | Rdbuf[5]);
	posy2 = ((Rdbuf[8] << 8) | Rdbuf[7]);

	if (touching == 2)
		touching = 1;
	else if (touching == 3)
		touching = 2;

#elif defined Unidisplay_10inch
	touching = Rdbuf[0];
	posx1 = ((Rdbuf[2] << 8) | Rdbuf[1]);
	posy1 = ((Rdbuf[4] << 8) | Rdbuf[3]);

	posx2 = ((Rdbuf[6] << 8) | Rdbuf[5]);
	posy2 = ((Rdbuf[8] << 8) | Rdbuf[7]);
#elif defined Touch_key
	touching = Rdbuf[0];
	oldtouching = Rdbuf[1];
	posx1 = Rdbuf[2];
	posy1 = Rdbuf[3];
#endif
	input_report_key(tsdata->input, BTN_TOUCH, (touching != 0 ? 1 : 0));
	input_report_key(tsdata->input, BTN_2, (touching == 2 ? 1 : 0));

	if (touching) {
		input_report_abs(tsdata->input, ABS_X, posx1);
		input_report_abs(tsdata->input, ABS_Y, posy1);
	}

	if (touching == 2) {
		input_report_abs(tsdata->input, ABS_HAT0X, posx2);
		input_report_abs(tsdata->input, ABS_HAT0Y, posy2);
	}
	if (touching > 2)
		touching = 2;

	if (!(touching)) {
		z = 0;
		w = 0;
	}
	input_report_abs(tsdata->input, ABS_MT_TOUCH_MAJOR, z);
	input_report_abs(tsdata->input, ABS_MT_WIDTH_MAJOR, w);
	input_report_abs(tsdata->input, ABS_MT_POSITION_X, posx1);
	input_report_abs(tsdata->input, ABS_MT_POSITION_Y, posy1);
	input_mt_sync(tsdata->input);

	if (touching == 2) {
		input_report_abs(tsdata->input, ABS_MT_TOUCH_MAJOR, z);
		input_report_abs(tsdata->input, ABS_MT_WIDTH_MAJOR, w);
		input_report_abs(tsdata->input, ABS_MT_POSITION_X, posx2);
		input_report_abs(tsdata->input, ABS_MT_POSITION_Y, posy2);
		input_mt_sync(tsdata->input);
	}
	input_sync(tsdata->input);
out:
	udelay(1000);
	enable_irq(tsdata->irq);
}

static irqreturn_t pixcir_ts_isr(int irq, void *dev_id)
{
	struct pixcir_i2c_ts_data *tsdata = dev_id;

	disable_irq_nosync(irq);
	queue_work(pixcir_wq, &tsdata->work.work);
	return IRQ_HANDLED;
}

static int pixcir_ts_open(struct input_dev *dev)
{
	return 0;
}

static void pixcir_ts_close(struct input_dev *dev)
{
}

static int touch_reset(void)
{
	if (gpio_request(TOUCH_RST_PIN, "TOUCH_RST_PIN")) {
		printk(KERN_ERR "gpio_request failed!\n");
		return -ENODEV;
	}

	s3c_gpio_setpull(TOUCH_RST_PIN, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(TOUCH_RST_PIN, S3C_GPIO_OUTPUT);
	gpio_direction_output(TOUCH_RST_PIN, 1);

	s3c_gpio_setpull(TOUCH_RST_PIN, S3C_GPIO_PULL_UP);
	gpio_set_value(TOUCH_RST_PIN, 1);
	udelay(1000);
	s3c_gpio_setpull(TOUCH_RST_PIN, S3C_GPIO_PULL_NONE);
	gpio_set_value(TOUCH_RST_PIN, 0);
	udelay(1000);
	gpio_free(TOUCH_RST_PIN);
	return 0;
}

static int pixcir_i2c_ts_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct pixcir_i2c_ts_data *tsdata;
	struct input_dev *input;
	int error = 0;
	unsigned char Wrbuf;

	error = touch_reset();
	if (error)
		goto fail;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "i2c check functionality failed\n");
		return -ENODEV;
	}

	tsdata = kzalloc(sizeof(*tsdata), GFP_KERNEL);
	if (!tsdata) {
		dev_err(&client->dev, "failed to allocate driver data!\n");
		error = -ENOMEM;
		goto fail1;
	}

	dev_set_drvdata(&client->dev, tsdata);

	input = input_allocate_device();
	if (!input) {
		dev_err(&client->dev, "failed to allocate input device!\n");
		error = -ENOMEM;
		goto fail2;
	}

	set_bit(EV_SYN, input->evbit);
	set_bit(EV_KEY, input->evbit);
	set_bit(EV_ABS, input->evbit);
	set_bit(BTN_TOUCH, input->keybit);
	set_bit(BTN_2, input->keybit);

	input_set_abs_params(input, ABS_X, TOUCHSCREEN_MINX,
						TOUCHSCREEN_MAXX, 0, 0);
	input_set_abs_params(input, ABS_Y, TOUCHSCREEN_MINY,
						TOUCHSCREEN_MAXY, 0, 0);
	input_set_abs_params(input, ABS_HAT0X, TOUCHSCREEN_MINX,
						TOUCHSCREEN_MAXX, 0, 0);
	input_set_abs_params(input, ABS_HAT0Y, TOUCHSCREEN_MINY,
						TOUCHSCREEN_MAXY, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_X, TOUCHSCREEN_MINX,
						TOUCHSCREEN_MAXX, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_Y, TOUCHSCREEN_MINY,
						TOUCHSCREEN_MAXY, 0, 0);
	input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input, ABS_MT_WIDTH_MAJOR, 0, 25, 0, 0);

	input->name = client->name;
	input->id.bustype = BUS_I2C;
	input->dev.parent = &client->dev;

	input->open = pixcir_ts_open;
	input->close = pixcir_ts_close;

	input_set_drvdata(input, tsdata);

	tsdata->client = client;
	tsdata->input = input;

	INIT_WORK(&tsdata->work.work, pixcir_ts_poscheck);

	tsdata->irq = client->irq;

	error = input_register_device(input);
	if (error) {
		dev_err(&client->dev, "Could not register input device\n");
		goto fail2;
	}

	error = gpio_request(TOUCH_INT_PIN, "GPX3");
	if (error) {
			dev_err(&client->dev, "gpio_request failed\n");
			error = -ENODEV;
			goto fail3;

	} else {
		s3c_gpio_cfgpin(TOUCH_INT_PIN, S3C_GPIO_SFN(0x0F));
		s3c_gpio_setpull(TOUCH_INT_PIN, S3C_GPIO_PULL_UP);
	}

#if defined(Unidisplay_9_7inch) || defined(Unidisplay_7inch) \
	|| defined(Unidisplay_7inch)
	Wrbuf = 0xcc;
	if (i2c_master_send(tsdata->client, &Wrbuf, 1) != 1) {
		dev_err(&client->dev, "i2c transfer failed\n");
		error = -ENODEV;
		goto fail3;
	}
#endif
	pixcir_wq = create_singlethread_workqueue("pixcir_wq");
	if (!pixcir_wq) {
		error = -ENOMEM;
		goto fail3;
	}

#ifdef NAS_7inch
	error = request_irq(tsdata->irq, pixcir_ts_isr, IRQF_DISABLED |
				IRQF_TRIGGER_FALLING, client->name, tsdata);
#elif defined Unidisplay_7inch
	error = request_irq(tsdata->irq, pixcir_ts_isr, IRQF_SAMPLE_RANDOM
				, client->name, tsdata);
#endif
	if (error) {
		dev_err(&client->dev, "Failed to request irq %d\n", \
			 tsdata->irq);
		goto fail3;
	}
	if (!error) {
		device_init_wakeup(&client->dev, 1);
		dev_info(&tsdata->client->dev, "probed successfully!\n");
		return 0;
	}
fail3:
	input_unregister_device(input);
	input = NULL;
fail2:
	kfree(tsdata);
fail1:
	dev_set_drvdata(&client->dev, NULL);
fail:
	return error;

}

static int pixcir_i2c_ts_remove(struct i2c_client *client)
{
	struct pixcir_i2c_ts_data *tsdata = dev_get_drvdata(&client->dev);
	free_irq(tsdata->irq, tsdata);
	input_unregister_device(tsdata->input);
	kfree(tsdata);
	dev_set_drvdata(&client->dev, NULL);
	return 0;
}

static int pixcir_i2c_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct pixcir_i2c_ts_data *tsdata = dev_get_drvdata(&client->dev);
	disable_irq(tsdata->irq);
	return 0;
}

static int pixcir_i2c_ts_resume(struct i2c_client *client)
{
	struct pixcir_i2c_ts_data *tsdata = dev_get_drvdata(&client->dev);
	enable_irq(tsdata->irq);
	return 0;
}

static const struct i2c_device_id pixcir_i2c_ts_id[] = {
	{ "vpad_ctp", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, pixcir_i2c_ts_id);

static struct i2c_driver pixcir_i2c_ts_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "pixcir i2c touchscreen driver",
	 },
	.probe = pixcir_i2c_ts_probe,
	.remove = pixcir_i2c_ts_remove,
	.suspend = pixcir_i2c_ts_suspend,
	.resume = pixcir_i2c_ts_resume,
	.id_table = pixcir_i2c_ts_id,
};

static int __init pixcir_i2c_ts_init(void)
{
	return i2c_add_driver(&pixcir_i2c_ts_driver);
}
module_init(pixcir_i2c_ts_init);

static void __exit pixcir_i2c_ts_exit(void)
{
	i2c_del_driver(&pixcir_i2c_ts_driver);
	if (pixcir_wq)
		destroy_workqueue(pixcir_wq);
}
module_exit(pixcir_i2c_ts_exit);

MODULE_AUTHOR("Bee <http://www.pixcir.com.cn>");
MODULE_DESCRIPTION("Pixcir Capacitive Touchscreen Driver");
MODULE_LICENSE(GPL);
