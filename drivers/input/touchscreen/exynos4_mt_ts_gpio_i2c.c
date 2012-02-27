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
#include <linux/fs.h>

#include <mach/irqs.h>
#include <asm/system.h>

#include <linux/delay.h>
#include <mach/regs-gpio.h>

#include "exynos4_mt_ts_gpio_i2c.h"
#include "exynos4_mt_ts.h"

#define GPD1CON			(S5P_VA_GPIO + 0xC0)
#define GPD1DAT			(S5P_VA_GPIO + 0xC4)


/* Touch I2C Address Define */
#define TOUCH_WR_ADDR		0xB8
#define TOUCH_RD_ADDR		0xB9

/* Touch I2C Port define */
#define GPIO_I2C_SDA_CON_PORT	(*(unsigned long *)GPD1CON)
#define GPIO_I2C_SDA_DAT_PORT	(*(unsigned long *)GPD1DAT)
#define GPIO_SDA_PIN		0

#define GPIO_I2C_CLK_CON_PORT	(*(unsigned long *)GPD1CON)
#define GPIO_I2C_CLK_DAT_PORT	(*(unsigned long *)GPD1DAT)
#define GPIO_CLK_PIN		1

#define DELAY_TIME 		5 /* us value */
#define PORT_CHANGE_DELAY_TIME	5
#define GPIO_CON_PORT_MASK	0xF
#define GPIO_CON_PORT_OFFSET	0x4

#define GPIO_CON_INPUT		0x0
#define GPIO_CON_OUTPUT		0x1

#define HIGH			1
#define LOW			0

/* static function prototype */
static void gpio_i2c_sda_port_control(unsigned char inout);
static void gpio_i2c_clk_port_control(unsigned char inout);

static unsigned char gpio_i2c_get_sda(void);
static void gpio_i2c_set_sda(unsigned char hi_lo);
static void gpio_i2c_set_clk(unsigned char hi_lo);

static void gpio_i2c_start(void);
static void gpio_i2c_stop(void);

static void gpio_i2c_send_ack(void);
static void gpio_i2c_send_noack(void);
static unsigned char gpio_i2c_chk_ack(void);

static void gpio_i2c_byte_write(unsigned char wdata);
static void gpio_i2c_byte_read(unsigned char *rdata);

static void gpio_i2c_sda_port_control(unsigned char inout)
{
	GPIO_I2C_SDA_CON_PORT &=
		(unsigned long)(~(GPIO_CON_PORT_MASK <<
			(GPIO_SDA_PIN * GPIO_CON_PORT_OFFSET)));
	GPIO_I2C_SDA_CON_PORT |=
		(unsigned long)((inout <<
			(GPIO_SDA_PIN * GPIO_CON_PORT_OFFSET)));
}

static void gpio_i2c_clk_port_control(unsigned char inout)
{
	GPIO_I2C_CLK_CON_PORT &=
		(unsigned long)(~(GPIO_CON_PORT_MASK <<
			(GPIO_CLK_PIN * GPIO_CON_PORT_OFFSET)));
	GPIO_I2C_CLK_CON_PORT |=
		(unsigned long)((inout <<
			(GPIO_CLK_PIN * GPIO_CON_PORT_OFFSET)));
}

static unsigned char gpio_i2c_get_sda(void)
{
	return GPIO_I2C_SDA_DAT_PORT & (HIGH << GPIO_SDA_PIN) ? 1 : 0;
}

static void gpio_i2c_set_sda(unsigned char hi_lo)
{
	if (hi_lo) {
		gpio_i2c_sda_port_control(GPIO_CON_INPUT);
		udelay(PORT_CHANGE_DELAY_TIME);
	} else {
		GPIO_I2C_SDA_DAT_PORT &= ~(HIGH << GPIO_SDA_PIN);
		gpio_i2c_sda_port_control(GPIO_CON_OUTPUT);
		udelay(PORT_CHANGE_DELAY_TIME);
	}
}

static void gpio_i2c_set_clk(unsigned char hi_lo)
{
	if (hi_lo) {
		gpio_i2c_clk_port_control(GPIO_CON_INPUT);
		udelay(PORT_CHANGE_DELAY_TIME);
	} else {
		GPIO_I2C_CLK_DAT_PORT &= ~(HIGH << GPIO_CLK_PIN);
		gpio_i2c_clk_port_control(GPIO_CON_OUTPUT);
		udelay(PORT_CHANGE_DELAY_TIME);
	}
}

static void gpio_i2c_start(void)
{
	/* Setup SDA, CLK output High */
	gpio_i2c_set_sda(HIGH);
	gpio_i2c_set_clk(HIGH);

	udelay(DELAY_TIME);

	/* SDA low before CLK low */
	gpio_i2c_set_sda(LOW);
	udelay(DELAY_TIME);

	gpio_i2c_set_clk(LOW);
	udelay(DELAY_TIME);
}

static void gpio_i2c_stop(void)
{
	/* Setup SDA, CLK output low */
	gpio_i2c_set_sda(LOW);
	gpio_i2c_set_clk(LOW);

	udelay(DELAY_TIME);

	/* SDA high after CLK high */
	gpio_i2c_set_clk(HIGH);
	udelay(DELAY_TIME);

	gpio_i2c_set_sda(HIGH);
	udelay(DELAY_TIME);
}

