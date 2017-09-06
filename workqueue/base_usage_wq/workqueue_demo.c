/*
 * work queue demo
 *
 * (C) 2017.09.06 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/sched.h>

static struct workqueue_struct *wq;
static struct work_struct work_demo1;
static struct work_struct work_demo2;

static void do_work1(void *arg)
{
    printk("Work queue 1.\n");
    mdelay(2000);
    schedule_work(&work_demo2);
}

static void do_work2(void *arg)
{
    printk("Work queue 2.\n");
    mdelay(2000);
    schedule_work(&work_demo1);
}

static __init int _workqueue_init(void)
{
    INIT_WORK(&work_demo1, do_work1);
    INIT_WORK(&work_demo2, do_work2);

    wq = create_singlethread_workqueue("wq-demo");
    schedule_work(&work_demo1);
    return 0;
}
device_initcall(_workqueue_init);
