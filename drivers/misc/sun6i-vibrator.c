/* Vibrator driver for sun6i platform 
 * ported from msm pmic vibrator driver
 *  by tom cubie <tangliang@reuuimllatech.com>
 *
 * Copyright (C) 2011 ReuuiMlla Technology.
 *
 * Copyright (C) 2008 HTC Corporation.
 * Copyright (C) 2007 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <linux/gpio.h>
#include <linux/module.h>

//#include <linux/ktime.h>

#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>
#include "../drivers/staging/android/timed_output.h"

static struct work_struct vibrator_work;
static struct hrtimer vibe_timer;
static spinlock_t vibe_lock;
static int vibe_state;
static int vibe_off;
static struct gpio_hdle {
	script_item_u	val;
	script_item_value_type_e  type;		
}vibe_gpio_hdle;

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_DATA_INFO = 1U << 1,
	DEBUG_SUSPEND = 1U << 2,
};
static u32 debug_mask = 0;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(KERN_DEBUG fmt , ## arg)

module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

static void set_sun6i_vibrator(int on)
{
	dprintk(DEBUG_DATA_INFO, "sun6i_vibrator on %d\n", on);
	if(on) {
		__gpio_set_value(vibe_gpio_hdle.val.gpio.gpio, !vibe_off);
	} else {
		__gpio_set_value(vibe_gpio_hdle.val.gpio.gpio, vibe_off);
	}
}

static void update_vibrator(struct work_struct *work)
{
	set_sun6i_vibrator(vibe_state);
}

static void vibrator_enable(struct timed_output_dev *dev, int value)
{
	unsigned long	flags;

	spin_lock_irqsave(&vibe_lock, flags);
	hrtimer_cancel(&vibe_timer);

	dprintk(DEBUG_DATA_INFO, "sun6i_vibrator enable %d\n", value);

	if (value <= 0)
		vibe_state = 0;
	else {
		value = (value > 15000 ? 15000 : value);
		vibe_state = 1;
		hrtimer_start(&vibe_timer,
			ktime_set(value / 1000, (value % 1000) * 1000000),
			HRTIMER_MODE_REL);
	}
	spin_unlock_irqrestore(&vibe_lock, flags);

	schedule_work(&vibrator_work);
}

static int vibrator_get_time(struct timed_output_dev *dev)
{
	struct timespec time_tmp;
	if (hrtimer_active(&vibe_timer)) {
		ktime_t r = hrtimer_get_remaining(&vibe_timer);
		time_tmp = ktime_to_timespec(r);
		//return r.tv.sec * 1000 + r.tv.nsec/1000000;
		return time_tmp.tv_sec* 1000 + time_tmp.tv_nsec/1000000;
	} else
		return 0;
}

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *timer)
{
	vibe_state = 0;
	schedule_work(&vibrator_work);
	dprintk(DEBUG_DATA_INFO, "sun6i_vibrator timer expired\n");
	return HRTIMER_NORESTART;
}

static struct timed_output_dev sun6i_vibrator = {
	.name = "vibrator",
	.get_time = vibrator_get_time,
	.enable = vibrator_enable,
};

static int __init sun6i_vibrator_init(void)
{
	int vibe_used;
	script_item_u	val;
	script_item_value_type_e  type;

	dprintk(DEBUG_INIT, "hello, sun6i_vibrator init\n");

	type = script_get_item("motor_para", "motor_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		printk(KERN_ERR "%s script_parser_fetch \"motor_para\" motor_used = %d\n",
				__FUNCTION__, val.val);
		goto exit1;
	}
	vibe_used = val.val;

	if(!vibe_used) {
		printk(KERN_ERR"%s motor is not used in config\n", __FUNCTION__);
		goto exit1;
	}

	vibe_gpio_hdle.type = script_get_item("motor_para", "motor_shake", &(vibe_gpio_hdle.val));
	if(SCIRPT_ITEM_VALUE_TYPE_PIO != vibe_gpio_hdle.type) {
		printk(KERN_ERR "vibrator motor_shake type err!");
		goto exit1;
	}
	dprintk(DEBUG_INIT, "value is: gpio %d, mul_sel %d, pull %d, drv_level %d, data %d\n", 
	    vibe_gpio_hdle.val.gpio.gpio, vibe_gpio_hdle.val.gpio.mul_sel, vibe_gpio_hdle.val.gpio.pull, 
	    vibe_gpio_hdle.val.gpio.drv_level, vibe_gpio_hdle.val.gpio.data);

	vibe_off = vibe_gpio_hdle.val.gpio.data;
	dprintk(DEBUG_INIT, "vibe_off is %d\n", vibe_off);

	if(0 != gpio_request(vibe_gpio_hdle.val.gpio.gpio, NULL)) {
		printk(KERN_ERR "ERROR: vibe Gpio_request is failed\n");
	}
	
	if (0 != sw_gpio_setall_range(&vibe_gpio_hdle.val.gpio, 1)) {
		printk(KERN_ERR "vibe gpio set err!");
		goto exit;
	}

	__gpio_set_value(vibe_gpio_hdle.val.gpio.gpio, vibe_off);

	INIT_WORK(&vibrator_work, update_vibrator);

	spin_lock_init(&vibe_lock);
	vibe_state = 0;
	hrtimer_init(&vibe_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vibe_timer.function = vibrator_timer_func;

	timed_output_dev_register(&sun6i_vibrator);

	dprintk(DEBUG_INIT, "sun6i_vibrator init end\n");

	return 0;
exit:
	gpio_free(vibe_gpio_hdle.val.gpio.gpio);
exit1:	
	return -1;
}

static void __exit sun6i_vibrator_exit(void)
{
	dprintk(DEBUG_INIT, "bye, sun6i_vibrator_exit\n");
	timed_output_dev_unregister(&sun6i_vibrator);
	gpio_free(vibe_gpio_hdle.val.gpio.gpio);	
}
module_init(sun6i_vibrator_init);
module_exit(sun6i_vibrator_exit);

/* Module information */
MODULE_DESCRIPTION("timed output vibrator device for sun6i");
MODULE_LICENSE("GPL");