static void gpio_i2c_send_ack(void)
{
	/* SDA Low */
	gpio_i2c_set_sda(LOW);
	udelay(DELAY_TIME);
	gpio_i2c_set_clk(HIGH);
	udelay(DELAY_TIME);
	gpio_i2c_set_clk(LOW);
	udelay(DELAY_TIME);
}

static void gpio_i2c_send_noack(void)
{
	/* SDA High */
	gpio_i2c_set_sda(HIGH);
	udelay(DELAY_TIME);
	gpio_i2c_set_clk(HIGH);
	udelay(DELAY_TIME);
	gpio_i2c_set_clk(LOW);
	udelay(DELAY_TIME);
}

static unsigned char gpio_i2c_chk_ack(void)
{
	unsigned char count = 0;
	unsigned char ret = 0;

	gpio_i2c_set_sda(LOW);
	udelay(DELAY_TIME);
	gpio_i2c_set_clk(HIGH);
	udelay(DELAY_TIME);

	gpio_i2c_sda_port_control(GPIO_CON_INPUT);
	udelay(PORT_CHANGE_DELAY_TIME);

	while (gpio_i2c_get_sda()) {
		if (count++ > 100) {
			ret = 1;
			break;
		} else {
			udelay(DELAY_TIME);
		}
	}

	gpio_i2c_set_clk(LOW);
	udelay(DELAY_TIME);

#if defined(DEBUG_GPIO_I2C)
	if (ret)
		printk(KERN_DEBUG"%s %d: no ack!!\n", __func__, ret);
	else
		printk(KERN_DEBUG"%s %d: ack !!\n" , __func__, ret);
#endif

	return ret;
}

static void gpio_i2c_byte_write(unsigned char wdata)
{
	unsigned char cnt, mask;

	for (cnt = 0, mask = 0x80; cnt < 8; cnt++, mask >>= 1) {
		if (wdata & mask)
			gpio_i2c_set_sda(HIGH);
		else
			gpio_i2c_set_sda(LOW);

		gpio_i2c_set_clk(HIGH);
		udelay(DELAY_TIME);
		gpio_i2c_set_clk(LOW);
		udelay(DELAY_TIME);
	}
}

static void gpio_i2c_byte_read(unsigned char *rdata)
{
	unsigned char cnt, mask;

	gpio_i2c_sda_port_control(GPIO_CON_INPUT);
	udelay(PORT_CHANGE_DELAY_TIME);

	for (cnt = 0, mask = 0x80, *rdata = 0; cnt < 8; cnt++, mask >>= 1) {
		gpio_i2c_set_clk(HIGH);
		udelay(DELAY_TIME);

		if (gpio_i2c_get_sda())
			*rdata |= mask;

		gpio_i2c_set_clk(LOW);
		udelay(DELAY_TIME);

	}
}

int exynos4_ts_write(unsigned char addr,
	unsigned char *wdata, unsigned char wsize)
{
	unsigned char cnt, ack;

	/* start */
	gpio_i2c_start();

	/* i2c address */
	gpio_i2c_byte_write(TOUCH_WR_ADDR);

	ack = gpio_i2c_chk_ack();
	if (ack) {
		printk(KERN_DEBUG "%s [write address] : no ack\n", __func__);
		goto write_stop;
	}

	/* register */
	gpio_i2c_byte_write(addr);

	ack = gpio_i2c_chk_ack();
	if (ack)
		printk(KERN_DEBUG "%s [write register] : no ack\n", __func__);

	if (wsize) {
		for (cnt = 0; cnt < wsize; cnt++) {
			gpio_i2c_byte_write(wdata[cnt]);

			ack = gpio_i2c_chk_ack();
			if (ack) {
				printk(KERN_DEBUG "%s [write register] :\
					no ack\n", __func__);
				goto write_stop;
			}
		}
	}

write_stop:

#if defined(CONFIG_TOUCHSCREEN_EXYNOS4_MT)
	if (wsize)
		gpio_i2c_stop();
#else
	gpio_i2c_stop();
#endif

#if defined(DEBUG_GPIO_I2C)
	printk(KERN_DEBUG"%s : %d\n", __func__, ack);
#endif
	return ack;
}

int exynos4_ts_read(unsigned char *rdata, unsigned char rsize)
{
	unsigned char ack, cnt;

	/* start */
	gpio_i2c_start();

	/* i2c address */
	gpio_i2c_byte_write(TOUCH_RD_ADDR);

	ack = gpio_i2c_chk_ack();
	if (ack) {
		printk(KERN_DEBUG "%s [write address] : no ack\n", __func__);
		goto read_stop;
	}

	for (cnt = 0; cnt < rsize; cnt++) {
		gpio_i2c_byte_read(&rdata[cnt]);

		if (cnt == rsize - 1)
			gpio_i2c_send_noack();
		else
			gpio_i2c_send_ack();
	}

read_stop:
	gpio_i2c_stop();

	return ack;
}

void exynos4_ts_port_init(void)
{
	gpio_i2c_set_sda(HIGH);
	gpio_i2c_set_clk(HIGH);
}
