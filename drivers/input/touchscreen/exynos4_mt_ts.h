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
#ifndef _EXYNOS4_TS_H_
#define _EXYNOS4_TS_H_

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#define EXYNOS4_TS_DEVICE_NAME	"exynos4_ts"

#define TOUCH_PRESS		1
#define TOUCH_RELEASE		0

/* Touch Configuration */

#define GPX3DAT			(S5P_VA_GPIO2 + 0xC64)
#define EINT_43CON		(S5P_VA_GPIO2 + 0xE0C)

/* Touch Interrupt define */
#define EXYNOS4_TS_IRQ		gpio_to_irq(EXYNOS4_GPX3(5))

#define TS_ABS_MIN_X		0
#define TS_ABS_MIN_Y		0
#define TS_ABS_MAX_X		1366
#define TS_ABS_MAX_Y		768

#define TS_X_THRESHOLD		1
#define TS_Y_THRESHOLD		1

#define TS_ATTB			(EXYNOS4_GPX3(5))

/* Interrupt Check port */
#define GET_INT_STATUS()	(((*(unsigned long *)GPX3DAT) & 0x01) ? 1 : 0)

/* touch register */
#define MODULE_CALIBRATION	0x37
#define MODULE_POWERMODE	0x14
#define MODULE_INTMODE		0x15
#define MODULE_INTWIDTH		0x16

#define PERIOD_10MS		(HZ/100) /* 10ms */
#define PERIOD_20MS		(HZ/50)	 /* 20ms */
#define PERIOD_50MS		(HZ/20)	 /* 50ms */

#define TOUCH_STATE_BOOT	0
#define TOUCH_STATE_RESUME	1

/* Touch hold event */
#define SW_TOUCH_HOLD		0x09

#if defined(CONFIG_TOUCHSCREEN_EXYNOS4_MT)

/* multi-touch data process struct */
struct touch_process_data {
	unsigned char finger_cnt;
	unsigned int x1;
	unsigned int y1;
	unsigned int x2;
	unsigned int y2;
};

#endif

struct exynos4_ts {

	struct input_dev *driver;

	/* seqlock_t */
	seqlock_t lock;
	unsigned int seq;

	/* timer */
	struct timer_list penup_timer;

	/* data store */
	unsigned int status;
	unsigned int x;
	unsigned int y;

	unsigned char rd[10];

	/* sysfs used */
	unsigned char hold_status;

	unsigned char sampling_rate;

	/* x data threshold (0-10) : default 3 */
	unsigned char threshold_x;
	/* y data threshold (0-10) : default 3 */
	unsigned char threshold_y;
	/* touch sensitivity (0-255) : default 0x14 */
	unsigned char sensitivity;

#if defined CONFIG_TOUCHSCREEN_EXYNOS4_MT

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend power;
#endif
#endif

};

extern struct exynos4_ts exynos4_ts;

#endif /* _EXYNOS4_TS_H_ */
